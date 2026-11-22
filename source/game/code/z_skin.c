// adapted from flute-of-age/src/code/z_skin.c
#include "skin.h"
#include "engine/stub.h"
#include "engine/asset.h"
#include "engine/gfx3d.h"

#include "actor.h"
#include "attributes.h"
#include "gfx.h"
#include "play_state.h"
#include "segmented_address.h"
#include "skin_matrix.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "z_math.h"

#include "game_types.h"
#include <stdbool.h>
#include <stddef.h>

MtxF gSkinLimbMatrices[60]; // holds matrices for each limb of the skeleton currently being drawn

void Skin_UpdateVertices(MtxF* mtx, SkinVertex* skinVertices, SkinLimbModif* modifEntry, void* vtxBuf, Vec3f* pos) {
    PRESENTATION_STUB("Skin_UpdateVertices");
}

void Skin_ApplyLimbModifications(GraphicsContext* gfxCtx, Skin* skin, s32 limbIndex, s32 arg3) {
    s32 modifCount;
    SkinLimb** skeleton;
    STACK_PAD(s32);
    SkinAnimatedLimbData* data;
    SkinLimbModif* modif;
    SkinLimbVtx* vtxEntry;
    s32 transformCount;
    f32 scale;
    SkinVertex* skinVertices;
    SkinTransformation* limbTransformations;
    void* vtxBuf;
    SkinLimbModif* modifications;
    Vec3f vtxPoint;
    Vec3f spD0;
    SkinTransformation* transformationEntry;


    skeleton = (SkinLimb**)Asset_Resolve(skin->skeletonHeader->segment);
    data = Asset_Resolve(((SkinLimb*)Asset_Resolve(skeleton[limbIndex]))->segment);
    modifications = (SkinLimbModif*)Asset_Resolve(data->limbModifications);

    vtxEntry = &skin->vtxTable[limbIndex];
    vtxBuf = vtxEntry->buf[vtxEntry->index];
    modifCount = data->limbModifCount;

    for (modif = modifications; modif < modifications + modifCount; modif++) {
        transformCount = modif->transformCount;
        skinVertices = (SkinVertex*)Asset_Resolve(modif->skinVertices);
        limbTransformations = (SkinTransformation*)Asset_Resolve(modif->limbTransformations);

        if (transformCount == 1) {
            Vec3f spAC;

            spAC.x = limbTransformations[0].x;
            spAC.y = limbTransformations[0].y;
            spAC.z = limbTransformations[0].z;

            SkinMatrix_Vec3fMtxFMultXYZ(&gSkinLimbMatrices[limbTransformations[0].limbIndex], &spAC, &vtxPoint);
        } else if (arg3 == 1) {
            Vec3f spA0;

            transformationEntry = &limbTransformations[modif->unk_4];

            spA0.x = transformationEntry->x;
            spA0.y = transformationEntry->y;
            spA0.z = transformationEntry->z;

            SkinMatrix_Vec3fMtxFMultXYZ(&gSkinLimbMatrices[transformationEntry->limbIndex], &spA0, &vtxPoint);
        } else {
            Vec3f phi_f20;

            phi_f20.x = phi_f20.y = phi_f20.z = 0.0f;

            for (transformationEntry = &limbTransformations[0];
                 transformationEntry < &limbTransformations[transformCount]; transformationEntry++) {
                Vec3f sp88;

                scale = transformationEntry->scale * 0.01f;

                sp88.x = transformationEntry->x;
                sp88.y = transformationEntry->y;
                sp88.z = transformationEntry->z;

                SkinMatrix_Vec3fMtxFMultXYZ(&gSkinLimbMatrices[transformationEntry->limbIndex], &sp88, &spD0);

                spD0.x *= scale;
                spD0.y *= scale;
                spD0.z *= scale;

                phi_f20.x += spD0.x;
                phi_f20.y += spD0.y;
                phi_f20.z += spD0.z;
            }

            vtxPoint.x = phi_f20.x;
            vtxPoint.y = phi_f20.y;
            vtxPoint.z = phi_f20.z;
        }

        Skin_UpdateVertices(&gSkinLimbMatrices[limbTransformations[modif->unk_4].limbIndex], skinVertices, modif,
                            vtxBuf, &vtxPoint);
    }

    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(vtxEntry->buf[vtxEntry->index])));

    vtxEntry->index = (vtxEntry->index == 0) ? 1 : 0;

}

/**
 * Draw a limb of type SKIN_LIMB_TYPE_ANIMATED, of the skeleton `skin` at index `limbIndex`
 * The vertices of this limb are modified dynamically
 */
void Skin_DrawAnimatedLimb(GraphicsContext* gfxCtx, Skin* skin, s32 limbIndex, s32 arg3, s32 drawFlags) {
    SkinLimb** skeleton;
    SkinAnimatedLimbData* data;


    skeleton = Asset_Resolve(skin->skeletonHeader->segment);
    data = Asset_Resolve(((SkinLimb*)Asset_Resolve(skeleton[limbIndex]))->segment);

    if (!(drawFlags & SKIN_DRAW_FLAG_CUSTOM_TRANSFORMS)) {
        Skin_ApplyLimbModifications(gfxCtx, skin, limbIndex, arg3);
    }

    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(data->dlist)));

}

/**
 * Draw a limb of type SKIN_LIMB_TYPE_NORMAL, of the skeleton `skin` at index `limbIndex`
 */
void Skin_DrawLimb(GraphicsContext* gfxCtx, Skin* skin, s32 limbIndex, DListRef dlistOverride, UNUSED s32 drawFlags) {
    PRESENTATION_STUB("Skin_DrawLimb");
}

void Skin_DrawImpl(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw,
                   SkinOverrideLimbDraw overrideLimbDraw, s32 setTranslation, s32 arg6, s32 drawFlags) {
    PRESENTATION_STUB("Skin_DrawImpl");
}

// allows specifying PostLimbDraw and setTranslation
void func_800A6330(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw, s32 setTranslation) {
    Skin_DrawImpl(actor, play, skin, postDraw, NULL, setTranslation, false, 0);
}

// allows specifying OverrideLimbDraw, PostLimbDraw and setTranslation
void func_800A6360(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw,
                   SkinOverrideLimbDraw overrideLimbDraw, s32 setTranslation) {
    Skin_DrawImpl(actor, play, skin, postDraw, overrideLimbDraw, setTranslation, false, 0);
}

// allows specifying OverrideLimbDraw, PostLimbDraw, setTranslation, and arg6
void func_800A6394(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw,
                   SkinOverrideLimbDraw overrideLimbDraw, s32 setTranslation, s32 arg6) {
    Skin_DrawImpl(actor, play, skin, postDraw, overrideLimbDraw, setTranslation, arg6, 0);
}

// allows specifying all variables
void func_800A63CC(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw,
                   SkinOverrideLimbDraw overrideLimbDraw, s32 setTranslation, s32 arg6, s32 drawFlags) {
    Skin_DrawImpl(actor, play, skin, postDraw, overrideLimbDraw, setTranslation, arg6, drawFlags);
}

void Skin_GetLimbPos(Skin* skin, s32 limbIndex, Vec3f* offset, Vec3f* dst) {
    MtxF mtxf;

    SkinMatrix_MtxFMtxFMult(&skin->mtx, &gSkinLimbMatrices[limbIndex], &mtxf);
    SkinMatrix_Vec3fMtxFMultXYZ(&mtxf, offset, dst);
}
