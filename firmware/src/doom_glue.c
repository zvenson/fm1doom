/* SPDX-License-Identifier: GPL-3.0-only */
/* FM-1 Doom: doomgeneric's platform (DG_*), the keys, the screen, and what newlib asks of an OS.
 * Included by fm1doom.c (the HAL, lcd.c, usb.c are in scope). */
#include "doomkeys.h"                           /* (doomgeneric.h wants newlib's headers; libc.c is ours) */
void doomgeneric_Create(int argc, char **argv);
void doomgeneric_Tick(void);

extern unsigned char *I_VideoBuffer;
struct dg_color { uint32_t b:8, g:8, r:8, a:8; };
extern struct dg_color colors[256];

/* ---- the screen: 320 x 200 palette indices -> 240 x 150 RGB565 (3 of every 4 pixels), rows 45..194 */
static uint16_t pal565[256];
static uint16_t dg_line[2][240];
static uint8_t xmap[240];
static uint8_t ymap[150];

/* ---- the HUD in the strips above and below the picture: ammo and weapon (or a message) on top, health and
 * armour below; redrawn when a value changes */
void fm1_hud(int *v, const char **msg, int *msg_gen);
static int hud_last[5] = {-999, -999, -999, -999, -999}, hud_msg_gen = -1;
static uint32_t hud_msg_until;
static uint8_t hud_msg_shown;
static const char *const WEAPON[9] = {"FIST", "PISTOL", "SHOTGUN", "CHAINGUN", "ROCKETS", "PLASMA", "BFG", "SAW", "SSG"};

static void itoa_s(char *b, int v, const char *suffix)
{
    char t[12];
    int n = 0, neg = v < 0;
    unsigned u = (unsigned)(neg ? -v : v);
    do { t[n++] = (char)('0' + u % 10u); u /= 10u; } while (u);
    if (neg) *b++ = '-';
    while (n) *b++ = t[--n];
    while (*suffix) *b++ = *suffix++;
    *b = 0;
}

static void hud_draw(void)
{
    int v[5], gen, i, top = 0, bottom = 0;
    const char *msg;
    char b[24];
    fm1_hud(v, &msg, &gen);
    if (gen != hud_msg_gen) {
        hud_msg_gen = gen;
        if (msg) { hud_msg_until = fm1_ms + 3000u; hud_msg_shown = 1; top = 1; }
    }
    if (hud_msg_shown && (int32_t)(fm1_ms - hud_msg_until) >= 0) { hud_msg_shown = 0; top = 1; }
    for (i = 0; i < 5; i++)
        if (v[i] != hud_last[i]) { if (i >= 2) top = 1; else bottom = 1; if (i == 4) bottom = 1; hud_last[i] = v[i]; }
    if (top) {
        if (hud_msg_shown && msg) {
            lcd_fill(0, 0, 240, 6, C_BLACK);
            draw_text_box(0, 6, 240, &FONT_S, msg, C_WHITE, 1);
            lcd_fill(0, 22, 240, 23, C_BLACK);
        } else {
            if (v[2] >= 0) itoa_s(b, v[2], ""); else b[0] = 0;
            draw_text_box(8, 6, 110, &FONT_L, b, RGB(240, 200, 40), 0);
            draw_text_box(118, 14, 114, &FONT_S, WEAPON[(unsigned)v[3] < 9u ? v[3] : 0], RGB(200, 190, 180), 2);
            lcd_fill(0, 0, 240, 6, C_BLACK);
            lcd_fill(0, 38, 240, 7, C_BLACK);
        }
    }
    if (bottom) {
        if (v[4]) {
            draw_text_box(0, 206, 240, &FONT_S, "DEAD - REC OR D5: AGAIN", RGB(255, 80, 60), 1);
        } else {
            itoa_s(b, v[0], "%");
            draw_text_box(8, 201, 112, &FONT_L, b, RGB(230, 50, 40), 0);
            itoa_s(b, v[1], "%");
            draw_text_box(120, 201, 112, &FONT_L, b, RGB(80, 200, 100), 2);
        }
    }
}

