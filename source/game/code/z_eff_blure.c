// adapted from flute-of-age/src/code/z_eff_blure.c
#include "libc64/math64.h"
#include "engine/stub.h"
#include "engine/gfx3d.h"
#include "attributes.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "printf.h"
#include "stack_pad.h"
#include "sys_math3d.h"
#include "sys_matrix.h"
#include "translation.h"
#include "z_lib.h"
#include "effect.h"
#include "skin_matrix.h"

#include "assets/objects/gameplay_keep/unk_eff_blure_tex.h"

void EffectBlure_AddVertex(EffectBlure* this, Vec3f* p1, Vec3f* p2) {
    EffectBlureElement* elem;
    s32 numElements;

    // Necessary to match
    if (this) {}
    if (this) {}

    if (this != NULL) {
        numElements = this->numElements;
        if (numElements >= 16) {
            PRINTF(T("ブラ─頂点追加処理:テーブルオーバー %d\n", "Blur - vertex addition processing: Table over %d\n"),
                   numElements);
            return;
        }

        elem = &this->elements[numElements];
        elem->state = 1;

        if (!(this->flags & 2)) {
            elem->p1.x = p1->x;
            elem->p1.y = p1->y;
            elem->p1.z = p1->z;
            elem->p2.x = p2->x;
            elem->p2.y = p2->y;
            elem->p2.z = p2->z;
        } else {
            Vec3f sp16C;
            Vec3f sp160;
            Vec3f sp154;
            f32 scale;
            MtxF sp110;
            MtxF spD0;
            MtxF sp90;
            MtxF sp50;
            Vec3f sp44;
            Vec3f sp38;

            sp16C.x = ((f32)(elem - 1)->p2.x + (f32)(elem - 1)->p1.x) * 0.5f;
            sp16C.y = ((f32)(elem - 1)->p2.y + (f32)(elem - 1)->p1.y) * 0.5f;
            sp16C.z = ((f32)(elem - 1)->p2.z + (f32)(elem - 1)->p1.z) * 0.5f;
            sp160.x = (p1->x + p2->x) * 0.5f;
            sp160.y = (p1->y + p2->y) * 0.5f;
            sp160.z = (p1->z + p2->z) * 0.5f;

            Math_Vec3f_Diff(&sp160, &sp16C, &sp154);
            scale = Math3D_Vec3fMagnitude(&sp154);
            if (!(fabsf(scale) < 0.008f)) {
                scale = 1.0f / scale;
                Math_Vec3f_Scale(&sp154, scale);

                SkinMatrix_SetTranslate(&sp110, sp160.x, sp160.y, sp160.z);
                SkinMatrix_SetRotateAxis(&spD0, this->addAngle, sp154.x, sp154.y, sp154.z);
                SkinMatrix_MtxFMtxFMult(&sp110, &spD0, &sp90);
                SkinMatrix_SetTranslate(&sp110, -sp160.x, -sp160.y, -sp160.z);
                SkinMatrix_MtxFMtxFMult(&sp90, &sp110, &sp50);
                SkinMatrix_Vec3fMtxFMultXYZ(&sp50, p1, &sp38);
                SkinMatrix_Vec3fMtxFMultXYZ(&sp50, p2, &sp44);

                elem->p1.x = sp38.x;
                elem->p1.y = sp38.y;
                elem->p1.z = sp38.z;
                elem->p2.x = sp44.x;
                elem->p2.y = sp44.y;
                elem->p2.z = sp44.z;
            }
        }

        elem->timer = 1;
        this->numElements++;
    }
}

void EffectBlure_AddSpace(EffectBlure* this) {
    EffectBlureElement* elem;
    s32 numElements;

    if (this != NULL) {
        numElements = this->numElements;
        if (numElements >= 16) {
            PRINTF(T("ブラ─空白追加処理:テーブルオーバー %d\n", "Blur - space addition processing: Table over %d\n"),
                   numElements);
            return;
        }

        elem = &this->elements[numElements];
        elem->state = 0;
        elem->timer = 1;

        this->numElements++;
    }
}

