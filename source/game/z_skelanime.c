// adapted from flute-of-age/src/code/z_skelanime.c
#include "animation.h"
#include "animation_legacy.h"

#include "actor.h"
#include "alignment.h"
#include "attributes.h"
#include "avoid_ub.h"
#include "gfx.h"
#include "play_state.h"
#include "printf.h"
#include "regs.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "translation.h"
#include "z_lib.h"
#include "z_math.h"
#include "foa_arena.h"
#include "engine/gfx3d.h"
#include "engine/asset.h"
#include "engine/player_anim.h"

#include "libu64/debug.h"
#include "game_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ANIM_INTERP 1

s32 ArenAnimation_Loop(PlayState* play, SkelAnime* skelAnime);
s32 ArenAnimation_Once(PlayState* play, SkelAnime* skelAnime);
s32 SkelAnime_LoopFull(SkelAnime* skelAnime);
s32 SkelAnime_Once(SkelAnime* skelAnime);
s32 SkelAnime_LoopPartial(SkelAnime* skelAnime);

/**
 * Draw a limb of type `LodLimb`
 * Near or far display list is specified via `lod`
 */
void SkelAnime_DrawLimbLod(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                           OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg, s32 lod) {
    LodLimb* limb;
    DListRef dList;
    Vec3f pos;
    Vec3s rot;


    Matrix_Push();
    limb = (LodLimb*)Asset_Resolve(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    dList = limb->dLists[lod];

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &dList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            GFX_TO(OPA, GFX3D_DRAW_CURRENT(dList));
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &dList, &rot, arg);
    }

    if (limb->child != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawLimbLod(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg, lod));
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawLimbLod(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg, lod));
    }

}

/**
 * Draw all limbs of type `LodLimb` in a given skeleton
 * Near or far display list is specified via `lod`
 */
void SkelAnime_DrawLod(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDrawOpa overrideLimbDraw,
                       PostLimbDrawOpa postLimbDraw, void* arg, s32 lod) {
    LodLimb* rootLimb;
    STACK_PAD(s32);
    DListRef dList;
    Vec3f pos;
    Vec3s rot;

    if (skeleton == NULL) {
        PRINTF_COLOR_RED();
        PRINTF(T("Si2_Lod_draw():skelがNULLです。\n", "Si2_Lod_draw(): skel is NULL.\n"));
        PRINTF_RST();
        return;
    }


    Matrix_Push();

    rootLimb = (LodLimb*)Asset_Resolve(skeleton[0]);
    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];
    dList = rootLimb->dLists[lod];

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &dList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            GFX_TO(OPA, GFX3D_DRAW_CURRENT(dList));
        }
    }
    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &dList, &rot, arg);
    }

    if (rootLimb->child != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawLimbLod(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg, lod));
    }

    Matrix_Pop();

}

/**
 * Draw a limb of type `LodLimb` contained within a flexible skeleton
 * Near or far display list is specified via `lod`
 */
void SkelAnime_DrawFlexLimbLod(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                               OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg, s32 lod,
                               s32* mtx) {
    LodLimb* limb;
    DListRef newDList;
    DListRef limbDList;
    Vec3f pos;
    Vec3s rot;

    Matrix_Push();

    limb = (LodLimb*)Asset_Resolve(skeleton[limbIndex]);
    limbIndex++;

    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    newDList = limbDList = limb->dLists[lod];

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &newDList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), newDList);
        } else if (limbDList != NULL) {
            if (1) {}
            Gfx3D_FlexAdd(Matrix_GetCurrent(), NULL);
        }
    }
    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &limbDList, &rot, arg);
    }
    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimbLod(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg, lod,
                                  mtx);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawFlexLimbLod(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg, lod,
                                  mtx);
    }
}

/**
 * Draws all limbs of type `LodLimb` in a given flexible skeleton
 * Limbs in a flexible skeleton have meshes that can stretch to line up with other limbs.
 * An array of matrices is dynamically allocated so each limb can access any transform to ensure its meshes line up.
 */
void SkelAnime_DrawFlexLod(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
                           OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg, s32 lod) {
    LodLimb* rootLimb;
    STACK_PAD(s32);
    DListRef newDList;
    DListRef limbDList;
    Vec3f pos;
    Vec3s rot;
    s32 mtx = 0; // native: matrix slots recorded by Gfx3D_FlexAdd

    if (skeleton == NULL) {
        PRINTF_COLOR_RED();
        PRINTF(T("Si2_Lod_draw_SV():skelがNULLです。\n", "Si2_Lod_draw_SV(): skel is NULL.\n"));
        PRINTF_RST();
        return;
    }


    Gfx3D_FlexBegin();
    Matrix_Push();

    rootLimb = (LodLimb*)Asset_Resolve(skeleton[0]);
    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];

    newDList = limbDList = rootLimb->dLists[lod];

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &newDList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), newDList);
        } else if (limbDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), NULL);
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &limbDList, &rot, arg);
    }
    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimbLod(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg, lod,
                                  &mtx);
    }

    Matrix_Pop();
    Gfx3D_FlexEnd(); // native pass 2: store matrices to GX slots and draw

}

/**
 * Draw a limb of type `StandardLimb` to the polyOpa buffer
 */
void SkelAnime_DrawLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                           OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg) {
    StandardLimb* limb;
    DListRef dList;
    Vec3f pos;
    Vec3s rot;

    Matrix_Push();

    limb = (StandardLimb*)Asset_Resolve(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];
    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;
    dList = limb->dList;

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &dList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            GFX_TO(OPA, GFX3D_DRAW_CURRENT(dList));
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &dList, &rot, arg);
    }

    if (limb->child != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawLimbOpa(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg));
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawLimbOpa(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg));
    }
}

/**
 * Draw all limbs of type `StandardLimb` in a given skeleton to the polyOpa buffer
 */
void SkelAnime_DrawOpa(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDrawOpa overrideLimbDraw,
                       PostLimbDrawOpa postLimbDraw, void* arg) {
    StandardLimb* rootLimb;
    STACK_PAD(s32);
    DListRef dList;
    Vec3f pos;
    Vec3s rot;

    if (skeleton == NULL) {
        PRINTF_COLOR_RED();
        PRINTF(T("Si2_draw():skelがNULLです。\n", "Si2_draw(): skel is NULL.\n"));
        PRINTF_RST();
        return;
    }


    Matrix_Push();
    rootLimb = (StandardLimb*)Asset_Resolve(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];
    dList = rootLimb->dList;

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &dList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            GFX_TO(OPA, GFX3D_DRAW_CURRENT(dList));
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &dList, &rot, arg);
    }

    if (rootLimb->child != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawLimbOpa(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg));
    }

    Matrix_Pop();

}

/**
 * Draw a limb of type `StandardLimb` contained within a flexible skeleton to the polyOpa buffer
 */
void SkelAnime_DrawFlexLimbOpa(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                               OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg,
                               s32* limbMatrices) {
    StandardLimb* limb;
    DListRef newDList;
    DListRef limbDList;
    Vec3f pos;
    Vec3s rot;


    Matrix_Push();

    limb = (StandardLimb*)Asset_Resolve(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    newDList = limbDList = limb->dList;

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &newDList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), newDList);
        } else if (limbDList != NULL) {
            if (1) {}
            Gfx3D_FlexAdd(Matrix_GetCurrent(), NULL);
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &limbDList, &rot, arg);
    }

    if (limb->child != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawFlexLimbOpa(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg,
                                  limbMatrices));
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawFlexLimbOpa(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg,
                                  limbMatrices));
    }
}

/**
 * Draw all limbs of type `StandardLimb` in a given flexible skeleton to the polyOpa buffer
 * Limbs in a flexible skeleton have meshes that can stretch to line up with other limbs.
 * An array of matrices is dynamically allocated so each limb can access any transform to ensure its meshes line up.
 */
void SkelAnime_DrawFlexOpa(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
                           OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* arg) {
    StandardLimb* rootLimb;
    STACK_PAD(s32);
    DListRef newDList;
    DListRef limbDList;
    Vec3f pos;
    Vec3s rot;
    s32 mtx = 0; // native: matrix slots recorded by Gfx3D_FlexAdd

    if (skeleton == NULL) {
        PRINTF_COLOR_RED();
        PRINTF(T("Si2_draw_SV():skelがNULLです。\n", "Si2_draw_SV(): skel is NULL.\n"));
        PRINTF_RST();
        return;
    }


    Gfx3D_FlexBegin();

    Matrix_Push();

    rootLimb = Asset_Resolve(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];

    newDList = limbDList = rootLimb->dList;

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &newDList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), newDList);
        } else if (limbDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), NULL);
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &limbDList, &rot, arg);
    }

    if (rootLimb->child != LIMB_DONE) {
        GFX_TO(OPA, SkelAnime_DrawFlexLimbOpa(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg,
                                  &mtx));
    }

    Matrix_Pop();
}

