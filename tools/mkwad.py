#!/usr/bin/env python3
"""fm1doom.wad: one FreeDM arena with only what it needs (BSD-licensed Freedoom data).
The map is renamed MAP01; TEXTURE1 / PNAMES are rebuilt for the textures the map uses; flats, sprites and
the status bar are a chosen subset. Usage: mkwad.py freedm.wad MAPxx out.wad"""
import struct, sys

def read_wad(path):
    d = open(path, "rb").read()
    _, n, off = struct.unpack_from("<4sii", d, 0)
    out = []
    for i in range(n):
        p, s, name = struct.unpack_from("<ii8s", d, off + 16 * i)
        out.append((name.rstrip(b"\0").decode("ascii", "replace").upper(), d[p:p + s]))
    return out

def write_wad(path, lumps):
    body, dirs, pos = b"", b"", 12
    for name, data in lumps:
        dirs += struct.pack("<ii8s", pos, len(data), name.encode()[:8])
        body += data; pos += len(data)
    open(path, "wb").write(struct.pack("<4sii", b"IWAD", len(lumps), 12 + len(body)) + body + dirs)

def s8(b): return b.rstrip(b"\0").decode().upper()

# the sprites kept: weapons (fist, pistol, shotgun), hits, the imp and the zombieman, pickups
SPRITES = ["ARM1", "PUNG", "PISG", "PISF", "SHTG", "SHTF", "PUFF", "BLUD", "BAL1", "TROO", "CLIP", "SHEL",
           "STIM", "MEDI", "SHOT", "TFOG", "BON1", "BON2", "AMMO", "SBOX"]
# rare or big textures painted with a common one (the flash holds ~260 KB of data)
REMAP = {"BROWNPIP": "BROWNHUG", "LOGO": "STONE4", "SW1BRN1": "SW1BROWN", "STARTAN2": "METAL", "WOOD6": "WOODMET1",
         "ZIMMER7": "ZIMMER2", "BROWN144": "BROWNHUG", "BIGBRIK1": "STONE4"}
UI_PRE = ("STB", "STT", "STG", "STY", "STK", "STC", "STF", "STAR", "STP", "STD")
KEEP = ("PLAYPAL", "COLORMAP", "SKY1")

# a deathmatch arena made a single-player level: big weapons and power-ups become what the mini WAD has,
# the deathmatch starts become imps; every thing on every skill
SWAP = {82: 2001, 2002: 2048, 2003: 2008, 2004: 2049, 2005: 2011, 2006: 2012}
MONSTERS = (3001,)

def things(raw):
    out, n = b"", 0
    for k in range(0, len(raw), 10):
        x, y, a, t, f = struct.unpack_from("<hhhhh", raw, k)
        if t == 11:
            t = MONSTERS[n % len(MONSTERS)]; n += 1
        t = SWAP.get(t, t)
        out += struct.pack("<hhhhh", x, y, a, t, 7)
    return out

def sidedefs(raw):
    out = b""
    for k in range(0, len(raw), 30):
        x, y, up, lo, mid, sec = struct.unpack_from("<hh8s8s8sh", raw, k)
        up, lo, mid = (REMAP.get(s8(t), s8(t)).encode().ljust(8, b"\0") for t in (up, lo, mid))
        out += struct.pack("<hh8s8s8sh", x, y, up, lo, mid, sec)
    return out

def main(src, mapname, out):
    L = read_wad(src); names = [n for n, _ in L]; data = dict(L)
    i = names.index(mapname)
    maplumps = [(k, things(v) if k == "THINGS" else sidedefs(v) if k == "SIDEDEFS" else v) for k, v in L[i + 1:i + 11]]
    m = dict(maplumps)
    tex, flats = set(), {"F_SKY1"}
    for k in range(0, len(m["SIDEDEFS"]), 30):
        tex |= {s8(t) for t in struct.unpack_from("<hh8s8s8sh", m["SIDEDEFS"], k)[2:5]} - {"-"}
    for k in range(0, len(m["SECTORS"]), 26):
        flats |= {s8(f) for f in struct.unpack_from("<hh8s8s", m["SECTORS"], k)[2:4]}
    tex.add("SKY1")
    pn = data["PNAMES"]
    pnames = [s8(pn[4 + 8 * j:12 + 8 * j]) for j in range(struct.unpack_from("<i", pn)[0])]
    t1 = data["TEXTURE1"]
    texdefs, used_p = [], []
    for j in range(struct.unpack_from("<i", t1)[0]):
        o = struct.unpack_from("<i", t1, 4 + 4 * j)[0]
        if s8(t1[o:o + 8]) not in tex:
            continue
        np_ = struct.unpack_from("<h", t1, o + 20)[0]
        head, pats = t1[o:o + 22], []
        for q in range(np_):
            ox, oy, pi, a, b = struct.unpack_from("<hhhhh", t1, o + 22 + 10 * q)
            p = pnames[pi]
            if p not in used_p:
                used_p.append(p)
            pats.append(struct.pack("<hhhhh", ox, oy, used_p.index(p), a, b))
        texdefs.append(head + b"".join(pats))
    n = len(texdefs)
    offs, pos = [], 4 + 4 * n
    for t in texdefs:
        offs.append(pos); pos += len(t)
    new_t1 = struct.pack("<i", n) + b"".join(struct.pack("<i", o) for o in offs) + b"".join(texdefs)
    new_pn = struct.pack("<i", len(used_p)) + b"".join(p.encode().ljust(8, b"\0") for p in used_p)
    flat_l = [(nm, data[nm]) for nm in names[names.index("F_START") + 1:names.index("F_END")] if nm in flats]
    s0, s1 = names.index("S_START"), names.index("S_END")
    spr_l = [(nm, data[nm]) for nm in names[s0 + 1:s1] if nm[:4] in SPRITES]
    ui = [(nm, data[nm]) for nm in names if nm.startswith(UI_PRE)]
    lumps = [(k, data[k]) for k in KEEP] + [("TEXTURE1", new_t1), ("PNAMES", new_pn)]
    lumps += [("MAP01", b"")] + maplumps
    lumps += [("P_START", b"")] + [(p, data[p]) for p in used_p] + [("P_END", b"")]
    lumps += [("F_START", b"")] + flat_l + [("F_END", b"")]
    lumps += [("S_START", b"")] + spr_l + [("S_END", b"")]
    lumps += ui
    write_wad(out, lumps)
    tot = sum(len(b) for _, b in lumps)
    print(f"{out}: {len(lumps)} lumps, {tot/1024:.1f} KB; {n} textures, {len(used_p)} patches, {len(flat_l)} flats, {len(spr_l)} sprite frames, {len(ui)} ui")

if __name__ == "__main__":
    main(*sys.argv[1:4])
