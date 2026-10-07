/* SPDX-License-Identifier: GPL-2.0-or-later */
/* FM-1 Doom: the sound effects, synthesized (the mini WAD has no DS* lumps: no room for them).
 * Each sound is a recipe: an oscillator (square, saw, triangle or pitched noise) sweeping from f0 to
 * f1, plus low-passed white noise, under one envelope (attack, then an exponential decay to the end).
 * fm1_sfx_render mixes the voices at 44.1 kHz (the FM-1's audio ISR; on the host a WAV). */
#include <stdint.h>
#include "doomtype.h"
#include "i_sound.h"
#include "sounds.h"

#define SFX_FS 44100
#define SFX_BLK 32                       /* envelope, sweep and wobble update every 32 samples */
#define NVOICE 8

enum { W_SQ, W_SAW, W_TRI, W_SH };      /* SH: noise, a new value at the oscillator's rate */
typedef struct {
    uint8_t wave;
    uint16_t f0, f1;                     /* Hz */
    uint16_t ms;                         /* length */
    uint8_t att;                         /* attack, ms */
    uint8_t osc, noi;                    /* levels 0..255 */
    uint8_t lp;                          /* noise low-pass, 1 (dark) .. 255 (open) */
    uint8_t wob;                         /* random pitch wobble (growls), 0..255 */
} sfx_recipe_t;

static const sfx_recipe_t RECIPE[] = {
    /*  wave    f0    f1    ms att osc noi  lp wob */
    {W_SQ,     0,    0,    0, 0,   0,   0,  0,  0},   /* 0 silent */
    {W_SH,  2600,  250,  170, 0, 220, 140, 170,  0},   /* 1 pistol, chaingun */
    {W_SQ,   120,   35,  420, 0, 150, 255,  80,  0},   /* 2 shotgun */
    {W_SQ,   900,  700,   70, 0, 120, 200, 200,  0},   /* 3 cock, switch, clack */
    {W_SQ,   150,   55,  100, 0, 220, 110,  60,  0},   /* 4 punch, claw hit */
    {W_SAW,  220,   70,  550, 5, 120, 220,  70, 20},   /* 5 rocket launch */
    {W_SH,   420,   35,  800, 0, 230, 200,  50,  0},   /* 6 explosion */
    {W_SAW,  260,  700,  420, 10, 90, 220, 110, 30},   /* 7 fireball whoosh */
    {W_SAW,   55,   85,  650, 40, 160, 120, 30, 60},   /* 8 door / platform start */
    {W_SAW,   90,   50,  600, 20, 160, 120, 30, 60},   /* 9 door close / stop */
    {W_SAW,  420,  190,  300, 5, 200,  40, 120, 90},   /* 10 player pain */
    {W_SAW,  520,   70,  950, 5, 210,  50, 100, 110},  /* 11 player death */
    {W_SQ,   190,  120,  140, 0, 190,   0,  0, 30},    /* 12 oof, no way */
    {W_SQ,   900, 1800,   90, 0, 150,   0,  0,  0},    /* 13 item up */
    {W_SQ,   500, 1400,  280, 0, 160,   0,  0,  0},    /* 14 weapon up */
    {W_TRI,  400, 1600,  700, 0, 230,   0,  0,  0},    /* 15 power up */
    {W_SH,   200, 2400,  800, 30, 200,  60, 150, 0},   /* 16 teleport */
    {W_SAW,  240,  110,  750, 20, 200,  70, 60, 180},  /* 17 imp sight: a screech */
    {W_SAW,  150,  125,  500, 30, 170,  50, 40, 160},  /* 18 imp idle growl */
    {W_SAW,  330,   55,  950, 10, 210,  80, 60, 170},  /* 19 imp death */
    {W_SAW,  280,  180,  280, 5, 190,  40, 80, 150},   /* 20 monster pain */
    {W_SQ,   220,  110,  130, 0, 120, 200, 150, 0},    /* 21 claw swing */
    {W_SAW,   90,   30,  600, 0, 120, 230,  40, 40},   /* 22 gibs */
    {W_TRI, 2000, 1900,  110, 0, 200,   0,  0,  0},    /* 23 tink, metal */
    {W_TRI, 1000,  500,  300, 0, 180,   0,  0,  0},    /* 24 item respawn */
    {W_SAW,   70,   70,  350, 20, 140, 100, 30, 80},   /* 25 stone, platform move */
};

