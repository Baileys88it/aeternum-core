#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Unit tests for ExtraInserts validator (stdlib unittest)."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from validate_extra_inserts import main

ROOT = Path(__file__).resolve().parents[2]
PROFILE = ROOT / "forever" / "client_profiles" / "1.60.1.70790"


class ValidateExtraInsertsTests(unittest.TestCase):
    def test_profile_extra_inserts_ok(self):
        self.assertEqual(main(["--profile", str(PROFILE)]), 0)

    def test_correct_conf_matches_profile(self):
        with tempfile.TemporaryDirectory() as td:
            conf = Path(td) / "worldserver.conf"
            conf.write_text(
                'Classic.ClientExtraInserts = "0x440071,0x440072"\n'
                'Classic.ServerExtraInserts = "0x470125"\n',
                encoding="utf-8",
            )
            self.assertEqual(main(["--profile", str(PROFILE), "--conf", str(conf)]), 0)

    def test_wrong_inserts_fail(self):
        with tempfile.TemporaryDirectory() as td:
            conf = Path(td) / "worldserver.conf"
            conf.write_text(
                'Classic.ClientExtraInserts = "0x440063,0x440064"\n'
                'Classic.ServerExtraInserts = "0x470125"\n',
                encoding="utf-8",
            )
            self.assertEqual(main(["--profile", str(PROFILE), "--conf", str(conf)]), 1)

    def test_inline_comment_fail(self):
        with tempfile.TemporaryDirectory() as td:
            conf = Path(td) / "worldserver.conf"
            conf.write_text(
                'Classic.ClientExtraInserts = "0x440071,0x440072"  # bad inline\n'
                'Classic.ServerExtraInserts = "0x470125"\n',
                encoding="utf-8",
            )
            self.assertEqual(main(["--profile", str(PROFILE), "--conf", str(conf)]), 1)


if __name__ == "__main__":
    unittest.main()
