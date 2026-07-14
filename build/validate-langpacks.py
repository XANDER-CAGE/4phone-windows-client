#!/usr/bin/env python3
"""Validate 4phone language packs against literal Translate calls."""

from __future__ import annotations

import ast
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MICROSIP_DIR = ROOT / "microsip"
PACKS = {
    "ru": MICROSIP_DIR / "langpacks" / "langpack_russian.txt",
    "uz": MICROSIP_DIR / "langpacks" / "langpack_uzbek.txt",
}
NON_TRANSLATABLE_KEYS = {
    "#",
    "*",
    "+",
    "-",
    "...",
    "0",
    "1",
    "1.",
    "2",
    "2.",
    "3",
    "3.",
    "4",
    "4.",
    "4phone",
    "5",
    "5.",
    "6",
    "6.",
    "7",
    "7.",
    "8",
    "8.",
    "9",
    "<",
    "<a>?</a>",
    "<a>Feature Codes</a>",
    "BLF",
    "C",
    "DNS SRV",
    "EC",
    "Email",
    "H.263+",
    "H.264",
    "ICE",
    "MP3",
    "Opus 2ch",
    "P",
    "R",
    "VAD",
    "VP8",
    "VP9",
    "WAV",
    "X",
    "_",
    "rport",
}

TRANSLATE_PATTERN = re.compile(
    r"Translate\s*\(\s*_T\s*\(\s*"
    r"((?:\"(?:\\.|[^\"\\])*\"\s*)+)"
    r"\)\s*\)",
    re.DOTALL,
)
RESOURCE_PATTERN = re.compile(
    r"^\s*(?:CAPTION|CONTROL|CTEXT|DEFPUSHBUTTON|GROUPBOX|"
    r"LTEXT|MENUITEM|POPUP|PUSHBUTTON|RTEXT|AUTOCHECKBOX|"
    r"AUTORADIOBUTTON)\s*"
    r"((?:\"(?:\\.|[^\"\\])*\"\s*)+)",
    re.DOTALL | re.MULTILINE,
)
STRING_PATTERN = re.compile(r"\"(?:\\.|[^\"\\])*\"")
PLACEHOLDER_PATTERN = re.compile(
    r"%(?!%)(?:[-+ #0]*\d*(?:\.\d+)?"
    r"(?:hh|h|ll|l|I64|I32|w|L)?[diuoxXfFeEgGaAcCsSpn])"
)


def unescape_langpack_value(value: str) -> str:
    result: list[str] = []
    index = 0
    while index < len(value):
        if value[index] != "\\" or index + 1 >= len(value):
            result.append(value[index])
            index += 1
            continue
        escaped = value[index + 1]
        result.append({"n": "\n", "r": "\r", "t": "\t"}.get(escaped, escaped))
        index += 2
    return "".join(result)


def load_langpack(path: Path) -> dict[str, str]:
    lines = path.read_text(encoding="utf-8-sig").splitlines()
    if not lines or lines[0] != "Language Pack":
        raise ValueError(f"{path}: invalid language pack header")

    entries: dict[str, str] = {}
    source: str | None = None
    for line in lines:
        if line.startswith("[") and line.endswith("]"):
            source = unescape_langpack_value(line[1:-1])
            continue
        if source is not None and line and not line.startswith(";"):
            entries[source] = unescape_langpack_value(line)
            source = None
    return entries


def decode_c_literals(value: str) -> str:
    return "".join(
        ast.literal_eval(token)
        for token in STRING_PATTERN.findall(value)
    )


def collect_literal_translate_keys() -> dict[str, set[Path]]:
    keys: dict[str, set[Path]] = {}
    for path in sorted(MICROSIP_DIR.rglob("*")):
        if path.suffix.lower() not in {".cpp", ".h", ".rc", ".rc2"}:
            continue
        content = path.read_text(encoding="utf-8-sig")
        pattern = (
            TRANSLATE_PATTERN
            if path.suffix.lower() in {".cpp", ".h"}
            else RESOURCE_PATTERN
        )
        for match in pattern.finditer(content):
            key = decode_c_literals(match.group(1))
            if not key or key in NON_TRANSLATABLE_KEYS:
                continue
            keys.setdefault(key, set()).add(path.relative_to(ROOT))
    return keys


def placeholders(value: str) -> list[str]:
    return PLACEHOLDER_PATTERN.findall(value.replace("%%", ""))


def main() -> int:
    packs = {code: load_langpack(path) for code, path in PACKS.items()}
    translate_keys = collect_literal_translate_keys()
    errors: list[str] = []

    for code, entries in packs.items():
        missing = sorted(set(translate_keys) - set(entries))
        for key in missing:
            locations = ", ".join(
                str(path) for path in sorted(translate_keys[key])
            )
            errors.append(f"{code}: missing [{key}] used by {locations}")

        for source, translated in entries.items():
            if placeholders(source) != placeholders(translated):
                errors.append(
                    f"{code}: placeholder mismatch for [{source}]: "
                    f"{placeholders(source)} != {placeholders(translated)}"
                )

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    print(
        "Validated "
        f"{len(translate_keys)} literal Translate keys, "
        f"{len(packs['ru'])} Russian entries, "
        f"{len(packs['uz'])} Uzbek entries"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