/* sfx id (sounds.h) -> recipe */
static uint8_t sfx_map(int id)
{
    switch (id) {
    case sfx_pistol: case sfx_chgun: return 1;
    case sfx_shotgn: case sfx_dshtgn: return 2;
    case sfx_sgcock: case sfx_dbopn: case sfx_dbcls: case sfx_dbload: case sfx_swtchn: case sfx_swtchx: return 3;
    case sfx_punch: case sfx_sawhit: return 4;
    case sfx_rlaunc: case sfx_plasma: case sfx_bfg: return 5;
    case sfx_rxplod: case sfx_firxpl: case sfx_barexp: return 6;
    case sfx_firsht: case sfx_flame: case sfx_flamst: case sfx_manatk: return 7;
    case sfx_doropn: case sfx_bdopn: case sfx_pstart: return 8;
    case sfx_dorcls: case sfx_bdcls: case sfx_pstop: return 9;
    case sfx_plpain: return 10;
    case sfx_pldeth: case sfx_pdiehi: return 11;
    case sfx_oof: case sfx_noway: return 12;
    case sfx_itemup: return 13;
    case sfx_wpnup: return 14;
    case sfx_getpow: return 15;
    case sfx_telept: return 16;
    case sfx_bgsit1: case sfx_bgsit2: case sfx_posit1: case sfx_posit2: case sfx_posit3: case sfx_sgtsit:
    case sfx_cacsit: case sfx_brssit: case sfx_kntsit: case sfx_skesit: case sfx_sssit: return 17;
    case sfx_bgact: case sfx_posact: case sfx_dmact: case sfx_skeact: return 18;
    case sfx_bgdth1: case sfx_bgdth2: case sfx_podth1: case sfx_podth2: case sfx_podth3: case sfx_sgtdth:
    case sfx_cacdth: case sfx_brsdth: case sfx_kntdth: case sfx_skedth: case sfx_ssdth: return 19;
    case sfx_popain: case sfx_dmpain: case sfx_vipain: case sfx_mnpain: case sfx_pepain: return 20;
    case sfx_claw: case sfx_skeswg: case sfx_sgtatk: case sfx_skepch: return 21;
    case sfx_slop: return 22;
    case sfx_tink: case sfx_metal: return 23;
    case sfx_itmbk: return 24;
    case sfx_stnmov: return 25;
    default: return 0;
    }
}

typedef struct {
    volatile uint8_t on;                 /* written last by StartSound, cleared by the renderer at the end */
    uint8_t wave, ol, nl, lp, wob;
    uint32_t phase, inc;                 /* Q32 per sample */
    int32_t dinc;                        /* per block (the sweep) */
    int32_t amp, att_step;               /* Q24 */
    uint32_t dmul;                       /* Q16 decay per block */
    uint32_t blocks, n;                  /* length, blocks done */
    uint32_t att_blocks;
    int32_t lpz, sh, wz, wt;
    volatile uint8_t gl, gr;             /* channel gains (vol, sep) */
} sfx_voice_t;

static sfx_voice_t V[NVOICE];
static uint32_t rnd = 0x1234567u;

static uint32_t xr(void)
{
    rnd ^= rnd << 13;
    rnd ^= rnd >> 17;
    rnd ^= rnd << 5;
    return rnd;
}

static void gains(sfx_voice_t *v, int vol, int sep)
{
    if (vol < 0) vol = 0;
    if (vol > 127) vol = 127;
    if (sep < 0) sep = 0;
    if (sep > 254) sep = 254;
    v->gl = (uint8_t)(vol * (254 - sep) / 254 + vol / 4);   /* (a bit of each side in the other) */
    v->gr = (uint8_t)(vol * sep / 254 + vol / 4);
}