/**
 * Copies frame data from the frame data table, indexed by the joint index table.
 * Indices below limit are copied from that entry in the static frame data table.
 * Indices above limit are offsets to a frame data array indexed by the frame.
 */
void SkelAnime_GetFrameData(AnimationHeader* animation, s32 frame, s32 limbCount, Vec3s* frameTable) {
    AnimationHeader* animHeader = Asset_Resolve(animation);
    JointIndex* jointIndices = Asset_Resolve(animHeader->jointIndices);
    s16* frameData = Asset_Resolve(animHeader->frameData);
    s16* staticData = &frameData[0];
    s16* dynamicData = &frameData[frame];
    u16 staticIndexMax = animHeader->staticIndexMax;
    s32 i;

    for (i = 0; i < limbCount; i++) {
        if ((frameTable == NULL) || (jointIndices == NULL) || (dynamicData == NULL) || (staticData == NULL)) {
            LOG_ADDRESS("out", frameTable, "../z_skelanime.c", 1392);
            LOG_ADDRESS("ref_tbl", jointIndices, "../z_skelanime.c", 1393);
            LOG_ADDRESS("frame_tbl", dynamicData, "../z_skelanime.c", 1394);
            LOG_ADDRESS("tbl", staticData, "../z_skelanime.c", 1395);
        }

        frameTable->x =
            (jointIndices->x >= staticIndexMax) ? dynamicData[jointIndices->x] : staticData[jointIndices->x];
        frameTable->y =
            (jointIndices->y >= staticIndexMax) ? dynamicData[jointIndices->y] : staticData[jointIndices->y];
        frameTable->z =
            (jointIndices->z >= staticIndexMax) ? dynamicData[jointIndices->z] : staticData[jointIndices->z];
        jointIndices++;
        frameTable++;
    }
}

s16 Animation_GetLength(void* animation) {
    AnimationHeaderCommon* common = Asset_Resolve(animation);

    return common->frameCount;
}

s16 Animation_GetLastFrame(void* animation) {
    AnimationHeaderCommon* common = Asset_Resolve(animation);
    // Loads an unsigned half for some reason.
    return (u16)common->frameCount - 1;
}

/**
 * Draw a limb of type `StandardLimb` to the specified display buffer
 */
void SkelAnime_DrawLimb(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                        OverrideLimbDraw overrideLimbDraw, PostLimbDraw postLimbDraw, void* arg) {
    StandardLimb* limb;
    DListRef dList;
    Vec3f pos;
    Vec3s rot;

    Matrix_Push();

    limb = (StandardLimb*)Asset_Resolve(skeleton[limbIndex]);
    limbIndex++;

    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    dList = limb->dList;

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &dList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            GFX3D_DRAW_CURRENT(dList);
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &dList, &rot, arg);
    }

    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawLimb(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawLimb(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg);
    }

    return;
}

/**
 * Draw all limbs of type `StandardLimb` in a given skeleton to the specified display buffer
 */
void SkelAnime_Draw(PlayState* play, void** skeleton, Vec3s* jointTable, OverrideLimbDraw overrideLimbDraw,
                    PostLimbDraw postLimbDraw, void* arg) {
    StandardLimb* rootLimb;
    STACK_PAD(s32);
    DListRef dList;
    Vec3f pos;
    Vec3s rot;

    if (skeleton == NULL) {
        PRINTF_COLOR_RED();
        PRINTF(T("Si2_draw2():skelがNULLです。NULLを返します。\n", "Si2_draw2(): skel is NULL. Returns NULL.\n"));
        PRINTF_RST();
        return;
    }

    Matrix_Push();

    rootLimb = (StandardLimb*)Asset_Resolve(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];

    dList = rootLimb->dList;

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &dList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (dList != NULL) {
            GFX3D_DRAW_CURRENT(dList);
        }
    }

    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &dList, &rot, arg);
    }

    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawLimb(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg);
    }

    Matrix_Pop();

    return;
}

/**
 * Draw a limb of type `StandardLimb` contained within a flexible skeleton to the specified display buffer
 */
void SkelAnime_DrawFlexLimb(PlayState* play, s32 limbIndex, void** skeleton, Vec3s* jointTable,
                            OverrideLimbDraw overrideLimbDraw, PostLimbDraw postLimbDraw, void* arg, s32* mtx) {
    StandardLimb* limb;
    DListRef newDList;
    DListRef limbDList;
    Vec3f pos;
    Vec3s rot;

    Matrix_Push();

    limb = (StandardLimb*)Asset_Resolve(skeleton[limbIndex]);
    limbIndex++;
    rot = jointTable[limbIndex];

    pos.x = limb->jointPos.x;
    pos.y = limb->jointPos.y;
    pos.z = limb->jointPos.z;

    newDList = limbDList = limb->dList;
    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, limbIndex, &newDList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), newDList);
        } else if (limbDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), NULL);
        }
    }
    if (postLimbDraw != NULL) {
        postLimbDraw(play, limbIndex, &limbDList, &rot, arg);
    }
    if (limb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimb(play, limb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg, mtx);
    }

    Matrix_Pop();

    if (limb->sibling != LIMB_DONE) {
        SkelAnime_DrawFlexLimb(play, limb->sibling, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg,
                                     mtx);
    }

    return;
}

/**
 * Draw all limbs of type `StandardLimb` in a given flexible skeleton to the specified display buffer
 * Limbs in a flexible skeleton have meshes that can stretch to line up with other limbs.
 * An array of matrices is dynamically allocated so each limb can access any transform to ensure its meshes line up.
 */
void SkelAnime_DrawFlex(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount,
                        OverrideLimbDraw overrideLimbDraw, PostLimbDraw postLimbDraw, void* arg) {
    StandardLimb* rootLimb;
    STACK_PAD(s32);
    DListRef newDList;
    DListRef limbDList;
    Vec3f pos;
    Vec3s rot;
    s32 mtx = 0; // native: matrix slots recorded by Gfx3D_FlexAdd

    if (skeleton == NULL) {
        PRINTF_COLOR_RED();
        PRINTF(T("Si2_draw2_SV():skelがNULLです。NULLを返します。\n", "Si2_draw2_SV(): skel is NULL. Returns NULL.\n"));
        PRINTF_RST();
        return;
    }

    Gfx3D_FlexBegin();
    Matrix_Push();
    rootLimb = (StandardLimb*)Asset_Resolve(skeleton[0]);

    pos.x = jointTable[0].x;
    pos.y = jointTable[0].y;
    pos.z = jointTable[0].z;

    rot = jointTable[1];

    newDList = limbDList = rootLimb->dList;

    if ((overrideLimbDraw == NULL) || !overrideLimbDraw(play, 1, &newDList, &pos, &rot, arg)) {
        Matrix_TranslateRotateZYX(&pos, &rot);
        if (newDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), newDList);
        } else if (limbDList != NULL) {
            Gfx3D_FlexAdd(Matrix_GetCurrent(), NULL);
        }
    }
    if (postLimbDraw != NULL) {
        postLimbDraw(play, 1, &limbDList, &rot, arg);
    }
    if (rootLimb->child != LIMB_DONE) {
        SkelAnime_DrawFlexLimb(play, rootLimb->child, skeleton, jointTable, overrideLimbDraw, postLimbDraw, arg,
                                     &mtx);
    }

    Matrix_Pop();
    Gfx3D_FlexEnd(); // native pass 2

    return;
}

/**
 * Unpacks frame data for the animation at the given frame into frameTable
 * Used by the legacy animation format
 */
s32 SkelAnime_GetFrameDataLegacy(LegacyAnimationHeader* animation, s32 frame, Vec3s* frameTable) {
    LegacyAnimationHeader* animHeader = Asset_Resolve(animation);
    s32 limbCount = animHeader->limbCount;
    LegacyJointKey* key = Asset_Resolve(animHeader->jointKey);
    s16* frameData = Asset_Resolve(animHeader->frameData);
    s16* staticData = &frameData[0];
    s16* dynamicData = &frameData[frame];
    s32 i;

    frameTable->x = frame < key->xMax ? dynamicData[key->x] : staticData[key->x];
    frameTable->y = frame < key->yMax ? dynamicData[key->y] : staticData[key->y];
    frameTable->z = frame < key->zMax ? dynamicData[key->z] : staticData[key->z];

    frameTable++;
    key++;

    for (i = 1; i <= limbCount; i++, key++, frameTable++) {
        frameTable->x = frame < key->xMax ? dynamicData[key->x] : staticData[key->x];
        frameTable->y = frame < key->yMax ? dynamicData[key->y] : staticData[key->y];
        frameTable->z = frame < key->zMax ? dynamicData[key->z] : staticData[key->z];
    }

    return limbCount;
}

