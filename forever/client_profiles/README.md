# Forever client profiles

**Policy:** exactly **one** active profile per deployment.

Canonical path: orever/client_profiles/<product>.<build>/

Active pointer: orever/client_profiles/ACTIVE (plain text profile id).

C++ foundation (M4): src/server/game/Forever/ (ClientProfile, ClassicOpcodeRemap).
Conf: Classic.ActiveClientProfile (optional; ExtraInserts still applied from conf).

Must-not-break gate: Catch2 [classic][profile][active][gate].

Italian: [README.it.md](README.it.md)

## IE-M2 Discovery stubs (not runtime-loaded yet)

Under `1.60.1.70790/`: `opcodes/critical.json`, `opcodes/unknown_policy.json`, `handshake/flags.json`, `layouts/gates.json`.
See `docs/m2/PROTOCOL_PROFILE_70790.md`. Loading remains `extra_inserts.conf` + conf until M2-Blocco1+.
