#!/usr/bin/env python3
"""What one map of a Doom IWAD needs: textures, patches, flats, sprites, and the sizes of the lump groups."""
import struct, sys, collections

def read_wad(path):
    d = open(path, "rb").read()
    kind, n, off = struct.unpack_from("<4sii", d, 0)
    lumps = []
    for i in range(n):
        p, s, name = struct.unpack_from("<ii8s", d, off + 16 * i)
        lumps.append((name.rstrip(b"\0").decode("ascii", "replace").upper(), d[p:p + s]))
    return lumps

def main(path, mapname):
    L = read_wad(path)
    names = [n for n, _ in L]
    i = names.index(mapname)
    m = dict(L[i + 1:i + 11])
    tex, flats = set(), set()
    for k in range(0, len(m["SIDEDEFS"]), 30):
        _, _, up, lo, mid, _ = struct.unpack_from("<hh8s8s8sh", m["SIDEDEFS"], k)
        for t in (up, lo, mid):
            t = t.rstrip(b"\0").decode().upper()
            if t != "-": tex.add(t)
    for k in range(0, len(m["SECTORS"]), 26):
        _, _, f, c, *_ = struct.unpack_from("<hh8s8shhh", m["SECTORS"], k)
        flats.add(f.rstrip(b"\0").decode().upper()); flats.add(c.rstrip(b"\0").decode().upper())
    things = collections.Counter()
    for k in range(0, len(m["THINGS"]), 10):
        x, y, a, t, fl = struct.unpack_from("<hhhhh", m["THINGS"], k)
        things[t] += 1
    # textures -> patches
    pn = L[names.index("PNAMES")][1]
    pnames = [pn[4 + 8 * j:12 + 8 * j].rstrip(b"\0").decode().upper() for j in range(struct.unpack_from("<i", pn)[0])]
    patches = set()
    t1 = L[names.index("TEXTURE1")][1]
    nt = struct.unpack_from("<i", t1)[0]
    for j in range(nt):
        o = struct.unpack_from("<i", t1, 4 + 4 * j)[0]
        tn = t1[o:o + 8].rstrip(b"\0").decode().upper()
        npatch = struct.unpack_from("<h", t1, o + 20)[0]
        if tn in tex:
            for q in range(npatch):
                _, _, pi, _, _ = struct.unpack_from("<hhhhh", t1, o + 22 + 10 * q)
                patches.add(pnames[pi])
    size = {n: len(b) for n, b in L}
    def sz(group): return sum(size.get(x, 0) for x in group)
    print(f"{mapname}: map lumps {sum(len(b) for b in m.values())} B; textures {len(tex)} from {len(patches)} patches {sz(patches)} B; flats {len(flats)} {sz(flats)} B")
    print("things:", dict(sorted(things.items())))
    grp = collections.defaultdict(int)
    for n, b in L:
        for pre in ("STF", "STT", "STG", "STY", "STK", "STB", "STC", "M_", "WI", "DS", "DP", "D_", "TITLE", "HELP", "CREDIT", "PLAYPAL", "COLORMAP", "GENMIDI", "ENDOOM", "SKY"):
            if n.startswith(pre): grp[pre] += len(b); break
    print({k: v for k, v in sorted(grp.items())})
    return L, tex, flats, things, patches

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "E1M1")
