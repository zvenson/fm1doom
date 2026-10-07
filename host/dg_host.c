/* SPDX-License-Identifier: GPL-2.0-or-later */
/* FM-1 Doom on the host: no window. A virtual clock (one game tic per call), scripted keys, every
 * Nth frame written as a PPM. Proves the mini WAD and the engine before anything goes to the FM-1. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "doomgeneric.h"
#include "doomkeys.h"

struct color { uint32_t b:8, g:8, r:8, a:8; };
extern struct color colors[256];

static uint32_t now_ms, frame_no;
static const char *outdir = "build/frames";
static int every = 35;

void DG_Init(void) {}
void DG_DrawFrame(void)
{
    char p[256];
    FILE *f;
    int i;
    if (frame_no++ % (unsigned)every)
        return;
    snprintf(p, sizeof p, "%s/f%05u.ppm", outdir, frame_no - 1);
    f = fopen(p, "wb");
    if (!f) return;
    fprintf(f, "P6 %d %d 255\n", DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    for (i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; i++) {
        struct color c = colors[DG_ScreenBuffer[i]];
        fputc(c.r, f); fputc(c.g, f); fputc(c.b, f);
    }
    fclose(f);
}
void DG_SleepMs(uint32_t ms) { now_ms += ms; }
uint32_t DG_GetTicksMs(void) { return now_ms; }
void DG_SetWindowTitle(const char *t) { (void)t; }

/* the script: (time ms, key, pressed) */
static const struct { uint32_t t; unsigned char key; int down; } SCRIPT[] = {
    {1500, KEY_UPARROW, 1}, {3000, KEY_UPARROW, 0},
    {3200, KEY_RIGHTARROW, 1}, {3900, KEY_RIGHTARROW, 0},
    {4000, KEY_FIRE, 1}, {4200, KEY_FIRE, 0},
    {4500, KEY_UPARROW, 1}, {7000, KEY_UPARROW, 0},
    {7200, KEY_USE, 1}, {7300, KEY_USE, 0},
};
static unsigned si;
int DG_GetKey(int *pressed, unsigned char *key)
{
    if (si < sizeof SCRIPT / sizeof SCRIPT[0] && now_ms >= SCRIPT[si].t) {
        *pressed = SCRIPT[si].down;
        *key = SCRIPT[si].key;
        si++;
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    uint32_t i, tics = argc > 1 ? (uint32_t)atoi(argv[1]) : 350;
    char *args[] = {"fm1doom", "-iwad", getenv("FM1_WAD") ? getenv("FM1_WAD") : "build/freedm.wad", "-warp", "1", "-skill", "3", "-nosound", "-nomusic",
                    "-kb", argc > 3 ? argv[3] : "4096", NULL};
    if (argc > 2) outdir = argv[2];
    doomgeneric_Create(11, args);
    for (i = 0; i < tics; i++) {
        doomgeneric_Tick();
        now_ms += 1000 / 35 + (i % 7 == 0);           /* ~35 tics a second */
    }
    printf("host: %u tics, %u frames\n", tics, frame_no);
    return 0;
}
