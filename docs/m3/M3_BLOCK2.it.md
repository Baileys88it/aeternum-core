# IE-M3 Blocco2 - Cookie HostAlloc (F2) + tabella hook pacchetti densa + shutdown graceful 60 s su tainted (F6)

**Milestone:** IE-M3 Blocco2 (autorizzato da maintainer, t668u)
**Data:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m3-runtime` (worktree `<ENGINE_ROOT>\wt_m3`) sopra `42c9f7ffea7d` (Blocco1, pushato). `aeternum/main` = `feat/m2-protocol` = `34ceddeba8d4`, tag annotato `M2-Complete`. **Blocco2 = solo commit locale - nessun push.**
**Produzione / isolata:** non toccate (nessun deploy, nessun restart). Build: `<ENGINE_ROOT>\build_m3` (RelWithDebInfo, `WITH_DYNAMIC_LINKING=OFF`, `AETERNUM_NATIVE_MODULE_RUNTIME` default **OFF**).
**EN:** [`M3_BLOCK2.md`](M3_BLOCK2.md) - **Blocco1:** [`M3_BLOCK1.it.md`](M3_BLOCK1.it.md) - **Design:** [`M3_RUNTIME_DESIGN.it.md`](M3_RUNTIME_DESIGN.it.md) (§10)

## 1. ABI (maintainer Q1: ABI v1, crescita solo via `struct_size`)

`AETERNUM_MODULE_ABI_VERSION` resta `1`. Campi aggiunti in coda ad `AeternumHostFT` (dopo `runtime_tainted`):

| Campo | Firma | Note |
|---|---|---|
| `module_self` | `AeternumModuleHandle (void)` | handle del modulo il cui crossing è in corso; `NULL` fuori da un crossing |
| `host_alloc` | `void* (AeternumModuleHandle, uint64_t size)` | `NULL` = rifiutato (0 byte, cap, nessun crossing, handle falsificato) |
| `host_free` | `AeternumStatus (AeternumModuleHandle, void*)` | `NULL` = OK; ogni rifiuto = no-op + log + modulo in Error |
| `subscribe_packet` | `AeternumStatus (AeternumModuleHandle, uint32_t dir, uint32_t opcode, AeternumPacketHookFn)` | solo dentro `on_init` |

Nuovi tipi C: `AETERNUM_PACKET_DIR_CMSG/SMSG`, `AETERNUM_HOOK_PASS/DROP`, `AeternumPacketView { struct_size, direction, opcode, size, data, session }` (in prestito, valida solo durante la chiamata), `AeternumPacketHookFn = uint32_t (*)(void* mod_ctx, AeternumPacketView const*)`. I moduli verificano prima `AETERNUM_FT_HAS(host, AeternumHostFT, subscribe_packet)`. `check_abi_no_stl.ps1` invariato. Nessun bridge `IModule` → FT (Q7).

Handle modulo = `(generation << 8) | (slot + 1)`: 32 slot (`kMaxNativeModules`), generation a 24 bit incrementata a ogni claim, quindi un handle di un boot/modulo precedente non si risolve mai. L'handle passato deve coincidere con lo slot del crossing in corso (`CrossingScope`, thread-local); altrimenti è uno **spoof** -> modulo in quarantena.

## 2. HostAlloc / HostFree (F2) - `internal/native_host.hpp`, `native/NativeHost.cpp`

Blocco = header 32 byte `{ magic "AEM3", slot, generation, reserved, size, check }` + byte utente (allineati a 16) + canary di coda 8 byte. `check` e canary dipendono da indirizzo + slot + generation + size.

| Regola | Valore |
|---|---|
| per blocco | 16 MiB |
| per modulo (vivi) | 64 MiB |
| globale (vivi) | 256 MiB, 16384 blocchi vivi |
| registro | tabella fissa open addressing (32768, cancellazione backward-shift) + ring di 256 free recenti; **consultato prima di ogni dereferenziazione** |

Esiti `host_free` (`HostFreeResult`): `Freed`, `Null` (OK); rifiuto = no-op + log `ERROR` + modulo in **Error** (`native_host_free_<causa>`):
`no_crossing`, `bad_handle` (spoof), `orphan` (puntatore mai restituito da `host_alloc`: stack, heap del modulo, puntatore interno), `stale` (già liberato = double free), `foreign` (blocco di un altro modulo/generation), `cookie_corrupt` (header o coda sovrascritti; blocco **abbandonato**, mai restituito alla CRT).
Stop pulito: blocchi vivi recuperati con WARN (report leak). Modulo in fault: blocchi **abbandonati** (contati in `leakedBlocks`, mai liberati - memoria potenzialmente incoerente).

## 3. Tabella hook densa - `internal/packet_hooks.hpp`, `native/NativeHooks.cpp`

- `inline constinit PacketHookTable g_packetHooks`: `cmsg[1984]`, `smsg[1671]` (`NUM_CMSG_OPCODES` / `NUM_SMSG_OPCODES`), indice = copia di `GetOpcodeArrayIndex` (`Opcodes.h`; Catch2 verifica tutte le 2^20 coppie group/id in entrambe le direzioni). Nota: `GetOpcodeArrayIndex` calcola `idInGroup` come `uint32`, quindi il suo `-1` di "miss" diventa `0xFFFFFFFF` (indice positivo; Trinity si affida a `IsValid()` `< ssize(table)`). `CmsgIndex/SmsgIndex` usano un id con segno (vero `-1`) e ogni accesso alla tabella è comunque limitato a 1984/1671. Per opcode: `std::atomic<uint32_t> count`, `staged`, `HookEntry entries[4]` `{fn, moduleSlot, generation, epoch}`. **Zero heap**, nessuna map (`aeternum-adapter-nomap-lint` ora scansiona entrambi i file come hot path: niente map, `new`/`malloc`/`std::vector`/`std::string`).
- **Iscrizione solo durante `on_init`** (finestra = slot il cui `on_init` è in esecuzione). Gli hook sono **staged**, pubblicati a fine `Boot()` (release-store di `count`) e la tabella viene **congelata**: ogni chiamata successiva (`on_start`, hook, world thread) = `AETERNUM_ERR_DENIED`. Gli hook staged di un modulo che fallisce prima di Active vengono ritirati. I thread di rete sono già attivi durante `OnStartup`, da qui stage-poi-publish.
- Codici: `DENIED` (nessun crossing / congelata / fuori da `on_init` / movimento / livello socket), `INVALID_ARGUMENT` (direzione errata, fn nulla, fn fuori dall'immagine del modulo), `NOT_FOUND` (opcode senza slot denso), `OVERFLOW` (5° hook sullo stesso opcode).
- **Nessun hook di movimento (Q4):** intero gruppo CMSG `0x41` e gruppo SMSG `0x5E` negati (`CMSG_MOVE_GUILD_BANK_ITEM 0x3E004F` non è movimento). CMSG di livello socket negati: `AUTH_SESSION`, `AUTH_CONTINUED_SESSION`, `PING`, `KEEP_ALIVE`, `LOG_DISCONNECT`, `ENABLE_NAGLE`, `CONNECT_TO_FAILED`, `ENTER_ENCRYPTED_MODE_ACK`.
- **Inbound:** `WorldSession::Update`, tutti e 4 i punti handler: dopo `sScriptMgr->OnPacketReceive`, prima di `opHandle->Call` (`DispatchInbound`). Nessun nuovo hook `ScriptMgr`.
- **Outbound (Q3, solo osservazione):** `WorldSocket::SendPacket` sul pacchetto logico prima di `TranslateOutbound` (`DispatchOutbound`, `session = NULL`: `_worldSession` non viene letto senza il suo lock). Un verdetto `DROP` viene contato (`dropIgnored`) e ignorato in Blocco2.
- Fast path: `count == 0` -> un load acquire + branch. Il codice di gioco **non ha dipendenza di link** dal runtime: il dispatch passa per un puntatore a funzione atomico impostato al publish; con `AETERNUM_NATIVE_MODULE_RUNTIME=OFF` nulla pubblica mai. Assunzione immagine singola = `WITH_DYNAMIC_LINKING=OFF` (invariato).
- **Ogni crossing protetto:** per chiamata `CrossingScope` (`in_flight++` prima del check di stato, F3), ricontrollo epoch + slot `Active` + generation, `aeternum_guard_call_hook` (SEH `__try/__except` / POSIX `sigsetjmp`; su POSIX handler installati in modo persistente quando esistono hook, `sigaltstack` per thread). Percorso di fault senza heap: motivo `snprintf` a buffer fisso `native_fault:hook:<dir>:0x<opcode>:code=..:rva=..`, slot in quarantena (nessun ulteriore crossing), log differito al world thread.

## 4. Shutdown graceful su tainted (F6 / maintainer Q6)

Ogni fault contenuto (crossing di init o hook) o violazione HostFree che porta un modulo in Error: processo **tainted** + **arm** atomico one-shot. Il world thread lo consuma: `WorldScript::OnUpdate` (adapter, compilato solo con runtime nativo ON) -> `PumpGracefulShutdown(Services().process)` -> `IProcessControl::RequestShutdown(60, reason)` = percorso esistente `World::ShutdownServ`, exit `SHUTDOWN_EXIT_CODE`, i giocatori vedono il normale countdown. Nessun `TerminateProcess`/`exit`, nessuna nuova API pubblica. Seam di test: `PumpGracefulShutdown(IProcessControl*)` con fake, `ResetGracefulShutdownForTest()`.
Ordine di shutdown: unpublish hook (nuova epoch) -> slot `Stopping` -> attesa `in_flight == 0` (budget 5 s) -> `on_stop`/`on_shutdown` -> FreeLibrary solo se pulito.

## 5. QA - Catch2 `[m3][blocco2]` (`tests/modules/NativeRuntimeBlocco2.cpp`)

Nuove varianti dummy in `tests/modules/native_dummy/aeternum_m3_dummy.c`: `aeternum_m3_dummy_hooks`, `_orphan`, `_double_free` (valori opcode in `aeternum_m3_dummy_opcodes.h`, verificati con `static_assert` contro `Opcodes.h`).

| Caso | Verifiche |
|---|---|
| ABI | crescita FT solo via `struct_size`, ABI 1, view = 32 byte, entry point NULL-safe |
| Geometria | 1984/1671 slot, indice = `GetOpcodeArrayIndex` per ogni group/id, policy movimento/socket |
| HostAlloc unit | nessun crossing, accoppiamento, cap, **orfano** (stack, heap del modulo, puntatore interno), stale/double free, foreign, spoof, corruzione coda/header (abbandonato), reclaim/abandon, generation |
| Dummy orfano | il modulo libera un puntatore mai ottenuto da `host_alloc` -> no-op, Error `native_host_free_orphan`, tainted + armato |
| Dummy double free | seconda free -> `stale`, Error, tainted |
| Vanilla | nessun hook: 0 pubblicati, fast path, dispatch no-op |
| Hook puliti | 8 pubblicati; 5° emote = `OVERFLOW`; movimento/ping = `DENIED`; iscrizione in `on_start` = `DENIED`; **iscrizione forzata dopo `on_init` fallisce** (`on_init` re-invocato, chiamata lato host); dispatch in/out; DROP ignorato; shutdown pulito: unpublish + free del blocco tenuto |
| Fault in hook | scrittura nulla in un hook -> contenuta, quarantena, nessun re-entry, tainted, pump -> un solo `RequestShutdown(60, ...)`, processo vivo |
| Fault di boot | fault in `on_init` -> armato, il pump emette lo shutdown 60 s una sola volta |

Numeri: Master Report (`tests.exe "[m3][blocco2]"`, `"[m3]"`, `"[m1]"`, suite completa su `build_m3`; log `<ENGINE_ROOT>\_buildlogs\m3b2_*`). Linux: guardia + runtime + dummy `hooks` compilati con clang (zig) su una macchina Linux ed eseguiti (8 pubblicati, 4 hook emote eseguiti, SIGSEGV nell'hook contenuto, shutdown 60 s richiesto una sola volta).

## 6. Non in Blocco2

Lista opcode dichiarata nel manifest (iscrizione oggi libera dentro `on_init` salvo le deny list), `DROP` CMSG / cancel SMSG, allowlist thread-id (F4.1), ring DB MPSC, CI CMake Linux, deploy su isolata / produzione.

## 7. Rollback

Revert del commit locale Blocco2. Con `AETERNUM_NATIVE_MODULE_RUNTIME=OFF` (default) le chiamate di dispatch si riducono a un load `count == 0` per pacchetto e nulla può pubblicare.
