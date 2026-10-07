#!/bin/sh
# FM-1 Doom firmware: the mini WAD, its flash image, then the app + update loader -> build/fm1doom.fwsc
# needs freedm-0.13.0/freedm.wad (https://github.com/freedoom/freedoom/releases), the JieLi toolchain and the AC79 SDK
set -e
cd "$(dirname "$0")"
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
mkdir -p build
python3 tools/mkwad.py freedm-0.13.0/freedm.wad MAP12 build/freedm.wad
python3 tools/mkimage.py build/freedm.wad build/fm1doom.img
python3 tools/build.py "$@"
