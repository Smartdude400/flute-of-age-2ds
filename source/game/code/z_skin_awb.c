// adapted from flute-of-age/src/code/z_skin_awb.c
#include "array_count.h"
#include "dma.h"
#include "engine/rdp_gbi.h"
#include "engine/stub.h"
#include "engine/asset.h"
#include "attributes.h"
#include "segmented_address.h"
#include "stack_pad.h"
#include "foa_arena.h"
#include "actor.h"
#include "play_state.h"
#include "skin.h"
#include "skin_matrix.h"

#include "src/overlays/actors/ovl_En_fHG/z_en_fhg.h"

/**
 * Initialises the Vtx buffers used for limb at index `limbIndex`
 */
void Skin_InitAnimatedLimb(UNUSED PlayState* play, Skin* skin, s32 limbIndex) {
    // (restored: original body, native RDP commands via engine/rdp_gbi.h)
    s32 i;
    SkinLimb** skeleton = Asset_Resolve(skin->skeletonHeader->segment);
    SkinAnimatedLimbData* animatedLimbData =
        Asset_Resolve(((SkinLimb*)Asset_Resolve(skeleton[limbIndex]))->segment);
    SkinLimbModif* limbModifications = Asset_Resolve(animatedLimbData->limbModifications);
    SkinLimbModif* modifEntry;
    SkinVertex* skinVtxEntry;

    for (i = 0; i < ARRAY_COUNT(skin->vtxTable->buf); i++) {
        Vtx* vtxBuf = skin->vtxTable[limbIndex].buf[i];

        for (modifEntry = limbModifications; modifEntry < limbModifications + animatedLimbData->limbModifCount;
             modifEntry++) {
            SkinVertex* skinVertices = Asset_Resolve(modifEntry->skinVertices);

            for (skinVtxEntry = skinVertices; skinVtxEntry < &skinVertices[modifEntry->vtxCount];) {
                Vtx* vtx = &vtxBuf[skinVtxEntry->index];

                vtx->n.flag = 0;
                vtx->n.tc[0] = skinVtxEntry->s;
                vtx->n.tc[1] = skinVtxEntry->t;
                vtx->n.a = skinVtxEntry->alpha;
                skinVtxEntry++;
            }
        }
    }
}

/**
 * Initializes a skin skeleton to looping animation, dynamically allocating the frame tables,
 * and dynamically allocating and initializing the Vtx and SkinLimbVtx buffers for its animated limbs
 */
void Skin_Init(PlayState* play, Skin* skin, SkeletonHeader* skeletonHeader, AnimationHeader* animationHeader) {
    s32 limbCount;
    s32 i;
    SkinLimb** skeleton;
    SkeletonHeader* virtSkelHeader = Asset_Resolve(skeletonHeader);

    skin->limbCount = virtSkelHeader->limbCount;
    skin->skeletonHeader = virtSkelHeader;

    skeleton = Asset_Resolve(skin->skeletonHeader->segment);
    limbCount = skin->skeletonHeader->limbCount;

    skin->vtxTable = FOA_ARENA_MALLOC(limbCount * sizeof(SkinLimbVtx), "../z_skin_awb.c", 212);

    ASSERT(skin->vtxTable != NULL, "pskin_awb->avb_tbl != NULL", "../z_skin_awb.c", 214);

    for (i = 0; i < limbCount; i++) {
        SkinLimbVtx* vtxEntry = &skin->vtxTable[i];
        SkinLimb* limb = Asset_Resolve(skeleton[i]);

        if ((limb->segmentType != SKIN_LIMB_TYPE_ANIMATED) || (limb->segment == NULL)) {
            vtxEntry->index = 0;

            vtxEntry->buf[0] = NULL;
            vtxEntry->buf[1] = NULL;
        } else {
            SkinAnimatedLimbData* animatedLimbData = Asset_Resolve(((void)0, limb->segment));

            vtxEntry->index = 0;

            vtxEntry->buf[0] =
                FOA_ARENA_MALLOC(animatedLimbData->totalVtxCount * 16 /* original vertex size */, "../z_skin_awb.c", 235);
            ASSERT(vtxEntry->buf[0] != NULL, "psavb->buf[0] != NULL", "../z_skin_awb.c", 237);

            vtxEntry->buf[1] =
                FOA_ARENA_MALLOC(animatedLimbData->totalVtxCount * 16 /* original vertex size */, "../z_skin_awb.c", 240);
            ASSERT(vtxEntry->buf[1] != NULL, "psavb->buf[1] != NULL", "../z_skin_awb.c", 242);

            Skin_InitAnimatedLimb(play, skin, i);
        }
    }

    SkelAnime_InitSkin(play, &skin->skelAnime, skeletonHeader, animationHeader);
}

