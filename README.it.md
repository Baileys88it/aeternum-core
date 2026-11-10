<p align="center"><img src="docs/assets/aeternum-logo.png" alt="Aeternum" width="220"></p>

# Aeternum Engine

**English:** [README.md](README.md)

**Launcher:** [Aeternum Launcher](../../../aeternum-launcher) è il launcher client abbinato a questo motore.

Aeternum è un server World of Warcraft Forever: un processo di login Battle.net (`bnetserver`) e un processo di mondo (`worldserver`) che simulano un reame Forever Classic. Il game loop, il map updater, il sistema di script e il percorso dei contenuti MySQL arrivano da TrinityCore (GPL-2.0). Sopra quell'albero questo repository aggiunge un profilo client Forever per la build **1.60.1.70862**, un adattatore di opcode caricato al boot sul socket del mondo, un host di moduli statici, una piccola API C++ congelata, metriche locali opzionali e un server HTTP di amministrazione opzionale in loopback.

Questa pagina descrive l'albero dei sorgenti com'è. Una riga **Parziale** o **Default off** è codice che si può leggere e, dove esiste un interruttore, accendere. Non è l'affermazione che la funzione sia il percorso di produzione. Qui sotto non ci sono numeri di giocatori, benchmark né date di consegna.

## Cosa sa fare il motore

| Area | Stato | Cosa significa in questo albero |
|---|---|---|
| Processi del reame Forever | **Implementato** | `bnetserver` (porta Battle.net + Login REST) e `worldserver` (porta di gioco). Librerie condivise per rete, log, database e utilità. |
| Simulazione del mondo | **Implementato** | Sessioni, mappe, istanze, creature, gameobject, spell, combattimento, quest, loot, posta, gilde e `ScriptMgr`, derivati da TrinityCore. I contenuti arrivano da MySQL. |
| Protocollo client Forever | **Implementato**, profilo obbligatorio | Gli opcode di mondo in ingresso e in uscita passano dalle tabelle dense del Protocol Adapter. Il `worldserver.conf.dist` spedito lascia il profilo vuoto, e in quel caso il dispatch dei pacchetti è fail-closed. |
| Un solo profilo client attivo | **Implementato** | `forever/client_profiles/1.60.1.70862` è il profilo attivo e `forever/client_profiles/ACTIVE` punta a quell'id. Il profilo precedente `1.60.1.70790` resta nell'albero come riferimento. Lo hot-swap di un secondo profilo non è implementato. |
| Contenuti world SQL | **Implementato** | Database auth, characters, world e hotfixes. SQL di base in `sql/base/`, aggiornamenti in `sql/updates/`. |
| Moduli statici in-process | **Implementato** | Le cartelle sotto `src/modules/` con un `CMakeLists.txt` sono linkate in `worldserver`. Moduli spediti: l'host dei moduli e Admin & Monitoring. |
| API C++ pubblica congelata 1.0 | **Implementato**, superficie piccola | [`include/aeternum/api`](include/aeternum/api/README.md). Admin, metriche, snapshot di config, lifecycle dei moduli, tipi di protocollo. |
| Kernel moduli (manifest, FSM, SHA-256) | **Implementato** per i moduli statici | `module.manifest.json` fail-closed, stati del lifecycle, `GET /modules/lifecycle` sull'admin HTTP. |
| ABI C per moduli nativi | **Implementato** come header e loader | [`include/aeternum/abi/AeternumABI.h`](include/aeternum/abi/README.md). Il loader è compilato di default e resta inattivo finché `Aeternum.Modules.NativeRuntime = 1`. |
| Metriche locali | **Implementato**, **default off** | File 10s → 1m → 1h → 1d, budget disco di circa 75 MB, report umano degli hotspot, rollup giornaliero. |
| Admin HTTP + testo Prometheus | **Implementato**, **default off**, solo loopback | Rotte in sola lettura. Il drain operatore esiste ed è **default off**. |
| Schema di configurazione | **Implementato** su worldserver | `worldserver --check-config`. Le chiavi `Aeternum.*` sconosciute generano un warning. Le chiavi obbligatorie mancanti interrompono l'avvio. |
| Moduli nativi `.dll` / `.so` | **Parziale**, **default off** | Esistono loader, recinto sui fault, tabella di hook sui pacchetti e sorgenti di esempio. Gli esempi non sono installati nell'output di build. |
| Guest WASM (wasm3) | **Parziale** | L'host dei moduli crea una VM con fuel al boot. L'upload dal vivo esiste solo sul server HTTP di telemetria, che è **default off**. |
| Kernel ECS di mappa | **Parziale**, **default off** | Opt-in per un solo map id e un solo instance id. Quella mappa salta il corpo legacy di `Map::Update`. |
| Edge TCP in-process / mesh | **Parziale**, non avviato | La libreria è linkata. `Aeternum.Edge.Enable` è ignorato. Worldserver non apre il listener dell'edge. |
| `aeternum_gateway` | **Parziale**, solo Windows | Processo IOCP separato. Non è collegato a `worldserver.conf` e non restituisce al client i byte del backend. |
| Telemetry HTTP | **Listener implementato**, **default off**, solo loopback | `GET /api/telemetry`, `POST /api/wasm/deploy` (operatore), `POST /api/wasm/kill` (operatore). Le chiavi non sono in `worldserver.conf.dist`. |
| CI ospitata su GitHub | **Assente** | Nessun workflow del motore sotto `.github/workflows/`. La suite Catch2 è stata rimossa e `BUILD_TESTING` è forzato a off. |
| `loginconfig` / `serverconfig` | **Assente** | Le impostazioni di login sono `bnetserver.conf`. Le impostazioni di mondo sono `worldserver.conf` più `worldserver.conf.d/*.conf`. |

