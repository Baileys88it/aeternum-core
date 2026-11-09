#!/usr/bin/env python3
"""
Aeternum IE-M2 Blocco1 - generate forever/client_profiles/<id>/opcodes/opcodes.json
from the in-tree source tables (read-only):

  src/server/game/Server/Protocol/Opcodes.h            enum values (core/logical ids)
  src/server/game/Server/Protocol/Opcodes.cpp          DEFINE_HANDLER / DEFINE_SERVER_OPCODE_HANDLER status
  src/server/game/Server/Protocol/ClassicClientOpcodes.inc  70009/70245 client overrides
  (this file, SERVER_* tables below)                   server overrides, blocked set, stub payloads
                                                       (IE-M2 Blocco5: moved here from ClassicOpcodes.cpp)
  forever/client_profiles/<id>/extra_inserts.conf      ExtraInserts (profile source of truth)

The translation is a faithful Python port of ClassicOpcodes::TranslateClientOpcode /
TranslateServerOpcode for ONE build (no conf / realm lookups): shift and insert indices
are passed explicitly (defaults = production values for 70790).
IE-M2 Blocco5: the C++ TranslateServerOpcode / IsServerOpcodeBlocked / GetServerStubPayload were removed;
this port + opcodes.json (flags F_BLOCKED / F_STUB + smsg_stub_payloads) are the only server-side source.

Output is deterministic (sorted, LF, no timestamps) so its SHA-256 is stable.
Usage:
  python contrib/aeternum/protocol/gen_opcode_table.py --repo . --profile 1.60.1.70790
  python ... --check      # exit 1 if the committed opcodes.json differs from a fresh generation
"""
import argparse
import json
import re
import sys
from pathlib import Path

INVALID = 0xFFFFFFFF

# HandlerStatus numeric values == aeternum::protocol::HandlerStatus (include/aeternum/api/protocol/opcode.hpp)
STATUS_MAP = {
    "STATUS_UNHANDLED": 0,
    "STATUS_IGNORED": 1,
    "STATUS_NEVER": 2,
    "STATUS_AUTHED": 3,
    "STATUS_LOGGEDIN": 4,
    "STATUS_TRANSFER": 5,
    "STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT": 6,
}

# flags (uint16) - mirrored in opcode_table.hpp
F_HANDLED = 0x0001      # status is an active session status (AUTHED/LOGGEDIN/TRANSFER/RECENTLY)
F_CRITICAL = 0x0002     # profile gate opcode (critical.json)
F_BLOCKED = 0x0004      # server opcode blocked for Classic (IsServerOpcodeBlocked)
F_STUB = 0x0008         # server opcode sent as placeholder payload (GetServerStubPayload)
F_OVERRIDE = 0x0010     # mapping comes from an explicit override table (.inc / ServerOpcodes)
F_ALIAS = 0x0020        # wire value reaches an already-mapped core via the generic fallback (not the preferred wire)
F_IGNORED_KNOWN = 0x0040  # profile ignored_known (opcodes/unknown_policy.json): drop + counter, never log

# IE-M2 Blocco5: server-side source data, moved verbatim from src/server/game/Server/Protocol/ClassicOpcodes.cpp
# (ServerOpcodes / BlockedServerOpcodes / StubServerPayloads; see git history there for the reverse-engineering notes).
# explicit retail core opcode -> Classic 70245 client opcode overrides
SERVER_OVERRIDES = {
    "SMSG_ACCOUNT_DATA_TIMES": 0x4601B5,  # same layout as retail, decoder 0x81A300
    "SMSG_GAME_TIME_UPDATE": 0x4601B6,
    "SMSG_GAME_TIME_SET": 0x4601B7,
    "SMSG_LOGIN_SET_TIME_SPEED": 0x4601B8,
    "SMSG_SERVER_TIME_OFFSET": 0x4601BF,  # decoder asserts datasize == 8
    "SMSG_MIRROR_VARS": 0x460371,  # decoder 0x83E330
    "SMSG_LAST_CATALOG_FETCH_RESPONSE": 0x460382,  # +2 region like MIRROR_VARS (decoder 0x83FCC0)
    "SMSG_QUERY_GUILD_FOLLOW_INFO_RESPONSE": 0x52002E,  # retail index - 1 (70058 guild switch reader)
}
# sent with the shifted number these hit a different Classic message (client JamClient size asserts) -> F_BLOCKED (never sent)
SERVER_BLOCKED = (
    "SMSG_HEALTH_UPDATE",  # does not exist in Classic (health is sent through UnitData)
    "SMSG_GUILD_CHALLENGE_COMPLETED",  # does not exist in Classic (guild switch has one case less)
    "SMSG_CHANNEL_LIST",  # Classic chat index 0x1D (reader rva 0xA10E70), layout not verified yet
)
# Classic layout differs; the client waits for the message -> F_STUB, this fixed placeholder payload (hex) is sent instead
SERVER_STUB_PAYLOADS = {
    "SMSG_ACCOUNT_ITEM_COLLECTION_DATA": "00000000",  # retail 10 bytes, Classic 0x460360 expects 4 (JamClient.cpp:167052)
    "SMSG_DAILY_QUESTS_RESET": "",  # retail int32 count; Classic expects no data (JamClientQuest.cpp:3723 datasize == 0)
}
MAX_STUB_PAYLOADS = 16  # == kMaxStubPayloads (opcode_table.hpp)
MAX_STUB_PAYLOAD_BYTES = 64  # == kMaxStubPayloadBytes