void EffectBlure_InitElements(EffectBlure* this) {
    EffectBlureElement* elem;
    s32 i;

    this->numElements = 0;

    for (i = 0; i < 16; i++) {
        elem = &this->elements[i];

        elem->state = 2;
        elem->p1.x = 0;
        elem->p1.y = 0;
        elem->p1.z = 0;
        elem->p2.x = 0;
        elem->p2.y = 0;
        elem->p2.z = 0;
        elem->timer = 0;
        elem->flags = 0;
    }
}

void EffectBlure_Init1(void* thisx, void* initParamsx) {
    EffectBlure* this = (EffectBlure*)thisx;
    EffectBlureInit1* initParams = (EffectBlureInit1*)initParamsx;

    if ((this != NULL) && (initParams != NULL)) {
        s32 i;

        EffectBlure_InitElements(this);

        for (i = 0; i < 4; i++) {
            this->p1StartColor[i] = initParams->p1StartColor[i];
            this->p2StartColor[i] = initParams->p2StartColor[i];
            this->p1EndColor[i] = initParams->p1EndColor[i];
            this->p2EndColor[i] = initParams->p2EndColor[i];
        }

        this->elemDuration = initParams->elemDuration;
        this->unkFlag = initParams->unkFlag;
        this->calcMode = initParams->calcMode;
        this->flags = 0;
        this->addAngleChange = 0;
        this->addAngle = 0;
        this->drawMode = 0;
        this->altPrimColor.r = 0;
        this->altPrimColor.g = 0;
        this->altPrimColor.b = 0;
        this->altPrimColor.a = 0;
        this->altEnvColor.r = 0;
        this->altEnvColor.g = 0;
        this->altEnvColor.b = 0;
        this->altEnvColor.a = 0;
        this->mode4Param = 1.0f;
    }
}

void EffectBlure_Init2(void* thisx, void* initParamsx) {
    EffectBlure* this = (EffectBlure*)thisx;
    EffectBlureInit2* initParams = (EffectBlureInit2*)initParamsx;

    if ((this != NULL) && (initParams != NULL)) {
        s32 i;

        EffectBlure_InitElements(this);

        for (i = 0; i < 4; i++) {
            this->p1StartColor[i] = initParams->p1StartColor[i];
            this->p2StartColor[i] = initParams->p2StartColor[i];
            this->p1EndColor[i] = initParams->p1EndColor[i];
            this->p2EndColor[i] = initParams->p2EndColor[i];
        }

        this->elemDuration = initParams->elemDuration;
        this->unkFlag = initParams->unkFlag;
        this->calcMode = initParams->calcMode;
        this->flags = initParams->flags;
        this->drawMode = initParams->drawMode;
        this->addAngleChange = initParams->addAngleChange;
        this->addAngle = 0;
        this->mode4Param = initParams->mode4Param;
        this->altPrimColor = initParams->altPrimColor;
        this->altEnvColor = initParams->altEnvColor;
    }
}

void EffectBlure_Destroy(UNUSED void* thisx) {
}

s32 EffectBlure_Update(void* thisx) {
    EffectBlure* this = (EffectBlure*)thisx;
    s32 i;

    if (this == NULL) {
        return 0;
    }

    if (this->numElements == 0) {
        return 0;
    }

    while (true) {
        if (this->elements[0].state == 0) {
            for (i = 0; i < 15; i++) {
                this->elements[i] = this->elements[i + 1];
            }

            this->elements[i].state = 2;
            this->elements[i].p1.x = 0;
            this->elements[i].p1.y = 0;
            this->elements[i].p1.z = 0;
            this->elements[i].p2.x = 0;
            this->elements[i].p2.y = 0;
            this->elements[i].p2.z = 0;
            this->elements[i].flags = 0;
            this->elements[i].timer = 0;

            this->numElements--;
            if (this->numElements <= 0) {
                this->numElements = 0;
                return 0;
            }
        } else {
            break;
        }
    }

    if (this->elements[0].state == 2) {
        return 0;
    }

    for (i = 0; i < this->numElements; i++) {
        this->elements[i].timer++;
    }

    if (this->elemDuration < this->elements[0].timer) {
        for (i = 0; i < 15; i++) {
            this->elements[i] = this->elements[i + 1];
        }

        this->elements[i].state = 2;
        this->elements[i].p1.x = 0;
        this->elements[i].p1.y = 0;
        this->elements[i].p1.z = 0;
        this->elements[i].p2.x = 0;
        this->elements[i].p2.y = 0;
        this->elements[i].p2.z = 0;
        this->elements[i].flags = 0;
        this->elements[i].timer = 0;

        this->numElements--;
        if (this->numElements <= 0) {
            this->numElements = 0;
            return 0;
        }
        return 0;
    }

    this->addAngle += this->addAngleChange;
    return 0;
}