## Processi e game loop

Con `SERVERS` attivo (default di CMake) si costruiscono due binari:

- **`bnetserver`** accetta la connessione Battle.net e Login REST. Default in [`bnetserver.conf.dist`](src/server/bnetserver/bnetserver.conf.dist): `BattlenetPort = 1119`, `LoginREST.Port = 8081`, `BindIP = "0.0.0.0"`. I percorsi di certificato e chiave di default sono `./bnetserver.cert.pem` e `./bnetserver.key.pem`. La build copia accanto al binario i PEM di esempio presenti nell'albero. Sostituiscili per ogni reame che avvii, e non versionare chiavi private.
- **`worldserver`** accetta le connessioni di gioco ed esegue il mondo. Default in [`worldserver.conf.dist`](src/server/worldserver/worldserver.conf.dist): `WorldServerPort = 8085`, `BindIP = "0.0.0.0"`. Non esiste un target `authserver` separato.

`World::Update` guida sessioni, mappe e script. Il lavoro delle mappe può andare sul pool di worker `MapUpdater` (`src/server/game/Maps/MapUpdater.cpp`). Con le metriche attive vengono registrate la durata del tick di mondo e la profondità delle code database (`world_tick_ms`, `db_queue_login`, `db_queue_character`, `db_queue_world`, `db_queue_hotfixes`). La soglia di overrun spedita è `Metric.Threshold.world_tick_ms = 100`.

In questo albero non c'è un orchestratore multi-reame. Un processo di mondo serve il realm id della sua config.

## Protocollo e profili client

I client Forever spostano i numeri di opcode tra una build e l'altra. Aeternum isola quel cambiamento in una directory di profilo e in una tabella densa caricata una volta al boot.

L'id del profilo attivo nell'albero è **`1.60.1.70862`** (`forever/client_profiles/ACTIVE`). Il pacchetto contiene:

