# Database layer

**Status:** Implemented

| Piece | Path | Role |
|---|---|---|
| DatabaseWorkerPool | `database/Database/DatabaseWorkerPool.*` | Thread pool + async queries |
| Prepared statements | Database env headers | Type-safe SQL |
| DBUpdater | `database/Updater/DBUpdater.*` | Applies `sql/updates` |
| Aeternum fixes | `sql/custom` | Operational Forever SQL + rollbacks |
| Metric (optional) | `common/Metric` | Influx line protocol when enabled |

Trinity Engine DBs: auth, characters, world, hotfixes on MySQL 8.0.46:3308.

**Sources:** listed above; `DatabaseLoader.*`.
