// adapted from flute-of-age/src/code/z_skin.c
#include "skin.h"
#include "dma.h"
#include "engine/rdp_gbi.h"
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

void Skin_UpdateVertices(MtxF* mtx, SkinVertex* skinVertices, SkinLimbModif* modifEntry, Vtx* vtxBuf, Vec3f* pos) {
    // (restored: original body, native RDP commands via engine/rdp_gbi.h)
    Vtx* vtx;
    SkinVertex* vertexEntry;

    for (vertexEntry = skinVertices; vertexEntry < &skinVertices[modifEntry->vtxCount]; vertexEntry++) {
        f32 xwTemp;
        f32 ywTemp;
        f32 zwTemp;
        Vec3f normal;
        Vec3f sp64;

        vtx = &vtxBuf[vertexEntry->index];

        vtx->n.ob[0] = pos->x;
        vtx->n.ob[1] = pos->y;
        vtx->n.ob[2] = pos->z;

        xwTemp = mtx->xw;
        ywTemp = mtx->yw;
        zwTemp = mtx->zw;

        mtx->xw = mtx->yw = mtx->zw = 0.0f;

        sp64.x = vertexEntry->normX;
        sp64.y = vertexEntry->normY;
        sp64.z = vertexEntry->normZ;

        SkinMatrix_Vec3fMtxFMultXYZ(mtx, &sp64, &normal);

        vtx->n.n[0] = normal.x;
        vtx->n.n[1] = normal.y;
        vtx->n.n[2] = normal.z;

        mtx->xw = xwTemp;
        mtx->yw = ywTemp;
        mtx->zw = zwTemp;
    }
}

void Skin_ApplyLimbModifications(GraphicsContext* gfxCtx, Skin* skin, s32 limbIndex, s32 arg3) {
    // (restored: original body, native RDP commands via engine/rdp_gbi.h)
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
    Vtx* vtxBuf;
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

    gSPSegment(POLY_OPA_DISP++, 0x08, vtxEntry->buf[vtxEntry->index]);

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
// (rewritten: native) The original pushed `gSkinLimbMatrices[limbIndex]` multiplied onto the skeleton's model
// matrix (skin->mtx), drew the list and popped back. The renderer takes whole model matrices: the same
// product (vertex * limb * skeleton, the game's row-vector order) is loaded, then the skeleton's matrix again.
static MtxF sSkinModel; // the skeleton's model matrix of the current draw

void Skin_DrawLimb(GraphicsContext* gfxCtx, Skin* skin, s32 limbIndex, DListRef dlistOverride, UNUSED s32 drawFlags) {
    DListRef gfx = dlistOverride;
    SkinLimb** skeleton = Asset_Resolve(skin->skeletonHeader->segment);

    (void)gfxCtx;
    if (dlistOverride == NULL) {
        gfx = ((SkinLimb*)Asset_Resolve(skeleton[limbIndex]))->segment;
    }
    if (gfx != NULL) {
        const MtxF* l = &gSkinLimbMatrices[limbIndex];
        MtxF m;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m.mf[i][j] = l->mf[i][0] * sSkinModel.mf[0][j] + l->mf[i][1] * sSkinModel.mf[1][j] +
                             l->mf[i][2] * sSkinModel.mf[2][j] + l->mf[i][3] * sSkinModel.mf[3][j];
            }
        }
        GFX_TO(OPA, Gfx3D_LoadMtxF(&m));
        GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(gfx)));
        GFX_TO(OPA, Gfx3D_LoadMtxF(&sSkinModel));
    }
}

void Skin_DrawImpl(Actor* actor, PlayState* play, Skin* skin, SkinPostDraw postDraw,
                   SkinOverrideLimbDraw overrideLimbDraw, s32 setTranslation, s32 arg6, s32 drawFlags) {
    // (restored: original body; the model matrix loads go to the native renderer)
    s32 i;
    s32 segmentType;
    SkinLimb** skeleton;
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    Gfx3D_SetTarget(GFX_BUF_OPA);
    if (!(drawFlags & SKIN_DRAW_FLAG_CUSTOM_TRANSFORMS)) {
        Skin_ApplyAnimTransformations(skin, gSkinLimbMatrices, actor, setTranslation);
    }

    skeleton = Asset_Resolve(skin->skeletonHeader->segment);

    if (!(drawFlags & SKIN_DRAW_FLAG_CUSTOM_MATRIX)) {
        sSkinModel = skin->mtx;
        GFX_TO(OPA, Gfx3D_LoadMtxF(&sSkinModel));
    } else {
        sSkinModel = *Matrix_GetCurrent(); // the caller's matrix (custom draws load it themselves)
    }

    for (i = 0; i < skin->skeletonHeader->limbCount; i++) {
        s32 shouldDraw = true;

        if (overrideLimbDraw != NULL) {
            shouldDraw = overrideLimbDraw(actor, play, i, skin);
        }

        segmentType = ((SkinLimb*)Asset_Resolve(skeleton[i]))->segmentType;

        if (segmentType == SKIN_LIMB_TYPE_ANIMATED && shouldDraw == true) {
            Skin_DrawAnimatedLimb(gfxCtx, skin, i, arg6, drawFlags);
        } else if (segmentType == SKIN_LIMB_TYPE_NORMAL && shouldDraw == true) {
            Skin_DrawLimb(gfxCtx, skin, i, NULL, drawFlags);
        }
    }

    if (postDraw != NULL) {
        postDraw(actor, play, skin);
    }
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
