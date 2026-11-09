/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_WORLD_SOCKET_H
#define TRINITYCORE_WORLD_SOCKET_H

#include "aeternum/abi/AeternumABI.h"
#include "AsyncCallbackProcessor.h"
#include "AuthDefines.h"
#include "DatabaseEnvFwd.h"
#include "IpConnectionLimit.h"
#include "MPSCQueue.h"
#include "MessageBuffer.h"
#include "Socket.h"
#include "WorldPacket.h"
#include "WorldPacketCrypt.h"
#include <array>
#include <atomic>
#include <mutex>

namespace JSON::RealmList
{
class RealmJoinTicket;
}

typedef struct z_stream_s z_stream;
class EncryptablePacket;
class WorldPacket;
class WorldSession;
enum ConnectionType : int8;
enum OpcodeClient : uint32;

class EncryptablePacket : public WorldPacket
{
public:
    EncryptablePacket(WorldPacket const& packet, bool encrypt) : WorldPacket(packet), _encrypt(encrypt)
    {
        SocketQueueLink.store(nullptr, std::memory_order_relaxed);
    }

    bool NeedsEncryption() const { return _encrypt; }

    std::atomic<EncryptablePacket*> SocketQueueLink;

private:
    bool _encrypt;
};

namespace WorldPackets::Auth
{
    class AuthSession;
    class AuthContinuedSession;
}

#pragma pack(push, 1)

struct PacketHeader
{
    uint32 Size;
    uint8 Tag[12];

    bool IsValidSize() const { return Size < 0x10000; }
};

struct IncomingPacketHeader : PacketHeader
{
    uint32 EncryptedOpcode;
};

#pragma pack(pop)

class TC_GAME_API WorldSocket final : public Trinity::Net::Socket<>
{
    static uint32 const MinSizeForCompression;

    static std::array<uint8, 32> const AuthCheckSeed;
    static std::array<uint8, 32> const SessionKeySeed;
    static std::array<uint8, 32> const ContinuedSessionSeed;
    static std::array<uint8, 32> const EncryptionKeySeed;

    using BaseSocket = Socket;

public:
    explicit WorldSocket(Trinity::Net::IoContextTcpSocket&& socket);
    ~WorldSocket();

    WorldSocket(WorldSocket const& right) = delete;
    WorldSocket(WorldSocket&& right) = delete;
    WorldSocket& operator=(WorldSocket const& right) = delete;
    WorldSocket& operator=(WorldSocket&& right) = delete;

    void Start() override;
    bool Update() override;

    void SendPacket(WorldPacket const& packet);
    // M11 internal transport seam, not a guest API. Caller holds scoped inbound capability.
    AeternumStatus AeternumEnqueueTimer(uint32 opcode,void const* payload,uint32 size) noexcept;

    ConnectionType GetConnectionType() const { return _type; }

    void SendAuthResponseError(uint32 code);
    void SetWorldSession(WorldSession* session, std::weak_ptr<WorldSocket> realmSocket = {});
    void SetSendBufferSize(std::size_t sendBufferSize) { _sendBufferSize = sendBufferSize; }

    void OnClose() override;
    Trinity::Net::SocketReadCallbackResult ReadHandler() override;

    void QueueQuery(QueryCallback&& queryCallback);

    void SendAuthSession();
    bool InitializeCompression();
    bool FreezeAeternumHandoff(uint8* dst, uint16 cap, uint16& n);
    bool ThawAeternumHandoff();
    bool EmitAeternumHandoffFrame(uint8 const* packet, uint16 n);
    bool AdoptAeternumShadowAuth(uint8 const* packet, uint16 n);

protected:
    bool ReadHeaderHandler();

    enum class ReadDataHandlerResult
    {
        Ok = 0,
        Error = 1,
        WaitingForQuery = 2
    };

    ReadDataHandlerResult ReadDataHandler();
private:
    /// writes network.opcode log
    void LogOpcodeText(OpcodeClient opcode) const;
    void LogOpcodeText(OpcodeClient opcode, std::scoped_lock<std::mutex> const& guard) const;
    /// sends and logs network.opcode without accessing WorldSession
    void SendPacketAndLogOpcode(WorldPacket const& packet);
    void WritePacketToBuffer(EncryptablePacket const& packet, MessageBuffer& buffer);
    uint32 CompressPacket(uint8* buffer, WorldPacket const& packet, uint32 wireOpcode);

    ReadDataHandlerResult HandleAuthSession(WorldPacket&& packet);
    void HandleAuthSessionCallback(WorldPackets::Auth::AuthSession const* authSession, JSON::RealmList::RealmJoinTicket* joinTicket, PreparedResultSet const* result);
    ReadDataHandlerResult HandleAuthContinuedSession(WorldPacket&& packet);
    void HandleAuthContinuedSessionCallback(WorldPackets::Auth::AuthContinuedSession const* authSession, PreparedResultSet const* result);
    void LoadSessionPermissionsCallback(PreparedQueryResult result);
    void SendEnterEncryptedMode();
    ReadDataHandlerResult HandleKeepAlive();
    ReadDataHandlerResult HandleLogDisconnect(WorldPacket&& packet) const;
    ReadDataHandlerResult HandleConnectToFailed(WorldPacket&& packet);
    ReadDataHandlerResult HandlePing(WorldPacket&& packet);
    ReadDataHandlerResult HandleEnterEncryptedModeAck();

    friend struct AeternumM11SocketTestAccess; // fixture accesses real queue/framer, never live sockets
    // Core-only route: bound on world thread with the validated session, consumed under ingress session lock.
    void BindAeternumTimerRealm(std::weak_ptr<WorldSocket> realmSocket, uint32 sessionId);
    AeternumStatus AeternumRouteTimer(uint32 sessionId, uint32 opcode, void const* payload, uint32 size) noexcept;
    std::weak_ptr<WorldSocket> _aeternumRealmSocket;
    uint32 _aeternumRealmSessionId = UINT32_MAX;
    std::atomic<bool> _aeternumTimerSent{false}; // REALM target; MPSC producers can include INSTANCE ingress
    ConnectionType _type;
    uint64 _key;

    std::array<uint8, 32> _serverChallenge;
    WorldPacketCrypt _authCrypt;
    SessionKey _sessionKey;
    std::array<uint8, 32> _encryptKey;

    TimePoint _lastPingTime;
    uint32 _overSpeedPings;

    std::mutex _worldSessionLock;
    WorldSession* _worldSession;
    std::atomic<bool> _authed;      // also set from the world thread (SetWorldSession), read by Update for the login timeout
    bool _canRequestHotfixes;
    bool _aeternumHandoffFrozen = false;
    bool _aeternumShadowAdopted = false;

    std::unique_ptr<Trinity::Net::IpConnectionLimit::Slot> _ipConnectionSlot;
    TimePoint _authDeadline;        // closed if not logged in by then (Network.AuthTimeout); TimePoint::max() = no limit

    MessageBuffer _headerBuffer;
    MessageBuffer _packetBuffer;
    MPSCQueue<EncryptablePacket, &EncryptablePacket::SocketQueueLink> _bufferQueue;
    std::size_t _sendBufferSize;

    z_stream* _compressionStream;

    QueryCallbackProcessor _queryProcessor;
    std::string _ipCountry;
};

#endif
