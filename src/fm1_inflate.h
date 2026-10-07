/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FM1_INFLATE_H
#define FM1_INFLATE_H
#include <stdint.h>
/* raw deflate into out[outlen]; the bytes written, -1 on bad data */
int fm1_inflate(const uint8_t *in, uint32_t inlen, uint8_t *out, uint32_t outlen);
#endif