/**
 * Used by legacy animation format
 */
s16 Animation_GetLimbCountLegacy(LegacyAnimationHeader* animation) {
    LegacyAnimationHeader* animHeader = Asset_Resolve(animation);

    return animHeader->limbCount;
}

/**
 * Used by legacy animation format
 */
s16 Animation_GetLengthLegacy(LegacyAnimationHeader* animation) {
    LegacyAnimationHeader* animHeader = Asset_Resolve(animation);

    return animHeader->frameCount;
}

/**
 * Used by legacy animation format
 */
s16 Animation_GetLastFrameLegacy(LegacyAnimationHeader* animation) {
    LegacyAnimationHeader* animHeader = Asset_Resolve(animation);

    return animHeader->frameCount - 1;
}

/**
 * Linearly interpolates the start and target frame tables with the given weight, putting the result in dst
 */
void SkelAnime_InterpFrameTable(s32 limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, f32 weight) {
    s32 i;
    s16 diff;
    s16 base;

    if (weight < 1.0f) {
        for (i = 0; i < limbCount; i++, dst++, start++, target++) {
            base = start->x;
            diff = target->x - base;
            dst->x = (s16)(diff * weight) + base;
            base = start->y;
            diff = target->y - base;
            dst->y = (s16)(diff * weight) + base;
            base = start->z;
            diff = target->z - base;
            dst->z = (s16)(diff * weight) + base;
        }
    } else {
        for (i = 0; i < limbCount; i++, dst++, target++) {
            dst->x = target->x;
            dst->y = target->y;
            dst->z = target->z;
        }
    }
}

static u32 sDisabledTransformTaskGroups = 0;
static u32 sCurAnimTaskGroup;

/**
 * Clear the current task queue. The discarded tasks will then not be processed.
 */
void AnimTaskQueue_Reset(AnimTaskQueue* animTaskQueue) {
    animTaskQueue->count = 0;
}

/**
 * Changes `sCurAnimTaskGroup` to the next group number.
 *
 * Task groups allow for disabling "transformative" tasks for a defined group.
 * For more information see `AnimTaskQueue_DisableTransformTasksForGroup`.
 *
 * Note that `sCurAnimTaskGroup` is not a whole number that increments, it is handled at the bit-level.
 * Every time the group number changes, a single bit moves 1 position to the left. This is an implementation detail
 * that allows for `sDisabledTransformTaskGroups` to compare against a set of bit flags.
 */
void AnimTaskQueue_SetNextGroup(UNUSED PlayState* play) {
    sCurAnimTaskGroup <<= 1;
}

/**
 * Marks the current task group as disabled so that "transformative" tasks are skipped.
 * A transformative task is one that will alter the appearance of an animation.
 * These include Copy, Interp, CopyUsingMap, and CopyUsingMapInverted.
 *
 * LoadPlayerFrame and ActorMovement, which don't alter the appearance of an existing animation,
 * will always run even if a group has its transformative tasks disabled.
 */
void AnimTaskQueue_DisableTransformTasksForGroup(UNUSED PlayState* play) {
    sDisabledTransformTaskGroups |= sCurAnimTaskGroup;
}

/**
 * Creates a new task and adds it to the queue, if there is room for it.
 *
 * The `type` value for the task gets set here, but all other
 * initialization must be handled by the caller.
 *
 * @return a pointer to the task, or NULL if it could not be added
 */
AnimTask* AnimTaskQueue_NewTask(AnimTaskQueue* animTaskQueue, s32 type) {
    AnimTask* task;
    s16 taskNumber = animTaskQueue->count;

    if (taskNumber >= ANIM_TASK_QUEUE_MAX) {
        return NULL;
    }

    animTaskQueue->count = taskNumber + 1;

    task = &animTaskQueue->tasks[taskNumber];
    task->type = type;

    return task;
}

#if !PLATFORM_GC
#define AREN_ANIMATION_OFFSET(addr, offset) \
    (((uintptr_t)_aren_animetionSegmentRomStart) + SEGMENT_OFFSET(addr) + (offset))
#else
#define AREN_ANIMATION_OFFSET(addr, offset)                                                                         \
    (((uintptr_t)_aren_animetionSegmentRomStart) + ((uintptr_t)(addr)) - ((uintptr_t)_aren_animetionSegmentStart) + \
     (offset))
#endif

/**
 * Creates a task which will load a single frame of animation data from the aren_animetion file.
 * The asynchronous DMA request to load the data is made as soon as the task is created.
 * When the task is processed later in the AnimTaskQueue, it will wait for the DMA to finish.
 */
void AnimTaskQueue_AddLoadPlayerFrame(PlayState* play, ArenAnimationHeader* animation, s32 frame, s32 limbCount,
                                      Vec3s* frameTable) {
    AnimTask* task = AnimTaskQueue_NewTask(&play->animTaskQueue, ANIMTASK_LOAD_PLAYER_FRAME);

    if (task != NULL) {
        ArenAnimationHeader* arenAnimHeader = Asset_Resolve(animation);

        // native: the original started an async DMA of this frame here (134 bytes; completes long
        // before the buffer is read). The DS reads the converted frame synchronously at the same point.
        task->data.loadPlayerFrame.animation = animation;
        task->data.loadPlayerFrame.frame = frame;
        task->data.loadPlayerFrame.limbCount = limbCount;
        task->data.loadPlayerFrame.frameTable = frameTable;
        PlayerAnim_ReadFrame((u32)(uintptr_t)arenAnimHeader->segment, frame, limbCount, frameTable);
    }
}

/**
 * Creates a task which will copy all vectors from the `src` frame table to the `dest` frame table.
 *
 * Note: This task is "transformative", meaning it will alter the appearance of an animation.
 * If this task's group is included in `sDisabledTransformTaskGroups`, this task will be skipped for that frame.
 */
void AnimTaskQueue_AddCopy(PlayState* play, s32 vecCount, Vec3s* dest, Vec3s* src) {
    AnimTask* task = AnimTaskQueue_NewTask(&play->animTaskQueue, ANIMTASK_COPY);

    if (task != NULL) {
        task->data.copy.group = sCurAnimTaskGroup;
        task->data.copy.vecCount = vecCount;
        task->data.copy.dest = dest;
        task->data.copy.src = src;
    }
}

/**
 * Creates a task which will interpolate between the `base` and `mod` frame tables.
 * The result of the interpolation will be placed in the original `base` table.
 *
 * Note: This task is "transformative", meaning it will alter the appearance of an animation.
 * If this task's group is included in `sDisabledTransformTaskGroups`, this task will be skipped for that frame.
 */
void AnimTaskQueue_AddInterp(PlayState* play, s32 vecCount, Vec3s* base, Vec3s* mod, f32 weight) {
    AnimTask* task = AnimTaskQueue_NewTask(&play->animTaskQueue, ANIMTASK_INTERP);

    if (task != NULL) {
        task->data.interp.group = sCurAnimTaskGroup;
        task->data.interp.vecCount = vecCount;
        task->data.interp.base = base;
        task->data.interp.mod = mod;
        task->data.interp.weight = weight;
    }
}

/**
 * Creates a task which will copy specified vectors from the `src` frame table to the `dest` frame table.
 * Exactly which vectors will be copied is specified by the `limbCopyMap`.
 *
 * The copy map is an array of true/false flags that specify which limbs should have their data copied.
 * Each index of the map corresponds to a limb number in the skeleton.
 * Every limb that has `true` listed will have its data copied.
 *
 * Note: This task is "transformative", meaning it will alter the appearance of an animation.
 * If this task's group is included in `sDisabledTransformTaskGroups`, this task will be skipped for that frame.
 */
void AnimTaskQueue_AddCopyUsingMap(PlayState* play, s32 vecCount, Vec3s* dest, Vec3s* src, u8* limbCopyMap) {
    AnimTask* task = AnimTaskQueue_NewTask(&play->animTaskQueue, ANIMTASK_COPY_USING_MAP);

    if (task != NULL) {
        task->data.copyUsingMap.group = sCurAnimTaskGroup;
        task->data.copyUsingMap.vecCount = vecCount;
        task->data.copyUsingMap.dest = dest;
        task->data.copyUsingMap.src = src;
        task->data.copyUsingMap.limbCopyMap = limbCopyMap;
    }
}

