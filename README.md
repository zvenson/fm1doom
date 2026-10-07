# FM-1 Doom

Doom on the M-VAVE FM-1 (JieLi AC791N, 1 MiB flash, ~430 KB RAM, 240×240 LCD). A side project of
[sloopDX](https://github.com/zvenson/dxsloop).

- **Engine:** [doomgeneric](https://github.com/ozkl/doomgeneric) (Chocolate Doom, GPL-2.0-or-later) in `src/`,
  changed for a small WAD (missing switch textures and music are skipped) and a zone in KiB (`-kb`).
- **Data:** one arena of [FreeDM](https://freedoom.github.io/) (Freedoom project, BSD-3-Clause), MAP12 made a
  single-player level: `tools/mkwad.py freedm.wad MAP12 build/freedm.wad` keeps only its textures (rare ones
  painted with common ones: 17 textures from 25 patches), its flats, the imp, fist / pistol / shotgun and the
  status bar: 562 KB raw, 276 KB with zlib per lump. The installer writes only the app area
  (0x4000..0x93000, ~570 KB), which the engine (~280 KB) and the data share.
- **Host build:** `sh host/build.sh`, then `build/fm1doom-host <tics> <frame dir> <zone KiB>`: headless, a
  virtual clock, scripted keys, every 35th frame a PPM.

## Status

1. [x] The engine runs the mini WAD on the host (renders, monsters fight back).
2. [x] RAM: lumps read in place (as from flash), no wipe: the level runs in a 200 KB zone (the FM-1 has ~350 KB).
3. [~] Compressed lumps: `tools/mkimage.py` deflates each lump that shrinks (flag in bit 31 of its filepos),
   `src/fm1_inflate.c` inflates it into the zone on first use (`tests/inflate_test.c`: every lump at three
   levels, byte-exact). Image 280 KB. Open: with everything deflated the zone needs ~400 KB (static status bar
   faces, level lumps inflated); next: faces loaded when drawn, map lumps kept raw, engine trimmed (f_finale,
   wi_stuff, saveg, sha1, statdump) and its tables made const, so engine + image fit the 572 KB app area and the
   zone the 336 KB POOL.
   Measured since: with the 64 KB screen outside the zone (it was the fragmented zone's largest block) the level
   runs in a 300 KB zone. On pi32v2 the engine is 222 KB of code and constants; f_finale, wi_stuff, i_scale,
   p_saveg, sha1, m_config, statdump can go (~38 KB), and `states` / `mobjinfo` (40 KB) move from RAM to flash.
   Budget: flash 224 KB engine + ~25 KB platform + 280 KB image = ~530 of 572 KB; RAM region 96 KB = engine data
   (~25 KB after `ticdata` shrinks) + the screen; POOL 336 KB = the zone.
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
