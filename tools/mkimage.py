#!/usr/bin/env python3
"""The WAD image in the FM-1's flash: an IWAD whose bigger lumps are raw deflate. A compressed lump's
filepos has bit 31 set and points at its compressed length (uint32 LE) followed by the deflate data; its
size stays the inflated size. Lumps that do not shrink are stored as they are (read in place).
Usage: mkimage.py in.wad out.img"""
import struct, sys, zlib
sys.path.insert(0, __file__.rsplit("/", 1)[0])
from mkwad import read_wad

def deflate(b):
    c = zlib.compressobj(9, zlib.DEFLATED, -15, 9)
    return c.compress(b) + c.flush()

def main(src, out):
    L = read_wad(src)
    body, dirs, pos, packed = b"", b"", 12, 0
    for name, b in L:
        c = deflate(b) if b else b""
        if b and len(c) + 4 < len(b) * 0.9:
            blob = struct.pack("<I", len(c)) + c
            fp = pos | 0x80000000
            packed += 1
        else:
            blob, fp = b, pos
        dirs += struct.pack("<II8s", fp, len(b), name.encode()[:8])
        body += blob
        pos += len(blob)
        if pos % 4:                                     # (keep lumps 4-byte aligned for the flash reads)
            pad = 4 - pos % 4; body += b"\0" * pad; pos += pad
    img = struct.pack("<4sii", b"IWAD", len(L), 12 + len(body)) + body + dirs
    open(out, "wb").write(img)
    print(f"{out}: {len(img)/1024:.1f} KB, {packed} of {len(L)} lumps compressed")

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
