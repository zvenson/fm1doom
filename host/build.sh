#!/bin/sh
# the host build of FM-1 Doom (headless): build/fm1doom-host
set -e
cd "$(dirname "$0")/.."
CFLAGS="-O1 -g -w -DNORMALUNIX -DCMAP256 -DDOOMGENERIC_RESX=320 -DDOOMGENERIC_RESY=200 -Isrc"
objs=""
for c in src/*.c host/dg_host.c; do
    o=build/hostobj/$(basename ${c%.c}).o
    if [ ! -f "$o" ] || [ "$c" -nt "$o" ]; then cc $CFLAGS -c "$c" -o "$o"; fi
    objs="$objs $o"
done
cc -o build/fm1doom-host $objs -lm