/**
 * Identical to `AnimTaskQueue_AddCopyUsingMap`, except the meaning of the flags in the `limbCopyMap` are inverted.
 * Any entry that specifies `false` will be copied, and any entry that specifies `true` will not.
 *
 * Note: This task is "transformative", meaning it will alter the appearance of an animation.
 * If this task's group is included in `sDisabledTransformTaskGroups`, this task will be skipped for that frame.
 */
void AnimTaskQueue_AddCopyUsingMapInverted(PlayState* play, s32 vecCount, Vec3s* dest, Vec3s* src, u8* limbCopyMap) {
    AnimTask* task = AnimTaskQueue_NewTask(&play->animTaskQueue, ANIMTASK_COPY_USING_MAP_INVERTED);

    if (task != NULL) {
        task->data.copyUsingMapInverted.group = sCurAnimTaskGroup;
        task->data.copyUsingMapInverted.vecCount = vecCount;
        task->data.copyUsingMapInverted.dest = dest;
        task->data.copyUsingMapInverted.src = src;
        task->data.copyUsingMapInverted.limbCopyMap = limbCopyMap;
    }
}

/**
 * Creates a task which will move an actor according to the translation of its root limb for the current frame.
 */
void AnimTaskQueue_AddActorMovement(PlayState* play, Actor* actor, SkelAnime* skelAnime, f32 moveDiffScaleY) {
    AnimTask* task = AnimTaskQueue_NewTask(&play->animTaskQueue, ANIMTASK_ACTOR_MOVE);

    if (task != NULL) {
        task->data.actorMovement.actor = actor;
        task->data.actorMovement.skelAnime = skelAnime;
        task->data.actorMovement.diffScaleY = moveDiffScaleY;
    }
}

/**
 * Wait for the DMA request submitted by `AnimTaskQueue_AddLoadPlayerFrame` to complete.
 */
void AnimTask_LoadPlayerFrame(UNUSED PlayState* play, AnimTaskData* data) {
    // native: nothing to wait for; the frame was read when the task was created
    (void)data;
}

/**
 * Copy all data from the `src` frame table to the `dest` table.
 */
void AnimTask_Copy(UNUSED PlayState* play, AnimTaskData* data) {
    AnimTaskCopy* task = &data->copy;

    if (!(task->group & sDisabledTransformTaskGroups)) {
        Vec3s* dest = task->dest;
        Vec3s* src = task->src;
        s32 i;

        for (i = 0; i < task->vecCount; i++) {
            *dest++ = *src++;
        }
    }
}

/**
 * Interpolate between the `base` and `mod` frame tables.
 */
void AnimTask_Interp(UNUSED PlayState* play, AnimTaskData* data) {
    AnimTaskInterp* task = &data->interp;

    if (!(task->group & sDisabledTransformTaskGroups)) {
        SkelAnime_InterpFrameTable(task->vecCount, task->base, task->base, task->mod, task->weight);
    }
}

/**
 * Copy all data from the `src` frame table to the `dest` table according to the copy map.
 */
void AnimTask_CopyUsingMap(UNUSED PlayState* play, AnimTaskData* data) {
    AnimTaskCopyUsingMap* task = &data->copyUsingMap;

    if (!(task->group & sDisabledTransformTaskGroups)) {
        Vec3s* dest = task->dest;
        Vec3s* src = task->src;
        u8* limbCopyMap = task->limbCopyMap;
        s32 i;

        for (i = 0; i < task->vecCount; i++, dest++, src++) {
            if (*limbCopyMap++) {
                *dest = *src;
            }
        }
    }
}

/**
 * Copy all data from the `src` frame table to the `dest` table according to the inverted copy map.
 */
void AnimTask_CopyUsingMapInverted(UNUSED PlayState* play, AnimTaskData* data) {
    AnimTaskCopyUsingMapInverted* task = &data->copyUsingMapInverted;

    if (!(task->group & sDisabledTransformTaskGroups)) {
        Vec3s* dest = task->dest;
        Vec3s* src = task->src;
        u8* limbCopyMap = task->limbCopyMap;
        s32 i;

        for (i = 0; i < task->vecCount; i++, dest++, src++) {
            if (!(*limbCopyMap++)) {
                *dest = *src;
            }
        }
    }
}

/**
 * Move an actor according to the translation of its root limb for the current animation frame.
 * The actor's current shape yaw will factor into the resulting movement.
 */
void AnimTask_ActorMovement(UNUSED PlayState* play, AnimTaskData* data) {
    AnimTaskActorMovement* task = &data->actorMovement;
    Actor* actor = task->actor;
    Vec3f diff;

    SkelAnime_UpdateTranslation(task->skelAnime, &diff, actor->shape.rot.y);

    actor->world.pos.x += diff.x * actor->scale.x;
    actor->world.pos.y += diff.y * actor->scale.y * task->diffScaleY;
    actor->world.pos.z += diff.z * actor->scale.z;
}

typedef void (*AnimTaskFunc)(struct PlayState* play, AnimTaskData* data);

/**
 * Update the AnimTaskQueue, processing all tasks in order.
 * Variables related to anim task groups are then reset for the next frame.
 */
void AnimTaskQueue_Update(PlayState* play, AnimTaskQueue* animTaskQueue) {
    static AnimTaskFunc animTaskFuncs[] = {
        AnimTask_LoadPlayerFrame,      AnimTask_Copy,          AnimTask_Interp, AnimTask_CopyUsingMap,
        AnimTask_CopyUsingMapInverted, AnimTask_ActorMovement,
    };
    AnimTask* task = animTaskQueue->tasks;

    while (animTaskQueue->count != 0) {
        animTaskFuncs[task->type](play, &task->data);
        task++;
        animTaskQueue->count--;
    }

    sCurAnimTaskGroup = 1 << 0;
    sDisabledTransformTaskGroups = 0;
}

/**
 * Initializes a skeleton to be used with Aren animations to a looping animation, dynamically allocating the frame
 * tables if not given.
 */
void SkelAnime_InitAren(PlayState* play, SkelAnime* skelAnime, FlexSkeletonHeader* skeletonHeaderSeg,
                        ArenAnimationHeader* animation, s32 flags, Vec3s* jointTable, Vec3s* morphTable,
                        UNUSED_NDEBUG s32 limbBufCount) {
    FlexSkeletonHeader* skeletonHeader = Asset_Resolve(skeletonHeaderSeg);
    s32 headerJointCount = skeletonHeader->sh.limbCount;
    s32 limbCount;
    size_t allocSize;

    skelAnime->initFlags = flags;
    limbCount = (flags & 2) ? headerJointCount : 1;

    if (flags & 1) {
        limbCount += headerJointCount;
    }
    if (flags & 4) {
        limbCount += headerJointCount;
    }

    skelAnime->limbCount = limbCount;
    skelAnime->dListCount = skeletonHeader->dListCount;

    allocSize = limbCount * sizeof(Vec3s);
    skelAnime->skeleton = Asset_Resolve(skeletonHeader->sh.segment);

    if (flags & 8) {
        allocSize += 2;
    }

    if (jointTable == NULL) {
        skelAnime->jointTable = FOA_ARENA_MALLOC(allocSize, "../z_skelanime.c", 2364);
        skelAnime->morphTable = FOA_ARENA_MALLOC(allocSize, "../z_skelanime.c", 2365);
    } else {
        ASSERT(limbBufCount == limbCount, "joint_buff_num == joint_num", "../z_skelanime.c", 2369);

        skelAnime->jointTable = (Vec3s*)ALIGN16((uintptr_t)jointTable);
        skelAnime->morphTable = (Vec3s*)ALIGN16((uintptr_t)morphTable);
    }

    if ((skelAnime->jointTable == NULL) || (skelAnime->morphTable == NULL)) {
        PRINTF_COLOR_RED();
        PRINTF(T("Skeleton_Info_Rom_SV_ct メモリアロケーションエラー\n",
                 "Skeleton_Info_Rom_SV_ct Memory allocation error\n"));
        PRINTF_RST();
    }

    ArenAnimation_Change(play, skelAnime, animation, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, 0.0f);
}

/**
 * Sets the update function of a SkelAnime that uses Aren animations based on its mode
 */
