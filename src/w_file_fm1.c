// SPDX-License-Identifier: GPL-2.0-or-later
// FM-1 Doom: the WAD image lies in the app's flash (XIP, tools/mkimage.py, linked as fm1_wad_image) and is read
// in place; the path is only a name (freedm.wad: the game mode).
#ifdef FM1_DEVICE
#include <string.h>
#include "w_file.h"
#include "z_zone.h"

extern const unsigned char fm1_wad_image[];
extern const unsigned int fm1_wad_len;
extern wad_file_class_t fm1_wad_file;

static wad_file_t *W_FM1_OpenFile(char *path)
{
    wad_file_t *w = Z_Malloc(sizeof(wad_file_t), PU_STATIC, 0);
    (void)path;
    w->file_class = &fm1_wad_file;
    w->mapped = (byte *)fm1_wad_image;
    w->length = fm1_wad_len;
    return w;
}

static void W_FM1_CloseFile(wad_file_t *wad) { Z_Free(wad); }

static size_t W_FM1_Read(wad_file_t *wad, unsigned int offset, void *buffer, size_t len)
{
    if (offset >= wad->length) return 0;
    if (len > wad->length - offset) len = wad->length - offset;
    memcpy(buffer, wad->mapped + offset, len);
    return len;
}

wad_file_class_t fm1_wad_file = { W_FM1_OpenFile, W_FM1_CloseFile, W_FM1_Read };
#endif
