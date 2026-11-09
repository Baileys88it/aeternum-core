# Seam hotfix / dati — profilo 1.60.1.70790

| Seam | Dove | Ruolo |
|---|---|---|
| DB hotfixes | Schema MySQL `hotfixes` (prod) / `hotfixes` (copia) | Override tipo DB2 a runtime |
| SQL custom | `sql/custom/hotfixes/*.sql` | Script versionati |
| Schema base | `sql/base/dev/hotfixes_database.sql` | Baseline vuota/dev |
| Riga auth | `auth.build_info` | Accettazione build client |
| Build realm | `realmlist.gamebuild` | Deve coincidere con `client_build` del profilo |
| Note auth profilo | `build_auth_keys.md` | Chiavi / porte attese |

Cambio build Forever = nuova cartella profilo + aspettative hotfix/auth lì; non modificare un profilo attivo già pubblicato.

EN: `hotfix_seams.md`