void ArenAnimation_SetUpdateFunction(SkelAnime* skelAnime) {
    if (skelAnime->mode <= ANIMMODE_LOOP_INTERP) {
        skelAnime->update.link = ArenAnimation_Loop;
    } else {
        skelAnime->update.link = ArenAnimation_Once;
    }
    skelAnime->morphWeight = 0.0f;
}

/**
 * Advances the current Aren animation and updates all frame tables. If the animation plays once, returns true when it
 * finishes.
 */
s32 ArenAnimation_Update(PlayState* play, SkelAnime* skelAnime) {
    return skelAnime->update.link(play, skelAnime);
}

/**
 * Requests an interpolation between the pose in jointTable to the one in morphTable, advancing the morph but not the
 * animation frame
 */
s32 ArenAnimation_Morph(PlayState* play, SkelAnime* skelAnime) {
    f32 prevMorphWeight = skelAnime->morphWeight;
    f32 updateRate = R_UPDATE_RATE * 0.5f;

    skelAnime->morphWeight -= skelAnime->morphRate * updateRate;

    if (skelAnime->morphWeight <= 0.0f) {
        ArenAnimation_SetUpdateFunction(skelAnime);
    }

    AnimTaskQueue_AddInterp(play, skelAnime->limbCount, skelAnime->jointTable, skelAnime->morphTable,
                            1.0f - (skelAnime->morphWeight / prevMorphWeight));
    return 0;
}

/**
 * Requests a load of the next frame of a Aren animation, advances the morph, and requests an interpolation between
 * jointTable and morphTable
 */
void ArenAnimation_AnimateFrame(PlayState* play, SkelAnime* skelAnime) {
    AnimTaskQueue_AddLoadPlayerFrame(play, skelAnime->animation, skelAnime->curFrame, skelAnime->limbCount,
                                     skelAnime->jointTable);
    if (skelAnime->morphWeight != 0) {
        f32 updateRate = R_UPDATE_RATE * 0.5f;

        skelAnime->morphWeight -= skelAnime->morphRate * updateRate;
        if (skelAnime->morphWeight <= 0.0f) {
            skelAnime->morphWeight = 0.0f;
        } else {
            AnimTaskQueue_AddInterp(play, skelAnime->limbCount, skelAnime->jointTable, skelAnime->morphTable,
                                    skelAnime->morphWeight);
        }
    }
}

/**
 * Advances a Aren animation that loops over its full length
 */
s32 ArenAnimation_Loop(PlayState* play, SkelAnime* skelAnime) {
    f32 updateRate = R_UPDATE_RATE * 0.5f;

    skelAnime->curFrame += skelAnime->playSpeed * updateRate;
    if (skelAnime->curFrame < 0.0f) {
        skelAnime->curFrame += skelAnime->animLength;
    } else if (skelAnime->animLength <= skelAnime->curFrame) {
        skelAnime->curFrame -= skelAnime->animLength;
    }
    ArenAnimation_AnimateFrame(play, skelAnime);
    return false;
}

/**
 * Advances a Aren animation that stops at endFrame and returns true when it is reached.
 */
s32 ArenAnimation_Once(PlayState* play, SkelAnime* skelAnime) {
    f32 updateRate = R_UPDATE_RATE * 0.5f;

    if (skelAnime->curFrame == skelAnime->endFrame) {
        ArenAnimation_AnimateFrame(play, skelAnime);
        return true;
    }
    skelAnime->curFrame += skelAnime->playSpeed * updateRate;
    if ((skelAnime->curFrame - skelAnime->endFrame) * skelAnime->playSpeed > 0.0f) {
        skelAnime->curFrame = skelAnime->endFrame;
    } else if (skelAnime->curFrame < 0.0f) {
        skelAnime->curFrame += skelAnime->animLength;
    } else if (skelAnime->animLength <= skelAnime->curFrame) {
        skelAnime->curFrame -= skelAnime->animLength;
    }
    ArenAnimation_AnimateFrame(play, skelAnime);
    return false;
}

/**
 * Sets a new morph and resets the morph weight for the current animation.
 */
void Animation_SetMorph(UNUSED PlayState* play, SkelAnime* skelAnime, f32 morphFrames) {
    skelAnime->morphWeight = 1.0f;
    skelAnime->morphRate = 1.0f / morphFrames;
}

/**
 * General way to set a new Aren animation, allowing choice of playback speed, start frame, end frame, play mode, and
 * number of transition frames. Positive morph frames morph from the current pose to the start pose of the new
 * animation, then start the new animation. Negative morph frames start the new animation immediately, modified by the
 * pose immediately before the animation change.
 */
void ArenAnimation_Change(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation, f32 playSpeed,
                          f32 startFrame, f32 endFrame, u8 mode, f32 morphFrames) {
    skelAnime->mode = mode;
    if ((morphFrames != 0.0f) && ((animation != skelAnime->animation) || (startFrame != skelAnime->curFrame))) {
        if (morphFrames < 0) {
            ArenAnimation_SetUpdateFunction(skelAnime);
            SkelAnime_CopyFrameTable(skelAnime, skelAnime->morphTable, skelAnime->jointTable);
            morphFrames = -morphFrames;
        } else {
            skelAnime->update.link = ArenAnimation_Morph;
            AnimTaskQueue_AddLoadPlayerFrame(play, animation, (s32)startFrame, skelAnime->limbCount,
                                             skelAnime->morphTable);
        }
        skelAnime->morphWeight = 1.0f;
        skelAnime->morphRate = 1.0f / morphFrames;
    } else {
        ArenAnimation_SetUpdateFunction(skelAnime);
        AnimTaskQueue_AddLoadPlayerFrame(play, animation, (s32)startFrame, skelAnime->limbCount, skelAnime->jointTable);
        skelAnime->morphWeight = 0.0f;
    }

    skelAnime->animation = animation;
    skelAnime->curFrame = 0.0f;
    skelAnime->startFrame = startFrame;
    skelAnime->curFrame = startFrame;
    skelAnime->endFrame = endFrame;
    skelAnime->animLength = Animation_GetLength(animation);
    skelAnime->playSpeed = playSpeed;
}

/**
 * Immediately changes to a Aren animation that plays once at the default speed.
 */
void ArenAnimation_PlayOnce(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation) {
    ArenAnimation_Change(play, skelAnime, animation, 1.0f, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_ONCE,
                         0.0f);
}

/**
 * Immediately changes to a Aren animation that plays once at the specified speed.
 */
void ArenAnimation_PlayOnceSetSpeed(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation,
                                    f32 playSpeed) {
    ArenAnimation_Change(play, skelAnime, animation, playSpeed, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_ONCE,
                         0.0f);
}

/**
 * Immediately changes to a Aren animation that loops at the default speed.
 */
void ArenAnimation_PlayLoop(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation) {
    ArenAnimation_Change(play, skelAnime, animation, 1.0f, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_LOOP,
                         0.0f);
}

/**
 * Immediately changes to a Aren animation that loops at the specified speed.
 */
void ArenAnimation_PlayLoopSetSpeed(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation,
                                    f32 playSpeed) {
    ArenAnimation_Change(play, skelAnime, animation, playSpeed, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_LOOP,
                         0.0f);
}

/**
 * Requests copying jointTable to morphTable
 */
void ArenAnimation_CopyJointToMorph(PlayState* play, SkelAnime* skelAnime) {
    AnimTaskQueue_AddCopy(play, skelAnime->limbCount, skelAnime->morphTable, skelAnime->jointTable);
}

/**
 * Requests copying morphTable to jointTable
 * unused
 */
void ArenAnimation_CopyMorphToJoint(PlayState* play, SkelAnime* skelAnime) {
    AnimTaskQueue_AddCopy(play, skelAnime->limbCount, skelAnime->jointTable, skelAnime->morphTable);
}

/**
 * Requests loading frame data from the Aren animation into morphTable
 */
void ArenAnimation_LoadToMorph(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation, f32 frame) {
    AnimTaskQueue_AddLoadPlayerFrame(play, animation, (s32)frame, skelAnime->limbCount, skelAnime->morphTable);
}

/**
 * Requests loading frame data from the Aren animation into jointTable
 */
void ArenAnimation_LoadToJoint(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation, f32 frame) {
    AnimTaskQueue_AddLoadPlayerFrame(play, animation, (s32)frame, skelAnime->limbCount, skelAnime->jointTable);
}

/**
 * Requests interpolating between jointTable and morphTable, placing the result in jointTable
 */
void ArenAnimation_InterpJointMorph(PlayState* play, SkelAnime* skelAnime, f32 weight) {
    AnimTaskQueue_AddInterp(play, skelAnime->limbCount, skelAnime->jointTable, skelAnime->morphTable, weight);
}

