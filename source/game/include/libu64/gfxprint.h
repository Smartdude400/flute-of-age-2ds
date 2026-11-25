// gfxprint.h (rewritten: native). The original debug text printer (GfxPrint), with its API; text is drawn
// by the native 2D layer (engine/gfx2d.c) with the same debug font converted from the ROM.
#ifndef LIBU64_GFXPRINT_H
#define LIBU64_GFXPRINT_H
#include "game_types.h"

#define GFX_CHAR_X_SPACING 8
#define GFX_CHAR_Y_SPACING 8

#define GFXP_FLAG_HIRAGANA (1 << 0)
#define GFXP_FLAG_RAINBOW (1 << 1)
#define GFXP_FLAG_SHADOW (1 << 2)
#define GFXP_FLAG_UPDATE (1 << 3)
#define GFXP_FLAG_ENLARGE (1 << 6)
#define GFXP_FLAG_OPEN (1 << 7)

struct Gfx;
typedef struct GfxPrint {
    struct Gfx* dList; // the caller's list cursor (handed back by GfxPrint_Close)
    u16 posX, posY;    // pixels
    u16 baseX, baseY;
    u8 flags;
    u8 r, g, b, a;
} GfxPrint;

void GfxPrint_SetColor(GfxPrint* this, u32 r, u32 g, u32 b, u32 a);
void GfxPrint_SetPosPx(GfxPrint* this, s32 x, s32 y);
void GfxPrint_SetPos(GfxPrint* this, s32 x, s32 y);
void GfxPrint_SetBasePosPx(GfxPrint* this, s32 x, s32 y);
void GfxPrint_Init(GfxPrint* this);
void GfxPrint_Destroy(GfxPrint* this);
void GfxPrint_Open(GfxPrint* this, struct Gfx* dList);
struct Gfx* GfxPrint_Close(GfxPrint* this);
s32 GfxPrint_Printf(GfxPrint* this, const char* fmt, ...);
#endif
