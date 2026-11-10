#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Validate Classic.ClientExtraInserts / ServerExtraInserts against a client profile."""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

KEYS = (
    "Classic.ClientExtraInserts",
    "Classic.ServerExtraInserts",
)

ASSIGN_RE = re.compile(
    r"^\s*(Classic\.(?:Client|Server)ExtraInserts)\s*=\s*(.*?)\s*$"
)
OPCODE_RE = re.compile(r"^(0[xX][0-9A-Fa-f]+|\d+)$")


def parse_opcode_list(raw: str) -> list[str]:
    raw = raw.strip()
    if len(raw) >= 2 and raw[0] == raw[-1] and raw[0] in "\"'":
        raw = raw[1:-1]
    if not raw.strip():
        return []
    out: list[str] = []
    for tok in raw.split(","):
        tok = tok.strip().strip("\"'")
        if not tok:
            continue
        out.append(tok)
    return out


def normalize_opcode(tok: str) -> int:
    return int(tok, 0)


def load_conf_values(conf_path: Path) -> tuple[dict[str, str], list[str]]:
    errors: list[str] = []
    values: dict[str, str] = {}
    text = conf_path.read_text(encoding="utf-8", errors="replace")
    for lineno, line in enumerate(text.splitlines(), 1):
        m = ASSIGN_RE.match(line)
        if not m:
            continue
        key, rhs = m.group(1), m.group(2)
        # Inline comment after value — forbidden for these keys
        # Detect `#` outside quotes
        in_quote = None
        for i, ch in enumerate(rhs):
            if ch in "\"'" and (i == 0 or rhs[i - 1] != "\\"):
                if in_quote is None:
                    in_quote = ch
                elif in_quote == ch:
                    in_quote = None
            elif ch == "#" and in_quote is None:
                errors.append(
                    f"{conf_path}:{lineno}: inline comment on {key} value line "
                    f"(ConfigMgr includes trailing text; put comments on their own # line)"
                )
                rhs = rhs[:i].rstrip()
                break
        values[key] = rhs
    return values, errors


def load_profile_expected(profile_dir: Path) -> tuple[dict[str, list[int]], list[str]]:
    errors: list[str] = []
    manifest = profile_dir / "manifest.json"
    if not manifest.is_file():
        return {}, [f"missing manifest.json in {profile_dir}"]
    data = json.loads(manifest.read_text(encoding="utf-8"))
    inserts = data.get("extra_inserts") or {}
    expected: dict[str, list[int]] = {}
    for key in KEYS:
        raw_list = inserts.get(key, [])
        try:
            expected[key] = [normalize_opcode(str(x)) for x in raw_list]
        except ValueError as exc:
            errors.append(f"manifest {key}: {exc}")
            expected[key] = []
    return expected, errors


def validate_tokens(key: str, tokens: list[str], where: str) -> list[str]:
    errors: list[str] = []
    for tok in tokens:
        if not OPCODE_RE.match(tok):
            errors.append(f"{where}: {key}: token '{tok}' is not an opcode number")
            continue
        try:
            normalize_opcode(tok)
        except ValueError:
            errors.append(f"{where}: {key}: token '{tok}' parse failed")
    return errors


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--conf", type=Path, help="worldserver.conf (or fragment) to check")
    ap.add_argument(
        "--profile",
        type=Path,
        required=True,
        help="forever/client_profiles/<id> directory",
    )
    ap.add_argument(
        "--extra-inserts-conf",
        type=Path,
        help="optional profile extra_inserts.conf to validate as well",
    )
    args = ap.parse_args(argv)

    errors: list[str] = []
    expected, err = load_profile_expected(args.profile)
    errors.extend(err)

    targets: list[tuple[str, Path, dict[str, str]]] = []
    if args.conf:
        vals, e = load_conf_values(args.conf)
        errors.extend(e)
        targets.append(("conf", args.conf, vals))
    ei = args.extra_inserts_conf
    if ei is None:
        cand = args.profile / "extra_inserts.conf"
        if cand.is_file():
            ei = cand
    if ei:
        vals, e = load_conf_values(ei)
        errors.extend(e)
        targets.append(("profile-extra_inserts", ei, vals))

    if not targets:
        errors.append("nothing to validate: pass --conf and/or ensure profile has extra_inserts.conf")

    for label, path, vals in targets:
        for key in KEYS:
            if key not in vals:
                # profile extra_inserts.conf must define both; live conf should too when checking
                if label.startswith("profile") or args.conf:
                    errors.append(f"{path}: missing {key}")
                continue
            tokens = parse_opcode_list(vals[key])
            errors.extend(validate_tokens(key, tokens, str(path)))
            try:
                got = [normalize_opcode(t) for t in tokens]
            except ValueError:
                continue
            exp = expected.get(key)
            if exp is not None and got != exp:
                errors.append(
                    f"{path}: {key}={got!r} does not match profile expected {exp!r}"
                )

    if errors:
        print("FAIL")
        for e in errors:
            print(f"  - {e}")
        return 1
    print("OK: ExtraInserts format and profile match")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:
        sys.exit(0)
    except Exception as exc:  # noqa: BLE001
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(2)
