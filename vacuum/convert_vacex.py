#!/usr/bin/env python3
"""Convert a vacex-style instance from stdin to vacuum_solver format.

Accepted vacex header forms:
- "W H" on one line
- "W" then "H" on the next line
Optional line "Board:" is ignored.

Board symbols accepted on input:
- '#': wall
- '*': dirt
- '@' or 'V' or 'S': start
- ':': charger
- ' ' or '_' or '.': empty

Output format for vacuum_solver:
- width on first line
- height on second line
- exactly h rows of w characters using # * @ : _
"""

from __future__ import annotations

import re
import sys


def fail(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    raise SystemExit(2)


def parse_dims(lines: list[str]) -> tuple[int, int, int]:
    if not lines:
        fail("empty input")

    i = 0
    while i < len(lines) and lines[i].strip() == "":
        i += 1
    if i >= len(lines):
        fail("missing dimensions")

    m = re.fullmatch(r"\s*(\d+)\s+(\d+)\s*", lines[i])
    if m:
        w = int(m.group(1))
        h = int(m.group(2))
        i += 1
    else:
        m1 = re.fullmatch(r"\s*(\d+)\s*", lines[i])
        if not m1:
            fail("first non-empty line must contain dimensions")
        w = int(m1.group(1))
        i += 1
        while i < len(lines) and lines[i].strip() == "":
            i += 1
        if i >= len(lines):
            fail("missing height line")
        m2 = re.fullmatch(r"\s*(\d+)\s*", lines[i])
        if not m2:
            fail("height line must be an integer")
        h = int(m2.group(1))
        i += 1

    if w <= 0 or h <= 0:
        fail("width and height must be positive")

    while i < len(lines) and lines[i].strip() == "":
        i += 1

    if i < len(lines) and lines[i].strip().lower() == "board:":
        i += 1

    return w, h, i


def normalize_row(row: str, w: int, y: int) -> tuple[str, int]:
    # Keep all spaces inside the row; only drop newline characters.
    row = row.rstrip("\r\n")

    if len(row) < w:
        row = row + (" " * (w - len(row)))
    elif len(row) > w:
        extra = row[w:]
        if extra.strip(" ") != "":
            fail(f"row {y} has non-space content beyond width {w}")
        row = row[:w]

    out = []
    starts = 0
    for c in row:
        if c == "#":
            out.append("#")
        elif c == "*":
            out.append("*")
        elif c in ("@", "V", "S"):
            out.append("@")
            starts += 1
        elif c == ":":
            out.append(":")
        elif c in (" ", "_", "."):
            out.append("_")
        else:
            fail(f"unexpected character {c!r} in board")

    return "".join(out), starts


def main() -> None:
    lines = sys.stdin.read().splitlines(True)
    w, h, idx = parse_dims(lines)

    if len(lines) - idx < h:
        fail(f"expected {h} board rows, found {len(lines) - idx}")

    board = []
    start_count = 0
    for y in range(h):
        norm, starts = normalize_row(lines[idx + y], w, y + 1)
        board.append(norm)
        start_count += starts

    if start_count == 0:
        fail("no start found (expected @, V, or S)")
    if start_count > 1:
        fail("multiple starts found")

    sys.stdout.write(f"{w}\n{h}\n")
    for row in board:
        sys.stdout.write(row)
        sys.stdout.write("\n")


if __name__ == "__main__":
    main()