/**
 * Requests loading frame data from the Aren animations and blending them, placing the result in jointTable
 */
void ArenAnimation_BlendToJoint(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation1, f32 frame1,
                                ArenAnimationHeader* animation2, f32 frame2, f32 blendWeight, Vec3s* blendTable) {
    Vec3s* alignedBlendTable;

    AnimTaskQueue_AddLoadPlayerFrame(play, animation1, (s32)frame1, skelAnime->limbCount, skelAnime->jointTable);

    alignedBlendTable = (Vec3s*)ALIGN16((uintptr_t)blendTable);

    AnimTaskQueue_AddLoadPlayerFrame(play, animation2, (s32)frame2, skelAnime->limbCount, alignedBlendTable);
    AnimTaskQueue_AddInterp(play, skelAnime->limbCount, skelAnime->jointTable, alignedBlendTable, blendWeight);
}

/**
 * Requests loading frame data from the Aren animations and blending them, placing the result in morphTable
 */
void ArenAnimation_BlendToMorph(PlayState* play, SkelAnime* skelAnime, ArenAnimationHeader* animation1, f32 frame1,
                                ArenAnimationHeader* animation2, f32 frame2, f32 blendWeight, Vec3s* blendTable) {
    Vec3s* alignedBlendTable;

    AnimTaskQueue_AddLoadPlayerFrame(play, animation1, (s32)frame1, skelAnime->limbCount, skelAnime->morphTable);

    alignedBlendTable = (Vec3s*)ALIGN16((uintptr_t)blendTable);

    AnimTaskQueue_AddLoadPlayerFrame(play, animation2, (s32)frame2, skelAnime->limbCount, alignedBlendTable);
    AnimTaskQueue_AddInterp(play, skelAnime->limbCount, skelAnime->morphTable, alignedBlendTable, blendWeight);
}

/**
 * Changes a looping animation to one that stops at the end. Unused
 */
void ArenAnimation_EndLoop(SkelAnime* skelAnime) {
    skelAnime->mode = ANIMMODE_ONCE;
    ArenAnimation_SetUpdateFunction(skelAnime);
}

/**
 * Checks if the current frame is after frame and the previous frame was before it.
 */
s32 Animation_OnFrameImpl(SkelAnime* skelAnime, f32 frame, f32 updateRate) {
    f32 updateSpeed = skelAnime->playSpeed * updateRate;
    f32 prevFrame = skelAnime->curFrame - updateSpeed;
    f32 curFrameDiff;
    f32 prevFrameDiff;

    if (prevFrame < 0.0f) {
        prevFrame += skelAnime->animLength;
    } else if (prevFrame >= skelAnime->animLength) {
        prevFrame -= skelAnime->animLength;
    }

    if ((frame == 0.0f) && (updateSpeed > 0.0f)) {
        frame = skelAnime->animLength;
    }

    curFrameDiff = prevFrame + updateSpeed - frame;
    prevFrameDiff = curFrameDiff - updateSpeed;
    if ((curFrameDiff * updateSpeed >= 0.0f) && (prevFrameDiff * updateSpeed < 0.0f)) {
        return true;
    }
    return false;
}

/**
 * Checks if the current Aren animation has reached the specified frame
 */
s32 ArenAnimation_OnFrame(SkelAnime* skelAnime, f32 frame) {
    f32 updateRate = R_UPDATE_RATE * 0.5f;

    return Animation_OnFrameImpl(skelAnime, frame, updateRate);
}

/**
 * Initializes a normal skeleton to a looping animation, dynamically allocating the frame tables if not provided.
 */
BAD_RETURN(s32) SkelAnime_Init(UNUSED PlayState* play, SkelAnime* skelAnime, SkeletonHeader* skeletonHeaderSeg,
                               AnimationHeader* animation, Vec3s* jointTable, Vec3s* morphTable,
                               UNUSED_NDEBUG s32 limbCount) {
    SkeletonHeader* skeletonHeader = Asset_Resolve(skeletonHeaderSeg);

    skelAnime->limbCount = skeletonHeader->limbCount + 1;
    skelAnime->skeleton = Asset_Resolve(skeletonHeader->segment);
    if (jointTable == NULL) {
        skelAnime->jointTable =
            FOA_ARENA_MALLOC(skelAnime->limbCount * sizeof(*skelAnime->jointTable), "../z_skelanime.c", 2968);
        skelAnime->morphTable =
            FOA_ARENA_MALLOC(skelAnime->limbCount * sizeof(*skelAnime->morphTable), "../z_skelanime.c", 2969);
    } else {
        ASSERT(limbCount == skelAnime->limbCount, "joint_buff_num == this->joint_num", "../z_skelanime.c", 2973);
        skelAnime->jointTable = jointTable;
        skelAnime->morphTable = morphTable;
    }
    if ((skelAnime->jointTable == NULL) || (skelAnime->morphTable == NULL)) {
        PRINTF_COLOR_RED();
        PRINTF(T("Skeleton_Info2_ct メモリアロケーションエラー\n", "Skeleton_Info2_ct Memory allocation error\n"));
        PRINTF_RST();
    }

    if (animation != NULL) {
        Animation_PlayLoop(skelAnime, animation);
    }
}

/**
 * Initializes a flex skeleton to a looping animation, dynamically allocating the frame tables if not given.
 */
BAD_RETURN(s32) SkelAnime_InitFlex(UNUSED PlayState* play, SkelAnime* skelAnime, FlexSkeletonHeader* skeletonHeaderSeg,
                                   AnimationHeader* animation, Vec3s* jointTable, Vec3s* morphTable,
                                   UNUSED_NDEBUG s32 limbCount) {
    FlexSkeletonHeader* skeletonHeader = Asset_Resolve(skeletonHeaderSeg);

    skelAnime->limbCount = skeletonHeader->sh.limbCount + 1;
    skelAnime->dListCount = skeletonHeader->dListCount;
    skelAnime->skeleton = Asset_Resolve(skeletonHeader->sh.segment);

    if (jointTable == NULL) {
        skelAnime->jointTable =
            FOA_ARENA_MALLOC(skelAnime->limbCount * sizeof(*skelAnime->jointTable), "../z_skelanime.c", 3047);

        skelAnime->morphTable =
            FOA_ARENA_MALLOC(skelAnime->limbCount * sizeof(*skelAnime->morphTable), "../z_skelanime.c", 3048);
    } else {
        ASSERT(limbCount == skelAnime->limbCount, "joint_buff_num == this->joint_num", "../z_skelanime.c", 3052);
        skelAnime->jointTable = jointTable;
        skelAnime->morphTable = morphTable;
    }
    if ((skelAnime->jointTable == NULL) || (skelAnime->morphTable == NULL)) {
        PRINTF_COLOR_RED();
        PRINTF(T("Skeleton_Info_Rom_SV_ct メモリアロケーションエラー\n",
                 "Skeleton_Info_Rom_SV_ct Memory allocation error\n"));
        PRINTF_RST();
    }

    if (animation != NULL) {
        Animation_PlayLoop(skelAnime, animation);
    }
}

/**
 * Initializes a skeleton with SkinLimbs to a looping animation, dynamically allocating the frame tables.
 */
BAD_RETURN(s32) SkelAnime_InitSkin(UNUSED PlayState* play, SkelAnime* skelAnime, SkeletonHeader* skeletonHeaderSeg,
                                   AnimationHeader* animation) {
    SkeletonHeader* skeletonHeader = Asset_Resolve(skeletonHeaderSeg);

    skelAnime->limbCount = skeletonHeader->limbCount + 1;
    skelAnime->skeleton = Asset_Resolve(skeletonHeader->segment);
    skelAnime->jointTable =
        FOA_ARENA_MALLOC(skelAnime->limbCount * sizeof(*skelAnime->jointTable), "../z_skelanime.c", 3120);
    skelAnime->morphTable =
        FOA_ARENA_MALLOC(skelAnime->limbCount * sizeof(*skelAnime->morphTable), "../z_skelanime.c", 3121);
    if ((skelAnime->jointTable == NULL) || (skelAnime->morphTable == NULL)) {
        PRINTF_COLOR_RED();
        PRINTF(T("Skeleton_Info2_skin2_ct メモリアロケーションエラー\n",
                 "Skeleton_Info2_skin2_ct Memory allocation error\n"));
        PRINTF_RST();
    }

    if (animation != NULL) {
        Animation_PlayLoop(skelAnime, animation);
    }
}