/* the mix: n stereo frames, Q15 into out[2n] (the caller scales) */
void fm1_sfx_render(int32_t *out, unsigned n)
{
    unsigned i, k, b;
    for (i = 0; i < 2u * n; i++)
        out[i] = 0;
    for (k = 0; k < NVOICE; k++) {
        sfx_voice_t *v = &V[k];
        if (!v->on)
            continue;
        for (b = 0; b < n; b += SFX_BLK) {
            uint32_t inc = v->inc;
            int32_t a, gl = v->gl, gr = v->gr;
            if (v->n >= v->blocks) {
                v->on = 0;
                break;
            }
            if (v->wob) {                       /* a random walk around the sweep: growls */
                if ((v->n & 7u) == 0u)
                    v->wt = (int32_t)(xr() >> 24) - 128;
                v->wz += (v->wt - v->wz) >> 2;
                inc += (uint32_t)(((int64_t)inc * v->wz * v->wob) >> 16);
            }
            if (v->n < v->att_blocks)
                v->amp += v->att_step;
            else
                v->amp = (int32_t)(((int64_t)v->amp * v->dmul) >> 16);
            a = v->amp >> 9;                    /* Q15 */
            for (i = b; i < b + SFX_BLK && i < n; i++) {
                int32_t o, s;
                uint32_t ph = v->phase;
                v->phase = ph + inc;
                switch (v->wave) {
                case W_SQ:  o = (ph & 0x80000000u) ? 12000 : -12000; break;
                case W_SAW: o = (int32_t)(ph >> 16) - 32768; o = o * 3 / 8; break;
                case W_TRI: o = (int32_t)((ph & 0x80000000u) ? ~ph : ph) >> 15; o = (o - 32768) / 2; break;
                default:    if (v->phase < ph) v->sh = (int32_t)(xr() >> 18) - 8192; o = v->sh * 3 / 2; break;
                }
                s = o * v->ol;
                if (v->nl) {
                    int32_t w = (int32_t)(xr() >> 17) - 16384;
                    v->lpz += ((w - v->lpz) * v->lp) >> 8;
                    s += v->lpz * v->nl;
                }
                s = ((s >> 8) * a) >> 15;      /* ±~16k at full level */
                out[2u * i] += (s * gl) >> 7;
                out[2u * i + 1u] += (s * gr) >> 7;
            }
            v->inc += (uint32_t)v->dinc;
            v->n++;
        }
    }
    for (i = 0; i < 2u * n; i++) {
        int32_t s = out[i];
        out[i] = s > 32767 ? 32767 : s < -32767 ? -32767 : s;
    }
}

static uint32_t hz_inc(uint32_t hz) { return (uint32_t)(((uint64_t)hz << 32) / SFX_FS); }

static boolean S_Init(boolean prefix) { (void)prefix; return true; }
static void S_Shutdown(void) { int k; for (k = 0; k < NVOICE; k++) V[k].on = 0; }
static int S_Lump(sfxinfo_t *s) { (void)s; return 0; }         /* (no lump: the recipe is found by id) */
static void S_Update(void) {}
static void S_Params(int ch, int vol, int sep) { if (ch >= 0 && ch < NVOICE) gains(&V[ch], vol, sep); }
static void S_Stop(int ch) { if (ch >= 0 && ch < NVOICE) V[ch].on = 0; }
static boolean S_Playing(int ch) { return ch >= 0 && ch < NVOICE && V[ch].on; }
static void S_Cache(sfxinfo_t *s, int n) { (void)s; (void)n; }

static int S_Start(sfxinfo_t *sfx, int ch, int vol, int sep)
{
    const sfx_recipe_t *r = &RECIPE[sfx_map((int)(sfx - S_sfx))];
    sfx_voice_t *v;
    uint32_t i0, i1;
    if (ch < 0 || ch >= NVOICE || !r->ms)
        return -1;
    v = &V[ch];
    v->on = 0;
    v->wave = r->wave; v->ol = r->osc; v->nl = r->noi; v->lp = r->lp; v->wob = r->wob;
    v->blocks = (uint32_t)r->ms * SFX_FS / 1000u / SFX_BLK + 1u;
    v->att_blocks = (uint32_t)r->att * SFX_FS / 1000u / SFX_BLK;
    i0 = hz_inc(r->f0); i1 = hz_inc(r->f1);
    v->inc = i0;
    v->dinc = (int32_t)(((int64_t)i1 - (int64_t)i0) / (int64_t)v->blocks);
    v->amp = v->att_blocks ? 0 : (1 << 24);
    v->att_step = v->att_blocks ? (int32_t)((1 << 24) / v->att_blocks) : 0;
    v->dmul = 65536u - 4u * 65536u / (v->blocks - v->att_blocks + 1u);   /* about -35 dB at the end */
    v->n = 0; v->phase = 0; v->lpz = 0; v->sh = 0; v->wz = 0; v->wt = 0;
    gains(v, vol, sep);
    v->on = 1;
    return ch;
}

static snddevice_t sfx_devices[] = {SNDDEVICE_SB, SNDDEVICE_PAS, SNDDEVICE_GUS, SNDDEVICE_WAVEBLASTER,
                                     SNDDEVICE_SOUNDCANVAS, SNDDEVICE_AWE32};
sound_module_t fm1_sound_module = {
    sfx_devices, sizeof sfx_devices / sizeof sfx_devices[0],
    S_Init, S_Shutdown, S_Lump, S_Update, S_Params, S_Start, S_Stop, S_Playing, S_Cache,
};
