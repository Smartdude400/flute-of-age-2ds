// adapted from flute-of-age/src/code/z_fbdemo_circle.c
#include "transition_circle.h"
#include "engine/stub.h"
#include "engine/gfx2d.h"
#include "engine/gfx3d.h"

#include "attributes.h"
#include "color.h"
#include "gfx.h"
#include "sfx.h"
#include "tex_len.h"
#include "transition.h"

typedef enum TransitionCircleDirection {
    /* 0 */ TRANS_CIRCLE_DIR_IN,
    /* 1 */ TRANS_CIRCLE_DIR_OUT
} TransitionCircleDirection;

// unused

#define sTransCircleNormalTex_WIDTH 16
#define sTransCircleNormalTex_HEIGHT 64

#define sTransCircleWaveTex_WIDTH 16
#define sTransCircleWaveTex_HEIGHT 64

#define sTransCircleRippleTex_WIDTH 16
#define sTransCircleRippleTex_HEIGHT 64

#define sTransCircleStarburstTex_WIDTH 16
#define sTransCircleStarburstTex_HEIGHT 64



void TransitionCircle_Start(void* thisx) {
    TransitionCircle* this = (TransitionCircle*)thisx;

    this->isDone = false;

    switch (this->appearanceType) {
        case TCA_WAVE:
            this->texture = NULL; // native: sTransCircleWaveTex (render-only) not converted yet
            break;

        case TCA_RIPPLE:
            this->texture = NULL; // native: sTransCircleRippleTex (render-only) not converted yet
            break;

        case TCA_STARBURST:
            this->texture = NULL; // native: sTransCircleStarburstTex (render-only) not converted yet
            break;

        default:
            this->texture = NULL; // native: sTransCircleNormalTex (render-only) not converted yet
            break;
    }

    if (this->speedType == TCS_FAST) {
        this->speed = 20;
    } else {
        this->speed = 10;
    }

    if (this->colorType == TCC_BLACK) {
        this->color.rgba = RGBA8(0, 0, 0, 255);
    } else if (this->colorType == TCC_WHITE) {
        this->color.rgba = RGBA8(160, 160, 160, 255);
    } else if (this->colorType == TCC_GRAY) {
        this->color.r = 100;
        this->color.g = 100;
        this->color.b = 100;
        this->color.a = 255;
    } else {
        this->speed = 40;
        this->color.rgba = (this->appearanceType == TCA_WAVE) ? RGBA8(0, 0, 0, 255) : RGBA8(160, 160, 160, 255);
    }

    if (this->direction != TRANS_CIRCLE_DIR_IN) {
        this->texY = (s32)(0.0f * (1 << 2));
        if (this->colorType == TCC_SPECIAL) {
            this->texY = (s32)(62.5f * (1 << 2));
        }
    } else {
        this->texY = (s32)(125.0f * (1 << 2));
        if (this->appearanceType == TCA_RIPPLE) {
            SFX_PLAY_CENTERED(NA_SE_OC_SECRET_WARP_OUT);
        }
    }
    // native: render-only matrix setup for the stubbed transition drawing removed
    // native: render-only matrix setup for the stubbed transition drawing removed
}

void* TransitionCircle_Init(void* thisx) {
    TransitionCircle* this = (TransitionCircle*)thisx;

    memset(this, 0, sizeof(TransitionCircle));
    return this;
}

void TransitionCircle_Destroy(UNUSED void* thisx) {
}

void TransitionCircle_Update(void* thisx, s32 updateRate) {
    TransitionCircle* this = (TransitionCircle*)thisx;

    if (this->direction != TRANS_CIRCLE_DIR_IN) {
        if (this->texY == 0) {
            if (this->appearanceType == TCA_RIPPLE) {
                SFX_PLAY_CENTERED(NA_SE_OC_SECRET_WARP_IN);
            }
        }
        this->texY += this->speed * 3 / updateRate;
        if (this->texY >= (s32)(125.0f * (1 << 2))) {
            this->texY = (s32)(125.0f * (1 << 2));
            this->isDone = true;
        }
    } else {
        this->texY -= this->speed * 3 / updateRate;
        if (this->colorType != TCC_SPECIAL) {
            if (this->texY <= (s32)(0.0f * (1 << 2))) {
                this->texY = (s32)(0.0f * (1 << 2));
                this->isDone = true;
            }
        } else {
            if (this->texY <= (s32)(62.5f * (1 << 2))) {
                this->texY = (s32)(62.5f * (1 << 2));
                this->isDone = true;
            }
        }
    }
}

void TransitionCircle_Draw(void* thisx) {
    PRESENTATION_STUB("TransitionCircle_Draw");
}

s32 TransitionCircle_IsDone(void* thisx) {
    TransitionCircle* this = (TransitionCircle*)thisx;

    return this->isDone;
}

void TransitionCircle_SetType(void* thisx, s32 type) {
    TransitionCircle* this = (TransitionCircle*)thisx;

    if (type & TC_SET_PARAMS) {
        // SetType is called twice for circles, the actual direction value will be set on the second call.
        // The direction set here will be overwritten on that second call.
        this->direction = (type >> 5) & 1;

        this->colorType = (type >> 3) & 3;
        this->speedType = type & 1;
        this->appearanceType = (type >> 1) & 3;
    } else if (type == TRANS_INSTANCE_TYPE_FILL_OUT) {
        this->direction = TRANS_CIRCLE_DIR_OUT;
    } else {
        this->direction = TRANS_CIRCLE_DIR_IN;
    }
}

void TransitionCircle_SetColor(void* thisx, u32 color) {
    TransitionCircle* this = (TransitionCircle*)thisx;

    this->color.rgba = color;
}

void TransitionCircle_SetUnkColor(void* thisx, u32 color) {
    TransitionCircle* this = (TransitionCircle*)thisx;

    this->unkColor.rgba = color;
}