void DG_Init(void)
{
    uint32_t i;
    for (i = 0; i < 240u; i++) xmap[i] = (uint8_t)(i * 4u / 3u);
    for (i = 0; i < 150u; i++) ymap[i] = (uint8_t)(i * 4u / 3u);
    lcd_fill(0, 0, 240, 240, C_BLACK);
}

void DG_DrawFrame(void)
{
    uint32_t i, y;
    for (i = 0; i < 256u; i++) {                /* (I_SetPalette may change it any frame: damage, pickups) */
        uint16_t v = RGB(colors[i].r, colors[i].g, colors[i].b);
        pal565[i] = (uint16_t)((v >> 8) | (v << 8));
    }
    lcd_window(0, 45, 239, 194);
    for (y = 0; y < 150u; y++) {
        const uint8_t *src = I_VideoBuffer + (uint32_t)ymap[y] * 320u;
        uint16_t *d = dg_line[y & 1u];
        for (i = 0; i < 240u; i++)
            d[i] = pal565[src[xmap[i]]];
        lcd_data(d, 480);                       /* DMA; the other line fills meanwhile */
    }
    hud_draw();
    fm1_service();
}

void DG_SleepMs(uint32_t ms)
{
    uint32_t t0 = fm1_ms;
    while ((uint32_t)(fm1_ms - t0) < ms)
        fm1_service();
}

uint32_t DG_GetTicksMs(void) { fm1_wdt_feed(); return fm1_ms; }   /* (also the watchdog: level loads) */
void fm1_idle(void) { fm1_wdt_feed(); }                            /* w_wad.c: each lump inflated */
void DG_SetWindowTitle(const char *t) { (void)t; }

/* ---- the keys. Note keys (0 = F3 .. 26 = G5), matrix buttons (OCT- 0, OCT+ 1, FX 2 ... PLAY 12, REC 13) */
static const struct { uint8_t note, key; } NOTEKEY[] = {
    {0, KEY_LEFTARROW}, {2, KEY_DOWNARROW}, {4, KEY_UPARROW}, {5, KEY_RIGHTARROW},   /* F3 G3 A3 B3: turn, walk */
    {1, KEY_STRAFE_L}, {3, KEY_STRAFE_R},                                           /* F#3 G#3: strafe */
    {19, KEY_FIRE}, {21, KEY_USE}, {23, KEY_RSHIFT},                                /* C5 D5 E5: fire, open, run */
    {20, '1'}, {22, '2'}, {25, '3'},                                                /* C#5 D#5 F#5: fist, pistol, shotgun */
};
static const struct { uint8_t btn, key; } BTNKEY[] = {
    {0, KEY_STRAFE_L}, {1, KEY_STRAFE_R}, {12, KEY_FIRE}, {13, KEY_USE}, {10, KEY_TAB},   /* OCT-/+, PLAY, REC, ARP: map */
};
#define KQ 32u
static uint8_t kq_key[KQ], kq_down[KQ];
static uint32_t kq_w, kq_r;
static void kq_put(uint8_t key, uint8_t down)
{
    if (kq_w - kq_r < KQ) {
        kq_key[kq_w % KQ] = key;
        kq_down[kq_w % KQ] = down;
        kq_w++;
    }
}

static uint32_t prev_notes, prev_btns, turn_until, turn_key;

static void keys_poll(void)
{
    uint32_t n = fm1_in.notes, b = fm1_in.buttons, i;
    int32_t steps;
    for (i = 0; i < sizeof NOTEKEY / sizeof NOTEKEY[0]; i++) {
        uint32_t m = 1u << NOTEKEY[i].note;
        if ((n ^ prev_notes) & m)
            kq_put(NOTEKEY[i].key, (n & m) != 0);
    }
    for (i = 0; i < sizeof BTNKEY / sizeof BTNKEY[0]; i++) {
        uint32_t m = 1u << BTNKEY[i].btn;
        if ((b ^ prev_btns) & m)
            kq_put(BTNKEY[i].key, (b & m) != 0);
    }
    prev_notes = n;
    prev_btns = b;
    /* KNOB 1 or SELECT turns: a detent holds the turn key ~70 ms */
    steps = fm1_enc_take(2) + fm1_enc_take(0);
    if (steps) {
        uint32_t k = steps > 0 ? KEY_RIGHTARROW : KEY_LEFTARROW;
        if (turn_key && turn_key != k)
            kq_put((uint8_t)turn_key, 0);
        if (turn_key != k)
            kq_put((uint8_t)k, 1);
        turn_key = k;
        turn_until = fm1_ms + 70u * (uint32_t)(steps > 0 ? steps : -steps);
    } else if (turn_key && (int32_t)(fm1_ms - turn_until) >= 0) {
        kq_put((uint8_t)turn_key, 0);
        turn_key = 0;
    }
}