void EffectBlure_UpdateFlags(EffectBlureElement* elem) {
    Vec3f sp64;
    Vec3f sp58;
    Vec3f sp4C;
    Vec3f sp40;
    EffectBlureElement* prev = elem - 1;
    EffectBlureElement* next = elem + 1;
    f32 sp34;
    f32 sp30;
    f32 sp2C;

    if (((elem - 1)->state == 0) || ((elem + 1)->state == 0)) {
        elem->flags &= ~3;
        elem->flags |= 2;
    } else {
        Math_Vec3s_DiffToVec3f(&sp64, &elem->p1, &prev->p1);
        Math_Vec3s_DiffToVec3f(&sp58, &elem->p2, &prev->p2);
        Math_Vec3s_DiffToVec3f(&sp4C, &next->p1, &elem->p1);
        Math_Vec3s_DiffToVec3f(&sp40, &next->p2, &elem->p2);

        if (Math3D_CosOut(&sp64, &sp4C, &sp34) || Math3D_CosOut(&sp58, &sp40, &sp30) ||
            Math3D_CosOut(&sp4C, &sp40, &sp2C)) {
            elem->flags &= ~3;
            elem->flags |= 0;
        } else if ((sp34 <= -0.5f) || (sp30 <= -0.5f) || (sp2C <= 0.7071f)) { // cos(45 degrees)
            elem->flags &= ~3;
            elem->flags |= 0;
        } else {
            elem->flags &= ~3;
            elem->flags |= 1;
        }
    }
}

void EffectBlure_GetComputedValues(EffectBlure* this, s32 index, f32 ratio, Vec3s* vec1, Vec3s* vec2,
                                   Color_RGBA8* color1, Color_RGBA8* color2) {
    Vec3s sp30;
    f32 mode4Param;
    EffectBlureElement* elem = &this->elements[index];

    switch (this->calcMode) {
        case 1:
            vec1->x = EffectSs_LerpS16(elem->p1.x, elem->p2.x, ratio);
            vec1->y = EffectSs_LerpS16(elem->p1.y, elem->p2.y, ratio);
            vec1->z = EffectSs_LerpS16(elem->p1.z, elem->p2.z, ratio);
            vec2->x = elem->p2.x;
            vec2->y = elem->p2.y;
            vec2->z = elem->p2.z;
            break;

        case 2:
            vec1->x = elem->p1.x;
            vec1->y = elem->p1.y;
            vec1->z = elem->p1.z;
            vec2->x = EffectSs_LerpS16(elem->p2.x, elem->p1.x, ratio);
            vec2->y = EffectSs_LerpS16(elem->p2.y, elem->p1.y, ratio);
            vec2->z = EffectSs_LerpS16(elem->p2.z, elem->p1.z, ratio);
            break;

        case 3:
            ratio *= 0.5f;
            vec1->x = EffectSs_LerpS16(elem->p1.x, elem->p2.x, ratio);
            vec1->y = EffectSs_LerpS16(elem->p1.y, elem->p2.y, ratio);
            vec1->z = EffectSs_LerpS16(elem->p1.z, elem->p2.z, ratio);
            vec2->x = EffectSs_LerpS16(elem->p2.x, elem->p1.x, ratio);
            vec2->y = EffectSs_LerpS16(elem->p2.y, elem->p1.y, ratio);
            vec2->z = EffectSs_LerpS16(elem->p2.z, elem->p1.z, ratio);
            ratio *= 2.0f;
            break;

        case 4:
            sp30.x = elem->p1.x - elem->p2.x;
            sp30.y = elem->p1.y - elem->p2.y;
            sp30.z = elem->p1.z - elem->p2.z;
            mode4Param = this->mode4Param - 1.0f;

            vec1->x = (sp30.x * 0.5f * mode4Param * ratio) + elem->p1.x;
            vec1->y = (sp30.y * 0.5f * mode4Param * ratio) + elem->p1.y;
            if (1) {} // Necessary to match
            vec1->z = (sp30.z * 0.5f * mode4Param * ratio) + elem->p1.z;

            vec2->x = -(sp30.x * 0.5f * mode4Param * ratio) + elem->p2.x;
            vec2->y = -(sp30.y * 0.5f * mode4Param * ratio) + elem->p2.y;
            vec2->z = -(sp30.z * 0.5f * mode4Param * ratio) + elem->p2.z;
            break;

        case 0:
        default:
            vec1->x = elem->p1.x;
            vec1->y = elem->p1.y;
            vec1->z = elem->p1.z;
            vec2->x = elem->p2.x;
            vec2->y = elem->p2.y;
            vec2->z = elem->p2.z;
            break;
    }

    sp30 = sp30; // Optimized out but seems necessary to match stack usage

    if (this->flags & 0x10) {
        color1->r = color1->g = color1->b = color1->a = 255;
        color2->r = color2->g = color2->b = color2->a = 255;
    } else {
        color1->r = EffectSs_LerpU8(this->p1StartColor[0], this->p1EndColor[0], ratio);
        color1->g = EffectSs_LerpU8(this->p1StartColor[1], this->p1EndColor[1], ratio);
        color1->b = EffectSs_LerpU8(this->p1StartColor[2], this->p1EndColor[2], ratio);
        color1->a = EffectSs_LerpU8(this->p1StartColor[3], this->p1EndColor[3], ratio);
        color2->r = EffectSs_LerpU8(this->p2StartColor[0], this->p2EndColor[0], ratio);
        color2->g = EffectSs_LerpU8(this->p2StartColor[1], this->p2EndColor[1], ratio);
        color2->b = EffectSs_LerpU8(this->p2StartColor[2], this->p2EndColor[2], ratio);
        color2->a = EffectSs_LerpU8(this->p2StartColor[3], this->p2EndColor[3], ratio);
    }
}

