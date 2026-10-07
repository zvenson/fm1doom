/* SPDX-License-Identifier: GPL-2.0-or-later */
/* FM-1 Doom: a small inflate (raw deflate, RFC 1951) for the compressed lumps of the WAD image in flash.
 * Canonical Huffman decoding by counts and symbols, as in Mark Adler's puff; no window of its own: the
 * output buffer is the window (a lump is inflated whole). Returns the bytes written, -1 on bad data. */
#include <stdint.h>
#include <string.h>
#include "fm1_inflate.h"

typedef struct { const uint8_t *in; uint32_t inlen, pos, bitbuf, bitcnt; uint8_t *out; uint32_t outlen, outpos; } st_t;
typedef struct { uint16_t count[16]; uint16_t symbol[288]; } huff_t;

static int bits(st_t *s, int need)
{
    uint32_t v = s->bitbuf;
    while (s->bitcnt < (uint32_t)need) {
        if (s->pos >= s->inlen) return -1;
        v |= (uint32_t)s->in[s->pos++] << s->bitcnt;
        s->bitcnt += 8;
    }
    s->bitbuf = v >> need;
    s->bitcnt -= need;
    return (int)(v & ((1u << need) - 1u));
}

static int decode(st_t *s, const huff_t *h)
{
    int code = 0, first = 0, index = 0, len, b;
    for (len = 1; len < 16; len++) {
        if ((b = bits(s, 1)) < 0) return -1;
        code |= b;
        if (code - h->count[len] < first)
            return h->symbol[index + (code - first)];
        index += h->count[len];
        first += h->count[len];
        first <<= 1;
        code <<= 1;
    }
    return -1;
}

static int construct(huff_t *h, const uint8_t *length, int n)
{
    uint16_t offs[16];
    int sym, len, left = 1;
    memset(h->count, 0, sizeof h->count);
    for (sym = 0; sym < n; sym++) h->count[length[sym]]++;
    if (h->count[0] == n) return 0;
    for (len = 1; len < 16; len++) {
        left <<= 1;
        left -= h->count[len];
        if (left < 0) return -1;
    }
    offs[1] = 0;
    for (len = 1; len < 15; len++) offs[len + 1] = offs[len] + h->count[len];
    for (sym = 0; sym < n; sym++)
        if (length[sym]) h->symbol[offs[length[sym]]++] = (uint16_t)sym;
    return left;
}

static const uint16_t LBASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const uint8_t LEXT[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const uint16_t DBASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const uint8_t DEXT[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static int codes(st_t *s, const huff_t *lc, const huff_t *dc)
{
    int sym, len, e;
    uint32_t dist;
    for (;;) {
        if ((sym = decode(s, lc)) < 0) return -1;
        if (sym < 256) {
            if (s->outpos >= s->outlen) return -1;
            s->out[s->outpos++] = (uint8_t)sym;
        } else if (sym == 256) {
            return 0;
        } else {
            sym -= 257;
            if (sym >= 29) return -1;
            if ((e = bits(s, LEXT[sym])) < 0) return -1;
            len = LBASE[sym] + e;
            if ((sym = decode(s, dc)) < 0 || sym >= 30) return -1;
            if ((e = bits(s, DEXT[sym])) < 0) return -1;
            dist = DBASE[sym] + (uint32_t)e;
            if (dist > s->outpos || s->outpos + (uint32_t)len > s->outlen) return -1;
            while (len--) { s->out[s->outpos] = s->out[s->outpos - dist]; s->outpos++; }
        }
    }
}

static huff_t lencode, distcode;                 /* (static: 1.2 KB off the small stack) */

static int fixed(st_t *s)                       /* (rebuilt each time: dynamic blocks share the tables) */
{
    uint8_t l[288];
    int i;
    for (i = 0; i < 144; i++) l[i] = 8;
    for (; i < 256; i++) l[i] = 9;
    for (; i < 280; i++) l[i] = 7;
    for (; i < 288; i++) l[i] = 8;
    construct(&lencode, l, 288);
    for (i = 0; i < 30; i++) l[i] = 5;
    construct(&distcode, l, 30);
    return codes(s, &lencode, &distcode);
}

static int dynamic(st_t *s)
{
    static const uint8_t ORDER[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    uint8_t lengths[320];
    int nlen, ndist, ncode, index, sym, len, e;
    if ((nlen = bits(s, 5)) < 0 || (ndist = bits(s, 5)) < 0 || (ncode = bits(s, 4)) < 0) return -1;
    nlen += 257; ndist += 1; ncode += 4;
    if (nlen > 286 || ndist > 30) return -1;
    for (index = 0; index < ncode; index++) {
        if ((e = bits(s, 3)) < 0) return -1;
        lengths[ORDER[index]] = (uint8_t)e;
    }
    for (; index < 19; index++) lengths[ORDER[index]] = 0;
    if (construct(&lencode, lengths, 19) != 0) return -1;
    index = 0;
    while (index < nlen + ndist) {
        if ((sym = decode(s, &lencode)) < 0) return -1;
        if (sym < 16) {
            lengths[index++] = (uint8_t)sym;
        } else {
            len = 0;
            if (sym == 16) {
                if (index == 0) return -1;
                len = lengths[index - 1];
                if ((e = bits(s, 2)) < 0) return -1;
                sym = 3 + e;
            } else if (sym == 17) {
                if ((e = bits(s, 3)) < 0) return -1;
                sym = 3 + e;
            } else {
                if ((e = bits(s, 7)) < 0) return -1;
                sym = 11 + e;
            }
            if (index + sym > nlen + ndist) return -1;
            while (sym--) lengths[index++] = (uint8_t)len;
        }
    }
    if (lengths[256] == 0) return -1;
    if (construct(&lencode, lengths, nlen) < 0) return -1;
    if (construct(&distcode, lengths + nlen, ndist) < 0) return -1;
    return codes(s, &lencode, &distcode);
}

int fm1_inflate(const uint8_t *in, uint32_t inlen, uint8_t *out, uint32_t outlen)
{
    st_t s;
    int last, type, err;
    memset(&s, 0, sizeof s);
    s.in = in; s.inlen = inlen; s.out = out; s.outlen = outlen;
    do {
        if ((last = bits(&s, 1)) < 0 || (type = bits(&s, 2)) < 0) return -1;
        if (type == 0) {                                 /* stored */
            uint32_t n;
            s.bitbuf = 0; s.bitcnt = 0;
            if (s.pos + 4 > s.inlen) return -1;
            n = s.in[s.pos] | s.in[s.pos + 1] << 8;
            s.pos += 4;
            if (s.pos + n > s.inlen || s.outpos + n > s.outlen) return -1;
            memcpy(s.out + s.outpos, s.in + s.pos, n);
            s.pos += n; s.outpos += n;
            err = 0;
        } else if (type == 1) {
            err = fixed(&s);
        } else if (type == 2) {
            err = dynamic(&s);
        } else {
            return -1;
        }
        if (err) return -1;
    } while (!last);
    return (int)s.outpos;
}
