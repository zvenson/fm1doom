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
2. [x] RAM: lumps read in place, no wipe, the screen outside the zone; renderer limits for one small arena
   (`FM1_SMALL`: 48 visplanes, openings ×28, 128 drawsegs, `viewangletox` as short); no Doom status bar or HUD
   font: health, armour, ammo, weapon and messages are the FM-1's own HUD in the strips above and below the
   picture; the sky is four copies of a 64-column slice. The host runs the level for 100 s in a 150 KB zone.
3. [x] The WAD image (raw deflate per lump, PLAYPAL / COLORMAP raw): 239 KB in the app's flash.
4. [x] The firmware: `./build.sh` -> `build/fm1doom.fwsc` (identity FM-1_980): sloopDX's boot guard, USB-MIDI,
   M-UPGRADE updater and USB rescue (OCT- at power-on), so the web installer can always put sloopDX back; the
   SDK's trimmed newlib plus `firmware/src/fm1_libc.c` (printf family, a growing heap, no files); the finale,
   intermission, savegames and the network checksum stubbed (the arena restarts at its exit).
   Flash 549 of 581 KB, RAM 227 KB + zone 195 KB.
5. [x] On the device (0.1). 0.3: Freedoom's logo at boot, then the controls.
6. [x] Sound (0.4): the effects synthesized (`src/fm1_sfx.c`: a sweeping oscillator plus filtered noise per
   effect, 8 voices at 44.1 kHz in the audio interrupt); no music. `host/sfx_demo.c` writes them all to a WAV.

## Controls (FM-1)

| | |
|---|---|
| F3 · B3 | turn left · right (or KNOB 1 / SELECT) |
| A3 · G3 | forward · back |
| F#3 · G#3, OCT− · OCT+ | strafe left · right |
| C5 or PLAY | fire |
| D5 or REC | open / use (and: again, after dying) |
| E5 | run |
| C#5 · D#5 · F#5 | fist · pistol · shotgun |
| ARP | the automap |
| FX | brightness (100 · 80 · 65 · 50 %) |

## Licences

- The engine (`src/`, from doomgeneric / Chocolate Doom / id Software's Doom source): GNU GPL 2.0 or later,
  `COPYING`. The FM-1 firmware build adds the GPL-3.0 hardware layer of Felucca / sloopDX, so the firmware as a
  whole is GPL-3.0.
- The game data built by `tools/mkwad.py` comes from FreeDM (the Freedoom project): BSD-3-Clause,
  `FREEDOOM-COPYING.txt`. No id Software data is used or needed.
- Doom is a trademark of ZeniMax Media; this project is not affiliated with id Software, ZeniMax, Bethesda or
  M-VAVE.
