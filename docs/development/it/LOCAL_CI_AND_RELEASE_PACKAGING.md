# CI locale e packaging di release (proposal)

**Data:** 2026-11-05 (Europe/Rome)  
**Branch:** `aeternum/main`  
**Stato:** Tooling locale + **proposal** di packaging. GitHub Actions restano **disabilitate**.  
**EN:** [`../en/LOCAL_CI_AND_RELEASE_PACKAGING.md`](../en/LOCAL_CI_AND_RELEASE_PACKAGING.md)

## 1) CI locale (script implementato)

Script: [`contrib/aeternum/ci/local_ci.ps1`](../../../contrib/aeternum/ci/local_ci.ps1)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File <ENGINE_ROOT>\source\contrib\aeternum\ci\local_ci.ps1
powershell -NoProfile -File ...\local_ci.ps1 -SkipBuild
powershell -NoProfile -File ...\local_ci.ps1 -Filter "[classic]"
```

Cosa fa: build incrementale RelWithDebInfo (`worldserver` / `bnetserver` / `tests`), esegue Catch2, verifica i helper runtime (`START_SERVER.bat` … `legacy.dll`).  
Cosa non fa: Actions, Trinity/`<SERVER_ROOT>`, DB `auth` di produzione, upload pubblico.

Allinearsi a [QUALITY_CHECKLIST.md](QUALITY_CHECKLIST.md) prima del push.

## 2) Proposal packaging di release (non ancora automatico)

Obiettivo: zip **lab/operator** della cartella runtime RelWithDebInfo (non un installer).

**Includere:** exe/dll (incluso `legacy.dll`), bat helper, `README.txt`, SQL setup, `*.conf.dist`, cartella `logs\` vuota.  
**Escludere:** `.conf` con secret, PEM privati, DataDir client, PDB (zip simboli opzionale), `_run\`, albero build, `.git`.

Nome suggerito: `aeternum-engine-<gitsha>-win64-relwithdebinfo.zip`.

Metadati futuri: `BUILD_INFO.txt` via CMake POST_BUILD (SHA, Boost/OpenSSL/MySQL). Firma/distribuzione: fuori scope; solo canale privato. Niente workflow Actions.

## 3) Moduli gameplay

Restano **solo proposal** — non entrare nello zip lab finché un ADR non dice altrimenti.

## 4) Passi successivi (opzionali)

1. Switch `-Package` su `local_ci.ps1`.  
2. Generare `BUILD_INFO.txt`.  
3. Actions disabilitate; CI locale sulla macchina di build prima di un tag.