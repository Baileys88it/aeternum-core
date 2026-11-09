# IE-M3 Blocco3 - Wiring Finale (observe WorldSocket + default CMake ON)

**Branch:** `feat/m3-runtime`  
**Scope:** commit locale sopra Blocco2 `106389396ee3`. **Nessun deploy pubblico in questo commit.** Solo isolata `_testinst_iem1` dopo build PASS.  
**Produzione / `_public_iem1`:** intatta in questa fase (niente backup, restart, merge, tag `M3-Complete`).

## 1. Default CMake

| Interruttore | Valore | Note |
|--------------|--------|------|
| CMake `AETERNUM_NATIVE_MODULE_RUNTIME` | **ON** (era OFF) | worldserver linka `aeternum-native-runtime` |
| `WITH_DYNAMIC_LINKING` | **OFF** | invariato |
| worldserver.conf `Aeternum.Modules.NativeRuntime` | default **0**; isolata **1** a vuoto | ON + config 1 + zero moduli nativi = runtime vuoto |

## 2. Wiring WorldSocket (solo observe)

- **Inbound:** `WorldSocket::ReadDataHandler` dopo `TranslateInbound` + `SetOpcode`, ramo default pacchetti di gioco, **prima** di `QueuePacket`.
- **Outbound:** `WorldSocket::SendPacket` prima di `TranslateOutbound`. Niente hook in `WritePacketToBuffer`.
- **No double-fire:** rimosso il helper inbound da `WorldSession::Update`.
- Fast path tabella vuota: un load acquire `count == 0` per pacchetto; zero heap; zero crash.

## 3. Config / schema

- Chiave nota: `Aeternum.Modules.NativeRuntime`.
- Isolata: `Aeternum.Modules.NativeRuntime = 1` senza moduli `"runtime":"native"`.

## 4. QA - Catch2 `[m3][blocco3]`

- Tabella vuota: `DispatchInbound` / `DispatchOutbound` no-op.
- Regressione blocco1/2 + `[m1]`/`[m2]`/suite.
- Lint ABI no-STL e no-map (WorldSocket.cpp in hot-path).

## 5. Evidenza boot isolata

- Porte `3119`/`28083`/`28085`, admin `127.0.0.1:9200`.
- Banner SHA = questo commit Blocco3; FATAL=0; log runtime nativo ON/vuoto.
- Pubblico invariato su `2119`/`18083`/`18085`/admin `9300`.

## 6. Rimandati

- Push di questo commit Blocco3.
- Dark Launch pubblico, merge `aeternum/main`, tag `M3-Complete`.
- Allowlist opcode da manifest; verdetto DROP; cancel SMSG.
