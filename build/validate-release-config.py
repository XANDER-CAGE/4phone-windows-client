#!/usr/bin/env python3
"""Validate release tool pins and the client update public key."""

from __future__ import annotations

import base64
import binascii
import json
import re
from pathlib import Path
from urllib.parse import urlparse


ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = ROOT / "build" / "release-config.json"
UPDATE_SERVICE_PATH = ROOT / "microsip" / "FourPhoneUpdateService.cpp"
CONST_PATH = ROOT / "microsip" / "const.h"
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")


def require_https_github(url: str) -> None:
    parsed = urlparse(url)
    if (
        parsed.scheme != "https"
        or parsed.hostname != "github.com"
        or parsed.query
        or parsed.fragment
    ):
        raise ValueError(f"release tool URL is not trusted: {url}")


def quoted_define(source: str, name: str) -> str:
    match = re.search(
        rf'^\s*#define\s+{re.escape(name)}\s+"([^"]+)"',
        source,
        flags=re.MULTILINE,
    )
    if not match:
        raise ValueError(f"missing {name} in const.h")
    return match.group(1)


def validate() -> None:
    config = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    if config.get("schemaVersion") != 1:
        raise ValueError("unsupported release config schema")
    if config.get("repository") != "XANDER-CAGE/4phone-windows-client":
        raise ValueError("unexpected release repository")
    if config.get("expectedAuthenticodePublisher") != "SignPath Foundation":
        raise ValueError("unexpected Authenticode publisher")

    for section_name, url_name, hash_name in (
        ("winSparkle", "archiveUrl", "archiveSha256"),
        ("innoSetup", "installerUrl", "installerSha256"),
        ("sbomTool", "executableUrl", "executableSha256"),
    ):
        section = config[section_name]
        version = section["version"]
        url = section[url_name]
        checksum = section[hash_name]
        if not re.fullmatch(r"\d+(?:\.\d+){2}", version):
            raise ValueError(f"invalid {section_name} version")
        require_https_github(url)
        if version not in url:
            raise ValueError(f"{section_name} URL does not contain its version")
        if not SHA256_PATTERN.fullmatch(checksum):
            raise ValueError(f"invalid {section_name} SHA-256")

    public_key = config["winSparkle"]["ed25519PublicKey"]
    try:
        public_key_bytes = base64.b64decode(public_key, validate=True)
    except (binascii.Error, ValueError) as error:
        raise ValueError("WinSparkle public key is not valid base64") from error
    if len(public_key_bytes) != 32:
        raise ValueError("WinSparkle public key must contain 32 bytes")

    update_source = UPDATE_SERVICE_PATH.read_text(encoding="utf-8")
    source_key_match = re.search(
        r'kEd25519PublicKey\s*=\s*\r?\n\s*"([^"]+)";',
        update_source,
    )
    if not source_key_match or source_key_match.group(1) != public_key:
        raise ValueError("client public key differs from release config")
    if (
        "https://api.4phone.uz/api/client-updates/windows/appcast.xml"
        not in update_source
    ):
        raise ValueError("client appcast URL is missing")

    const_source = CONST_PATH.read_text(encoding="utf-8")
    display_version = quoted_define(const_source, "_GLOBAL_VERSION")
    build_version = quoted_define(const_source, "_GLOBAL_BUILD_VERSION")
    if not re.fullmatch(r"[0-9A-Za-z][0-9A-Za-z.+_-]{0,63}", display_version):
        raise ValueError("invalid display version")
    if not re.fullmatch(r"\d+(?:\.\d+){3}", build_version):
        raise ValueError("invalid build version")
    numeric_match = re.search(
        r"^\s*#define\s+_GLOBAL_VERSION_COMMA\s+"
        r"(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)",
        const_source,
        flags=re.MULTILINE,
    )
    if not numeric_match or ".".join(numeric_match.groups()) != build_version:
        raise ValueError("numeric versions in const.h differ")


if __name__ == "__main__":
    validate()
    print("Release configuration is valid.")
