#!/usr/bin/env python3
"""Generate and validate a single-release WinSparkle appcast."""

from __future__ import annotations

import argparse
import base64
import binascii
import re
from datetime import datetime, timezone
from email.utils import format_datetime
from pathlib import Path
from urllib.parse import urlparse
import xml.etree.ElementTree as ET


SPARKLE_NAMESPACE = "http://www.andymatuschak.org/xml-namespaces/sparkle"
REPOSITORY = "XANDER-CAGE/4phone-windows-client"
DOWNLOAD_PREFIX = f"https://github.com/{REPOSITORY}/releases/download/"
VERSION_PATTERN = re.compile(r"^\d+(?:\.\d+){3}$")
DISPLAY_VERSION_PATTERN = re.compile(r"^[0-9A-Za-z][0-9A-Za-z.+_-]{0,63}$")


def parse_timestamp(value: str) -> datetime:
    normalized = value[:-1] + "+00:00" if value.endswith("Z") else value
    try:
        parsed = datetime.fromisoformat(normalized)
    except ValueError as error:
        raise argparse.ArgumentTypeError(
            "published-at must be an ISO-8601 timestamp"
        ) from error
    if parsed.tzinfo is None:
        raise argparse.ArgumentTypeError("published-at must include a timezone")
    return parsed.astimezone(timezone.utc)


def validate_https_url(value: str, *, expected_prefix: str) -> None:
    parsed = urlparse(value)
    if (
        parsed.scheme != "https"
        or parsed.hostname != "github.com"
        or not value.startswith(expected_prefix)
        or parsed.query
        or parsed.fragment
    ):
        raise ValueError(f"untrusted release URL: {value}")


def validate_signature(value: str) -> None:
    try:
        decoded = base64.b64decode(value, validate=True)
    except (binascii.Error, ValueError) as error:
        raise ValueError("signature must be valid base64") from error
    if len(decoded) != 64:
        raise ValueError("signature must contain a 64-byte Ed25519 signature")


def sparkle(name: str) -> str:
    return f"{{{SPARKLE_NAMESPACE}}}{name}"


def generate(args: argparse.Namespace) -> None:
    installer = args.installer.resolve()
    if not installer.is_file():
        raise FileNotFoundError(f"installer not found: {installer}")
    if installer.name != "4phone-setup.exe":
        raise ValueError("installer asset must be named 4phone-setup.exe")
    if not VERSION_PATTERN.fullmatch(args.version):
        raise ValueError("version must contain four numeric components")
    if not DISPLAY_VERSION_PATTERN.fullmatch(args.short_version):
        raise ValueError("short version has an unsupported format")
    if args.channel == "stable" and "beta" in args.short_version.lower():
        raise ValueError("stable appcast cannot contain a beta version")
    if args.channel == "beta" and "beta" not in args.short_version.lower():
        raise ValueError("beta appcast must contain a beta version")

    validate_signature(args.signature)
    validate_https_url(args.download_url, expected_prefix=DOWNLOAD_PREFIX)
    if not args.download_url.endswith("/4phone-setup.exe"):
        raise ValueError("download URL must target 4phone-setup.exe")
    validate_https_url(
        args.release_notes_url,
        expected_prefix=f"https://github.com/{REPOSITORY}/releases/tag/",
    )

    ET.register_namespace("sparkle", SPARKLE_NAMESPACE)
    rss = ET.Element("rss", {"version": "2.0"})
    channel = ET.SubElement(rss, "channel")
    ET.SubElement(channel, "title").text = f"4phone Windows {args.channel}"
    ET.SubElement(channel, "link").text = "https://4phone.uz"
    ET.SubElement(channel, "description").text = (
        f"Signed 4phone Windows {args.channel} updates"
    )
    ET.SubElement(channel, "language").text = "en"

    item = ET.SubElement(channel, "item")
    ET.SubElement(item, "title").text = f"4phone {args.short_version}"
    ET.SubElement(item, "pubDate").text = format_datetime(args.published_at)
    ET.SubElement(item, sparkle("releaseNotesLink")).text = (
        args.release_notes_url
    )
    ET.SubElement(
        item,
        "enclosure",
        {
            "url": args.download_url,
            "length": str(installer.stat().st_size),
            "type": "application/octet-stream",
            sparkle("version"): args.version,
            sparkle("shortVersionString"): args.short_version,
            sparkle("edSignature"): args.signature,
            sparkle("os"): "windows",
            sparkle("installerArguments"): "/SILENT /SP-",
        },
    )
    ET.SubElement(item, sparkle("minimumSystemVersion")).text = "10.0"

    ET.indent(rss, space="  ")
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(rss).write(
        output,
        encoding="utf-8",
        xml_declaration=True,
        short_empty_elements=True,
    )
    with output.open("ab") as stream:
        stream.write(b"\n")

    parsed = ET.parse(output)
    enclosure = parsed.find("./channel/item/enclosure")
    if enclosure is None:
        raise RuntimeError("generated appcast has no enclosure")
    expected = {
        "url": args.download_url,
        "length": str(installer.stat().st_size),
        sparkle("version"): args.version,
        sparkle("shortVersionString"): args.short_version,
        sparkle("edSignature"): args.signature,
    }
    for name, value in expected.items():
        if enclosure.get(name) != value:
            raise RuntimeError(f"generated appcast has invalid {name}")


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("--channel", choices=("beta", "stable"), required=True)
    result.add_argument("--version", required=True)
    result.add_argument("--short-version", required=True)
    result.add_argument("--signature", required=True)
    result.add_argument("--installer", type=Path, required=True)
    result.add_argument("--download-url", required=True)
    result.add_argument("--release-notes-url", required=True)
    result.add_argument("--published-at", type=parse_timestamp, required=True)
    result.add_argument("--output", type=Path, required=True)
    return result


if __name__ == "__main__":
    generate(parser().parse_args())
