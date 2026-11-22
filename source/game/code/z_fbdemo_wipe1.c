// adapted from flute-of-age/src/code/z_fbdemo_wipe1.c
#include "transition_wipe.h"
#include "engine/stub.h"
#include "engine/gfx2d.h"
#include "engine/gfx3d.h"

#include "attributes.h"
#include "gfx.h"
#include "save.h"
#include "stack_pad.h"
#include "tex_len.h"
#include "transition_instances.h"

typedef enum TransitionWipeDirection {
    /* 0 */ TRANS_WIPE_DIR_IN,
    /* 1 */ TRANS_WIPE_DIR_OUT
} TransitionWipeDirection;


#define sTransWipeTex_WIDTH 64
#define sTransWipeTex_HEIGHT 64


// unused.

void TransitionWipe_Start(void* thisx) {
    TransitionWipe* this = (TransitionWipe*)thisx;

    this->isDone = false;

    if (this->direction != TRANS_WIPE_DIR_IN) {
        this->texY = (s32)(83.25f * (1 << 2));
    } else {
        this->texY = (s32)(153.0f * (1 << 2));
    }
    // native: render-only matrix setup for the stubbed transition drawing removed
    // native: render-only matrix setup for the stubbed transition drawing removed
}

void* TransitionWipe_Init(void* thisx) {
    TransitionWipe* this = (TransitionWipe*)thisx;

    memset(this, 0, sizeof(TransitionWipe));
    return this;
}

void TransitionWipe_Destroy(UNUSED void* thisx) {
}

void TransitionWipe_Update(void* thisx, s32 updateRate) {
    TransitionWipe* this = (TransitionWipe*)thisx;

    if (this->direction != TRANS_WIPE_DIR_IN) {
        this->texY += (((void)0, gSaveContext.transWipeSpeed) * 3) / updateRate;
        if (this->texY >= (s32)(153.0f * (1 << 2))) {
            this->texY = (s32)(153.0f * (1 << 2));
            this->isDone = true;
        }
    } else {
        this->texY -= (((void)0, gSaveContext.transWipeSpeed) * 3) / updateRate;
        if (this->texY <= (s32)(83.25f * (1 << 2))) {
            this->texY = (s32)(83.25f * (1 << 2));
            this->isDone = true;
        }
    }
}

void TransitionWipe_Draw(void* thisx) {
    PRESENTATION_STUB("TransitionWipe_Draw");
}

s32 TransitionWipe_IsDone(void* thisx) {
    TransitionWipe* this = (TransitionWipe*)thisx;

    return this->isDone;
}

void TransitionWipe_SetType(void* thisx, s32 type) {
    TransitionWipe* this = (TransitionWipe*)thisx;

    if (type == TRANS_INSTANCE_TYPE_FILL_OUT) {
        this->direction = TRANS_WIPE_DIR_OUT;
    } else {
        this->direction = TRANS_WIPE_DIR_IN;
    }

    if (this->direction != TRANS_WIPE_DIR_IN) {
        this->texY = (s32)(83.25f * (1 << 2));
    } else {
        this->texY = (s32)(153.0f * (1 << 2));
    }
}

void TransitionWipe_SetColor(void* thisx, u32 color) {
    TransitionWipe* this = (TransitionWipe*)thisx;

    this->color.rgba = color;
}

void TransitionWipe_SetUnkColor(void* thisx, u32 color) {
    TransitionWipe* this = (TransitionWipe*)thisx;

    this->unkColor.rgba = color;
}