- `manifest.json` (id del profilo, build client, lista di integrità)
- `extra_inserts.conf` (insert di opcode client e server)
- `opcodes/opcodes.json` e `opcodes/unknown_policy.json` (coperti dall'integrità)
- note di seam sotto `seams/`

Al caricamento del mondo, `Aeternum::ProtocolAdapterShadow::LoadAtStartup` (`src/server/game/Server/Protocol/AdapterShadow.cpp`) legge `Classic.ActiveClientProfile`, risolve `forever/client_profiles/` (vedi sotto), verifica gli SHA-256 e riempie le tabelle in ingresso e in uscita. `WorldSocket` usa quelle tabelle per gli opcode client e per gli opcode server. Gli opcode `ignored_known` vengono scartati con un contatore e senza riga di log. Gli opcode sconosciuti vengono scartati, contati e segnalati con il rate limit di `unknown_policy.json`.

Se l'id del profilo è vuoto, la directory del profilo non si trova, o il caricamento fallisce, la funzione scrive una riga fatal e lascia le tabelle scariche. I pacchetti successivi vengono scartati e i pacchetti in uscita non partono. Quella funzione, da sola, non ferma il processo. Il dist spedito ha `Classic.ActiveClientProfile = ""`, quindi un mondo copiato dal dist si avvia e poi rifiuta il traffico di gioco finché non imposti il profilo.

Quando punti un reame alla 70862, imposta questi valori insieme:

| Chiave | Ruolo | Default del dist |
|---|---|---|
| `Classic.ActiveClientProfile` | Nome della directory del profilo | vuoto |
| `Classic.ProfileDataRoot` | Directory che contiene `forever/client_profiles/`. Vuoto: cerca la working directory del processo e fino a sei genitori. | vuoto |
| `Classic.ProfileExtraInserts.AutoApply` | `1` carica gli insert dal profilo, `0` usa le liste in conf, `-1` si accende quando l'id del profilo è impostato | `-1` |
| `Classic.ClientExtraInserts` / `Classic.ServerExtraInserts` | Liste di riserva. Ignorate quando il file del profilo si carica. | vuoto |

`forever/client_profiles/ACTIVE` viene confrontato con l'id in conf e un disallineamento finisce nel log. Il file puntatore, da solo, non carica l'adapter. `build_info` dell'auth e `gamebuild` del reame devono comunque coincidere con il client; il manifest del profilo registra la build **70862** come build descritta da questo pacchetto. Riavvia il mondo dopo un cambio di profilo. Lo hot-swap a runtime delle tabelle opcode non è implementato.

Controllo offline del testo ExtraInserts:

```bash
python tools/config-validator/validate_extra_inserts.py \
  --conf path/to/worldserver.conf \
  --profile forever/client_profiles/1.60.1.70862
```

`include/aeternum/api/protocol/adapter.hpp` è lo scheletro pubblico precedente. L'adapter in esecuzione è `src/aeternum/protocol_adapter/`.

## Dati e contenuti

I contenuti del mondo (creature, spawn, quest, item, hotfix e il resto dello schema world) si caricano da MySQL o da un server compatibile. Il dist del mondo richiede quattro stringhe di connessione:

- `LoginDatabaseInfo`
- `WorldDatabaseInfo`
- `CharacterDatabaseInfo`
- `HotfixDatabaseInfo`

Il dist spedisce valori segnaposto locali su `127.0.0.1` porta `3306`, con l'utente e i nomi di schema del template upstream (`auth`, `world`, `characters`, `hotfixes`). Sostituiscili prima di avviare i processi. Non versionare file `.conf` compilati.

Applica l'SQL da `sql/base/` e `sql/updates/` (`auth`, `characters`, `world`, `hotfixes`) nell'ordine consueto, poi `sql/custom/` se lo usi. `DataDir` (default del dist `"."`) deve puntare ai dati client estratti: dbc, maps, vmaps e mmaps. L'opzione CMake `TOOLS` è accesa di default e costruisce gli estrattori di map, vmap e mmap.

Per i cambiamenti di contenuto preferisci l'SQL. Un SDK di authoring di livello più alto non è in questo albero. L'SQL dei moduli non è applicato dall'updater del database di core; `DBUpdater` scansiona solo `sql/updates` e `sql/custom`.

## Configurazione

### File che esistono

| File | Quando viene letto |
|---|---|
| `worldserver.conf` | Lo copi tu da `worldserver.conf.dist`. Config principale del mondo. Su Windows il dist viene copiato accanto a `worldserver` quando `COPY_CONF` è acceso (il default). Su Unix la stessa opzione lo copia nella directory `etc` della build e lo installa in `CONF_DIR`. |
| `bnetserver.conf` | Lo copi tu da `bnetserver.conf.dist`. Stessa regola di copia per `bnetserver`. |
| `worldserver.conf.d/*.conf` | Ogni file `*.conf` in quella directory, caricato dopo il file principale. `*.conf.dist` è ignorato. |
| `modules/<id>/module.manifest.json` | Letto al boot del mondo da `Aeternum.Modules.ManifestDir`, oppure da `<directory di worldserver.conf>/modules` quando quella chiave è vuota. |

La build CMake del mondo copia `src/modules/admin_monitoring/conf/admin_monitoring.conf.dist` in `worldserver.conf.d/` e copia il manifest di quel modulo in `modules/admin.monitoring/module.manifest.json`. Rinomina il dist dell'admin in `admin_monitoring.conf` prima che le chiavi `Admin.*` abbiano effetto. Finché non lo fai, i default del codice tengono l'HTTP admin spento (`Admin.Enable` false, `Admin.Port` 0).

In questo albero non ci sono file `loginconfig` o `serverconfig`, e non c'è una regola di install che li emetta. L'albero [`modules/`](modules/) alla radice del repo (esempi nativi e `modules/wasm/mod_test`) è il sorgente di build di moduli autonomi. Non viene copiato nell'output di worldserver. La directory runtime `modules/` accanto alla config parte con il solo manifest dell'admin.

`worldserver --check-config` carica i file di conf, esegue `Trinity::ConfigSchema`, stampa i problemi ed esce senza aprire i database. `bnetserver` non ha il flag `--check-config`. Le chiavi obbligatorie, salvo override di `Config.Schema.Required`, sono `RealmID`, `DataDir`, `LoginDatabaseInfo`, `WorldDatabaseInfo` e `CharacterDatabaseInfo`. `Config.Schema.FailFast` nel codice vale true di default, quindi una chiave obbligatoria mancante interrompe l'avvio. Le chiavi sconosciute sotto `Admin.`, `Metric.`, `Crash.`, `Config.Schema.`, `Forever.`, `Aeternum.` e `Module.` sono warning quando `Config.Schema.WarnUnknown` è true (default del codice). I warning non bloccano l'avvio e non impediscono al codice di leggere la chiave.

### Interruttori e dove stanno

Lascia i default spenti finché non intendi abilitare la funzione.

**Admin & Monitoring** — le chiavi stanno in `admin_monitoring.conf.dist`. Hanno effetto solo dopo che rinomini quel file in `.conf`.

```ini
Admin.Enable = 0
Admin.BindIP = "127.0.0.1"
Admin.Port = 0
Admin.Token = ""
Admin.TokenFromEnv = "AETERNUM_ADMIN_TOKEN"
Admin.TokenMinLength = 32
Admin.OperatorTokenFromEnv = "AETERNUM_ADMIN_OPERATOR_TOKEN"
Admin.Drain.Enable = 0
Admin.Drain.MinDelaySec = 120
```

Il bind deve essere loopback (`127.0.0.1`, `::1` o `localhost`). `0.0.0.0` e `::` sono rifiutati. Un peer non di loopback è rifiutato anche dopo l'accept. Preferisci la variabile d'ambiente nominata da `Admin.TokenFromEnv`. Un `Admin.Token` non vuoto nel file produce un warning; il valore non viene scritto nel log. La lunghezza minima di default è 32. Gli auth falliti sono rate-limited (`Admin.AuthFailMax`, `Admin.AuthFailWindowSec`).

**Metriche** — le chiavi sono già in `worldserver.conf.dist`. `Metric.Enable = 0` è il default. `Metric.Sink` di default è `"local"`.

```ini
Metric.Enable = 0
Metric.Sink = "local"
Metric.Local.Directory = "Logs/metrics"
Metric.Local.BudgetMB = 75
Metric.Local.RetentionDays = 14
Metric.Local.FlushSeconds = 10
Metric.Local.MaxSeries = 256
```

`Metric.Sink` accetta anche `influx` e `both`. `Metric.ConnectionInfo` è la riga InfluxDB e resta inutilizzata finché il sink è `local`.

**Host dei moduli** — le legge l'host. Non sono scritte in `worldserver.conf.dist`. Default del codice:

| Chiave | Default | Effetto |
|---|---|---|
| `Aeternum.Modules.ManifestDir` | vuoto → `<directory della conf>/modules` | Radice dei manifest. |
| `Aeternum.Modules.RequireManifest` | `0` | `1` rifiuta un modulo statico senza manifest. `0` lo carica e scrive un warning. |
| `Aeternum.Modules.NativeRuntime` | `0` | `1` scansiona la radice dei manifest cercando `"runtime": "native"`. Viene letta solo se il binario è stato costruito con `AETERNUM_NATIVE_MODULE_RUNTIME` (default CMake **ON**). |
| `Aeternum.Modules.HotReload` | `0` | Consente a un modulo nativo il cui manifest dice `hot-reloadable` di ricaricarsi al sync point del world thread. |

`ManifestDir`, `RequireManifest` e `NativeRuntime` sono nell'allowlist dello schema. `HotReload` no, quindi impostarla produce un warning sotto `Aeternum.*`.

**Chiavi Edge nel dist del mondo** — presenti, e ignorate:

```ini
Aeternum.Edge.Enable = 0
Aeternum.Edge.BindIP = "127.0.0.1"
Aeternum.Edge.Port = 0
Aeternum.Edge.MaxSessions = 128
Aeternum.Edge.ConnectTimeoutMs = 2000
Aeternum.Edge.IdleTimeoutMs = 30000
Aeternum.Edge.BackendAddresses = ""
Aeternum.Edge.BackendPorts = ""
```

Questi nomi non sono nell'allowlist dello schema, quindi una conf copiata dal dist avvisa una volta per chiave. Se `Aeternum.Edge.Enable` è true, l'avvio del mondo scrive che la chiave è ignorata perché l'edge di trasporto è `aeternum_gateway`, e poi non fa bind. Vedi [Edge, gateway, telemetria, WASM ed ECS](#edge-gateway-telemetria-wasm-ed-ecs).

**Telemetry HTTP ed ECS** — assenti anche loro dal dist. Aggiungerle produce un warning (stanno sotto `Aeternum.` e non sono in allowlist). Default usati dal codice:

```ini
Aeternum.TelemetryHttp.Enable = 0
Aeternum.TelemetryHttp.BindIP = "127.0.0.1"
Aeternum.TelemetryHttp.Port = 0

Aeternum.ECS.Enabled = 0
Aeternum.ECS.OwnedMapId = -1
Aeternum.ECS.OwnedInstanceId = -1
Aeternum.ECS.Capacity = 1024
Aeternum.ECS.Workers = 16
```

Gli override da ambiente seguono la regola di config già esistente: una variabile `TC_` in upper snake case sostituisce il valore del file (`TC_METRIC_ENABLE`, e lo stesso schema per le altre chiavi).

## Moduli e SDK pubblico

### Cos'è un modulo oggi

Un modulo statico è una directory in `src/modules/` con il proprio `CMakeLists.txt`. `trinity_add_module` lo linka in `worldserver` e chiama il suo loader di script da `AddModuleScripts()` dopo gli script di core. I `CONFIGS` opzionali vengono copiati in `worldserver.conf.d/`. Il `MANIFEST` opzionale viene copiato in `modules/<id>/module.manifest.json`. Dettagli e l'esempio di loader sono in [`src/modules/README.md`](src/modules/README.md).

Due moduli sono collegati:

| Modulo | Opzione CMake | Default | Manifest installato |
|---|---|---|---|
| Host dei moduli `aeternum-sdk` | `BUILD_AETERNUM_MODULE_SDK` | ON | no (caricamento legacy + warning, salvo che tu richieda i manifest) |
| Admin & Monitoring | `BUILD_ADMIN_MONITORING` | ON | `admin.monitoring` versione `0.3.0`, `critical: false` |

L'host (`src/modules/sdk`) parte all'avvio del mondo. Valida i manifest, percorre il lifecycle statico (Discover → Validated → Loaded → Active, oppure Error) e allo shutdown ferma i moduli in ordine inverso. Il fallimento di un modulo critical interrompe l'avvio attraverso la porta di controllo del processo. Il fallimento di un modulo non critical disabilita quel modulo e prosegue. Gli header dell'host sono ancora draft: l'adapter Trinity sotto `src/modules/sdk/adapters/trinity` include header di gioco. La superficie congelata è `include/aeternum/api`, non quegli header draft.

Header 1.0 congelati e chi li usa:

| Header | Usato da |
|---|---|
| `status.hpp` | gli altri header pubblici |
| `ops/admin.hpp`, `ops/metrics.hpp`, `ops/runtime.hpp`, `ops/services.hpp` | modulo admin e adapter ops Trinity |
| `config/snapshot.hpp` | schema di config / `--check-config` |
| `sdk/lifecycle.hpp` | host dei moduli |
| `protocol/opcode.hpp`, `protocol/profile.hpp` | tipi di profilo e opcode |
| `protocol/adapter.hpp` | scheletro pubblico; il socket usa `src/aeternum/protocol_adapter` |
| `data_loaded.hpp`, `ecs_map.hpp` | hook di boot e driver di mappa ECS opzionale |

L'ABI C in `include/aeternum/abi/AeternumABI.h` è il confine di una libreria nativa: tipi C semplici, `aeternum_module_entry`, tabella di funzioni host controllata con `struct_size`. Un passo della build CMake esegue `contrib/aeternum/ci/check_abi_no_stl.ps1` così gli header ABI non includono la libreria standard C++. Lo script è PowerShell; un host senza PowerShell fa fallire quel target custom.

### Runtime nativo (default off)

`AETERNUM_NATIVE_MODULE_RUNTIME` di default è **ON**, quindi il loader del sistema operativo (`LoadLibraryExW` su Windows, `dlopen` su POSIX) è linkato. A runtime non si carica nulla finché `Aeternum.Modules.NativeRuntime = 1`.

Quando è impostato, l'host scansiona ogni sottodirectory della radice dei manifest. Un manifest con `"runtime": "native"` viene hashato (SHA-256 dei file elencati), controllato sulla versione ABI e caricato. La directory non deve essere un symlink o un reparse point. Un fallimento nativo critical interrompe il boot e fa rollback. Un fault contenuto marca il processo come tainted e chiede al world thread uno shutdown pulito dopo 60 secondi (`World::ShutdownServ`). Quel percorso è best-effort: una scrittura selvaggia prima del fault sta fuori dal recinto, e un loop infinito non viene interrotto. Il rilevatore di freeze `MaxCoreStuckTime` resta il watchdog del world thread.

Gli hook sui pacchetti sono una tabella densa, al massimo quattro callback per opcode, sottoscritti da `on_init` e poi congelati. Gli opcode di movimento si possono sottoscrivere. Gli opcode client che `WorldSocket` gestisce a livello di socket restano negati. Le allocazioni host sono verificate con un cookie (modulo, generation, size); una free orfana o una double free viene rifiutata.

Per eseguire un modulo nativo lo compili tu, metti `module.manifest.json` e la libreria in `<manifest dir>/<id>/` e imposti `Aeternum.Modules.NativeRuntime = 1`. I sorgenti sotto `modules/native/` (`mod_chatguard`, `mod_combat_override`, `mod_anticheat`, `mod_data_mutator`, `mod_holographic_ui`) sono progetti CMake autonomi per quell'ABI. I loro README descrivono esperimenti locali sugli hook. La build di worldserver non li installa, e non sono un set di funzioni di gameplay supportato.

`Aeternum.Modules.HotReload = 1` riguarda solo un modulo nativo che dichiara `loadPolicy` `hot-reloadable`. Il reload avviene al sync point già esistente del world thread.

## Metriche e admin HTTP

### Metriche locali

Con `Metric.Enable = 1` e `Metric.Sink` che include `local`:

1. I campioni si aggregano nelle finestre **10s → 1m → 1h → 1d** in file `.mlines` sotto `Metric.Local.Directory` (default `Logs/metrics`).
2. La cardinalità si ferma a `Metric.Local.MaxSeries` (default 256). Le label che farebbero esplodere il numero di serie non vengono tenute.
3. L'uso disco è limitato da `Metric.Local.BudgetMB` (default 75) e `Metric.Local.RetentionDays` (default 14). I file più vecchi escono per primi. Il gzip delle finestre grosse è `Metric.Local.Compress` (default 0).
4. `latest_report.txt` elenca le serie in cima e un hint breve dal catalogo metriche (`src/common/Metric/MetricHumanReport.cpp`).
5. Circa una volta ogni 24 ore di flush, `daily_YYYYMMDD.txt` registra avg / p95 / max per serie più i conteggi warn, error e fatal. Quel file rientra nello stesso budget.

Il catalogo e gli hint di soglia sono in [docs/architecture/it/METRIC_CATALOG.md](docs/architecture/it/METRIC_CATALOG.md).

### Admin HTTP

Il modulo è linkato di default. Il listener parte solo quando `Admin.Enable` è true e `Admin.Port` è diverso da zero, il che richiede il file rinominato in `worldserver.conf.d`. Rotte:

| Metodo e path | Auth | Corpo |
|---|---|---|
| `GET /health` | nessuna | liveness JSON |
| `GET /uptime` | token di lettura | secondi di uptime |
| `GET /sessions` | token di lettura | conteggi di sessione |
| `GET /modules/lifecycle` | token di lettura | id modulo, flag critical, stato FSM, motivo |
| `GET /metrics` e `GET /metrics/dense` | token di lettura | testo Prometheus (`?dense=1` oppure il path `/dense`) |
| `GET /metrics/store` | token di lettura | riepilogo dello store locale |
| `GET /report/latest` e `GET /report/daily` | token di lettura | i file di report delle metriche |
| `POST /admin/drain?delay_sec=` | token operatore | pianifica lo shutdown di `World`, solo se `Admin.Drain.Enable = 1` |

Invia il token come `Authorization: Bearer` o `X-Admin-Token`. `/health` è l'unica rotta senza autenticazione. Tutto ciò che non è GET, salvo il POST di drain, è rifiutato. Il ritardo del drain è almeno `Admin.Drain.MinDelaySec` (default 120). Il log di audit di default è `Logs/admin_audit.log`, con rotazione a 2 MB e tre file conservati. I token non vengono scritti in quel log.

I crash dump usano `Crash.Directory` (vuoto significa la directory di default del codice), `Crash.BudgetMB` (default 256) e `Crash.RetentionDays` (default 30). Quelle chiavi si applicano all'avvio del mondo anche con l'HTTP admin spento, purché siano visibili alla config. Sono documentate nel dist dell'admin; se quel file si chiama ancora `.conf.dist`, valgono i default del codice.

## Edge, gateway, telemetria, WASM ed ECS

Questi pezzi sono nell'albero dai milestone successivi. Qui sotto c'è il comportamento a runtime, non il tag del milestone.

### Edge in-process (libreria M13)

`aeternum-meshing` (`src/modules/sdk/meshing/AsyncEdge.cpp`) è un relay TCP opaco e limitato: slot di sessione fissi (tetto nel codice 1024, esempio nel dist 128), backend IPv4, frame di handoff opzionale sul link verso il backend. È linkato nell'host dei moduli. L'avvio del mondo non chiama `Start`. `Aeternum.Edge.*` non riempie quella config. I cambi mappa chiamano `NotifyMapChange`, che restituisce disabled finché altro codice non ha attaccato un edge in esecuzione. Niente in `src/server` ne attacca uno. Tratta le chiavi del dist come un esempio commentato che lo schema non ha ancora in allowlist.

### Gateway Windows (M15)

Su Windows, CMake costruisce `aeternum_gateway` da `src/modules/sdk/gateway/Main.cpp`. Non è linkato in `worldserver` e non legge `worldserver.conf`.

```text
aeternum_gateway --bind 127.0.0.1 --port 3725 --backend 127.0.0.1:8085
```

`--backend` è obbligatorio (senza, il processo esce con codice 2). Ripetilo fino a 16 volte. Gli indirizzi sono IPv4. Il bind di default della CLI è `127.0.0.1`, la porta di default è `3725`, i worker sono 4 e il tetto della tabella sessioni è 50000 (`kMaxCapacity`). Quel tetto è la dimensione della tabella, non un numero di connessioni misurato.

Cosa fa davvero il processo: accetta TCP, legge al massimo 128 byte per completamento, avvolge quei byte in un header interno di 16 byte e invia il frame a un backend scelto con lo slot modulo il numero di backend. Il thread che legge il socket del backend scarta quei byte. Non li riscrive verso il client accettato. Non è un proxy trasparente davanti a `worldserver`, e non viene avviato dal processo di mondo.

### Telemetry HTTP (default off)

`Aeternum.TelemetryHttp.Enable` di default è false. Una porta fuori da 1–65535 è rifiutata. Il listener rifiuta un bind non di loopback e rifiuta un peer non di loopback. Riutilizza i resolver dei token admin (`Admin.Token` / `Admin.TokenFromEnv` per la lettura, `Admin.OperatorToken` / `Admin.OperatorTokenFromEnv` per l'operatore). Un token deve essere di 32–256 caratteri ASCII visibili. Gli header `Origin` e `Cookie` da browser sono rifiutati.

| Metodo e path | Ruolo | Risultato |
|---|---|---|
| `GET /api/telemetry` | lettura | snapshot JSON del profiler in-process (tetto di 256 identità) più lo stato di generation dei moduli WASM |
| `POST /api/wasm/deploy` | operatore | accetta un corpo `application/wasm` o `application/octet-stream`, oppure un corpo multipart, fino a 256 KiB, e lo carica nella pipeline WASM hot |
| `POST /api/wasm/kill` | operatore | JSON `{"module","generation"}`. Imposta un flag di abort cooperativo per quella generation. L'HTTP 200 significa che il flag è stato impostato, non che il guest si è fermato. |

### WASM

A ogni avvio dell'host dei moduli il processo crea un ambiente wasm3 ( `dep/wasm3` incorporato), budget fuel di default 1.000.000 (intervallo ammesso 1..10.000.000), stack VM di 64 KiB e al massimo 1 MiB di memoria lineare. Non ci sono import WASI, filesystem o rete. Se esiste `<manifest dir>/wasm/mod_test.wasm`, quel file viene caricato ed eseguito; la build non installa un esempio. Il fallimento dell'init è una riga di error, non un abort del boot.

`modules/wasm/mod_test` è una build separata (`build.py` più un clang del WASI SDK). Il percorso di deploy hot (`/api/wasm/deploy`) carica in quella VM una pipeline con un solo export (`aeternum_boss_tick` sotto l'identità `hot/Aeternum_BossAI`). Il fuel limita i passi compilati; una callback dell'host non viene interrotta. È un interprete incorporato con un tetto di byte, non una sandbox di sistema operativo per codice non fidato.

### Kernel ECS (default off)

`Aeternum.ECS.Enabled` di default è false. Quando è true, la factory viene installata solo per la coppia `Aeternum.ECS.OwnedMapId` e `Aeternum.ECS.OwnedInstanceId` (entrambi devono essere ≥ 0 e devono coincidere con quella mappa). `Capacity` deve essere 1..4096 e `Workers` deve essere 1..16, altrimenti la factory non restituisce un driver. Il registro tiene al massimo 16 kernel mappa/istanza.

`Map::Update` chiama allora `CustomUpdate` e ritorna. Il corpo legacy dell'update di mappa non gira per quell'istanza. Le altre mappe restano sull'updater legacy. Accenderlo è un passaggio di ownership per una istanza, non un sistema in più accanto al tick di mappa normale.

## Build

Dipendenze tipiche: CMake 3.24 o più recente, una toolchain C++, Boost, OpenSSL, un client MySQL o MariaDB, e zlib. Il `CMakeLists.txt` di radice forza `BUILD_TESTING` a off. Il commento lì registra che la suite Catch2 dei milestone è stata rimossa, quindi non c'è un target `tests` da costruire.

```bash
cmake -S . -B build
cmake --build build --config RelWithDebInfo --target worldserver --target bnetserver
```

Opzioni di cache utili (default in `cmake/options.cmake` e `src/modules/sdk/CMakeLists.txt`):

| Opzione | Default | Effetto |
|---|---|---|
| `SERVERS` | ON | `worldserver` e `bnetserver` |
| `TOOLS` | ON | tool di map / vmap / mmap |
| `SCRIPTS` | `static` | script linkati nel binario di mondo |
| `COPY_CONF` | ON | copia `*.conf.dist`, il dist admin e il manifest admin nell'output |
| `BUILD_AETERNUM_MODULE_SDK` | ON | host dei moduli |
| `BUILD_ADMIN_MONITORING` | ON | modulo admin |
| `AETERNUM_NATIVE_MODULE_RUNTIME` | ON | compila il loader nativo; l'interruttore a runtime resta spento |
| `WITH_DYNAMIC_LINKING` | OFF | hot-swap degli script Trinity. Indipendente dal loader dei moduli nativi. |

Su Windows, `aeternum_gateway` si costruisce insieme al module SDK. I binari vanno in `build/bin/<Config>/` per i generatori multi-config. Il layout di install Unix usa `bin/` e il `CONF_DIR` configurato.

`contrib/aeternum/ci/local_ci.ps1` configura ancora una build e poi richiede `tests.exe`. Quel target non c'è più, quindi lo script si ferma lì. Non abilita le GitHub Actions. L'unico workflow GitHub Actions Ã¨ `.github/workflows/neutrality.yml`, che controlla che non vengano committati riferimenti a un server specifico; non costruisce `worldserver`.

## Esecuzione

1. Crea quattro schemi vuoti e applica `sql/base` più `sql/updates` per auth, characters, world e hotfixes. Punta `DataDir` ai dati client estratti.
2. Copia `bnetserver.conf.dist` in `bnetserver.conf` e `worldserver.conf.dist` in `worldserver.conf`. Imposta le quattro stringhe database, il realm id, le porte e `BindIP` sulle interfacce che intendi aprire. I default del dist ascoltano su `0.0.0.0`.
3. Imposta `Classic.ActiveClientProfile = "1.60.1.70862"`. Se la working directory del processo non è dentro questo repository (o entro sei genitori), imposta `Classic.ProfileDataRoot` alla radice dei sorgenti.
4. Metti i percorsi PEM di Login REST e di bnet su file che appartengono a questo host. I PEM di esempio copiati dalla build non sono un certificato di deploy.
5. Facoltativo: `worldserver --check-config` prima del primo avvio reale.
6. Avvia `bnetserver`, poi `worldserver`.
7. Lascia `Metric.Enable`, l'HTTP admin, il runtime nativo, l'HTTP di telemetria e l'ECS sui loro default spenti finché non hai un motivo per accenderne uno. Rinominare `admin_monitoring.conf.dist` in `.conf` è ciò che rende visibile il blocco `Admin.*`; `Admin.Enable` in quel file è spedito a `0`.

Non puntare questa build a un database o a una porta che un altro reame sta già usando, salvo che la migrazione sia deliberata.

## Documentazione

Documenti rivolti a chi opera il server e che seguono il layout sopra:

- Percorso del motore: [docs/engine/it/00_INDEX.md](docs/engine/it/00_INDEX.md)
- Launcher client: [Aeternum Launcher](../../../aeternum-launcher)
- Direzione dei profili client: [docs/architecture/it/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md](docs/architecture/it/CLIENT_PROFILE_AND_UPDATE_ISOLATION.md) e [forever/client_profiles/README.it.md](forever/client_profiles/README.it.md)
- Catalogo metriche: [docs/architecture/it/METRIC_CATALOG.md](docs/architecture/it/METRIC_CATALOG.md)
- Note del modulo admin: [docs/architecture/it/ADMIN_MONITORING_MODULE.md](docs/architecture/it/ADMIN_MONITORING_MODULE.md)
- Layout dei moduli statici: [src/modules/README.md](src/modules/README.md)
- API pubblica: [include/aeternum/api/README.md](include/aeternum/api/README.md)
- ABI C: [include/aeternum/abi/README.md](include/aeternum/abi/README.md)

Gli scritti di milestone sotto `docs/m2/`, `docs/m3/`, `docs/development/` e `docs/independent_engine/` registrano come sono atterrati i singoli blocchi. Parecchi dicono ancora "solo locale" o descrivono un default precedente. Dove non coincidono con questa pagina, questa pagina segue i sorgenti: `WorldSocket`, `worldserver.conf.dist`, `ConfigSchema.cpp` e `src/modules/sdk/CMakeLists.txt`.

## Contributi e licenza

Come contribuire (stesso modello di TrinityCore):

1. Fai il fork del repository.
2. Crea nel tuo fork un branch per ogni fix (`git checkout -b fix/descrizione-breve`).
3. Fai commit, push del branch sul tuo fork e apri una pull request verso `main`.

`main` Ã¨ protetto: nessuno ci scrive direttamente, ogni modifica passa da una pull request con una review approvata, e force push e cancellazione sono bloccati. Le modifiche devono restare neutre: niente nomi di realm, indirizzi, password, chiavi, percorsi locali o rate personalizzati, e le nuove opzioni spente di default. Dettagli, nomi dei file SQL e formato delle segnalazioni sono in [CONTRIBUTING.md](CONTRIBUTING.md). I problemi di sicurezza si segnalano come indicato in [SECURITY.md](SECURITY.md), mai in una issue pubblica.

Licenza: **GPL-2.0**. I termini sono in [COPYING](COPYING). La storia del copyright è in [AUTHORS](AUTHORS). Questo programma è software libero; puoi ridistribuirlo e/o modificarlo secondo i termini della GNU General Public License versione 2. Opera derivata da TrinityCore, anch'essa GPL-2.0. Gli header di copyright esistenti restano autoritativi.