int DG_GetKey(int *pressed, unsigned char *key)
{
    keys_poll();
    if (kq_r == kq_w)
        return 0;
    *key = kq_key[kq_r % KQ];
    *pressed = kq_down[kq_r % KQ];
    kq_r++;
    return 1;
}

/* ---- errors and crashes on the screen (then the boot guard counts a failed boot) */
static char err_text[160];
static uint32_t err_len;

static void dg_show(const char *title, const char *text, uint32_t ms)
{
    uint32_t t0;
    lcd_sync();
    lcd_fill(0, 0, 240, 240, RGB(120, 0, 0));
    draw_text_box(0, 20, 240, &FONT_S, title, C_WHITE, 1);
    draw_text_box(8, 60, 224, &FONT_S, text, C_WHITE, 0);
    t0 = fm1_ms;
    while ((uint32_t)(fm1_ms - t0) < ms)
        fm1_service();                          /* the installer still reaches it */
}

static void hex8(char *b, uint32_t v)
{
    uint32_t i;
    for (i = 0; i < 8u; i++) b[i] = "0123456789ABCDEF"[(v >> (28u - 4u * i)) & 15u];
    b[8] = 0;
}

static void fm1_fault(const fm1_crash_t *c)
{
    char b[40];
    uint32_t t0;
    lcd_sync();
    lcd_fill(0, 0, 240, 240, RGB(120, 0, 0));
    draw_text_box(0, 20, 240, &FONT_S, "DOOM CRASH", C_WHITE, 1);
    hex8(b, c->vec); draw_text_box(10, 60, 220, &FONT_S, b, C_WHITE, 0);
    hex8(b, c->pc); draw_text_box(10, 80, 220, &FONT_S, b, C_WHITE, 0);
    hex8(b, c->emu); draw_text_box(10, 100, 220, &FONT_S, b, C_WHITE, 0);
    hex8(b, c->rets); draw_text_box(10, 120, 220, &FONT_S, b, C_WHITE, 0);
    t0 = fm1_ticks();
    while ((uint32_t)(fm1_ticks() - t0) < 15000u * 1000u * FM1_TICKS_PER_US)
        fm1_wdt_feed();
    fm1_reboot();
}

