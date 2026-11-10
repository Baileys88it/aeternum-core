# Profili client Forever

**Policy:** esattamente **un** profilo attivo per deployment.

Percorso canonico: orever/client_profiles/<product>.<build>/

Puntatore attivo: orever/client_profiles/ACTIVE.

Foundation C++ (M4): src/server/game/Forever/.
Conf: Classic.ActiveClientProfile.

Gate must-not-break: Catch2 [classic][profile][active][gate].

EN: [README.md](README.md)

## Stub IE-M2 Discovery (non ancora caricati a runtime)

Profilo attivo: `1.60.1.70862` (vedi `ACTIVE`). `1.60.1.70790` è il profilo precedente, conservato come riferimento.

Sotto `1.60.1.70790/`: `opcodes/critical.json`, `opcodes/unknown_policy.json`, `handshake/flags.json`, `layouts/gates.json`.
Vedi `docs/m2/PROTOCOL_PROFILE_70790.it.md`. Il load resta `extra_inserts.conf` + conf fino a M2-Blocco1+.
