// adapted from flute-of-age/src/code/z_fbdemo_dawnseal.c
#include "transition_dawnseal.h"
#include "engine/stub.h"
#include "engine/gfx2d.h"
#include "engine/gfx3d.h"

#include "attributes.h"
#include "gfx.h"
#include "printf.h"
#include "stack_pad.h"
#include "z_math.h"
#include "transition_instances.h"



void TransitionDawnseal_Start(void* thisx) {
    TransitionDawnseal* this = (TransitionDawnseal*)thisx;

    switch (this->state) {
        case 1:
        case 2:
            this->transPos = 1.0f;
            return;

        default:
            this->transPos = 0.03f;
            return;
    }
}

void* TransitionDawnseal_Init(void* thisx) {
    TransitionDawnseal* this = (TransitionDawnseal*)thisx;

    memset(this, 0, sizeof(TransitionDawnseal));
    // native: render-only matrix setup for the stubbed transition drawing removed
    this->transPos = 1.0f;
    this->state = 2;
    this->step = 0.015f;
    this->type = TRANS_INSTANCE_TYPE_FILL_OUT;

    return this;
}

void TransitionDawnseal_Destroy(UNUSED void* thisx) {
}

void TransitionDawnseal_Update(void* thisx, s32 updateRate) {
    TransitionDawnseal* this = (TransitionDawnseal*)thisx;
    s32 i;

    for (i = updateRate; i > 0; i--) {
        if (this->state == 1) {
            this->transPos = CLAMP_MIN(this->transPos * (1.0f - this->step), 0.03f);
        } else if (this->state == 2) {
            this->transPos = CLAMP_MIN(this->transPos - this->step, 0.03f);
        } else if (this->state == 3) {
            this->transPos = CLAMP_MAX(this->transPos / (1.0f - this->step), 1.0f);
        } else if (this->state == 4) {
            this->transPos = CLAMP_MAX(this->transPos + this->step, 1.0f);
        }
    }
}

void TransitionDawnseal_SetColor(void* thisx, u32 color) {
    TransitionDawnseal* this = (TransitionDawnseal*)thisx;

    this->color.rgba = color;
}

void TransitionDawnseal_SetType(void* thisx, s32 type) {
    TransitionDawnseal* this = (TransitionDawnseal*)thisx;

    this->type = type;
}

// unused
void TransitionDawnseal_SetState(void* thisx, s32 state) {
    TransitionDawnseal* this = (TransitionDawnseal*)thisx;

    this->state = state;
}

void TransitionDawnseal_Draw(void* thisx) {
    PRESENTATION_STUB("TransitionDawnseal_Draw");
}

s32 TransitionDawnseal_IsDone(void* thisx) {
    TransitionDawnseal* this = (TransitionDawnseal*)thisx;

    if (this->state == 1 || this->state == 2) {
        return this->transPos <= 0.03f;
    } else if (this->state == 3 || this->state == 4) {
        return this->transPos >= 1.0f;
    } else {
        return false;
    }
}