/**
 * Sets the SkelAnime's update function based on its current mode.
 */
void SkelAnime_SetUpdate(SkelAnime* skelAnime) {
    if (skelAnime->mode <= ANIMMODE_LOOP_INTERP) {
        skelAnime->update.normal = SkelAnime_LoopFull;
    } else if (skelAnime->mode <= ANIMMODE_ONCE_INTERP) {
        skelAnime->update.normal = SkelAnime_Once;
    } else {
        skelAnime->update.normal = SkelAnime_LoopPartial;
    }
}

/**
 * Advances the current animation and updates all frame tables. If the animation plays once, returns true when it
 * finishes.
 */
s32 SkelAnime_Update(SkelAnime* skelAnime) {
    return skelAnime->update.normal(skelAnime);
}

/**
 * Morphs from the pose in jointTable to the one in morphTable, advancing the morph but not the animation frame
 */
s32 SkelAnime_Morph(SkelAnime* skelAnime) {
    f32 prevMorphWeight = skelAnime->morphWeight;
    f32 updateRate = R_UPDATE_RATE * (1.0f / 3.0f);

    skelAnime->morphWeight -= skelAnime->morphRate * updateRate;
    if (skelAnime->morphWeight <= 0.0f) {
        SkelAnime_SetUpdate(skelAnime);
        skelAnime->morphWeight = 0.0f;
    }
    SkelAnime_InterpFrameTable(skelAnime->limbCount, skelAnime->jointTable, skelAnime->jointTable,
                               skelAnime->morphTable, 1.0f - (skelAnime->morphWeight / prevMorphWeight));
    return 0;
}

/**
 * Performs a tapered morph from the pose in jointTable to the one in morphTable, advancing the morph but not the
 * animation frame
 */
s32 SkelAnime_MorphTaper(SkelAnime* skelAnime) {
    s16 prevPhase = skelAnime->morphWeight * 0x4000;
    s16 curPhase;
    f32 prevWeight;
    f32 curWeight;
    f32 updateRate = R_UPDATE_RATE * (1.0f / 3.0f);

    skelAnime->morphWeight -= skelAnime->morphRate * updateRate;
    if (skelAnime->morphWeight <= 0.0f) {
        SkelAnime_SetUpdate(skelAnime);
        skelAnime->morphWeight = 0.0f;
    }
    curPhase = skelAnime->morphWeight * 0x4000;
    if (skelAnime->taper <= ANIMTAPER_DECEL) {
        prevWeight = 1.0f - Math_CosS(prevPhase);
        curWeight = 1.0f - Math_CosS(curPhase);
    } else {
        prevWeight = Math_SinS(prevPhase);
        curWeight = Math_SinS(curPhase);
    }
    if (curWeight != 0.0f) {
        curWeight /= prevWeight;
    } else {
        curWeight = 0.0f;
    }
    SkelAnime_InterpFrameTable(skelAnime->limbCount, skelAnime->jointTable, skelAnime->jointTable,
                               skelAnime->morphTable, 1.0f - curWeight);
    return 0;
}

/**
 * Gets frame data for the current frame as modified by morphTable and advances the morph
 */
void SkelAnime_AnimateFrame(SkelAnime* skelAnime) {
    Vec3s nextjointTable[100];

    SkelAnime_GetFrameData(skelAnime->animation, skelAnime->curFrame, skelAnime->limbCount, skelAnime->jointTable);
    if (skelAnime->mode & ANIM_INTERP) {
        s32 frame = skelAnime->curFrame;
        f32 partialFrame = skelAnime->curFrame - frame;

        if (++frame >= (s32)skelAnime->animLength) {
            frame = 0;
        }
        SkelAnime_GetFrameData(skelAnime->animation, frame, skelAnime->limbCount, nextjointTable);
        SkelAnime_InterpFrameTable(skelAnime->limbCount, skelAnime->jointTable, skelAnime->jointTable, nextjointTable,
                                   partialFrame);
    }
    if (skelAnime->morphWeight != 0) {
        f32 updateRate = R_UPDATE_RATE * (1.0f / 3.0f);

        skelAnime->morphWeight -= skelAnime->morphRate * updateRate;
        if (skelAnime->morphWeight <= 0.0f) {
            skelAnime->morphWeight = 0.0f;
        } else {
            SkelAnime_InterpFrameTable(skelAnime->limbCount, skelAnime->jointTable, skelAnime->jointTable,
                                       skelAnime->morphTable, skelAnime->morphWeight);
        }
    }
}

/**
 * Advances an animation that loops over its full length and updates the frame tables
 */
s32 SkelAnime_LoopFull(SkelAnime* skelAnime) {
    f32 updateRate = R_UPDATE_RATE * (1.0f / 3.0f);

    skelAnime->curFrame += skelAnime->playSpeed * updateRate;
    if (skelAnime->curFrame < 0.0f) {
        skelAnime->curFrame += skelAnime->animLength;
    } else if (skelAnime->animLength <= skelAnime->curFrame) {
        skelAnime->curFrame -= skelAnime->animLength;
    }
    SkelAnime_AnimateFrame(skelAnime);
    return false;
}

/**
 * Advances an animation that loops over part of its length and updates the frame tables
 */
s32 SkelAnime_LoopPartial(SkelAnime* skelAnime) {
    f32 updateRate = R_UPDATE_RATE * (1.0f / 3.0f);

    skelAnime->curFrame += skelAnime->playSpeed * updateRate;
    if (skelAnime->curFrame < skelAnime->startFrame) {
        skelAnime->curFrame = (skelAnime->curFrame - skelAnime->startFrame) + skelAnime->endFrame;
    } else if (skelAnime->endFrame <= skelAnime->curFrame) {
        skelAnime->curFrame = (skelAnime->curFrame - skelAnime->endFrame) + skelAnime->startFrame;
    }

    SkelAnime_AnimateFrame(skelAnime);
    return false;
}

/**
 * Advances an animation that stops at endFrame and returns true when it is reached.
 */
s32 SkelAnime_Once(SkelAnime* skelAnime) {
    f32 updateRate = R_UPDATE_RATE * (1.0f / 3.0f);

    if (skelAnime->curFrame == skelAnime->endFrame) {
        SkelAnime_GetFrameData(skelAnime->animation, (s32)skelAnime->curFrame, skelAnime->limbCount,
                               skelAnime->jointTable);
        SkelAnime_AnimateFrame(skelAnime);
        return true;
    }

    skelAnime->curFrame += skelAnime->playSpeed * updateRate;

    if ((skelAnime->curFrame - skelAnime->endFrame) * skelAnime->playSpeed > 0.0f) {
        skelAnime->curFrame = skelAnime->endFrame;
    } else if (skelAnime->curFrame < 0.0f) {
        skelAnime->curFrame += skelAnime->animLength;
    } else if (skelAnime->animLength <= skelAnime->curFrame) {
        skelAnime->curFrame -= skelAnime->animLength;
    }
    SkelAnime_AnimateFrame(skelAnime);
    return false;
}

/**
 * Fully general way to set a new animation, allowing choice of playback speed, start frame, end frame, play mode,
 * number of transition frames, and tapering of the transition. Positive morph frames morph from the current pose to the
 * start pose of the new animation, then start the new animation. Negative morph frames start the new animation
 * immediately, modified by the pose immediately before the animation change.
 */
void Animation_ChangeImpl(SkelAnime* skelAnime, AnimationHeader* animation, f32 playSpeed, f32 startFrame, f32 endFrame,
                          u8 mode, f32 morphFrames, s8 taper) {
    skelAnime->mode = mode;
    if ((morphFrames != 0.0f) && ((animation != skelAnime->animation) || (startFrame != skelAnime->curFrame))) {
        if (morphFrames < 0) {
            SkelAnime_SetUpdate(skelAnime);
            SkelAnime_CopyFrameTable(skelAnime, skelAnime->morphTable, skelAnime->jointTable);
            morphFrames = -morphFrames;
        } else {
            if (taper != ANIMTAPER_NONE) {
                skelAnime->update.normal = SkelAnime_MorphTaper;
                skelAnime->taper = taper;
            } else {
                skelAnime->update.normal = SkelAnime_Morph;
            }
            SkelAnime_GetFrameData(animation, startFrame, skelAnime->limbCount, skelAnime->morphTable);
        }
        skelAnime->morphWeight = 1.0f;
        skelAnime->morphRate = 1.0f / morphFrames;
    } else {
        SkelAnime_SetUpdate(skelAnime);
        SkelAnime_GetFrameData(animation, startFrame, skelAnime->limbCount, skelAnime->jointTable);
        skelAnime->morphWeight = 0.0f;
    }

    skelAnime->animation = animation;
    skelAnime->startFrame = startFrame;
    skelAnime->endFrame = endFrame;
    skelAnime->animLength = Animation_GetLength(animation);
    if (skelAnime->mode >= ANIMMODE_LOOP_PARTIAL) {
        skelAnime->curFrame = 0.0f;
    } else {
        skelAnime->curFrame = startFrame;
        if (skelAnime->mode <= ANIMMODE_LOOP_INTERP) {
            skelAnime->endFrame = skelAnime->animLength - 1.0f;
        }
    }
    skelAnime->playSpeed = playSpeed;
}

