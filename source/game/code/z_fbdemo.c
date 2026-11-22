// adapted from flute-of-age/src/code/z_fbdemo.c
/**
 * @file z_fbdemo.c
 *
 * This file implements an unused transition system that takes the current screen, partitions it into large tiles, and
 * can apply an effect to them.
 *
 * The screen is divided into 7 rows and 10 columns of tiles. (`gScreenWidth`/ 10 = `gScreenHeight` / 7 = 0x20)
 *
 * @note The only coded effect has a visual effect to blend the tiles to a single point, which looks like the screen
 * gets sucked into.
 */
#include "transition_tile.h"
#include "engine/stub.h"
#include "engine/gfx2d.h"
#include "engine/gfx3d.h"

#include "attributes.h"
#include "libc64/malloc.h"
#include "libc64/sleep.h"
#include "libu64/debug.h"
#include "gfx.h"
#include "printf.h"
#include "z_math.h"
#include "transition_instances.h"

// color framebuffer
extern u16 D_0F000000[];

// Unused background; a blue rectangle with a grey border that fills the screen


#define SET_VERTEX(vtx, x, y, z, s, t, nx, ny, nz, alpha) \
    {                                                     \
        Vtx_tn* vtxn = &(vtx)->n;                         \
        vtxn->ob[0] = (x);                                \
        vtxn->ob[1] = (y);                                \
        vtxn->ob[2] = (z);                                \
        vtxn->flag = 0;                                   \
        vtxn->tc[0] = (s);                                \
        vtxn->tc[1] = (t);                                \
        vtxn->n[0] = (nx);                                \
        vtxn->n[1] = (ny);                                \
        vtxn->n[2] = (nz);                                \
        vtxn->a = (alpha);                                \
    }                                                     \
    (void)0

void TransitionTile_InitGraphics(TransitionTile* this) {
    PRESENTATION_STUB("TransitionTile_InitGraphics");
}

void TransitionTile_InitVtxData(TransitionTile* this) {
    s32 row;
    s32 col;

    for (row = 0; row < this->rows + 1; row++) {
        for (col = 0; col < this->cols + 1; col++) {
            (this->vtxData + col + row * (this->cols + 1))->x = col * 0x20;
            (this->vtxData + col + row * (this->cols + 1))->y = row * 0x20;
        }
    }
}

void TransitionTile_Destroy(TransitionTile* this) {
    PRESENTATION_STUB("TransitionTile_Destroy");
}

TransitionTile* TransitionTile_Init(TransitionTile* this, s32 cols, s32 rows) {
    PRESENTATION_STUB("TransitionTile_Init");
    return 0;
}

void TransitionTile_SetVtx(TransitionTile* this) {
    PRESENTATION_STUB("TransitionTile_SetVtx");
}

void TransitionTile_Draw(TransitionTile* this) {
    PRESENTATION_STUB("TransitionTile_Draw");
}

/**
 * Blends tiles which has the visual effect of sucking those tiles into a single point
 */
void TransitionTile_Suck(TransitionTile* this) {
    s32 row;
    s32 col;
    f32 diffX;
    f32 diffY;
    f32 scale;

    for (row = 0; row < this->rows + 1; row++) {
        for (col = 0; col < this->cols + 1; col++) {
            diffX = (this->vtxData + col + row * (this->cols + 1))->x - (this->vtxData + 5 + 4 * (this->cols + 1))->x;
            diffY = (this->vtxData + col + row * (this->cols + 1))->y - (this->vtxData + 5 + 4 * (this->cols + 1))->y;
            scale = (SQ(diffX) + SQ(diffY)) / 100.0f;
            if (scale != 0.0f) {
                if (scale < 1.0f) {
                    scale = 1.0f;
                }
                (this->vtxData + col + row * (this->cols + 1))->x -= diffX / scale;
                (this->vtxData + col + row * (this->cols + 1))->y -= diffY / scale;
            }
        }
    }
}

void TransitionTile_Update(UNUSED TransitionTile* this) {
}

s32 func_800B23F0(UNUSED TransitionTile* this) {
    return 0;
}