INTEGRITY_FILES = ["opcodes/opcodes.json", "opcodes/unknown_policy.json"]

ACTIVE_STATUSES = {"STATUS_AUTHED", "STATUS_LOGGEDIN", "STATUS_TRANSFER", "STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT"}
# client opcodes processed by WorldSocket itself before the table (AUTH_SESSION, PING, KEEP_ALIVE, ...)
CLIENT_SOCKET_STATUSES = {"STATUS_NEVER"}

# ClassicOpcodes.cpp constants
CLASSIC_CLIENT_CONNECTION_GROUP = 0x45
FIRST_RETAIL_CLIENT_GROUP = 0x2A
FIRST_RETAIL_SERVER_GROUP = 0x45
LAST_RETAIL_SERVER_GROUP = 0x6A
CHAT_RETAIL_SERVER_GROUP = 0x4A
GUILD_RETAIL_SERVER_GROUP = 0x51


def grp(op):
    return op >> 16


def with_grp(op, g):
    return (g << 16) | (op & 0xFFFF)


def parse_enum(text, prefix):
    out = {}
    for m in re.finditer(r"^\s*(%s_\w+)\s*=\s*(0x[0-9A-Fa-f]+|UNKNOWN_OPCODE)\s*," % prefix, text, re.M):
        name, val = m.group(1), m.group(2)
        out[name] = None if val == "UNKNOWN_OPCODE" else int(val, 16)
    return out


def parse_status(text, macro, prefix):
    out = {}
    for m in re.finditer(r"%s\((%s_\w+),\s*(STATUS_\w+)" % (macro, prefix), text):
        out[m.group(1)] = m.group(2)
    return out


def parse_inc(text):
    pairs = []
    for m in re.finditer(r"\{\s*(0x[0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\}", text):
        pairs.append((int(m.group(1), 16), int(m.group(2), 16)))
    return pairs


def block(text, start_marker):
    i = text.index(start_marker)
    j = text.index("};", i)
    return text[i:j]


def server_overrides(smsg_vals):
    out = {}
    for name, wire in SERVER_OVERRIDES.items():
        core = smsg_vals.get(name)
        if core is not None:
            out[core] = wire
    return out


def check_server_policy(smsg_vals):
    for name in list(SERVER_BLOCKED) + list(SERVER_STUB_PAYLOADS):
        if smsg_vals.get(name) is None:
            raise SystemExit("server policy: %s is not a defined SMSG" % name)
    both = set(SERVER_BLOCKED) & set(SERVER_STUB_PAYLOADS)
    if both:
        raise SystemExit("server policy: blocked and stub at the same time: %s" % sorted(both))
    if len(SERVER_STUB_PAYLOADS) > MAX_STUB_PAYLOADS:
        raise SystemExit("server policy: too many stub payloads")
    for name, hx_ in SERVER_STUB_PAYLOADS.items():
        if len(hx_) % 2 or len(hx_) // 2 > MAX_STUB_PAYLOAD_BYTES or any(c not in "0123456789abcdef" for c in hx_):
            raise SystemExit("server policy: bad stub payload hex for %s" % name)


def parse_inserts(conf_text, key):
    m = re.search(r'^\s*%s\s*=\s*"([^"]*)"' % re.escape(key), conf_text, re.M)
    if not m or not m.group(1).strip():
        return []
    return sorted(int(v.strip(), 16) for v in m.group(1).split(",") if v.strip())


