#pragma once
#include "AuthDefines.h"
#include "WorldPacketCrypt.h"
#include "aeternum/sdk/internal/meshing.hpp"
#include <cstring>
#include <optional>
namespace aeternum::sdk::internal::handoff {
constexpr std::uint32_t Magic=0x31485341u; // ASH1
constexpr std::uint16_t Version=1;
constexpr std::uint8_t CipherAes256Gcm=1;
#pragma pack(push,1)
struct Packet {
 std::uint32_t magic=Magic;
 std::uint16_t version=Version;
 std::uint8_t cipher=CipherAes256Gcm;
 std::uint8_t pad=0;
 std::uint32_t accountId=0;
 std::uint32_t security=0;
 std::uint64_t clientCounter=0;
 std::uint64_t serverCounter=0;
 std::uint8_t sessionKey[SESSION_KEY_LENGTH]{};
 std::uint8_t cryptKey[32]{};
};
#pragma pack(pop)
static_assert(sizeof(Packet)==104);
inline bool FreezeExport(WorldPacketCrypt const& crypt,SessionKey const& sessionKey,std::uint32_t account,std::uint32_t security,Packet& out){
 WorldPacketCrypt::Key key{};std::uint64_t cc=0,sc=0;if(!crypt.Export(key,cc,sc))return false;
 out={};out.magic=Magic;out.version=Version;out.cipher=CipherAes256Gcm;out.accountId=account;out.security=security;
 out.clientCounter=cc;out.serverCounter=sc;std::memcpy(out.sessionKey,sessionKey.data(),SESSION_KEY_LENGTH);
 std::memcpy(out.cryptKey,key.data(),key.size());return true;
}
inline bool Encode(Packet const& p,unsigned char* dst,std::uint16_t cap,std::uint16_t& n){
 if(p.magic!=Magic||p.version!=Version||p.cipher!=CipherAes256Gcm||cap<sizeof(Packet))return false;
 std::memcpy(dst,&p,sizeof(Packet));n=sizeof(Packet);return true;
}
inline bool Decode(unsigned char const* src,std::uint16_t n,Packet& p){
 if(!src||n!=sizeof(Packet))return false;std::memcpy(&p,src,sizeof(Packet));
 return p.magic==Magic&&p.version==Version&&p.cipher==CipherAes256Gcm;
}
inline bool EncodeBackendFrame(Packet const& p,unsigned char* dst,std::uint16_t cap,std::uint16_t& n){
 std::uint16_t payload=0;if(cap<8||!Encode(p,dst+8,std::uint16_t(cap-8),payload))return false;
 std::uint32_t magic=meshing::StateMagic;std::uint16_t pad=0;
 std::memcpy(dst,&magic,4);std::memcpy(dst+4,&payload,2);std::memcpy(dst+6,&pad,2);n=std::uint16_t(8+payload);return true;
}
inline bool DecodeBackendFrame(unsigned char const* src,std::uint16_t n,Packet& p){
 if(!src||n<8)return false;std::uint32_t magic=0;std::uint16_t payload=0;std::memcpy(&magic,src,4);std::memcpy(&payload,src+4,2);
 if(magic!=meshing::StateMagic||payload!=sizeof(Packet)||n!=std::uint16_t(8+payload))return false;return Decode(src+8,payload,p);
}
struct ShadowAuth {
 std::uint32_t accountId=0,security=0;
 SessionKey sessionKey{};
 WorldPacketCrypt::Key cryptKey{};
 std::uint64_t clientCounter=0,serverCounter=0;
 static std::optional<ShadowAuth> Adopt(Packet const& p){
  if(p.magic!=Magic||p.version!=Version||p.cipher!=CipherAes256Gcm)return std::nullopt;
  ShadowAuth s;s.accountId=p.accountId;s.security=p.security;
  std::memcpy(s.sessionKey.data(),p.sessionKey,SESSION_KEY_LENGTH);
  std::memcpy(s.cryptKey.data(),p.cryptKey,s.cryptKey.size());
  s.clientCounter=p.clientCounter;s.serverCounter=p.serverCounter;return s;
 }
 static std::optional<ShadowAuth> Adopt(unsigned char const* src,std::uint16_t n){
  Packet p{};if(!Decode(src,n,p))return std::nullopt;return Adopt(p);
 }
 bool Rehydrate(WorldPacketCrypt& crypt)const{crypt.Restore(cryptKey,clientCounter,serverCounter);return crypt.IsInitialized();}
};
inline bool FillControl(meshing::HandoffControl& c,Packet const& p){
 std::uint16_t n=0;return Encode(p,c.state.data(),std::uint16_t(c.state.size()),n)&&(c.stateBytes=n,true);
}
}
