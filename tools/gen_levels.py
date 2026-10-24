#!/usr/bin/env python3
"""
Takes the level and script tables out of VVVVVV's source code.

VVVVVV has its rooms (Otherlevel.cpp, Spacestation2.cpp, Labclass.cpp,
WarpClass.cpp, Finalclass.cpp, Tower.cpp: ~1.1 MB of `static const short`
tables) and its scripts (Scripts.cpp, TerminalScripts.cpp: `static const
char* lines[]`) in the code. On the Game & Watch the code is copied to RAM:
these tables go to the asset pack instead (external flash, read in place).

For every source file given, this writes a copy where each table is
replaced by a lookup:

    static const short contents[] = { ... };
        -> const short* contents = vv_level_table(ID);
    static const char* lines[] = { ... }; ... filllines(lines);
        -> (the declaration removed) ... vv_fill_script(commands, ID);

and writes all the tables to one blob (vv_levels.bin, put in the pack by
vv_pack.py) and a header with its hash (the binary checks it matches).

Blob layout (little endian):
    "GWVL01\\0\\0"  u32 count  u32 hash (FNV-1a of what follows the header)
    count x (u32 offset, u32 size)   offsets from the start of the blob
    tables: shorts (int16) or scripts (u32 n, then n NUL terminated lines),
    each table 4 byte aligned.

Usage: gen_levels.py --out-dir build/gen --blob build/vv_levels.bin \\
           --header build/gen/vv_levels_gen.h src/vvvvvv/Otherlevel.cpp ...
"""
import argparse
import os
import re
import struct
import sys

MAGIC = b"GWVL01\0\0"


def skip_ws_comments(s, i):
    n = len(s)
    while i < n:
        if s[i] in " \t\r\n":
            i += 1
        elif s.startswith("//", i):
            j = s.find("\n", i)
            i = n if j < 0 else j + 1
        elif s.startswith("/*", i):
            j = s.find("*/", i + 2)
            if j < 0:
                raise ValueError("unterminated comment")
            i = j + 2
        elif s[i] == "#":
            raise ValueError("preprocessor line inside a table at %d" % i)
        else:
            break
    return i


ESCAPES = {"n": "\n", "t": "\t", "\\": "\\", '"': '"', "'": "'", "r": "\r", "0": "\0"}


def parse_string(s, i):
    assert s[i] == '"'
    i += 1
    out = []
    while True:
        c = s[i]
        if c == '"':
            return "".join(out), i + 1
        if c == "\\":
            e = s[i + 1]
            if e == "x":
                m = re.match(r"[0-9a-fA-F]+", s[i + 2:])
                out.append(chr(int(m.group(0), 16)))
                i += 2 + len(m.group(0))
                continue
            if e not in ESCAPES:
                raise ValueError("escape \\%s" % e)
            out.append(ESCAPES[e])
            i += 2
            continue
        if c == "\n":
            raise ValueError("newline in string")
        out.append(c)
        i += 1


def parse_table(s, i, kind):
    """s[i] is just after '{'. Returns (values, index after '};')."""
    vals = []
    while True:
        i = skip_ws_comments(s, i)
        if s[i] == "}":
            i += 1
            i = skip_ws_comments(s, i)
            if s[i] != ";":
                raise ValueError("expected ;")
            return vals, i + 1
        if kind == "short":
            m = re.match(r"-?\s*\d+", s[i:])
            if not m:
                raise ValueError("number expected at %r" % s[i:i + 20])
            vals.append(int(m.group(0).replace(" ", "")))
            i += len(m.group(0))
        else:
            if s[i] != '"':
                raise ValueError("string expected at %r" % s[i:i + 30])
            parts = []
            while s[i] == '"':  # adjacent literals concatenate
                txt, i = parse_string(s, i)
                parts.append(txt)
                i = skip_ws_comments(s, i)
            vals.append("".join(parts))
        i = skip_ws_comments(s, i)
        if s[i] == ",":
            i += 1
        elif s[i] != "}":
            raise ValueError("expected , or } at %r" % s[i:i + 20])