/**
 * General way to set a new animation, allowing choice of playback speed, start frame, end frame, play mode, and number
 * of transition frames. Positive morph frames morph from the current pose to the start pose of the new animation, then
 * start the new animation. Negative morph frames start the new animation immediately, modified by the pose immediately
 * before the animation change.
 */
void Animation_Change(SkelAnime* skelAnime, AnimationHeader* animation, f32 playSpeed, f32 startFrame, f32 endFrame,
                      u8 mode, f32 morphFrames) {
    Animation_ChangeImpl(skelAnime, animation, playSpeed, startFrame, endFrame, mode, morphFrames, ANIMTAPER_NONE);
}

/**
 * Immediately changes to an animation that plays once at the default speed.
 */
void Animation_PlayOnce(SkelAnime* skelAnime, AnimationHeader* animation) {
    Animation_Change(skelAnime, animation, 1.0f, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_ONCE, 0.0f);
}

/**
 * Smoothly transitions to an animation that plays once at the default speed.
 * Positive morph frames morph from the current pose to the start pose of the new animation, then start the new
 * animation. Negative morph frames start the new animation immediately, modified by the pose immediately before the
 * animation change.
 */
void Animation_MorphToPlayOnce(SkelAnime* skelAnime, AnimationHeader* animation, f32 morphFrames) {
    Animation_Change(skelAnime, animation, 1.0f, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_ONCE, morphFrames);
}

/**
 * Immediately changes to an animation that plays once at the specified speed.
 */
void Animation_PlayOnceSetSpeed(SkelAnime* skelAnime, AnimationHeader* animation, f32 playSpeed) {
    Animation_Change(skelAnime, animation, playSpeed, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_ONCE, 0.0f);
}

/**
 * Immediately changes to an animation that loops at the default.
 */
void Animation_PlayLoop(SkelAnime* skelAnime, AnimationHeader* animation) {
    Animation_Change(skelAnime, animation, 1.0f, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_LOOP, 0.0f);
}

/**
 * Smoothly transitions to a looping animation, specifying the number of frames for the transition.
 * Positive morph frames morph from the current pose to the start pose of the new animation, then start the new
 * animation. Negative morph frames start the new animation immediately, modified by the pose immediately before the
 * animation change.
 */
void Animation_MorphToLoop(SkelAnime* skelAnime, AnimationHeader* animation, f32 morphFrames) {
    Animation_Change(skelAnime, animation, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, morphFrames);
}

/**
 * Immediately changes to an animation that loops at the specified speed.
 */
void Animation_PlayLoopSetSpeed(SkelAnime* skelAnime, AnimationHeader* animation, f32 playSpeed) {
    Animation_Change(skelAnime, animation, playSpeed, 0.0f, Animation_GetLastFrame(animation), ANIMMODE_LOOP, 0.0f);
}

/**
 * Changes a looping animation to one that stops at the end. Unused
 */
void Animation_EndLoop(SkelAnime* skelAnime) {
    skelAnime->mode = ANIMMODE_ONCE;
    skelAnime->endFrame = skelAnime->animLength;
    SkelAnime_SetUpdate(skelAnime);
}

/**
 * Reverses the current animation.
 */
void Animation_Reverse(SkelAnime* skelAnime) {
    f32 startFrame = skelAnime->startFrame;

    skelAnime->startFrame = skelAnime->endFrame;
    skelAnime->playSpeed = -skelAnime->playSpeed;
    skelAnime->endFrame = startFrame;
}

/**
 * Copies the src frame table to the dst frame table if copyFlag for that limb is true.
 * Used only by En_Test
 */
void SkelAnime_CopyFrameTableTrue(SkelAnime* skelAnime, Vec3s* dst, Vec3s* src, u8* copyFlag) {
    s32 i;

    for (i = 0; i < skelAnime->limbCount; i++, dst++, src++) {
        if (*copyFlag++) {
            *dst = *src;
        }
    }
}

/**
 * Copies the src frame table to the dst frame table if copyFlag for that limb is false.
 * Unused.
 */
void SkelAnime_CopyFrameTableFalse(SkelAnime* skelAnime, Vec3s* dst, Vec3s* src, u8* copyFlag) {
    s32 i;

    for (i = 0; i < skelAnime->limbCount; i++, dst++, src++) {
        if (!*copyFlag++) {
            *dst = *src;
        }
    }
}

/**
 * Updates translation of the root limb and calculates `pos`, the difference between
 * the old and new positions of the root limb as rotated by `angle`. Used to allow
 * animations to change an actor's position.
 */
void SkelAnime_UpdateTranslation(SkelAnime* skelAnime, Vec3f* diff, s16 angle) {
    f32 x;
    f32 z;
    f32 sin;
    f32 cos;

    // If `ANIM_FLAG_UPDATE_XZ` behaved as expected, it would also be checked here
    if (skelAnime->movementFlags & ANIM_FLAG_ADJUST_STARTING_POS) {
        diff->x = diff->z = 0.0f;
    } else {
        x = skelAnime->jointTable[0].x;
        z = skelAnime->jointTable[0].z;
        sin = Math_SinS(angle);
        cos = Math_CosS(angle);
        diff->x = x * cos + z * sin;
        diff->z = z * cos - x * sin;

        x = skelAnime->prevTransl.x;
        z = skelAnime->prevTransl.z;
        sin = Math_SinS(skelAnime->prevRot);
        cos = Math_CosS(skelAnime->prevRot);
        diff->x -= x * cos + z * sin;
        diff->z -= z * cos - x * sin;
    }

    skelAnime->prevRot = angle;

    skelAnime->prevTransl.x = skelAnime->jointTable[0].x;
    skelAnime->jointTable[0].x = skelAnime->baseTransl.x;

    skelAnime->prevTransl.z = skelAnime->jointTable[0].z;
    skelAnime->jointTable[0].z = skelAnime->baseTransl.z;

    if (skelAnime->movementFlags & ANIM_FLAG_UPDATE_Y) {
        if (skelAnime->movementFlags & ANIM_FLAG_ADJUST_STARTING_POS) {
            diff->y = 0.0f;
        } else {
            diff->y = skelAnime->jointTable[0].y - skelAnime->prevTransl.y;
        }

        skelAnime->prevTransl.y = skelAnime->jointTable[0].y;
        skelAnime->jointTable[0].y = skelAnime->baseTransl.y;
    } else {
        diff->y = 0.0f;
        skelAnime->prevTransl.y = skelAnime->jointTable[0].y;
    }

    skelAnime->movementFlags &= ~ANIM_FLAG_ADJUST_STARTING_POS;
}

/**
 * Checks if the current animation is at the specified frame
 */
s32 Animation_OnFrame(SkelAnime* skelAnime, f32 frame) {
    return Animation_OnFrameImpl(skelAnime, frame, 1.0f);
}

/**
 * Frees the frame tables for a skelAnime with dynamically allocated tables.
 */
void SkelAnime_Free(SkelAnime* skelAnime, UNUSED PlayState* play) {
    if (skelAnime->jointTable != NULL) {
        FOA_ARENA_FREE(skelAnime->jointTable, "../z_skelanime.c", 3729);
    } else {
        PRINTF(T("now_joint あきまへん！！\n", "now_joint is freed!!\n"));
    }

    if (skelAnime->morphTable != NULL) {
        FOA_ARENA_FREE(skelAnime->morphTable, "../z_skelanime.c", 3731);
    } else {
        PRINTF(T("morf_joint あきまへん！！\n", "morf_joint is freed!!\n"));
    }
}

/**
 * Copies the src frame table to the dst frame table.
 */
void SkelAnime_CopyFrameTable(SkelAnime* skelAnime, Vec3s* dst, Vec3s* src) {
    s32 i;

    for (i = 0; i < skelAnime->limbCount; i++) {
        *dst++ = *src++;
    }
}
