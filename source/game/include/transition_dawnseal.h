// adapted from flute-of-age/include/transition_dawnseal.h (ultra64.h -> game_types.h)
#ifndef TRANSITION_DAWNSEAL_H
#define TRANSITION_DAWNSEAL_H

#include "game_types.h"
#include "color.h"

typedef struct TransitionDawnseal {
    /* 0x000 */ Color_RGBA8_u32 color;
    /* 0x004 */ f32 transPos;
    /* 0x008 */ f32 step;
    /* 0x00C */ s32 state;
    /* 0x010 */ s32 type;
    // (platform matrix field removed: transition geometry is drawn natively)
    /* 0x058 */ s32 frame;
    // (platform matrix field removed: transition geometry is drawn natively)
} TransitionDawnseal; // size = 0x1E0

void TransitionDawnseal_Start(void* thisx);
void* TransitionDawnseal_Init(void* thisx);
void TransitionDawnseal_Destroy(void* thisx);
void TransitionDawnseal_Update(void* thisx, s32 updateRate);
void TransitionDawnseal_SetColor(void* thisx, u32 color);
void TransitionDawnseal_SetType(void* thisx, s32 type);
void TransitionDawnseal_Draw(void* thisx); // native draw
s32 TransitionDawnseal_IsDone(void* thisx);

#endif
