// engine/ptex.h (rewritten: native)
// .ptex texture files (format written by tools/pica_tex.py): header + texel data already in the
// PICA200 tiled layout (8x8 tiles, Morton order inside a tile, first row = top of the image).
#pragma once
#include <citro3d.h>
#include "engine/types.h"

typedef struct PTexHeader {
    char magic[4];   // "PTEX"
    u16 width, height;
    u8 format;       // GPU_TEXCOLOR
    u8 flags;
    u16 pad;
    u32 dataSize;
} PTexHeader;

bool PTex_Load(const char* path, C3D_Tex* out); // RomFS path ("/font/x.ptex"); linear memory
