#!/usr/bin/env python3
"""Validate the XML formats supported by the 4phone contact importer."""

from __future__ import annotations

import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "tests" / "fixtures" / "xml"


def test_contacts() -> None:
    root = ET.parse(FIXTURES / "contacts.xml").getroot()
    assert root.tag == "contacts"
    assert root.attrib == {"refresh": "300", "silent": "1"}
    contact = root.find("contact")
    assert contact is not None
    assert contact.attrib["name"] == "Алишер & Анна"
    assert contact.attrib["comment"] == "Основной <офис>"
    assert contact.attrib["presence"] == "1"


def test_yealink() -> None:
    root = ET.parse(FIXTURES / "yealink-phonebook.xml").getroot()
    assert root.tag == "YealinkIPPhoneBook"
    unit = root.find("./Menu/Unit")
    assert unit is not None
    assert unit.attrib["Phone1"] == "101"
    assert unit.attrib["Phone3"] == "+998901110101"


def test_generic() -> None:
    root = ET.parse(FIXTURES / "generic-directory.xml").getroot()
    entries = [
        node
        for node in root.iter()
        if node.tag.casefold() in {"entry", "directoryentry"}
    ]
    assert len(entries) == 2
    phones = [
        child.text
        for child in entries[0]
        if child.tag.casefold() in {"extension", "telephone"}
    ]
    assert phones == ["102", "+998711110102", "+998901110102"]


def test_cmarkup_removed() -> None:
    source_extensions = {".cpp", ".h", ".vcxproj", ".filters"}
    remaining = []
    for path in (ROOT / "microsip").rglob("*"):
        if path.suffix.lower() not in source_extensions:
            continue
        text = path.read_text(encoding="utf-8-sig")
        if "CMarkup" in text or "Markup.h" in text or "Markup.cpp" in text:
            remaining.append(path.relative_to(ROOT))
    assert not remaining, f"CMarkup references remain: {remaining}"


def main() -> int:
    test_contacts()
    test_yealink()
    test_generic()
    test_cmarkup_removed()
    print("Validated contacts, Yealink and generic XML compatibility fixtures")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
