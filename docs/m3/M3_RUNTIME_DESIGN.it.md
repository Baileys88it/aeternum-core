# M3 — Module Runtime: design (Discovery, SOLO DESIGN)

**Fase:** Discovery M3 (design) — decisioni chiuse da maintainer t667u; **Blocco 1 implementato** (§9), **Blocco 2 implementato** (§10) | **Data:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m3-runtime`, nato da `aeternum/main` = `a52e748262` (M1-Complete); **rebase su `feat/m2-protocol` `34ceddeba8d4`** (Q2, tag `M2-Complete` assente). Le sezioni 1-7 mantengono le citazioni della Discovery (prese su main / in SOLA LETTURA da `wt_m2`).
**Autorizzazione:** maintainer t663u. **Priorità:** STABILITÀ > CORRETTEZZA > COMPATIBILITÀ > PERFORMANCE > SEMPLICITÀ.
**Etichette:** ESISTENTE (nel tree oggi) · BRANCH-M2 (solo in `feat/m2-protocol`) · PROPOSTO (design M3, non implementato) · APERTO (decisione di maintainer).
**Da non confondere con** `docs/development/*/M3_REPORT.md` (vecchio milestone Metrics/Measurement).

---

## 0. Sintesi delle decisioni

| # | Decisione | Stato |
|---|---|---|
| T1 | Ordine FSM: Discovered → Validated (manifest + SHA-256, **nessun** LoadLibrary) → Loaded (immagine OS mappata + entry risolto) → OnLoad/OnInit/OnStart → Active (hook pubblicati) | IMPLEMENTATO (Blocco 1) |
| T2 | Il loader M3 è un percorso **nuovo** del Module Manager con handle OS. Trinity `WITH_DYNAMIC_LINKING` / `ScriptReloadMgr` **non** si usa per i moduli SDK | IMPLEMENTATO (Blocco 1) |
| T3 | Solo `on-boot-only`. Nessun FreeLibrary a caldo. FreeLibrary solo a shutdown pulito, senza fault precedenti, `in_flight == 0` | IMPLEMENTATO (Blocco 1) |
| T4 | Fail closed, allineato a `ModuleHost`: errore di load/entry/ABI/hash → Rejected/Error, server resta su; abort critico solo dove M1 già abortisce | IMPLEMENTATO (Blocco 1) |
| T5 | Registrazione dinamica solo via `aeternum_module_entry` → `AeternumModuleFT`. `IModule`/`RegisterModule` resta Level 1 statico | IMPLEMENTATO (Blocco 1) |
| T6 | Ogni attraversamento della FT avvolto da D11 (`InvokeNoexcept`) **più** SEH (Windows) / wrapper di segnali (Linux) | IMPLEMENTATO (Blocco 1) |
| T7 | Hook di pacchetto su opcode logici: inbound in `WorldSession::Update` subito prima di `opHandle->Call`; outbound attorno a `SendPacket` prima di `TranslateOutbound`; tabella preallocata, zero heap | PROPOSTO |
| — | Hot-reload | **Fuori scope M3** (opzione futura con gate, richiede la checklist §5/§6 di DYNAMIC_LOADING_AND_HOT_RELOAD.md) |
| — | Sandbox fuori processo | **Non default, non M3.** Solo hardening opzionale futuro |

---

## 1. Cosa esiste oggi (ispezionato su `aeternum/main` a52e748262)

| Fatto | Citazione | Etichetta |
|---|---|---|
| ABI C: `extern "C"`, `AETERNUM_MODULE_ABI_VERSION 1u`, codici di stato, handle opachi `AeternumPlayerHandle` / `AeternumSessionHandle` / `AeternumModuleHandle` / `AeternumMapHandle`, `AeternumStringView`, `AeternumOutBuffer` | `include/aeternum/abi/AeternumABI.h:69-102` | ESISTENTE |
| FT modulo: `struct_size`, `on_load`, `on_init`, `on_start`, `on_stop`, `on_shutdown`, `metrics_observe` | `AeternumABI.h:119-127` | ESISTENTE |
| FT host: solo `log_info` e `db_async_query` | `AeternumABI.h:129-136` | ESISTENTE |
| Entry point `aeternum_module_entry(host_abi_version, host, out)` prescritto; uguaglianza stretta `aeternum_abi_compatible` | `AeternumABI.h:144-152` | ESISTENTE (solo dichiarazione) |
| **Zero chiamanti** di `aeternum_module_entry`; **nessun** `LoadLibrary` / `dlopen` / `GetProcAddress` in `src/modules/sdk` | `git grep` su main (solo `ScriptReloadMgr.cpp:256-258`, HotSwap Trinity) | GAP |
| Guard D11: `InvokeNoexcept`, `AETERNUM_C_API_BEGIN/END` (catch `std::exception` e `...`) | `src/modules/sdk/include/aeternum/sdk/abi_guard.hpp:49-75` | ESISTENTE |
| FSM `Discovered → Validated → Loaded → Active`, qualsiasi → `Error`; transizioni illegali restituiscono false | `sdk/internal/module_manager.hpp:45-52`, `Discover` `:115`, `MarkValidated` `:130`, `MarkLoaded` `:141`, `MarkActive` `:152`, `MarkError` `:166` | ESISTENTE |
| Boot host: `TrinityOpsAdapter.cpp:142` `ModuleSdkWorldScript::OnStartup` → `HostDiscoverAndStart(ResolveManifestBootOptions())`; shutdown `:143` → `HostStopAndShutdown` | `src/modules/sdk/adapters/trinity/TrinityOpsAdapter.cpp:138-143` | ESISTENTE |
| `HostDiscoverAndStart`: avvistamento in registry → Discover (`ModuleHost.cpp:180-187`); gate manifest solo se `!planned.empty()` (`:191`); reject critico → `StopNow(Error)` (`:229-235`); `PlanStaticModules` (`:240`); `OnLoad+OnInit` → `MarkLoaded` (`:261-282`); `OnStart` → `MarkActive` (`:284-306`); rollback + abort su fallimento critico (`:264-277`) | `src/modules/sdk/src/ModuleHost.cpp:166-314` | ESISTENTE |
| Moduli oggi: `IModule` statico + `RegisterModule`, solo `ModuleRuntime::Static` | `sdk/module.hpp:25-29,65-75`; `admin_monitoring/AdminMonitoringScripts.cpp:104,193` | ESISTENTE |
| Gate manifest: id coerente, `abiVersion == AETERNUM_MODULE_ABI_VERSION`, `hot-reloadable` rifiutato, SHA-256 di `integrity.file` / `integrity.files[]` (path sotto `<manifestDir>/<id>/`) | `sdk/internal/manifest_gate.hpp:184-242`, reject hot-reload `:212-213`; hashing `sha256_file.hpp:133` | ESISTENTE |
| Integrity assente = ancora accettato in M1 (transitorio) | `manifest_gate.hpp:33` | ESISTENTE (M3 lo irrigidisce, §3) |
| `loadPolicy` default `on-boot-only` | `sdk/manifest.hpp:55-72` | ESISTENTE |
| Lint ABI: nessun header STL sotto `include/aeternum/abi/**` | `contrib/aeternum/ci/check_abi_no_stl.ps1` | ESISTENTE |
| Ring MPSC lock-free per callback DB, `DbCallbackRing8192` | `sdk/db_callback_ring.hpp:55-142` | ESISTENTE (non ancora collegato a una FT host) |
| `ConfigReloadHook` = slot puntatore a funzione C (già C-safe) | `include/aeternum/api/ops/services.hpp:33-43` | ESISTENTE |
| Regola P-c zero-heap + `static_assert` sulla firma di `OnPacketReceive` + tetto 100 pacchetti/sessione/tick | `src/server/game/Server/WorldSession.cpp:386-403` | ESISTENTE |
| `ScriptMgr::OnPacketReceive` copia il pacchetto (`WorldPacket copy(packet)`) se c'è uno ServerScript registrato | `src/server/game/Scripting/ScriptMgr.cpp:1494-1500` | ESISTENTE (debito) |
| D4: nessun event bus su movimento/aure fino al design M3 | `DYNAMIC_LOADING_AND_HOT_RELOAD.md` §0 | DECISIONE |

**Gap che M3 colma:** un loader generico a livello OS (`.dll` / `.so`) dentro il Module Manager, un chiamante lato host di `aeternum_module_entry`, i trampolini FT con contenimento dei fault, una tabella di hook sui pacchetti e la superficie ABI per la sottoscrizione agli hook. Oggi nessuno di questi esiste.

**Due semi-ABI (documentate, non risolte ora):** `IModule` statico (vtable C++, Level 1, stesso binario) e la `AeternumModuleFT` C (dinamica). M3 **non** costruisce il ponte; un adattatore opzionale statico→FT può arrivare dopo. Nessuna vtable C++ attraversa il confine DLL.

---

## 2. Caricamento dinamico a livello OS (A)

### 2.1 Discovery
- PROPOSTO: oltre all'avvistamento in registry (`ModuleHost.cpp:180-187`), scansione di `<manifestDir>/<id>/module.manifest.json` (stessa radice di `ManifestBootOptions::manifestDir`, default `modules`, `manifest_gate.hpp:59-63`) per i manifest con `"runtime": "native"`. File visto → `Discover(id, critical)`. **Nessuna mappatura** in questo passo.
- Entrambi i tipi (statici e nativi) alimentano lo stesso `PlanStaticModules` (dipendenze, profili, cicli; `static_runtime.hpp:58`).
- Zero moduli nativi + zero statici = boot vanilla invariato (`ModuleHost.cpp:191`).

### 2.2 Validated (nessun LoadLibrary)
Parse del manifest + allowlist dei path (`IsSafeIntegrityRelativePath`) + SHA-256 di **ogni** file dichiarato + `abiVersion` + `loadPolicy` (default `on-boot-only`) + plan. **Sul percorso nativo un hash mancante è Rejected** (più severo di M1, dove l'integrity assente è ancora accettata, `manifest_gate.hpp:33`). Il binario nativo deve essere una delle voci di `integrity.files[]`. Un modulo rifiutato **non** arriva mai a LoadLibrary/dlopen.

### 2.3 Loaded (T1)
1. Mappare **lo stesso path canonico appena hashato**:
   - Windows: `LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)` (nessuna ricerca in PATH/CWD).
   - Linux: `dlopen(path, RTLD_NOW | RTLD_LOCAL)` (simboli non risolti falliscono al load, nessuna interposizione).
2. Risolvere **solo** `aeternum_module_entry` (`GetProcAddress` / `dlsym`). Assente → Error.
3. Chiamare l'entry dentro il wrapper F1 / D11 (§5) con `AETERNUM_MODULE_ABI_VERSION` e la FT host.
4. Verificare `out.info.abi_version` (stretto, `aeternum_abi_compatible`), `out.ft.struct_size`, `out.info.id == id del manifest`, `load_policy` e `critical` coerenti col manifest. Incoerenza → Error.
5. `MarkLoaded`. **Gli hook non sono ancora pubblicati.**

**Estensione semantica di "Loaded" (gli stati non vengono rinominati):** per i moduli nativi `Loaded` significa "immagine OS mappata + entry risolto e accettato". Per l'`IModule` statico resta il significato M1 "dopo OnLoad+OnInit" (`ModuleHost.cpp:281-282`).

### 2.4 Start → Active
Sequenza ModuleHost esistente attraverso la FT: `on_load`, `on_init` (sottoscrizioni hook ammesse solo qui; il modulo resta Loaded) → `on_start` → `MarkActive` + hook pubblicati all'epoch corrente (§4.4).

### 2.5 Load policy / hot-reload
- `on-boot-only` è l'unica policy nativa in M3. `runtime-restart` = caricamento al boot successivo (come in M1). `hot-reloadable` resta rifiutato (`manifest_gate.hpp:212-213`).
- Hot-reload: **fuori scope M3**; un'eventuale opzione futura deve passare la checklist §5/§6 di DYNAMIC_LOADING_AND_HOT_RELOAD.md ed è vietata per moduli che passano STL attraverso l'ABI (D3).

### 2.6 Non è l'HotSwap di Trinity (T2)
Trinity `WITH_DYNAMIC_LINKING` (`cmake/options.cmake:39`) e `ScriptReloadMgr` (`ScriptReloadMgr.cpp:256-258`) caricano librerie C++ di `ScriptObject` con STL attraverso il confine — viola D2/D3. Restano spenti. Il loader M3 è un percorso separato di proprietà del Module Manager (un handle OS per record di modulo).

### 2.7 Policy di fallimento (T4)

| Fallimento | Esito | Boot |
|---|---|---|
| Manifest invalido / hash diverso / hash mancante (nativo) | Rejected → Error | critico: abort come M1 (`ModuleHost.cpp:229-235`); non critico: skip |
| LoadLibrary / dlopen fallisce, entry assente, ABI/struct_size/id incoerenti | Error, immagine rilasciata (del modulo ha girato solo loader/static init) | non critico skip; critico come abort di lifecycle M1 |
| `on_load` / `on_init` / `on_start` fallisce o va in fault | Error | come `ModuleHost.cpp:261-303` |
| Mai `TC_LOG_FATAL` per questi casi; il server resta su salvo modulo critico | | |

---

## 3. Integrazione FSM ↔ ABI C (B)

- I record FSM restano in `ModuleManager` (`module_manager.hpp:79-192`; lì la STL è ammessa — lato host, solo boot). PROPOSTO, nuovi dati host per record: handle OS, copia di `AeternumModuleFT`, `mod_ctx`, range dell'immagine, codice di fault, contatore `in_flight`.
- **Nessuna STL al confine.** Tutto ciò che attraversa la DLL: primitivi, `AeternumStringView`, `AeternumOutBuffer`, handle opachi, puntatori a funzione C + `void* user`. `std::function` / lambda con capture **vietate** (heap + mismatch CRT). Il lint ABI (`check_abi_no_stl.ps1`) deve coprire anche il nuovo header degli hook (PROPOSTO).
- **Crescita ABI (PROPOSTO):** la sottoscrizione agli hook richiede nuove funzioni host. Opzioni: accodarle a `AeternumHostFT` protette da `struct_size` e portare `AETERNUM_MODULE_ABI_VERSION` a 2. Poiché la compatibilità è uguaglianza stretta (`AeternumABI.h:149-152`) e il gate rifiuta `abiVersion != AETERNUM_MODULE_ABI_VERSION` (`manifest_gate.hpp:207-208`), un bump invalida i manifest v1 (solo `admin.monitoring`, statico). → APERTO Q1.
- **Regole FT host (PROPOSTO):** `log_info` thread-safe; `db_async_query` solo asincrono, completamento consegnato sul world thread tramite drain di `DbCallbackRing8192` (`db_callback_ring.hpp:142`). Nessun DB sincrono dai moduli.
- **Fault → FSM** (PROPOSTO): sui percorsi caldi il trampolino non può allocare né prendere `g_mutex` (`ModuleHost.cpp:24`). Imposta un bit atomico "disabled" per modulo e un codice di fault in uno slot preallocato; il world thread lo converte in `MarkError(id, reason)` al tick successivo.

---

## 4. Event Hooking System (C)

### 4.1 Cosa vedono i moduli
Solo opcode logici (dopo l'adapter M2). Un hook prima di `TranslateInbound` vedrebbe opcode wire: **fuori scope**.

Vista POD PROPOSTA (C, niente STL):
```
AeternumPacketView { uint32 logical_opcode; uint8 const* data; uint32 size; uint32 direction; }
callback: uint32 (AETERNUM_ABI_CALL *)(void* user, AeternumSessionHandle s, AeternumPacketView const* v)
verdetto: PASS | DROP   (DROP solo con capability dichiarata)
```
La vista è in prestito: valida solo durante la chiamata, sola lettura, nessuna riscrittura del payload in M3.

### 4.2 Punto di aggancio inbound (T7)
- Catena BRANCH-M2: `WorldSocket::ReadDataHandler` → `ProtocolAdapterShadow::TranslateInbound(rawOpcode, logicalOpcode)` (`wt_m2 WorldSocket.cpp:420`) → `packet.SetOpcode(opcode)` (`:434`) → `_worldSession->QueuePacket` (`:485`). Gli opcode a livello socket (PING, AUTH_SESSION, KEEP_ALIVE, …, `:439-458`) sono gestiti prima della sessione e **non sono agganciabili**.
- **Punto di hook:** `WorldSession::Update`, subito **prima** di `opHandle->Call(this, *packet)` (siti ESISTENTI `WorldSession.cpp:441, 456, 469, 490`, dopo `AntiDOS.EvaluateOpcode`), dentro il tetto di 100 pacchetti/sessione/tick (`:386, 402-403`).
- **Non** tramite `ScriptMgr::OnPacketReceive` finché fa `WorldPacket copy(packet)` (`ScriptMgr.cpp:1494-1500`): quella copia viola P-c.
- Threading: `WorldSession::Update` gira sul world thread e, per gli opcode thread-safe, sui map thread (`MapSessionFilter`). Gli hook devono essere rientranti e non bloccanti (contratto, §5.4).

### 4.3 Punto di aggancio outbound (T7)
- Se abilitato: ispezione/annullamento sul pacchetto **logico** attorno a `SendPacket`, **prima** di `TranslateOutbound`: `WorldSession::SendPacket` (ESISTENTE `WorldSession.cpp:226`, accanto a `sScriptMgr->OnPacketSend` `:307`, prima di `m_Socket[conIdx]->SendPacket` `:310`).
- Dopo `TranslateOutbound` (`wt_m2 WorldSocket.cpp:525`, block/stub `:526-545`, compressione/cifratura in `WritePacketToBuffer` `:553+`): **solo audit**, non è un hook di modulo.
- Outbound in M3 = APERTO Q3 (solo osservazione vs annullamento).

### 4.4 Tabella degli hook (PROPOSTO)
- Due tabelle dense preallocate al boot, indicizzate dall'indice logico del core (`GetOpcodeArrayIndex`, `Opcodes.h:1081/2531`; dimensioni `NUM_CMSG_OPCODES = 1984`, `NUM_SMSG_OPCODES = 1671`).
- Per opcode: array fisso di `kMaxHooksPerOpcode` slot (proposta 4) `{fn, user, moduleIndex}` + `subCount` atomico. Tetto globale fisso di moduli (proposta 32) con bit "enabled" atomici.
- Percorso rapido: `subCount == 0` → una load + un branch. Altrimenti scansione limitata ≤ `kMaxHooksPerOpcode`. **Zero heap** anche su skip e su fault. Niente `std::unordered_map` / `std::map` (stessa regola del lint M2 `aeternum-adapter-nomap-lint`, `wt_m2 tests/CMakeLists.txt:76`).
- Sottoscrizione solo durante `on_init` (boot); solo opcode dichiarati nel manifest; tabella congelata dopo il boot.
- Pubblicazione ad `Active` con un **epoch** (Security F3); contatore `in_flight` per modulo attorno a ogni chiamata.
- Gli hook non devono fare DB sincrono, I/O o attese; DB solo tramite FT asincrona + ring MPSC.
- D4: gli opcode di movimento/aure non sono agganciabili di default → APERTO Q4.

### 4.5 Dipendenza dal merge
I punti di aggancio in `WorldSocket` / adapter esistono solo in `feat/m2-protocol`. L'interfaccia degli hook arriva con M3; il cablaggio in `WorldSession::Update` / `SendPacket` arriva quando M2 viene unito in `aeternum/main` (APERTO Q2).

---

## 5. Sicurezza e anti-crash (D) — policy di primo taglio

I finding F1/F3/F5 del Security Engineer sono integrati dove le decisioni del Tech Lead li nominano (F5 = ordine di caricamento §2, F1 = wrapper SEH/segnali, F3 = epoch/in_flight/unload). L'affinamento restante spetta al Security Engineer.

### 5.1 Ancora di fiducia
SHA-256 + Validated prima di qualsiasi LoadLibrary (§2.2). Si carica lo stesso path canonico appena hashato. PROPOSTO: tenere il file aperto con share deny-write durante hash e mappatura (Windows) per chiudere la finestra TOCTOU; equivalente Linux da definire. Costruttori statici / `DllMain` girano dentro LoadLibrary: l'SDK vieta lavoro lì e la chiamata al loader stessa gira dentro il wrapper.

### 5.2 Contenimento dei fault (F1 / D11, T6)
- Ogni attraversamento (entry, `on_*`, `metrics_observe`, hook, callback DB) passa da un trampolino host:
  - C++: `InvokeNoexcept` / `AETERNUM_C_API_*` (`abi_guard.hpp:49-75`). Il `catch` C++ da solo **non** cattura un access violation.
  - Windows: `__try/__except` in un trampolino solo C (nessun oggetto C++ da srotolare in quel frame); il filtro gestisce access violation, istruzione illegale, divisione per zero, stack overflow e anche il codice di eccezione C++ MSVC se sfugge da una funzione `extern "C"`. Sono gestiti solo i fault con indirizzo dentro l'immagine del modulo; un fault nel codice core non viene inghiottito (crash normale + dump).
  - Linux: `sigsetjmp` thread-local armato solo durante la chiamata al modulo, handler `SIGSEGV/SIGBUS/SIGFPE/SIGILL` con `sigaltstack`, PC verificato contro l'immagine del modulo (`dladdr`).
- Dopo un fault: modulo → Error, hook disabilitati, nessun rientro, il mondo continua salvo `critical` (allora stop controllato via `ProcessExit::Error`, come `ModuleHost.cpp:250`).
- **Limite onesto:** il contenimento in-process è best effort. Una scrittura selvaggia prima del fault può aver già corrotto la memoria del core. PROPOSTO: dopo ogni fault contenuto il processo è marcato "tainted" in `/modules/lifecycle` e si raccomanda un riavvio alla prima finestra utile. Un loop infinito non è interrompibile in-process; il freeze detector esistente (`MaxCoreStuckTime`) resta l'ultima linea.

### 5.3 Ownership della memoria
- Allocare e liberare dallo stesso lato. I dati che attraversano l'ABI sono in prestito (viste valide solo durante la chiamata) oppure allocati/liberati tramite allocatori ABI documentati (PROPOSTO `host_alloc/host_free` con contatori per modulo). L'heap privato del modulo resta privato.
- Rilevazione leak all'unload: contatori ≠ 0 → WARN (fattibile solo per l'allocatore host).

### 5.4 Thread
Nessun thread di proprietà di un modulo può chiamare nel loop di update del mondo o nel loop di rete. La FT host verifica il thread chiamante dove serve e restituisce `AETERNUM_ERR_DENIED`. Nessun DB sincrono su world/map thread (P-b).

### 5.5 Unload (T3, F3)
- M3 è on-boot-only: nessun FreeLibrary a caldo.
- Shutdown pulito (`HostStopAndShutdown`, `ModuleHost.cpp:316-333`): bit disabled → hook non pubblicati e slot azzerati (nuovo epoch) → attesa `in_flight == 0` → `on_stop` / `on_shutdown` → FreeLibrary/dlclose **solo** se nessun fault precedente e `in_flight == 0`.
- Dopo un fault: Error, immagine lasciata mappata di proposito (mai FreeLibrary), nessun rientro.

### 5.6 Non in M3
Sandbox fuori processo / WASM: solo hardening opzionale futuro (MODULE_SECURITY_MODEL.md §8). Il requisito di maintainer è `.dll/.so` in RAM.

---

## 6. Verifica pianificata (non implementata)
- Catch2 `[m3]`: ordine FSM (nessun LoadLibrary prima di Validated), hash mancante su nativo = Rejected, entry assente / ABI incoerente = Error con server su, fault in un hook → Error + nessun rientro, percorso rapido `subCount == 0`, nessuna heap sul percorso hook, unload solo con `in_flight == 0`.
- Lint: `check_abi_no_stl.ps1` sul nuovo header hook; lint no-map sulla tabella hook.
- Modulo di test `.dll/.so` che va in fault di proposito (solo isolata).

## 7. Domande aperte per maintainer (CHIUSE — vedi §8)
1. ABI v2 (uguaglianza stretta) vs mantenere v1 e far crescere `AeternumHostFT` solo via `struct_size`.
2. Ordine di merge: unire `feat/m2-protocol` in `aeternum/main` prima dell'implementazione M3, oppure rebase di `feat/m3-runtime` su M2.
3. Hook outbound in M3: solo osservazione, o anche annullamento.
4. D4: hook su opcode di movimento/aure ammessi in M3 (dichiarati + con budget) o ancora vietati.
5. Interruttore del runtime nativo: opzione di build e/o chiave di config, default OFF.
6. Policy dopo fault contenuto: flag "tainted" + riavvio raccomandato vs riavvio controllato automatico.
7. Ponte statico→FT per `IModule`: milestone successiva o mai.

## 8. Decisioni (maintainer t667u) — Q1–Q7 chiuse

| # | Domanda | Decisione (vincolante) |
|---|---|---|
| Q1 | ABI v2 vs v1 + `struct_size` | **ABI v1.** `AETERNUM_MODULE_ABI_VERSION` resta `1`; `AeternumHostFT` cresce solo aggiungendo campi in coda, protetti da `struct_size` (`AETERNUM_FT_HAS`) |
| Q2 | Merge M2 prima vs rebase | **Rebase di `feat/m3-runtime` su M2.** Tag `M2-Complete` assente → base = punta di `feat/m2-protocol` `34ceddeba8d4`; nessun merge in `aeternum/main`, nessun tag creato |
| Q3 | Hook outbound | **Hook SMSG solo observe** (niente cancel) |
| Q4 | Hook movimento/aure (D4) | **Nessun hook sugli opcode di movimento** |
| Q5 | Interruttore runtime nativo | **Flag di build, default ON** (Blocco3): CMake `AETERNUM_NATIVE_MODULE_RUNTIME` (+ config `Aeternum.Modules.NativeRuntime`, default 0; isolata vuota = 1) |
| Q6 | Policy fault contenuto | **Tainted + graceful restart entro 60 s** (policy). Blocco 1: Error + niente re-entry + processo vivo + flag "tainted"; il restart è un blocco successivo |
| Q7 | Ponte `IModule` statico → FT C | **Vietato in modo permanente** |

## 9. Blocco 1 — implementato (loader OS + wrapper C-ABI)

Dettagli, file e QA: [`M3_BLOCK1.it.md`](M3_BLOCK1.it.md). Sintesi:
- §2.1–2.4 / T1, T3, T4: `native_runtime.hpp` + `native/NativeRuntime.cpp` (Discovered → Validated con SHA-256 di ogni file, **nessun** mapping → `LoadLibraryExW(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)` / `dlopen(RTLD_NOW | RTLD_LOCAL)` → entry → `on_load/on_init/on_start` → Active).
- §5.2 / T6: trampolini C `native/NativeGuardWin.c` (`__try/__except`) e `native/NativeGuardPosix.c` (`sigsetjmp` + `sigaltstack`) attorno a ogni crossing di inizializzazione.
- Q1: `AeternumHostFT` + `log_error`, `runtime_tainted` (in coda, `struct_size`).
- Non ancora: tabella hook / wiring `WorldSession` (§4), `HostAlloc` (§5.3), restart 60 s (Q6), ring DB (§3). → tabella hook, HostAlloc e shutdown 60 s: Blocco 2 (§10).

## 10. Blocco 2 — implementato (HostAlloc F2 + tabella hook densa + shutdown 60 s su tainted F6)

Dettagli, file e QA: [`M3_BLOCK2.it.md`](M3_BLOCK2.it.md). Git: `aeternum/main` avanzato in fast-forward a `34ceddeba8d4`, tag annotato `M2-Complete`, `feat/m3-runtime` (Blocco1 `42c9f7ffea7d`) pushato; Blocco2 solo locale. Sintesi rispetto alle sezioni PROPOSED:
- §5.3: `host_alloc`/`host_free` in coda ad `AeternumHostFT` (ABI v1, Q1) con cookie (slot modulo + generation + size), canary di coda, cap per blocco/modulo/globale; free orfane, stale (doppie), foreign, falsificate e corrotte = no-op + log + modulo in Error. Report leak allo stop pulito.
- §4.4: `cmsg[1984]` / `smsg[1671]` densi, ≤ 4 hook per opcode, `count` atomico, zero heap, 32 slot modulo con generation; iscrizione solo in `on_init`, staged e pubblicata una volta a fine `Boot()`, tabella congelata. **Deviazione:** la regola "solo opcode dichiarati nel manifest" è rinviata (solo deny list: gruppi movimento CMSG `0x41` / SMSG `0x5E` per Q4, CMSG di livello socket).
- §4.2: dispatch inbound nei 4 punti `opHandle->Call` di `WorldSession::Update`, dopo `OnPacketReceive`. §4.3: outbound solo osservazione (Q3) in `WorldSocket::SendPacket` sul pacchetto logico prima di `TranslateOutbound` (non in `WorldSession::SendPacket`; copre ogni percorso di invio). `DROP` CMSG contato e ignorato.
- §5.2 / §5.5 / F3: ogni chiamata hook protetta (`aeternum_guard_call_hook`), `in_flight` + epoch + generation; unpublish → drain → `on_stop`.
- Q6: fault contenuto ⇒ tainted + arm one-shot ⇒ world thread `RequestShutdown(60, reason)` (`World::ShutdownServ` esistente, nessun `TerminateProcess`).
