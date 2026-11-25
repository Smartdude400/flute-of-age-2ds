// gfxprint.c (rewritten: native) - see libu64/gfxprint.h. Character cells and colours as the original
// printer; the glyphs come from the converted debug font of the native 2D layer.
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "libu64/gfxprint.h"
#include "engine/gfx2d.h"

void GfxPrint_Init(GfxPrint* this) {
    memset(this, 0, sizeof(*this));
    this->flags = GFXP_FLAG_SHADOW | GFXP_FLAG_UPDATE;
    this->r = this->g = this->b = this->a = 255;
}

void GfxPrint_Destroy(GfxPrint* this) {
    (void)this;
}

void GfxPrint_Open(GfxPrint* this, struct Gfx* dList) {
    this->dList = dList;
    this->flags |= GFXP_FLAG_OPEN;
}

struct Gfx* GfxPrint_Close(GfxPrint* this) {
    this->flags &= ~GFXP_FLAG_OPEN;
    return this->dList;
}

void GfxPrint_SetColor(GfxPrint* this, u32 r, u32 g, u32 b, u32 a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

void GfxPrint_SetPosPx(GfxPrint* this, s32 x, s32 y) {
    this->posX = this->baseX + x;
    this->posY = this->baseY + y;
}

void GfxPrint_SetPos(GfxPrint* this, s32 x, s32 y) {
    GfxPrint_SetPosPx(this, x * GFX_CHAR_X_SPACING, y * GFX_CHAR_Y_SPACING);
}

void GfxPrint_SetBasePosPx(GfxPrint* this, s32 x, s32 y) {
    this->baseX = x;
    this->baseY = y;
}

s32 GfxPrint_Printf(GfxPrint* this, const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    s32 n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    char* line = buf;
    for (;;) {
        char* nl = strchr(line, '\n');
        if (nl != NULL) {
            *nl = '\0';
        }
        if (this->flags & GFXP_FLAG_SHADOW) {
            Gfx2D_Text(this->posX + 1, this->posY + 1, 0, 0, 0, this->a, line);
        }
        Gfx2D_Text(this->posX, this->posY, this->r, this->g, this->b, this->a, line);
        if (nl == NULL) {
            this->posX += strlen(line) * GFX_CHAR_X_SPACING;
            break;
        }
        this->posX = this->baseX;
        this->posY += GFX_CHAR_Y_SPACING;
        line = nl + 1;
    }
    return n;
}
