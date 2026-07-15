#!/usr/bin/env python3
"""Tests for release metadata generation."""

from __future__ import annotations

import base64
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "build" / "generate-appcast.py"
SPARKLE_NAMESPACE = "http://www.andymatuschak.org/xml-namespaces/sparkle"
SIGNATURE = base64.b64encode(bytes(range(64))).decode("ascii")


class GenerateAppcastTests(unittest.TestCase):
    def run_generator(
        self,
        directory: Path,
        *,
        channel: str = "stable",
        version: str = "3.22.3.7",
        short_version: str = "3.22.3-4phone.2",
        signature: str = SIGNATURE,
    ) -> subprocess.CompletedProcess[str]:
        installer = directory / "4phone-setup.exe"
        installer.write_bytes(b"test installer payload")
        output = directory / "appcast.xml"
        return subprocess.run(
            [
                sys.executable,
                str(GENERATOR),
                "--channel",
                channel,
                "--version",
                version,
                "--short-version",
                short_version,
                "--signature",
                signature,
                "--installer",
                str(installer),
                "--download-url",
                (
                    "https://github.com/XANDER-CAGE/4phone-windows-client/"
                    "releases/download/v3.22.3-4phone.2/4phone-setup.exe"
                ),
                "--release-notes-url",
                (
                    "https://github.com/XANDER-CAGE/4phone-windows-client/"
                    "releases/tag/v3.22.3-4phone.2"
                ),
                "--published-at",
                "2026-07-14T17:00:00Z",
                "--output",
                str(output),
            ],
            check=False,
            capture_output=True,
            text=True,
        )

    def test_generates_valid_stable_appcast(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            result = self.run_generator(directory)
            self.assertEqual(result.returncode, 0, result.stderr)

            output = directory / "appcast.xml"
            raw = output.read_text(encoding="utf-8")
            self.assertIn("xmlns:sparkle=", raw)
            document = ET.parse(output)
            enclosure = document.find("./channel/item/enclosure")
            self.assertIsNotNone(enclosure)
            assert enclosure is not None
            self.assertEqual(enclosure.get("length"), "22")
            self.assertEqual(
                enclosure.get(f"{{{SPARKLE_NAMESPACE}}}edSignature"),
                SIGNATURE,
            )
            self.assertEqual(
                enclosure.get(f"{{{SPARKLE_NAMESPACE}}}version"),
                "3.22.3.7",
            )

    def test_rejects_invalid_signature(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            result = self.run_generator(
                Path(temporary),
                signature=base64.b64encode(b"too short").decode("ascii"),
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("64-byte Ed25519", result.stderr)

    def test_rejects_beta_version_in_stable_channel(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            result = self.run_generator(
                Path(temporary),
                short_version="3.22.3-4phone.2-beta.1",
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("stable appcast", result.stderr)


if __name__ == "__main__":
    unittest.main()
