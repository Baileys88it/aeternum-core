# IE-M3 Blocco1 - Loader a livello OS + wrapper C-ABI (.dll/.so nativi, on-boot-only)

**Milestone:** IE-M3 Blocco1 (autorizzato da maintainer, t667u)
**Data:** 2026-11-06 (Europe/Rome)
**Branch:** `feat/m3-runtime` (worktree `<ENGINE_ROOT>\wt_m3`), rebase su `feat/m2-protocol` `34ceddeba8d4` (tag `M2-Complete` assente). **Solo commit locale - nessun push.**
**Produzione / isolata:** non toccate. Albero di build: `<ENGINE_ROOT>\build_m3` (RelWithDebInfo, `WITH_DYNAMIC_LINKING=OFF`).
**EN:** [`M3_BLOCK1.md`](M3_BLOCK1.md) - **Design:** [`M3_RUNTIME_DESIGN.it.md`](M3_RUNTIME_DESIGN.it.md) (§8 decisioni, §9 stato)

## 1. Interruttore (maintainer Q5)

| Interruttore | Default | Effetto |
|---|---|---|
| CMake `AETERNUM_NATIVE_MODULE_RUNTIME` | **OFF** | OFF: `ModuleHost` non ha alcun percorso nativo (escluso in compilazione); percorso statico `IModule` e boot vanilla invariati. ON: loader linkato in `aeternum-sdk` |
| worldserver.conf `Aeternum.Modules.NativeRuntime` | **0** | Letto solo se compilato ON; 1 = scansione di `<Aeternum.Modules.ManifestDir>/<id>/` per `"runtime": "native"` |