/**
 * Frees the dynamically allocated Vtx and SkinLimbVtx buffers and tables
 */
void Skin_Free(PlayState* play, Skin* skin) {
    if (skin->vtxTable != NULL) {
        s32 i;

        for (i = 0; i < skin->limbCount; i++) {
            if (skin->vtxTable[i].buf[0] != NULL) {
                FOA_ARENA_FREE(skin->vtxTable[i].buf[0], "../z_skin_awb.c", 276);
                skin->vtxTable[i].buf[0] = NULL;
            }
            if (skin->vtxTable[i].buf[1] != NULL) {
                FOA_ARENA_FREE(skin->vtxTable[i].buf[1], "../z_skin_awb.c", 280);
                skin->vtxTable[i].buf[1] = NULL;
            }
        }

        if (skin->vtxTable != NULL) {
            FOA_ARENA_FREE(skin->vtxTable, "../z_skin_awb.c", 286);
        }

        SkelAnime_Free(&skin->skelAnime, play);
    }
}

s32 func_800A698C(Skin* skin, SkinLimb** skeleton, MtxF* limbMatrices, u8 parentIndex, u8 limbIndex) {
    STACK_PAD(s32);
    SkinLimb* limb = Asset_Resolve(skeleton[limbIndex]);
    MtxF* mtx;
    s32 ret;
    MtxF sp28;

    if (parentIndex == LIMB_DONE) {
        SkinMatrix_GetClear(&mtx);
    } else {
        mtx = &limbMatrices[(s32)parentIndex];
    }

    SkinMatrix_MtxFMtxFMult(mtx, &limbMatrices[limbIndex], &sp28);
    SkinMatrix_MtxFCopy(&sp28, &limbMatrices[limbIndex]);

    if (limb->child != LIMB_DONE) {
        ret = func_800A698C(skin, skeleton, limbMatrices, limbIndex, limb->child);
        if (ret) {
            return ret;
        }
    }

    if (limb->sibling != LIMB_DONE) {
        ret = func_800A698C(skin, skeleton, limbMatrices, parentIndex, limb->sibling);
        if (ret) {
            return ret;
        }
    }

    return false;
}

/**
 * Recursively applies matrix transformations to each limb
 */
s32 Skin_ApplyAnimTransformations(Skin* skin, MtxF* limbMatrices, Actor* actor, s32 setTranslation) {
    s32 i;
    STACK_PAD(s32);
    f32 yRot;
    f32 xRot;
    f32 zRot;
    s32 ret;
    f32 yTransl;
    f32 xTransl;
    f32 zTransl;
    SkinLimb** skeleton = Asset_Resolve(skin->skeletonHeader->segment);
    Vec3s* jointRot = &skin->skelAnime.jointTable[0];

    jointRot++;

    xRot = jointRot->x;
    yRot = jointRot->y;
    zRot = jointRot->z;

    if (setTranslation) {
        jointRot--; // access joint table entry 0 for translation data

        xTransl = jointRot->x;
        yTransl = jointRot->y;
        zTransl = jointRot->z;

        jointRot++;

        if (setTranslation == SKIN_TRANSFORM_IS_FHG) {
            EnfHG* horse = (EnfHG*)actor;

            yRot += horse->turnRot;
        }

        SkinMatrix_SetTranslateRotateZYX(&limbMatrices[0], xRot, yRot, zRot, xTransl, yTransl, zTransl);
    } else {
        SkinMatrix_SetTranslateRotateZYX(&limbMatrices[0], xRot, yRot, zRot, 0.0f, 0.0f, 0.0f);
    }

    jointRot++;

    for (i = 1; i < skin->skeletonHeader->limbCount; i++) {
        SkinLimb* limb = Asset_Resolve(skeleton[i]);

        xTransl = limb->jointPos.x;
        yTransl = limb->jointPos.y;
        zTransl = limb->jointPos.z;
        xRot = jointRot->x;
        yRot = jointRot->y;
        zRot = jointRot->z;
        jointRot++;
        SkinMatrix_SetTranslateRotateZYX(&limbMatrices[i], xRot, yRot, zRot, xTransl, yTransl, zTransl);
    }

    SkinMatrix_SetTranslateRotateYXZScale(
        &skin->mtx, actor->scale.x, actor->scale.y, actor->scale.z, actor->shape.rot.x, actor->shape.rot.y,
        actor->shape.rot.z, actor->world.pos.x, actor->world.pos.y + (actor->shape.yOffset * actor->scale.y),
        actor->world.pos.z);

    ret = func_800A698C(skin, Asset_Resolve(skin->skeletonHeader->segment), limbMatrices, LIMB_DONE, 0);
    if (!ret) {
        return ret;
    }

    return false;
}