class Translator:
    def __init__(self, client_map, server_overrides, client_inserts, server_inserts, shift, client_insert_index, main_insert_index):
        self.client_map = client_map
        self.server_overrides = server_overrides
        self.client_inserts = client_inserts
        self.server_inserts = server_inserts
        self.shift = shift
        self.client_insert_index = client_insert_index
        self.main_insert_index = main_insert_index

    # Forever::OpcodeRemap::RemoveClientExtraInserts
    def remove_client_inserts(self, op):
        below = 0
        for ins in self.client_inserts:
            if grp(ins) != grp(op):
                continue
            if ins == op:
                return INVALID
            if ins < op:
                below += 1
        return op - below

    # ClassicOpcodes::TranslateClientOpcode70245
    def client70245(self, op):
        if op in self.client_map:
            return self.client_map[op], True
        g = grp(op)
        if FIRST_RETAIL_CLIENT_GROUP + 1 <= g <= CLASSIC_CLIENT_CONNECTION_GROUP:
            return with_grp(op, g - 1), False
        return op, False

    # ClassicOpcodes::TranslateClientOpcode
    def client(self, wire):
        """returns (core, from_override, used_build_shift)"""
        op = self.remove_client_inserts(wire)
        if op == INVALID:
            return INVALID, False, False
        shifted = False
        if self.shift:
            if grp(op) == CLASSIC_CLIENT_CONNECTION_GROUP + self.shift:
                op = with_grp(op, CLASSIC_CLIENT_CONNECTION_GROUP)
                shifted = True
            if grp(op) == CLASSIC_CLIENT_CONNECTION_GROUP - 1 and (op & 0xFFFF) > self.client_insert_index:
                op -= 1
                shifted = True
        core, ovr = self.client70245(op)
        return core, ovr, shifted

    # ClassicOpcodes::TranslateServerOpcode70245
    def server70245(self, core):
        if core in self.server_overrides:
            return self.server_overrides[core], True
        g = grp(core)
        if FIRST_RETAIL_SERVER_GROUP <= g <= LAST_RETAIL_SERVER_GROUP:
            idx = core & 0xFFFF
            if g == FIRST_RETAIL_SERVER_GROUP and 0x17E <= idx <= 0x1C0:
                idx -= 1
            if g == CHAT_RETAIL_SERVER_GROUP and idx >= 4:
                idx += 2
            if g == GUILD_RETAIL_SERVER_GROUP and idx >= 0x1C:
                idx -= 1
            return ((g + 1) << 16) | idx, False
        return core, False

    # ClassicOpcodes::TranslateServerOpcode
    def server(self, core):
        op, ovr = self.server70245(core)
        if self.shift:
            g = grp(op)
            if g == FIRST_RETAIL_SERVER_GROUP + 1 and (op & 0xFFFF) >= self.main_insert_index:
                op += 1
            if FIRST_RETAIL_SERVER_GROUP + 1 <= g <= LAST_RETAIL_SERVER_GROUP + 1:
                op = with_grp(op, g + self.shift)
        for ins in self.server_inserts:  # AddServerExtraInserts (sorted ascending)
            if grp(ins) == grp(op) and ins <= op:
                op += 1
        return op, ovr


