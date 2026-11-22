// adapted from flute-of-age/src/code/z_fbdemo_fade.c
#include "transition_fade.h"
#include "engine/stub.h"
#include "engine/gfx2d.h"
#include "engine/gfx3d.h"

#include "attributes.h"
#include "main.h"
#include "printf.h"
#include "regs.h"
#include "translation.h"
#include "z_lib.h"
#include "save.h"
#include "transition_instances.h"

typedef enum TransitionFadeDirection {
    /* 0 */ TRANS_FADE_DIR_IN,
    /* 1 */ TRANS_FADE_DIR_OUT
} TransitionFadeDirection;

typedef enum TransitionFadeType {
    /* 0 */ TRANS_FADE_TYPE_NONE,
    /* 1 */ TRANS_FADE_TYPE_ONE_WAY,
    /* 2 */ TRANS_FADE_TYPE_FLASH
} TransitionFadeType;


void TransitionFade_Start(void* thisx) {
    TransitionFade* this = (TransitionFade*)thisx;

    switch (this->type) {
        case TRANS_FADE_TYPE_NONE:
            break;

        case TRANS_FADE_TYPE_ONE_WAY:
            this->timer = 0;
            this->color.a = (this->direction != TRANS_FADE_DIR_IN) ? 255 : 0;
            break;

        case TRANS_FADE_TYPE_FLASH:
            this->color.a = 0;
            break;
    }
    this->isDone = false;
}

void* TransitionFade_Init(void* thisx) {
    TransitionFade* this = (TransitionFade*)thisx;

    memset(this, 0, sizeof(TransitionFade));
    return this;
}

void TransitionFade_Destroy(UNUSED void* thisx) {
}

void TransitionFade_Update(void* thisx, s32 updateRate) {
    s32 alpha;
    s16 newAlpha;
    TransitionFade* this = (TransitionFade*)thisx;

    switch (this->type) {
        case TRANS_FADE_TYPE_NONE:
            break;

        case TRANS_FADE_TYPE_ONE_WAY:
            ((TransitionFade*)thisx)->timer += updateRate;
            if (this->timer >= gSaveContext.transFadeDuration) {
                this->timer = gSaveContext.transFadeDuration;
                this->isDone = true;
            }
            if ((u32)gSaveContext.transFadeDuration == 0) {
                PRINTF(VT_COL(RED, WHITE) T("０除算! ZCommonGet fade_speed に０がはいってる",
                                            "Divide by 0! Zero is included in ZCommonGet fade_speed") VT_RST);
            }

            alpha = (255.0f * this->timer) / ((void)0, gSaveContext.transFadeDuration);
            this->color.a = (this->direction != TRANS_FADE_DIR_IN) ? 255 - alpha : alpha;
            break;

        case TRANS_FADE_TYPE_FLASH:
            newAlpha = this->color.a;
            if (R_TRANS_FADE_FLASH_ALPHA_STEP != 0) {
                if (R_TRANS_FADE_FLASH_ALPHA_STEP < 0) {
                    if (Math_StepToS(&newAlpha, 255, 255)) {
                        R_TRANS_FADE_FLASH_ALPHA_STEP = 150;
                    }
                } else {
                    Math_StepToS(&R_TRANS_FADE_FLASH_ALPHA_STEP, 20, 60);
                    if (Math_StepToS(&newAlpha, 0, R_TRANS_FADE_FLASH_ALPHA_STEP)) {
                        R_TRANS_FADE_FLASH_ALPHA_STEP = 0;
                        this->isDone = true;
                    }
                }
            }
            this->color.a = newAlpha;
            break;
    }
}

void TransitionFade_Draw(void* thisx) {
    // native: full-screen fill with the transition colour (original: filled rectangle on the RDP)
    TransitionFade* this = (TransitionFade*)thisx;
    Color_RGBA8_u32* color = &this->color;

    if (color->a != 0) {
        Gfx2D_FillScreen(color->r, color->g, color->b, color->a);
    }
}

s32 TransitionFade_IsDone(void* thisx) {
    TransitionFade* this = (TransitionFade*)thisx;

    return this->isDone;
}

void TransitionFade_SetColor(void* thisx, u32 color) {
    TransitionFade* this = (TransitionFade*)thisx;

    this->color.rgba = color;
}

void TransitionFade_SetType(void* thisx, s32 type) {
    TransitionFade* this = (TransitionFade*)thisx;

    if (type == TRANS_INSTANCE_TYPE_FILL_OUT) {
        this->type = TRANS_FADE_TYPE_ONE_WAY;
        this->direction = TRANS_FADE_DIR_OUT;
    } else if (type == TRANS_INSTANCE_TYPE_FILL_IN) {
        this->type = TRANS_FADE_TYPE_ONE_WAY;
        this->direction = TRANS_FADE_DIR_IN;
    } else if (type == TRANS_INSTANCE_TYPE_FADE_FLASH) {
        this->type = TRANS_FADE_TYPE_FLASH;
    } else {
        this->type = TRANS_FADE_TYPE_NONE;
    }
}
