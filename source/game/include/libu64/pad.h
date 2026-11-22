// adapted from flute-of-age/include/libu64/pad.h
// The platform controller record type is replaced by the game-side PadState (same fields and
// widths: button bitmask in the game's BTN_* layout, signed stick axes, error number).
#ifndef LIBU64_PAD_H
#define LIBU64_PAD_H

#include "game_types.h"

typedef struct PadState {
    /* 0x00 */ u16 button;
    /* 0x02 */ s8 stick_x;
    /* 0x03 */ s8 stick_y;
    /* 0x04 */ u8 errNo;
} PadState; // size = 0x06

typedef struct Input {
    /* 0x00 */ PadState cur;
    /* 0x06 */ PadState prev;
    /* 0x0C */ PadState press; // X/Y store delta from last frame
    /* 0x12 */ PadState rel; // X/Y store adjusted
} Input; // size = 0x18

#define MAXCONTROLLERS 4

void PadUtils_Init(Input* input);
void PadUtils_ResetPressRel(Input* input);
u32 PadUtils_CheckCurExact(Input* input, u16 value);
u32 PadUtils_CheckCur(Input* input, u16 key);
u32 PadUtils_CheckPressed(Input* input, u16 key);
u32 PadUtils_CheckReleased(Input* input, u16 key);
u16 PadUtils_GetCurButton(Input* input);
u16 PadUtils_GetPressButton(Input* input);
s8 PadUtils_GetCurX(Input* input);
s8 PadUtils_GetCurY(Input* input);
void PadUtils_SetRelXY(Input* input, s32 x, s32 y);
s8 PadUtils_GetRelXImpl(Input* input);
s8 PadUtils_GetRelYImpl(Input* input);
s8 PadUtils_GetRelX(Input* input);
s8 PadUtils_GetRelY(Input* input);
s8 PadUtils_GetPressX(Input* input);
s8 PadUtils_GetPressY(Input* input);
void PadUtils_UpdateRelXY(Input* input);

// Pad manager (rewritten, engine/input.c): polled every VBlank, consumed once per game frame.
void PadMgr_RequestPadData(Input* inputs, s32 gameRequest);

#endif