def generate(repo: Path, profile_id: str, shift: int, client_insert_index: int, main_insert_index: int):
    proto = repo / "src/server/game/Server/Protocol"
    h = (proto / "Opcodes.h").read_text(encoding="utf-8", errors="replace")
    cpp = (proto / "Opcodes.cpp").read_text(encoding="utf-8", errors="replace")
    inc = (proto / "ClassicClientOpcodes.inc").read_text(encoding="utf-8", errors="replace")
    pdir = repo / "forever/client_profiles" / profile_id
    conf = (pdir / "extra_inserts.conf").read_text(encoding="utf-8", errors="replace")
    manifest = json.loads((pdir / "manifest.json").read_text(encoding="utf-8"))
    critical_names = set((manifest.get("critical_client_opcodes") or {}).keys())
    policy = json.loads((pdir / "opcodes/unknown_policy.json").read_text(encoding="utf-8"))
    ignored = {}
    for e in policy.get("ignored_known", []):
        if e.get("direction", "cmsg") != "cmsg":
            raise SystemExit("ignored_known: only cmsg supported in Blocco1")
        ignored[int(e["logical"], 16)] = e["name"]

    cmsg = parse_enum(h, "CMSG")
    smsg = parse_enum(h, "SMSG")
    cstat = parse_status(cpp, "DEFINE_HANDLER", "CMSG")
    sstat = parse_status(cpp, "DEFINE_SERVER_OPCODE_HANDLER", "SMSG")
    inc_pairs = parse_inc(inc)
    client_map = dict(inc_pairs)
    check_server_policy(smsg)
    blocked = set(SERVER_BLOCKED)
    stubs = set(SERVER_STUB_PAYLOADS)

    tr = Translator(client_map, server_overrides(smsg),
                    parse_inserts(conf, "Classic.ClientExtraInserts"),
                    parse_inserts(conf, "Classic.ServerExtraInserts"),
                    shift, client_insert_index, main_insert_index)

    core_to_cname = {v: k for k, v in cmsg.items() if v is not None}
    for lg, nm in ignored.items():
        if lg in core_to_cname:
            raise SystemExit("ignored_known 0x%06X is a named enum opcode (%s): remove it from unknown_policy.json" % (lg, core_to_cname[lg]))
        core_to_cname[lg] = nm

    # --- client: scan every wire candidate in the Classic client group window ---
    max_idx = max((v & 0xFFFF) for v in core_to_cname) + 0x10
    wires_by_core = {}
    rows = []
    for g in range(FIRST_RETAIL_CLIENT_GROUP, CLASSIC_CLIENT_CONNECTION_GROUP + shift + 2):
        for idx in range(0, max_idx + 1):
            wire = (g << 16) | idx
            core, ovr, shifted = tr.client(wire)
            if core == INVALID or core not in core_to_cname:
                continue
            name = core_to_cname[core]
            st = cstat.get(name, "STATUS_UNHANDLED")
            flags = 0
            if core in ignored:
                flags |= F_IGNORED_KNOWN
            elif st in ACTIVE_STATUSES or st in CLIENT_SOCKET_STATUSES:
                flags |= F_HANDLED
            if name in critical_names:
                flags |= F_CRITICAL
            if ovr:
                flags |= F_OVERRIDE
            rows.append([wire, core, STATUS_MAP[st], flags, name])
            # preference: wire in the current build numbering (shift applied) > explicit override > highest wire
            wires_by_core.setdefault(core, []).append((0 if shifted else 1, 0 if ovr else 1, -wire, wire))
    # mark aliases: for a core reached by several wires only the preferred one feeds the logical->wire map
    preferred = {c: sorted(ws)[0][3] for c, ws in wires_by_core.items()}
    for r in rows:
        if preferred[r[1]] != r[0]:
            r[3] |= F_ALIAS
    rows.sort(key=lambda r: r[0])

    missing_ignored = [lg for lg in ignored if lg not in wires_by_core]
    if missing_ignored:
        raise SystemExit("ignored_known without a reachable wire: %s" % missing_ignored)

    # --- server: forward translate every defined SMSG ---
    srows = []
    for name, core in smsg.items():
        if core is None:
            continue
        wire, ovr = tr.server(core)
        st = sstat.get(name, "STATUS_UNHANDLED")
        flags = 0
        if name in blocked:
            flags |= F_BLOCKED
        if name in stubs:
            flags |= F_STUB
        if ovr:
            flags |= F_OVERRIDE
        srows.append([wire, core, STATUS_MAP[st], flags, name])
    srows.sort(key=lambda r: r[0])
    # server wire collisions (two cores -> same wire) would make the reverse map ambiguous: alias-flag the later ones
    seen = {}
    for r in srows:
        if r[0] in seen:
            r[3] |= F_ALIAS
        else:
            seen[r[0]] = r[1]

    def hx(v):
        return "0x%06X" % v

    doc = {
        "format": 1,
        "profile_id": profile_id,
        "client_build": manifest.get("client_build"),
        "generator": "contrib/aeternum/protocol/gen_opcode_table.py",
        "params": {
            "opcode_group_shift": shift,
            "client_main_group_insert_index": hx(client_insert_index),
            "server_main_group_insert_index": hx(main_insert_index),
            "client_extra_inserts": [hx(v) for v in tr.client_inserts],
            "server_extra_inserts": [hx(v) for v in tr.server_inserts],
        },
        "columns": ["wire", "logical", "status", "flags", "name"],
        "counts": {
            "cmsg": len(rows),
            "cmsg_handled": sum(1 for r in rows if r[3] & F_HANDLED and not r[3] & F_ALIAS),
            "ignored_known": sum(1 for r in rows if r[3] & F_IGNORED_KNOWN and not r[3] & F_ALIAS),
            "cmsg_aliases": sum(1 for r in rows if r[3] & F_ALIAS),
            "smsg": len(srows),
            "smsg_blocked": sum(1 for r in srows if r[3] & F_BLOCKED),
            "smsg_stub": sum(1 for r in srows if r[3] & F_STUB),
            "smsg_wire_collisions": sum(1 for r in srows if r[3] & F_ALIAS),
            "cmsg_enum": sum(1 for v in cmsg.values() if v is not None),
            "cmsg_enum_unreachable": sum(1 for c in core_to_cname if c not in wires_by_core and c not in ignored),
            "smsg_enum": sum(1 for v in smsg.values() if v is not None),
            "inc_entries": len(inc_pairs),
        },
        "ignored_known": [{"logical": hx(lg), "name": nm,
                           "wires": [hx(w) for _, _, _, w in sorted(wires_by_core[lg])]} for lg, nm in sorted(ignored.items())],
        # IE-M2 Blocco5: payload for every F_STUB row (replaces ClassicOpcodes::GetServerStubPayload)
        "smsg_stub_payloads": [{"logical": hx(smsg[nm]), "name": nm, "payload_hex": SERVER_STUB_PAYLOADS[nm]}
                               for nm in sorted(SERVER_STUB_PAYLOADS, key=lambda n: smsg[n])],
        "cmsg": rows,
        "smsg": srows,
    }
    return doc


