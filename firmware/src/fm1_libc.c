/* SPDX-License-Identifier: GPL-3.0-only */
/* FM-1 Doom: what the SDK's trimmed newlib lacks. A formatter (vsnprintf and the printf family: stderr and
 * stdout go to _write, doom_glue.c keeps stderr for the error screen), a heap that only grows (newlib's _sbrk;
 * Doom mallocs a few KB at start, its zone does the rest), and no files (fopen fails: Doom keeps its defaults).
 * Built with newlib's headers, apart from the platform's unity file. */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int _write(int fd, const char *p, int n);
void *_sbrk(int incr);
void _exit(int code);

/* ---- the formatter: flags - 0 + space, width and precision (or *), h l ll z, d i u x X o c s p % (f as %d) */
typedef struct { char *buf; size_t cap, n; } out_t;
static void put(out_t *o, char c) { if (o->n + 1 < o->cap) o->buf[o->n] = c; o->n++; }

int vsnprintf(char *buf, size_t cap, const char *f, va_list ap)
{
    out_t o = {buf, cap, 0};
    for (; *f; f++) {
        int left = 0, zero = 0, plus = 0, space = 0, width = 0, prec = -1, lng = 0, neg = 0, len, i, pad;
        char tmp[24], *s;
        unsigned long long v;
        unsigned base = 10;
        if (*f != '%') { put(&o, *f); continue; }
        for (f++;; f++) {
            if (*f == '-') left = 1; else if (*f == '0') zero = 1; else if (*f == '+') plus = 1;
            else if (*f == ' ') space = 1; else if (*f == '#') ; else break;
        }
        if (*f == '*') { width = va_arg(ap, int); if (width < 0) { left = 1; width = -width; } f++; }
        else while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
        if (*f == '.') {
            f++; prec = 0;
            if (*f == '*') { prec = va_arg(ap, int); f++; }
            else while (*f >= '0' && *f <= '9') prec = prec * 10 + (*f++ - '0');
        }
        while (*f == 'l' || *f == 'h' || *f == 'z') { if (*f == 'l') lng++; if (*f == 'z') lng = 1; f++; }
        switch (*f) {
        case 'c': tmp[0] = (char)va_arg(ap, int); tmp[1] = 0; s = tmp; len = 1; prec = -1; goto text;
        case 's': s = va_arg(ap, char *); if (!s) s = "(null)";
            len = 0; while (s[len] && (prec < 0 || len < prec)) len++;
            prec = -1;
        text:
            pad = width > len ? width - len : 0;
            if (!left) while (pad--) put(&o, ' ');
            for (i = 0; i < len; i++) put(&o, s[i]);
            if (left) while (pad-- > 0) put(&o, ' ');
            continue;
        case '%': put(&o, '%'); continue;
        case 'p': v = (uintptr_t)va_arg(ap, void *); base = 16; break;
        case 'x': case 'X': base = 16; /* fall through */
        case 'o': if (*f == 'o') base = 8; /* fall through */
        case 'u':
            v = lng >= 2 ? va_arg(ap, unsigned long long) : lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned);
            break;
        case 'f': case 'g': case 'e': { double d = va_arg(ap, double); long long w = (long long)d;
            neg = w < 0; v = (unsigned long long)(neg ? -w : w); break; }
        case 'd': case 'i': default: {
            long long w = lng >= 2 ? va_arg(ap, long long) : lng ? va_arg(ap, long) : va_arg(ap, int);
            neg = w < 0; v = (unsigned long long)(neg ? -w : w); break; }
        }
        len = 0;
        do { unsigned d = (unsigned)(v % base); tmp[len++] = (char)(d < 10 ? '0' + d : (*f == 'X' ? 'A' : 'a') + d - 10); v /= base; } while (v);
        while (prec >= 0 && len < prec && len < (int)sizeof tmp) tmp[len++] = '0';
        {
            char sign = neg ? '-' : plus ? '+' : space ? ' ' : 0;
            int total = len + (sign != 0);
            pad = width > total ? width - total : 0;
            if (!left && !(zero && prec < 0)) while (pad > 0) { put(&o, ' '); pad--; }
            if (sign) put(&o, sign);
            if (!left && zero && prec < 0) while (pad > 0) { put(&o, '0'); pad--; }
            while (len) put(&o, tmp[--len]);
            while (left && pad > 0) { put(&o, ' '); pad--; }
        }
    }
    if (cap) buf[o.n < cap ? o.n : cap - 1] = 0;
    return (int)o.n;
}

int snprintf(char *b, size_t n, const char *f, ...) { va_list ap; int r; va_start(ap, f); r = vsnprintf(b, n, f, ap); va_end(ap); return r; }
int sprintf(char *b, const char *f, ...) { va_list ap; int r; va_start(ap, f); r = vsnprintf(b, 1u << 16, f, ap); va_end(ap); return r; }

static int vout(int fd, const char *f, va_list ap)
{
    char b[160];
    int n = vsnprintf(b, sizeof b, f, ap);
    _write(fd, b, n < (int)sizeof b ? n : (int)sizeof b - 1);
    return n;
}
int vfprintf(FILE *fp, const char *f, va_list ap) { (void)fp; return vout(2, f, ap); }
int fprintf(FILE *fp, const char *f, ...) { va_list ap; int r; va_start(ap, f); r = vfprintf(fp, f, ap); va_end(ap); return r; }
int printf(const char *f, ...) { va_list ap; int r; va_start(ap, f); r = vout(1, f, ap); va_end(ap); return r; }
int puts(const char *s) { _write(1, s, (int)strlen(s)); _write(1, "\n", 1); return 1; }
int putchar(int c) { char ch = (char)c; _write(1, &ch, 1); return c; }

/* ---- the heap: grows only (a header keeps each block's size for realloc) */
void *malloc(size_t n)
{
    size_t *p;
    n = (n + 7u) & ~(size_t)7u;
    p = _sbrk((int)(n + 8u));
    if (p == (void *)-1) return NULL;
    p[0] = n;
    return p + 2;
}
void free(void *p) { (void)p; }
void *calloc(size_t a, size_t b) { void *p = malloc(a * b); if (p) memset(p, 0, a * b); return p; }
void *realloc(void *p, size_t n)
{
    void *q;
    if (!p) return malloc(n);
    if (((size_t *)p)[-2] >= n) return p;
    q = malloc(n);
    if (q) memcpy(q, p, ((size_t *)p)[-2]);
    return q;
}
char *strdup(const char *s) { size_t n = strlen(s) + 1; char *d = malloc(n); if (d) memcpy(d, s, n); return d; }

/* ---- no files, no environment */
FILE *fopen(const char *p, const char *m) { (void)p; (void)m; return NULL; }
int fclose(FILE *f) { (void)f; return 0; }
int fflush(FILE *f) { (void)f; return 0; }
size_t fread(void *p, size_t a, size_t b, FILE *f) { (void)p; (void)a; (void)b; (void)f; return 0; }
size_t fwrite(const void *p, size_t a, size_t b, FILE *f) { (void)p; (void)f; return a * b; }
int fseek(FILE *f, long o, int w) { (void)f; (void)o; (void)w; return -1; }
long ftell(FILE *f) { (void)f; return 0; }
int remove(const char *p) { (void)p; return -1; }
int rename(const char *a, const char *b) { (void)a; (void)b; return -1; }
char *getenv(const char *n) { (void)n; return NULL; }
int sscanf(const char *s, const char *f, ...) { (void)s; (void)f; return 0; }
double atof(const char *s) { return (double)atoi(s); }
void exit(int code) { _exit(code); }
