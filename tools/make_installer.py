#!/usr/bin/env python3
"""The FM-1 Doom installer page: sloopDX's installer (web/index_pkg.html, fm1pkg.js, fm1ota.js: the same,
tested update path) with Doom's package and texts. Usage:
  make_installer.py <sloopdx repo> build/fm1doom.fwsc <version> <out dir>
writes <out>/index.html and <out>/firmware/fm1doom-<version>.fwsc (the page sits at the site's /doom/)."""
import hashlib, json, re, shutil, sys
from pathlib import Path

BLK, KEEP, BLOCKS = 512, 16, 20   # (as sloopDX's make_site.py; checked against its product_of below)


def main(sloopdx, pkg, version, out):
    web = Path(sloopdx) / "web"
    sys.path.insert(0, str(web))
    import make_site                                    # sloopDX's own: product_of, strip_module
    raw = Path(pkg).read_bytes()
    product = make_site.product_of(raw)
    if not re.fullmatch(r"FM-1_98\d", product) or b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: identity {product!r} or loader marker missing")
    html = (web / "index_pkg.html").read_text(encoding="utf-8")
    lib = make_site.strip_module((web / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        make_site.strip_module((web / "fm1ota.js").read_text(encoding="utf-8"))
    name = f"fm1doom-{version}.fwsc"
    meta = json.dumps({"version": version, "product": product, "pkg": "firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest()})
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta)
    swaps = [
        (r"<title>.*?</title>", "<title>FM-1 Doom: Doom on the M-VAVE FM-1, installed from the browser</title>"),
        (r'<meta name="description" content=".*?">', '<meta name="description" content="Doom (one Freedoom arena) on the M-VAVE FM-1, installed from Chrome or Edge. Back to sloopDX with its installer.">\n<meta name="robots" content="noindex">'),
        (r'<meta property="og:title" content=".*?">', '<meta property="og:title" content="FM-1 Doom">'),
        ("<!--LOGO-->", "<b>FM-1 <span style=\"color:var(--led)\">DOOM</span></b>"),
        (r'<div class="links">.*?</div>', '<div class="links"><a href="../">sloopDX</a><a href="../webapp/installer/">Back to sloopDX</a>'
         '<a class="gh" href="https://github.com/zvenson/fm1doom">GitHub</a></div>'),
        ('<p class="eyebrow">Firmware for the M-VAVE FM-1</p>', '<p class="eyebrow">An experiment for the M-VAVE FM-1</p>'),
        ("<h1>Install <b>sloopDX</b></h1>", "<h1>It runs <b>Doom</b></h1>"),
        (r'<p class="lead">.*?</p>', '<p class="lead">One Freedoom arena on the FM-1: imps, a shotgun, the automap. Health and ammo on the FM-1\'s screen. Back to sloopDX any time with its installer: your sloopDX projects and banks stay untouched.</p>'),
        (r'<div class="dxanim".*?</div>', ""),
        (r'<aside class="side">.*?</aside>', '''<aside class="side">
      <section class="card">
        <h2>Controls</h2>
        <p class="small"><b>F3 · B3</b> or <b>KNOB 1</b>: turn &nbsp; <b>A3 · G3</b>: forward · back &nbsp; <b>F#3 · G#3</b> or <b>OCT− · OCT+</b>: strafe</p>
        <p class="small"><b>C5</b> or <b>PLAY</b>: fire &nbsp; <b>D5</b> or <b>REC</b>: open, use &nbsp; <b>E5</b>: run &nbsp; <b>C#5 · D#5 · F#5</b>: fist · pistol · shotgun &nbsp; <b>ARP</b>: map</p>
        <h2>Back to sloopDX</h2>
        <p class="small">Open the <a href="../webapp/installer/">sloopDX installer</a> and press Install. If the FM-1 does not answer: hold OCT− while switching it on (USB rescue), then install.</p>
      </section>
    </aside>'''),
        (r"<footer><div class=\"wrap\">.*?<span data-t=\"license\">", '<footer><div class="wrap">\n  FM-1 Doom: the Doom engine of doomgeneric / Chocolate Doom (GPL-2.0-or-later), the FM-1 platform of sloopDX / SLOOP / Felucca (GPL-3.0). Game data: FreeDM by the Freedoom project (BSD-3-Clause). Doom is a trademark of ZeniMax Media; not affiliated with id Software, ZeniMax, Bethesda or M-VAVE.<br>\n  <span data-t="license">'),
        ("https://github.com/zvenson/dxsloop", "https://github.com/zvenson/fm1doom"),
        ("github.com/zvenson/dxsloop", "github.com/zvenson/fm1doom"),
        ("../../impressum.html", "../impressum.html"),
        ("../../", "../"),
    ]
    for old, new in swaps:
        if old.startswith(("<", "../", "https", "github")) and not any(c in old for c in "*?\\("):
            if old not in html:
                raise SystemExit(f"installer: {old[:40]!r} not found; update make_installer.py")
            html = html.replace(old, new)
        else:
            html, n = re.subn(old, lambda m: new, html, count=1, flags=re.S)
            if not n:
                raise SystemExit(f"installer: {old[:40]!r} not found; update make_installer.py")
    html = re.sub(r"\bsloopDX \$\{meta.version\}", "FM-1 Doom ${meta.version}", html)
    out = Path(out)
    (out / "firmware").mkdir(parents=True, exist_ok=True)
    for old in (out / "firmware").glob("fm1doom-*.fwsc"):
        old.unlink()
    shutil.copy(pkg, out / "firmware" / name)
    (out / "index.html").write_text(html, encoding="utf-8")
    print(f"installer: {out / 'index.html'} ({len(html)} B), firmware/{name} ({len(raw)} B, {product})")


if __name__ == "__main__":
    main(*sys.argv[1:5])