void EffectBlure_SetupSmooth(UNUSED EffectBlure* this, GraphicsContext* gfxCtx) {

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 38);

}

// original name: "SQ_NoInterpolate_disp"
void EffectBlure_DrawElemNoInterpolation(EffectBlure* this, EffectBlureElement* elem, s32 index,
                                         GraphicsContext* gfxCtx) {
    PRESENTATION_STUB("EffectBlure_DrawElemNoInterpolation");
}

void EffectBlure_DrawElemHermiteInterpolation(EffectBlure* this, EffectBlureElement* elem, s32 index,
                                              GraphicsContext* gfxCtx) {
    PRESENTATION_STUB("EffectBlure_DrawElemHermiteInterpolation");
}

void EffectBlure_DrawSmooth(EffectBlure* this2, GraphicsContext* gfxCtx) {
    PRESENTATION_STUB("EffectBlure_DrawSmooth");
}

void EffectBlure_SetupSimple(GraphicsContext* gfxCtx, UNUSED EffectBlure* this, UNUSED void* vtx) {
    PRESENTATION_STUB("EffectBlure_SetupSimple");
}

void EffectBlure_SetupSimpleAlt(GraphicsContext* gfxCtx, EffectBlure* this, UNUSED void* vtx) {
    PRESENTATION_STUB("EffectBlure_SetupSimpleAlt");
}

void (*sSetupHandlers[])(GraphicsContext* gfxCtx, EffectBlure* this, void* vtx) = {
    EffectBlure_SetupSimple,
    EffectBlure_SetupSimpleAlt,
};

s32 D_80115788 = 0; // unused

// original name: "EffectBlureInfo2_disp_makeDisplayList"
void EffectBlure_DrawSimpleVertices(GraphicsContext* gfxCtx, EffectBlure* this, void* vtx) {
    PRESENTATION_STUB("EffectBlure_DrawSimpleVertices");
}



void EffectBlure_DrawSimple(EffectBlure* this2, GraphicsContext* gfxCtx) {
    PRESENTATION_STUB("EffectBlure_DrawSimple");
}

void EffectBlure_Draw(void* thisx, GraphicsContext* gfxCtx) {
    PRESENTATION_STUB("EffectBlure_Draw");
}

