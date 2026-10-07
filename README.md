# FM-1 Doom

Doom on the M-VAVE FM-1 (JieLi AC791N, 1 MiB flash, ~430 KB RAM, 240×240 LCD). A side project of
[sloopDX](https://github.com/zvenson/dxsloop).

- **Engine:** [doomgeneric](https://github.com/ozkl/doomgeneric) (Chocolate Doom, GPL-2.0-or-later) in `src/`,
  changed for a small WAD (missing switch textures and music are skipped) and a zone in KiB (`-kb`).
- **Data:** one arena of [FreeDM](https://freedoom.github.io/) (Freedoom project, BSD-3-Clause), MAP12 made a
  single-player level: `tools/mkwad.py freedm.wad MAP12 build/freedm.wad` keeps only its textures, flats, the
  imp, the zombieman, fist / pistol / shotgun and the status bar (~770 KB raw, ~280 KB compressed).
- **Host build:** `sh host/build.sh`, then `build/fm1doom-host <tics> <frame dir> <zone KiB>`: headless, a
  virtual clock, scripted keys, every 35th frame a PPM.

## Status

1. [x] The engine runs the mini WAD on the host (renders, monsters fight back).
2. [ ] RAM: lumps read straight from flash (no copies), no wipe; target zone ≤ 300 KB.
3. [ ] Compressed lumps (zlib per lump), the WAD image in the app's flash.
4. [ ] The FM-1 platform: LCD (320×200 → 240×150), keys and knobs, timer, packaging for the web installer.
5. [ ] On the device.

## Licences

- The engine (`src/`, from doomgeneric / Chocolate Doom / id Software's Doom source): GNU GPL 2.0 or later,
  `COPYING`. The FM-1 firmware build adds the GPL-3.0 hardware layer of Felucca / sloopDX, so the firmware as a
  whole is GPL-3.0.
- The game data built by `tools/mkwad.py` comes from FreeDM (the Freedoom project): BSD-3-Clause,
  `FREEDOOM-COPYING.txt`. No id Software data is used or needed.
- Doom is a trademark of ZeniMax Media; this project is not affiliated with id Software, ZeniMax, Bethesda or
  M-VAVE.