def dump(doc):
    # one row per line: diff-friendly and deterministic
    lines = ["{"]
    keys = [k for k in doc if k not in ("cmsg", "smsg")]
    for k in keys:
        lines.append("  %s: %s," % (json.dumps(k), json.dumps(doc[k], sort_keys=True)))
    for k, last in (("cmsg", False), ("smsg", True)):
        lines.append("  %s: [" % json.dumps(k))
        rows = doc[k]
        for i, r in enumerate(rows):
            lines.append("    " + json.dumps(r, separators=(",", ":")) + ("," if i + 1 < len(rows) else ""))
        lines.append("  ]" + ("" if last else ","))
    lines.append("}")
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=".")
    ap.add_argument("--profile", default="1.60.1.70790")
    ap.add_argument("--shift", type=int, default=1, help="Classic.OpcodeGroupShift (1 for build >= 70291)")
    ap.add_argument("--client-insert-index", type=lambda s: int(s, 0), default=0x13B)
    ap.add_argument("--main-insert-index", type=lambda s: int(s, 0), default=0x98)
    ap.add_argument("--out", default=None)
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()
    repo = Path(a.repo).resolve()
    out = Path(a.out) if a.out else repo / "forever/client_profiles" / a.profile / "opcodes/opcodes.json"
    text = dump(generate(repo, a.profile, a.shift, a.client_insert_index, a.main_insert_index))
    pdir = repo / "forever/client_profiles" / a.profile
    if a.check:
        cur = out.read_bytes().decode("utf-8") if out.exists() else ""
        if cur != text:
            print("FAIL opcodes.json is stale: re-run gen_opcode_table.py", file=sys.stderr)
            return 1
        if a.out is None and not integrity_up_to_date(pdir):
            print("FAIL manifest.json integrity.files hashes stale: re-run gen_opcode_table.py", file=sys.stderr)
            return 1
        print("PASS opcodes.json + manifest integrity up to date")
        return 0
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    d = json.loads(text)
    print("wrote", out, "counts", json.dumps(d["counts"]))
    if a.out is None:
        write_integrity(pdir)
        print("updated", pdir / "manifest.json", "integrity.files", INTEGRITY_FILES)
    return 0


def sha256_file(p):
    import hashlib
    return hashlib.sha256(p.read_bytes()).hexdigest()


def integrity_up_to_date(pdir):
    m = json.loads((pdir / "manifest.json").read_text(encoding="utf-8"))
    files = (m.get("integrity") or {}).get("files") or []
    want = {f: sha256_file(pdir / f) for f in INTEGRITY_FILES}
    have = {e["path"]: e["sha256"].lower() for e in files}
    return have == want


def write_integrity(pdir):
    mp = pdir / "manifest.json"
    m = json.loads(mp.read_text(encoding="utf-8"))
    m["load_policy"] = "on-boot-only"
    m["integrity"] = {"files": [{"path": f, "sha256": sha256_file(pdir / f)} for f in INTEGRITY_FILES]}
    with open(mp, "w", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(m, indent=2, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    sys.exit(main())
