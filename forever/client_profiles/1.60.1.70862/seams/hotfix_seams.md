# Hotfix / data seams — profile 1.60.1.70790

| Seam | Location | Role |
|---|---|---|
| Hotfixes DB | MySQL schema `hotfixes` (prod) / `hotfixes` (isolated copy) | DB2-like overrides at runtime |
| SQL custom | `sql/custom/hotfixes/*.sql` | Versioned apply scripts |
| Base schema | `sql/base/dev/hotfixes_database.sql` | Empty/dev baseline |
| Auth build row | `auth.build_info` | Client build acceptance |
| Realm build | `realmlist.gamebuild` | Must match profile `client_build` |
| Profile auth notes | `build_auth_keys.md` | Expected keys / ports |

Changing Forever build = new profile folder + hotfix/auth expectations in that folder; do not edit an active profile in place after publish.

IT: `hotfix_seams.it.md`