Distinto da Trinity `WITH_DYNAMIC_LINKING` / `ScriptReloadMgr` (non usati, restano OFF). La libreria `aeternum-native-runtime` viene compilata anche per Catch2 con `BUILD_TESTING=ON` (link solo nei test; il worldserver non la riceve finché l'opzione è OFF).

## 2. Manifest (chiavi native, opzionali; assenti = statico, M1 invariato)

```json
{ "id": "my.module", "abiVersion": 1, "runtime": "native", "library": "my.module.dll", "critical": false,
  "integrity": { "files": [ { "path": "my.module.dll", "sha256": "<64 hex>" } ] } }
```
Il gate statico M1 rifiuta un manifest `"runtime": "native"` per un id compilato nel binario (`runtime_native_not_static_module`).

## 3. Ordine di caricamento (F5, fail-closed) - `native_runtime.hpp`, `native/NativeRuntime.cpp`

| FSM | Cosa succede | Mai |
|---|---|---|
| Discovered | visto `<root>/<id>/module.manifest.json` con `"runtime": "native"` (id già di un modulo statico: saltato) | nessun mapping |
| Validated | parse; `id` = cartella; `abiVersion` = 1; `hot-reloadable` rifiutato; `library` sicura + `.dll`/`.so`; **integrity obbligatoria** (hash assente = Rejected, più stretto di M1); forma ambigua = Rejected; ogni path relativo con charset allowlist, niente `..`, niente assoluti; cartella del modulo e ogni componente del path verificati contro symlink / junction / reparse point e `canonical(path) == root/id/path`; SHA-256 EVP di **ogni** file, confronto in tempo costante; l'immagine deve essere tra i file hashati. Windows: l'immagine è aperta deny-write (`FILE_SHARE_READ`), path finale ricontrollato, hash **attraverso quell'handle**, handle tenuto fino al mapping | **nessun LoadLibrary/dlopen** |
| Loaded | `LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR \| LOAD_LIBRARY_SEARCH_SYSTEM32)` / `dlopen(path, RTLD_NOW \| RTLD_LOCAL)` sullo **stesso path canonico** (Linux: dev/inode ricontrollati contro l'fd hashato); range immagine da header PE / `dl_iterate_phdr`; risolto solo `aeternum_module_entry` (assente = unload + Error); entry chiamato dentro la guardia F1; controlli: `aeternum_abi_compatible` (uguaglianza v1), `ft.struct_size` in `[AETERNUM_MODULE_FT_V1_BASE_SIZE, 4096]`, `info.id` (deve stare nell'immagine) = id del manifest, `load_policy` e `critical` = manifest, ogni puntatore FT dentro l'immagine. **Nessun hook pubblicato** (nessuna tabella hook nel Blocco1) | mai hash-dopo-load come unico controllo |
| Active | `on_load` → `on_init` → `on_start`, ognuno sotto guardia | |
| Error | motivo `native_<causa>[:dettaglio]` in `/modules/lifecycle` | mai `TC_LOG_FATAL`; solo un modulo nativo **critical** abortisce il boot (`StopNow(Error)` + rollback come M1) |

Regole di rilascio: fallimento pulito (map/entry/ABI/id/policy) = immagine rilasciata. Fault contenuto = immagine **lasciata mappata**, niente re-entry. Shutdown (`HostStopAndShutdown`, prima i nativi): `on_stop`/`on_shutdown` sotto guardia, poi FreeLibrary/dlclose solo senza fault e con `in_flight == 0`.

Per record: handle OS, lock del file, copia FT (`min(struct_size, sizeof)`), `struct_size` dichiarato, `mod_ctx`, range immagine, ultimo fault (codice + indirizzo), `faulted` (permanente), `in_flight`.

## 4. Guardia F1 - `native_guard.h`, `native/NativeGuardWin.c`, `native/NativeGuardPosix.c`

Trampolini in C puro (nessun oggetto C++ nel frame protetto) attorno a **ogni** crossing di inizializzazione: `aeternum_module_entry`, `on_load`, `on_init`, `on_start` (e `on_stop`, `on_shutdown` allo shutdown).
- Windows: `__try/__except`; il filtro accetta AV, in-page, bounds, misalignment, istruzione illegale/privilegiata, div0 e overflow interi/float, stack overflow (`_resetstkoflw` dopo il blocco) e l'eccezione C++ MSVC `0xE06D7363` solo se `ExceptionInformation[3]` è la base dell'immagine del modulo (violazione D11 del modulo). **Solo** se l'indirizzo del fault è dentro l'immagine. Mai mascherati: breakpoint, single-step, guard page, heap corruption, fail-fast/GS (`0xC0000409`).
- Linux: `sigsetjmp` thread-local armato solo durante la chiamata; `SIGSEGV/SIGBUS/SIGFPE/SIGILL` installati con `SA_SIGINFO | SA_ONSTACK` per la chiamata (disposizioni precedenti ripristinate), `sigaltstack` per thread; PC da `ucontext` confrontato con l'immagine; altrimenti inoltro alla disposizione precedente.
- Un fault nel codice core (fuori immagine) **non** viene assorbito (crash normale / dump).
- Fault contenuto → modulo Error, `faulted` permanente, niente re-entry, processo marcato **tainted** (`IsNativeRuntimeTainted()`, Host FT `runtime_tainted()`, log ERROR "graceful restart recommended (policy Q6: 60s; not automatic in Blocco1)").

**Non coperto dalla guardia (F0):** `DllMain` / costruttori statici girano dentro `LoadLibrary`, prima di qualsiasi crossing: contratto = `DllMain` banale (il dummy Catch2 ritorna `TRUE` e non fa nulla). Un crash lì è di fatto fatale - per questo l'integrità è verificata **prima** del mapping. Scritture selvagge prima del fault, corruzione heap/GS, thread del modulo, `ExitProcess`, VEH installati dal modulo e hang restano fuori dal contenimento in-process (hang = `MaxCoreStuckTime` esistente).

## 5. Host FT (maintainer Q1: ABI v1 + `struct_size`)

`AETERNUM_MODULE_ABI_VERSION` resta `1`. `AeternumHostFT` in coda: `log_error(AeternumStringView)`, `runtime_tainted(void)`. Helper in `AeternumABI.h`: `AETERNUM_FT_FIELD_END`, `AETERNUM_FT_HAS(ft, type, field)`, `AETERNUM_HOST_FT_V1_BASE_SIZE`, `AETERNUM_MODULE_FT_V1_BASE_SIZE`, `AETERNUM_FT_MAX_STRUCT_SIZE`. L'host passa `struct_size = sizeof(AeternumHostFT)`; i moduli verificano `AETERNUM_FT_HAS` prima di usare un campo aggiunto. `db_async_query` ritorna `AETERNUM_ERR_UNSUPPORTED` nel Blocco1 (ring DB non collegato). Niente STL, eccezioni o `std::function` attraverso l'ABI; `check_abi_no_stl.ps1` invariato e PASS. Nessuna API di iscrizione hook (tabella hook = blocco successivo; SMSG solo observe Q3, niente hook di movimento Q4). Nessun ponte `IModule` → FT (Q7).

## 6. QA - Catch2 `[m3][blocco1]` (`tests/modules/NativeRuntimeBlocco1.cpp`)

Immagini dummy (vere `.dll`, CRT statica, importano solo `KERNEL32.dll`): `tests/modules/native_dummy/aeternum_m3_dummy.c` compilato come `aeternum_m3_dummy_{ok,fault_init,fault_entry,bad_abi}`. Fixture sotto `build_m3/tests/m3b1_fixtures/`, hash calcolati dal test.

| Caso | Risultato |
|---|---|
| SHA-256 valido | Validated con 0 chiamate di mapping e immagine non mappata → 1 `LoadLibraryExW` → entry/on_load/on_init/on_start in ordine → Active; shutdown on_stop/on_shutdown → FreeLibrary (immagine non più mappata) |
| Digest corrotto / immagine corrotta / una cifra hex invertita | Error `native_sha256_mismatch:<file>`, **0** chiamate di mapping, nessun codice del modulo eseguito, immagine non mappata |
| Regole F5 | hash assente, integrity ambigua, immagine non hashata, library con `..` / assoluta, path extra non sicuro, hot-reloadable → Rejected con 0 mapping; il gate statico rifiuta il manifest nativo |
| Symlink di cartella che esce dalla root | `native_module_dir_reparse` (saltato con SUCCEED se l'host vieta la creazione di symlink) |
| Scrittura su null in `on_init` | `native_fault:on_init:code=0xC0000005:rva=...`, indirizzo del fault dentro l'immagine, `faulted`, tainted, `on_start` mai chiamato, crossing successivi rifiutati (niente re-entry), immagine lasciata mappata, **processo di test vivo** |
| Scrittura su null in `aeternum_module_entry` | `native_fault:entry:...`, mai Loaded, niente re-entry, processo vivo |
| Entry dichiara abi 2 | `native_abi_mismatch:2`, immagine rilasciata, non tainted |
| Mismatch critical | richiesto `abortStartup` (semantica M1), nessun FATAL |
| Zero moduli nativi / solo manifest statico | nulla scoperto, nulla mappato |

Numeri: vedi Master Report (`tests.exe "[m3][blocco1]"` e `"[m1]"` su `build_m3`).
Linux: `NativeGuardPosix.c` e il dummy sono stati compilati ed eseguiti con gcc su una macchina Linux (fault in entry/on_init contenuti due volte nello stesso processo; un fault nel core continua a crashare con SIGSEGV). La build CMake completa su Linux non è eseguita in questo blocco.

## 7. Non nel Blocco1

Tabella hook di pacchetto / wiring `WorldSession` + `SendPacket`, `HostAlloc` con cap + cookie (F2), allowlist thread-id sulle API host (F4.1, nessuna API world/map esposta finora), ring MPSC DB, graceful restart automatico a 60 s, build CI Linux, deploy produzione / isolata.

## 8. Rollback

Revert del commit locale Blocco1. Con `AETERNUM_NATIVE_MODULE_RUNTIME=OFF` (default) il binario worldserver non contiene alcun percorso nativo.
