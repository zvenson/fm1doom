/* every lump of a WAD through zlib (raw deflate, written by the test script) and back through fm1_inflate */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fm1_inflate.h"
int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "rb");
    uint32_t n = 0, bad = 0, raw, cl;
    while (fread(&raw, 4, 1, f) == 1 && fread(&cl, 4, 1, f) == 1) {
        uint8_t *a = malloc(raw), *c = malloc(cl), *o = malloc(raw + 1);
        fread(a, 1, raw, f); fread(c, 1, cl, f);
        if (fm1_inflate(c, cl, o, raw) != (int)raw || memcmp(a, o, raw)) bad++;
        n++; free(a); free(c); free(o);
    }
    printf("inflate: %u lumps, %u bad\n", n, bad);
    return bad != 0;
}