/* ---- newlib: stdout / stderr into a small buffer (I_Error's text), no files, a heap after the pool */
int _write(int fd, const char *p, int n)
{
    int i;
    if (fd == 2)
        for (i = 0; i < n; i++) {
            if (err_len + 1u >= sizeof err_text) err_len = 0;
            err_text[err_len++] = p[i] == '\n' ? ' ' : p[i];
            err_text[err_len] = 0;
        }
    return n;
}
int _read(int fd, char *p, int n) { (void)fd; (void)p; (void)n; return 0; }
int _open(const char *path, int flags, int mode) { (void)path; (void)flags; (void)mode; return -1; }
int _close(int fd) { (void)fd; return -1; }
int _lseek(int fd, int off, int whence) { (void)fd; (void)off; (void)whence; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
struct stat;
int _fstat(int fd, struct stat *st) { (void)fd; (void)st; return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }
int mkdir(const char *p, int m) { (void)p; (void)m; return -1; }
struct timeval { long tv_sec, tv_usec; };
int _gettimeofday(struct timeval *tv, void *tz) { (void)tz; if (tv) { tv->tv_sec = (long)(fm1_ms / 1000u); tv->tv_usec = (long)(fm1_ms % 1000u) * 1000; } return 0; }

extern uint8_t _heap_start[], _heap_end[];
static uint8_t *heap_top;
void *_sbrk(int incr)
{
    uint8_t *p;
    if (!heap_top) heap_top = _heap_start;
    if (heap_top + incr > _heap_end) {
        dg_show("DOOM: OUT OF HEAP", "", 10000);
        return (void *)-1;
    }
    p = heap_top;
    heap_top += incr;
    return p;
}

void _exit(int code)
{
    dg_show("DOOM STOPPED", err_len ? err_text : "exit", 20000);
    (void)code;
    bootguard.pending = 0;
    fm1_reboot();
    for (;;) ;
}

/* the zone: the rest of the RAM (fm1doom.ld); i_system.c I_ZoneBase takes it */
extern uint8_t _zone_start[], _zone_end[];
uint8_t *fm1_zone(int *size)
{
    *size = (int)(_zone_end - _zone_start);
    return _zone_start;
}

/* the boot logo: Freedoom's, 1.5 x (238 x 55), centred, ~2.5 s */
#include "fm1_logo.h"
static void logo_screen(void)
{
    uint32_t x, y, t0, ow = LOGO_W * 3u / 2u, oh = LOGO_H * 3u / 2u;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    lcd_window((240u - ow) / 2u, (240u - oh) / 2u, (240u - ow) / 2u + ow - 1u, (240u - oh) / 2u + oh - 1u);
    for (y = 0; y < oh; y++) {
        const uint8_t *src = LOGO_PX + (y * 2u / 3u) * LOGO_W;
        uint16_t *d = dg_line[y & 1u];
        for (x = 0; x < ow; x++)
            d[x] = LOGO_PAL[src[x * 2u / 3u]];
        lcd_data(d, ow * 2u);
    }
    t0 = fm1_ms;
    while ((uint32_t)(fm1_ms - t0) < 2500u)
        fm1_service();
}

/* the controls, before the game: until a key or button, at most 8 s */
static void controls_screen(void)
{
    static const char *const L[][2] = {          /* (8 px a character: 13 left, 16 right) */
        {"A3 / G3", "FORWARD / BACK"}, {"F3 / B3", "TURN (KNOB 1)"}, {"F#3 / G#3", "STRAFE (OCT-/+)"},
        {"C5", "FIRE (PLAY)"}, {"D5", "OPEN, USE (REC)"}, {"E5", "RUN"},
        {"C#5 D#5 F#5", "WEAPON 1 2 3"}, {"ARP", "MAP"}, {"DEAD?", "D5 OR REC"},
    };
    uint32_t i, t0;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 4, 240, &FONT_S, "FM-1 DOOM", RGB(255, 60, 40), 1);
    for (i = 0; i < sizeof L / sizeof L[0]; i++) {
        draw_text_box(4, 28 + i * 21, 104, &FONT_S, L[i][0], RGB(240, 200, 40), 0);
        draw_text_box(108, 28 + i * 21, 128, &FONT_S, L[i][1], C_WHITE, 0);
    }
    draw_text_box(0, 224, 240, &FONT_S, "PRESS ANY KEY", RGB(130, 130, 130), 1);
    t0 = fm1_ms;
    while ((uint32_t)(fm1_ms - t0) < 8000u) {
        fm1_service();
        if ((uint32_t)(fm1_ms - t0) > 400u && (fm1_in.notes || fm1_in.buttons))
            break;
    }
    while (fm1_in.notes || fm1_in.buttons)
        fm1_service();                              /* (the key that ends it does not walk) */
    prev_notes = prev_btns = 0;
    lcd_fill(0, 0, 240, 240, C_BLACK);
}

static void doom_main(void)
{
    logo_screen();
    controls_screen();
    static char *argv[] = {"fm1doom", "-iwad", "freedm.wad", "-warp", "1", "-skill", "3", "-nosound", "-nomusic", 0};
    doomgeneric_Create(9, argv);
    for (;;)
        doomgeneric_Tick();
}
