// adapted from flute-of-age/include/transition_tile.h (ultra64.h -> game_types.h)
#ifndef TRANSITION_TILE_H
#define TRANSITION_TILE_H

#include "game_types.h"

typedef struct TransitionTileVtxData {
    /* 0x0 */ f32 x;
    /* 0x4 */ f32 y;
} TransitionTileVtxData; // size = 0x8

typedef struct TransitionTile {
    /* 0x00 */ s32 cols;
    /* 0x04 */ s32 rows;
    /* 0x08 */ s32 frame;
    /* 0x0C */ TransitionTileVtxData* vtxData;
    // native: vertex/matrix/display-list buffers removed (tile transition drawn natively)
    /* 0xDC */ u16* zBuffer;
} TransitionTile; // size = 0xE0

TransitionTile* TransitionTile_Init(TransitionTile* this, s32 cols, s32 rows);
void TransitionTile_Destroy(TransitionTile* this);
void TransitionTile_Update(TransitionTile* this);
void TransitionTile_Draw(TransitionTile* this); // native draw

#endif