SHORT_RE = re.compile(r"static\s+const\s+short\s+(\w+)\s*\[\s*\]\s*=\s*\{")
LINES_RE = re.compile(r"static\s+const\s+char\s*\*\s*lines\s*\[\s*\]\s*=\s*\{")
FILL_RE = re.compile(r"filllines\s*\(\s*lines\s*\)\s*;")


def process(src, tables):
    out = []
    pos = 0
    while True:
        ms = SHORT_RE.search(src, pos)
        ml = LINES_RE.search(src, pos)
        cands = [m for m in (ms, ml) if m]
        if not cands:
            out.append(src[pos:])
            break
        m = min(cands, key=lambda x: x.start())
        out.append(src[pos:m.start()])
        if m is ms:
            vals, end = parse_table(src, m.end(), "short")
            tid = len(tables)
            tables.append(("short", vals))
            out.append("const short* %s = vv_level_table(%d); /* %d values */" % (m.group(1), tid, len(vals)))
            out.append("\n" * src.count("\n", m.start(), end))  # keep the line numbers
            pos = end
        else:
            vals, end = parse_table(src, m.end(), "str")
            tid = len(tables)
            tables.append(("str", vals))
            mf = FILL_RE.search(src, end)
            if not mf:
                raise ValueError("lines without filllines")
            # nothing else may use `lines` between the table and filllines()
            between = src[end:mf.start()]
            if re.search(r"\blines\b", between):
                raise ValueError("lines used before filllines")
            out.append("/* script table %d: %d lines */" % (tid, len(vals)))
            out.append("\n" * src.count("\n", m.start(), end))
            out.append(between)
            out.append("vv_fill_script(commands, %d);" % tid)
            pos = mf.end()
    return "".join(out)


def build_blob(tables):
    payloads = []
    for kind, vals in tables:
        if kind == "short":
            data = struct.pack("<%dh" % len(vals), *vals)
        else:
            data = struct.pack("<I", len(vals)) + b"".join(v.encode("utf-8") + b"\0" for v in vals)
        data += b"\0" * (-len(data) % 4)
        payloads.append(data)
    head = 16 + 8 * len(tables)
    index = b""
    off = head
    for p in payloads:
        index += struct.pack("<II", off, len(p))
        off += len(p)
    body = index + b"".join(payloads)
    h = 0x811C9DC5
    for b in body:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return MAGIC + struct.pack("<II", len(tables), h) + body, h


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--blob", required=True)
    ap.add_argument("--header", required=True)
    ap.add_argument("sources", nargs="+")
    a = ap.parse_args()
    os.makedirs(a.out_dir, exist_ok=True)
    tables = []
    for path in a.sources:
        with open(path, encoding="utf-8") as f:
            src = f.read()
        n0 = len(tables)
        try:
            gen = process(src, tables)
        except ValueError as e:
            sys.exit("gen_levels: %s: %s" % (path, e))
        gen = ('#include "vv_levels.h" /* gen_levels.py */\n#line 1 "%s"\n' % path) + gen
        out = os.path.join(a.out_dir, os.path.basename(path))
        with open(out, "w", encoding="utf-8") as f:
            f.write(gen)
        print("gen_levels: %s: %d tables" % (os.path.basename(path), len(tables) - n0))
    blob, h = build_blob(tables)
    with open(a.blob, "wb") as f:
        f.write(blob)
    with open(a.header, "w") as f:
        f.write("/* gen_levels.py */\n#define VV_LEVELS_COUNT %d\n#define VV_LEVELS_HASH 0x%08Xu\n" % (len(tables), h))
    print("gen_levels: %d tables, %d bytes, hash %08X" % (len(tables), len(blob), h))


if __name__ == "__main__":
    main()
