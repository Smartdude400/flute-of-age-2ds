// adapted from flute-of-age/src/code/z_eff_shield_particle.c
#include "libc64/qrand.h"
#include "engine/stub.h"
#include "engine/gfx3d.h"
#include "array_count.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "printf.h"
#include "translation.h"
#include "effect.h"
#include "light.h"
#include "play_state.h"
#include "skin_matrix.h"

#include "assets/objects/gameplay_keep/gameplay_keep_0x38FB0.h"


// original name: "EffectShieldParticle_ct"
void EffectShieldParticle_Init(void* thisx, void* initParamsx) {
    EffectShieldParticle* this = (EffectShieldParticle*)thisx;
    EffectShieldParticleInit* initParams = (EffectShieldParticleInit*)initParamsx;
    EffectShieldParticleElement* elem;

    if ((this != NULL) && (initParams != NULL)) {
        this->numElements = initParams->numElements;
        if (this->numElements > ARRAY_COUNT(this->elements)) {
            PRINTF_COLOR_RED();
            PRINTF(T("EffectShieldParticle_ct():パーティクル数がオーバしてます。\n",
                     "EffectShieldParticle_ct(): Number of particles exceeded.\n"));
            PRINTF_RST();
            return;
        }

        this->position = initParams->position;
        this->primColorStart = initParams->primColorStart;
        this->envColorStart = initParams->envColorStart;
        this->primColorMid = initParams->primColorMid;
        this->envColorMid = initParams->envColorMid;
        this->primColorEnd = initParams->primColorEnd;
        this->envColorEnd = initParams->envColorEnd;
        this->deceleration = initParams->deceleration;
        this->maxInitialSpeed = initParams->maxInitialSpeed;
        this->lengthCutoff = initParams->lengthCutoff;
        this->duration = initParams->duration;
        this->timer = 0;

        for (elem = &this->elements[0]; elem < &this->elements[this->numElements]; elem++) {
            elem->initialSpeed = (Rand_ZeroOne() * (this->maxInitialSpeed * 0.5f)) + (this->maxInitialSpeed * 0.5f);
            elem->endX = 0.0f;
            elem->startXChange = 0.0f;
            elem->startX = 0.0f;
            elem->endXChange = elem->initialSpeed;
            elem->yaw = Rand_ZeroOne() * 65534.0f;
            elem->pitch = Rand_ZeroOne() * 65534.0f;
        }

        this->lightDecay = initParams->lightDecay;
        if (this->lightDecay == true) {
            this->lightInfo.type = LIGHT_POINT_NOGLOW;
            this->lightInfo.params.point = initParams->lightPoint;
            this->lightNode =
                LightContext_InsertLight(Effect_GetPlayState(), &Effect_GetPlayState()->lightCtx, &this->lightInfo);
        } else {
            this->lightNode = NULL;
        }
    }
}

void EffectShieldParticle_Destroy(void* thisx) {
    EffectShieldParticle* this = (EffectShieldParticle*)thisx;

    if ((this != NULL) && (this->lightDecay == true)) {
        if (this->lightNode == Effect_GetPlayState()->lightCtx.listHead) {
            Effect_GetPlayState()->lightCtx.listHead = this->lightNode->next;
        }
        LightContext_RemoveLight(Effect_GetPlayState(), &Effect_GetPlayState()->lightCtx, this->lightNode);
    }
}

s32 EffectShieldParticle_Update(void* thisx) {
    EffectShieldParticle* this = (EffectShieldParticle*)thisx;
    EffectShieldParticleElement* elem;

    if (this == NULL) {
        return 0;
    }

    for (elem = &this->elements[0]; elem < &this->elements[this->numElements]; elem++) {
        elem->endXChange -= this->deceleration;
        if (elem->endXChange < 0.0f) {
            elem->endXChange = 0.0f;
        }

        if (elem->startXChange > 0.0f) {
            elem->startXChange -= this->deceleration;
            if (elem->startXChange < 0.0f) {
                elem->startXChange = 0.0f;
            }
        }

        elem->endX += elem->endXChange;
        elem->startX += elem->startXChange;

        if ((elem->startXChange == 0.0f) && (this->lengthCutoff < (elem->endX - elem->startX))) {
            elem->startXChange = elem->initialSpeed;
        }
    }

    if (this->lightDecay == true) {
        this->lightInfo.params.point.radius /= 2;
    }

    this->timer++;

    if (this->duration < this->timer) {
        return 1;
    }

    return 0;
}

void EffectShieldParticle_GetColors(EffectShieldParticle* this, Color_RGBA8* primColor, Color_RGBA8* envColor) {
    s32 halfDuration = this->duration * 0.5f;

    if (halfDuration == 0) {
        primColor->r = this->primColorStart.r;
        primColor->g = this->primColorStart.g;
        primColor->b = this->primColorStart.b;
        primColor->a = this->primColorStart.a;
        envColor->r = this->envColorStart.r;
        envColor->g = this->envColorStart.g;
        envColor->b = this->envColorStart.b;
        envColor->a = this->envColorStart.a;
    } else if (this->timer < halfDuration) {
        f32 ratio = this->timer / (f32)halfDuration;

        primColor->r = this->primColorStart.r + (this->primColorMid.r - this->primColorStart.r) * ratio;
        primColor->g = this->primColorStart.g + (this->primColorMid.g - this->primColorStart.g) * ratio;
        primColor->b = this->primColorStart.b + (this->primColorMid.b - this->primColorStart.b) * ratio;
        primColor->a = this->primColorStart.a + (this->primColorMid.a - this->primColorStart.a) * ratio;
        envColor->r = this->envColorStart.r + (this->envColorMid.r - this->envColorStart.r) * ratio;
        envColor->g = this->envColorStart.g + (this->envColorMid.g - this->envColorStart.g) * ratio;
        envColor->b = this->envColorStart.b + (this->envColorMid.b - this->envColorStart.b) * ratio;
        envColor->a = this->envColorStart.a + (this->envColorMid.a - this->envColorStart.a) * ratio;
    } else {
        f32 ratio = (this->timer - halfDuration) / (f32)halfDuration;

        primColor->r = this->primColorMid.r + (this->primColorEnd.r - this->primColorMid.r) * ratio;
        primColor->g = this->primColorMid.g + (this->primColorEnd.g - this->primColorMid.g) * ratio;
        primColor->b = this->primColorMid.b + (this->primColorEnd.b - this->primColorMid.b) * ratio;
        primColor->a = this->primColorMid.a + (this->primColorEnd.a - this->primColorMid.a) * ratio;
        envColor->r = this->envColorMid.r + (this->envColorEnd.r - this->envColorMid.r) * ratio;
        envColor->g = this->envColorMid.g + (this->envColorEnd.g - this->envColorMid.g) * ratio;
        envColor->b = this->envColorMid.b + (this->envColorEnd.b - this->envColorMid.b) * ratio;
        envColor->a = this->envColorMid.a + (this->envColorEnd.a - this->envColorMid.a) * ratio;
    }
}

void EffectShieldParticle_Draw(void* thisx, GraphicsContext* gfxCtx) {
    PRESENTATION_STUB("EffectShieldParticle_Draw");
}

