/* SPDX-License-Identifier: GPL-2.0-or-later */
/* FM-1 Doom: every synthesized effect once, into a WAV (to listen to on the computer).
 *   cc -O2 -w -Isrc -o build/sfx_demo host/sfx_demo.c src/fm1_sfx.c src/sounds.c && build/sfx_demo build/sfx.wav */
#include <stdio.h>
#include <stdint.h>
#include "doomtype.h"
#include "i_sound.h"
#include "sounds.h"

extern sound_module_t fm1_sound_module;
void fm1_sfx_render(int32_t *out, unsigned n);

static const int DEMO[] = {sfx_pistol, sfx_shotgn, sfx_sgcock, sfx_punch, sfx_bgsit1, sfx_bgact, sfx_firsht,
                           sfx_firxpl, sfx_claw, sfx_popain, sfx_bgdth1, sfx_doropn, sfx_dorcls, sfx_swtchn,
                           sfx_itemup, sfx_wpnup, sfx_getpow, sfx_plpain, sfx_oof, sfx_slop, sfx_pldeth, sfx_telept,
                           sfx_barexp, sfx_stnmov, sfx_tink, sfx_itmbk};

static void put32(FILE *f, uint32_t v) { fputc(v, f); fputc(v >> 8, f); fputc(v >> 16, f); fputc(v >> 24, f); }

int main(int argc, char **argv)
{
    FILE *f = fopen(argc > 1 ? argv[1] : "build/sfx.wav", "wb");
    unsigned i, k, frames = 0, n = sizeof DEMO / sizeof DEMO[0];
    int32_t buf[2 * 441];
    if (!f) return 1;
    fwrite("RIFF\0\0\0\0WAVEfmt \x10\0\0\0\x01\0\x02\0", 1, 24, f);
    put32(f, 44100); put32(f, 44100 * 4); fwrite("\x04\0\x10\0data\0\0\0\0", 1, 12, f);
    for (i = 0; i < n; i++) {
        fm1_sound_module.StartSound(&S_sfx[DEMO[i]], 0, 100, 128);
        for (k = 0; k < 110; k++) {                         /* 1.1 s each */
            unsigned j;
            fm1_sfx_render(buf, 441);
            for (j = 0; j < 2 * 441; j++) { int16_t s = (int16_t)buf[j]; fwrite(&s, 2, 1, f); }
            frames += 441;
        }
    }
    fseek(f, 4, SEEK_SET); put32(f, 36 + frames * 4);
    fseek(f, 40, SEEK_SET); put32(f, frames * 4);
    fclose(f);
    printf("%u effects, %.1f s\n", n, frames / 44100.0);
    return 0;
}
