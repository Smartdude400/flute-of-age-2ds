// adapted from flute-of-age/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c
/*
 * File: z_en_viewer.c
 * Overlay: ovl_En_Viewer
 * Description: Cutscene Actors
 */

#include "src/overlays/actors/ovl_En_Viewer/z_en_viewer.h"
#include "engine/asset.h"
#include "engine/gfx3d.h"
#include "src/overlays/actors/ovl_Boss_Varkon/z_boss_varkon.h"
#include "src/overlays/actors/ovl_En_Varkon_Mant/z_en_varkon_mant.h"

#include "libc64/qrand.h"
#include "array_count.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "ichain.h"
#include "regs.h"
#include "segmented_address.h"
#include "seqcmd.h"
#include "sequence.h"
#include "sfx.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "z_lib.h"
#include "audio.h"
#include "play_state.h"
#include "save.h"
#include "skin.h"

#include "assets/objects/object_zl4/object_zl4.h"
#include "assets/objects/gameplay_keep/eff_fire.h"
#include "assets/objects/object_horse_elara/object_horse_elara.h"
#include "assets/objects/object_horse_varkon/object_horse_varkon.h"
#include "assets/objects/object_im/object_im.h"
#include "assets/objects/object_gndd/object_gndd.h"
#include "assets/objects/object_varkon/object_varkon.h"
#include "assets/objects/object_opening_demo1/object_opening_demo1.h"


#define FLAGS ACTOR_FLAG_UPDATE_CULLING_DISABLED

void EnViewer_Init(Actor* thisx, PlayState* play);
void EnViewer_Destroy(Actor* thisx, PlayState* play);
void EnViewer_Update(Actor* thisx, PlayState* play);
void EnViewer_Draw(Actor* thisx, PlayState* play);

void EnViewer_UpdatePosition(EnViewer* this, PlayState* play);
void EnViewer_DrawFireEffects(EnViewer* this2, PlayState* play);
void EnViewer_UpdateVarkonosCape(PlayState* play, EnViewer* this);
void EnViewer_InitImpl(EnViewer* this, PlayState* play);
void EnViewer_UpdateImpl(EnViewer* this, PlayState* play);

static u8 sHorseSfxPlayed = false;

ActorProfile En_Viewer_Profile = {
    /**/ ACTOR_EN_VIEWER,
    /**/ ACTORCAT_ITEMACTION,
    /**/ FLAGS,
    /**/ OBJECT_GAMEPLAY_KEEP,
    /**/ sizeof(EnViewer),
    /**/ EnViewer_Init,
    /**/ EnViewer_Destroy,
    /**/ EnViewer_Update,
    /**/ EnViewer_Draw,
};

static InitChainEntry sInitChain[] = {
    ICHAIN_F32(cullingVolumeScale, 300, ICHAIN_STOP),
};

static EnViewerInitData sInitData[] = {
    /* ENVIEWER_TYPE_0_HORSE_ELARA */
    { OBJECT_HORSE_ELARA, OBJECT_HORSE_ELARA, 1, 0, ENVIEWER_SHADOW_HORSE, 20, ENVIEWER_DRAW_HORSE, &gHorseElaraSkel,
      &gHorseElaraGallopingAnim },
    /* ENVIEWER_TYPE_1_ILYA */
    { OBJECT_IM, OBJECT_OPENING_DEMO1, 1, 0, ENVIEWER_SHADOW_NONE, 10, ENVIEWER_DRAW_ILYA, &gIlyaSkel,
      &object_opening_demo1_Anim_0029CC },
    /* ENVIEWER_TYPE_2_ELARA */
    { OBJECT_ZL4, OBJECT_OPENING_DEMO1, 1, 0, ENVIEWER_SHADOW_NONE, 10, ENVIEWER_DRAW_ELARA, &gChildElaraSkel,
      &object_opening_demo1_Anim_000450 },
    /* ENVIEWER_TYPE_3_VARKONOS */
    { OBJECT_GNDD, OBJECT_GNDD, 1, -6, ENVIEWER_SHADOW_NONE, 10, ENVIEWER_DRAW_VARKONOS, &gYoungVarkonosSkel,
      &gYoungVarkonosHorsebackIdleAnim },
    /* ENVIEWER_TYPE_4_HORSE_VARKONOS */
    { OBJECT_HORSE_VARKON, OBJECT_HORSE_VARKON, 1, 0, ENVIEWER_SHADOW_HORSE, 20, ENVIEWER_DRAW_HORSE, &gHorseVarkonSkel,
      &gHorseVarkonRearingAnim },
    /* ENVIEWER_TYPE_5_VARKONOS */
    { OBJECT_GNDD, OBJECT_GNDD, 1, -6, ENVIEWER_SHADOW_NONE, 10, ENVIEWER_DRAW_VARKONOS, &gYoungVarkonosSkel,
      &gYoungVarkonosHorsebackRideAnim },
    /* ENVIEWER_TYPE_6_HORSE_VARKONOS */
    { OBJECT_HORSE_VARKON, OBJECT_HORSE_VARKON, 1, 0, ENVIEWER_SHADOW_HORSE, 20, ENVIEWER_DRAW_HORSE, &gHorseVarkonSkel,
      &gHorseVarkonGallopingAnim },
    /* ENVIEWER_TYPE_7_VARKONOS */
    { OBJECT_GNDD, OBJECT_GNDD, 1, -6, ENVIEWER_SHADOW_NONE, 10, ENVIEWER_DRAW_VARKONOS, &gYoungVarkonosSkel,
      &gYoungVarkonosArmsCrossedAnim },
    /* ENVIEWER_TYPE_8_VARKONOS */
    { OBJECT_GNDD, OBJECT_GNDD, 1, -6, ENVIEWER_SHADOW_NONE, 10, ENVIEWER_DRAW_VARKONOS, &gYoungVarkonosSkel,
      &gYoungVarkonosWalkAnim },
    /* ENVIEWER_TYPE_9_VARKONOS */
    { OBJECT_VARKON, OBJECT_VARKON, 1, -6, ENVIEWER_SHADOW_NONE, 10, ENVIEWER_DRAW_VARKONOS, &gVarkonosSkel,
      &gVarkonosEndingFloatAnim },
};

static EnVarkonMant* sVarkonosCape;

static Vec3f sVarkonosNeckWorldPos;

void EnViewer_SetupAction(EnViewer* this, EnViewerActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void EnViewer_Init(Actor* thisx, PlayState* play) {
    EnViewer* this = (EnViewer*)thisx;
    u8 type;

    Actor_ProcessInitChain(&this->actor, sInitChain);
    EnViewer_SetupAction(this, EnViewer_InitImpl);
    sHorseSfxPlayed = false;
    type = PARAMS_GET_NOMASK(this->actor.params, 8);
    this->unused = 0;
    this->state = 0;
    this->isVisible = false;
    if (type == ENVIEWER_TYPE_3_VARKONOS || type == ENVIEWER_TYPE_5_VARKONOS || type == ENVIEWER_TYPE_7_VARKONOS ||
        type == ENVIEWER_TYPE_8_VARKONOS || type == ENVIEWER_TYPE_9_VARKONOS) {
        sVarkonosCape = (EnVarkonMant*)Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_EN_VARKON_MANT,
                                                          0.0f, 0.0f, 0.0f, 0, 0, 0, 35);
    }
}

void EnViewer_Destroy(Actor* thisx, PlayState* play) {
    EnViewer* this = (EnViewer*)thisx;

    Skin_Free(play, &this->skin);
}

void EnViewer_InitAnimVarkonosOrElara(EnViewer* this, PlayState* play, void* skeletonHeaderSeg,
                                       AnimationHeader* anim) {
    s16 type = PARAMS_GET_NOMASK(this->actor.params, 8);

    if (type == ENVIEWER_TYPE_2_ELARA || type == ENVIEWER_TYPE_3_VARKONOS || type == ENVIEWER_TYPE_5_VARKONOS ||
        type == ENVIEWER_TYPE_7_VARKONOS || type == ENVIEWER_TYPE_8_VARKONOS || type == ENVIEWER_TYPE_9_VARKONOS) {
        SkelAnime_InitFlex(play, &this->skin.skelAnime, skeletonHeaderSeg, NULL, NULL, NULL, 0);
    } else {
        SkelAnime_Init(play, &this->skin.skelAnime, skeletonHeaderSeg, NULL, NULL, NULL, 0);
    }

    Asset_SetCpuSegment(6, play->objectCtx.slots[this->animObjectSlot].dsObj);
    if (type == ENVIEWER_TYPE_3_VARKONOS || type == ENVIEWER_TYPE_7_VARKONOS || type == ENVIEWER_TYPE_8_VARKONOS ||
        type == ENVIEWER_TYPE_9_VARKONOS) {
        Animation_PlayLoopSetSpeed(&this->skin.skelAnime, anim, 1.0f);
    } else {
        Animation_PlayLoopSetSpeed(&this->skin.skelAnime, anim, 3.0f);
    }
}

void EnViewer_InitAnimIlya(EnViewer* this, PlayState* play, void* skeletonHeaderSeg, AnimationHeader* anim) {
    SkelAnime_InitFlex(play, &this->skin.skelAnime, skeletonHeaderSeg, NULL, NULL, NULL, 0);
    Asset_SetCpuSegment(6, play->objectCtx.slots[this->animObjectSlot].dsObj);
    Animation_PlayLoopSetSpeed(&this->skin.skelAnime, anim, 3.0f);
}

void EnViewer_InitAnimHorse(EnViewer* this, PlayState* play, void* skeletonHeaderSeg, AnimationHeader* anim) {
    u8 type;

    Skin_Init(play, &this->skin, skeletonHeaderSeg, anim);
    type = PARAMS_GET_NOMASK(this->actor.params, 8);
    if (!(type == ENVIEWER_TYPE_3_VARKONOS || type == ENVIEWER_TYPE_4_HORSE_VARKONOS ||
          type == ENVIEWER_TYPE_7_VARKONOS || type == ENVIEWER_TYPE_8_VARKONOS ||
          type == ENVIEWER_TYPE_9_VARKONOS)) {
        Animation_PlayLoopSetSpeed(&this->skin.skelAnime, anim, 3.0f);
    } else {
        Animation_PlayOnceSetSpeed(&this->skin.skelAnime, anim, 1.0f);
    }
}

static EnViewerInitAnimFunc sInitAnimFuncs[] = {
    EnViewer_InitAnimVarkonosOrElara,
    EnViewer_InitAnimHorse,
    EnViewer_InitAnimVarkonosOrElara,
    EnViewer_InitAnimIlya,
};

static ActorShadowFunc sShadowDrawFuncs[] = {
    NULL,
    ActorShadow_DrawCircle,
    ActorShadow_DrawHorse,
};

void EnViewer_InitImpl(EnViewer* this, PlayState* play) {
    EnViewerInitData* initData = &sInitData[PARAMS_GET_NOMASK(this->actor.params, 8)];
    s32 skelObjectSlot = Object_GetSlot(&play->objectCtx, initData->skeletonObject);

    ASSERT(skelObjectSlot >= 0, "bank_ID >= 0", "../z_en_viewer.c", 576);

    this->animObjectSlot = Object_GetSlot(&play->objectCtx, initData->animObject);
    ASSERT(this->animObjectSlot >= 0, "this->anime_bank_ID >= 0", "../z_en_viewer.c", 579);

    if (!Object_IsLoaded(&play->objectCtx, skelObjectSlot) ||
        !Object_IsLoaded(&play->objectCtx, this->animObjectSlot)) {
        this->actor.flags &= ~ACTOR_FLAG_INSIDE_CULLING_VOLUME;
        return;
    }

    this->isVisible = true;
    this->actor.objectSlot = skelObjectSlot;
    Actor_SetObjectDependency(play, &this->actor);
    Actor_SetScale(&this->actor, initData->scale / 100.0f);
    ActorShape_Init(&this->actor.shape, initData->yOffset * 100, sShadowDrawFuncs[initData->shadowType],
                    initData->shadowScale);
    this->drawFuncIndex = initData->drawType;
    sInitAnimFuncs[this->drawFuncIndex](this, play, initData->skeletonHeaderSeg, initData->anim);
    EnViewer_SetupAction(this, EnViewer_UpdateImpl);
}

static s16 sTimer = 0;

void EnViewer_UpdateImpl(EnViewer* this, PlayState* play) {
    u8 type = PARAMS_GET_NOMASK(this->actor.params, 8);
    u16 csCurFrame;
    s32 animFinished;

    if (type == ENVIEWER_TYPE_2_ELARA) {
        if (gSaveContext.sceneLayer == 5) {
            csCurFrame = play->csCtx.curFrame;
            if (csCurFrame == 792) {
                Actor_PlaySfx(&this->actor, NA_SE_VO_Z0_SURPRISE);
            } else if (csCurFrame == 845) {
                Actor_PlaySfx(&this->actor, NA_SE_VO_Z0_THROW);
            }
        }
    } else if (type == ENVIEWER_TYPE_7_VARKONOS) {
        Actor_SetScale(&this->actor, 0.3f);
        this->actor.cullingVolumeDistance = 10000.0f;
        this->actor.cullingVolumeScale = 10000.0f;
        this->actor.cullingVolumeDownward = 10000.0f;
    } else if (type == ENVIEWER_TYPE_3_VARKONOS) {
        if (gSaveContext.sceneLayer == 4) {
            switch (play->csCtx.curFrame) {
                case 20:
                case 59:
                case 71:
                case 129:
                case 140:
                case 219:
                case 280:
                case 320:
                case 380:
                case 409:
                case 438:
                    SFX_PLAY_CENTERED(NA_SE_SY_DEMO_CUT);
                    break;
            }
        }
        if (gSaveContext.sceneLayer == 5) {
            if (play->csCtx.curFrame == 1508) {
                Actor_PlaySfx(&this->actor, NA_SE_EN_FANTOM_ST_LAUGH);
            }
            if (play->csCtx.curFrame == 1545) {
                Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_DEMO_6K, 32.0f, 101.0f, 1226.0f, 0, 0, 0,
                                   0xC);
            }
        }
        if (play->csCtx.curFrame == 1020) {
            SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_FANFARE, 0, 0, NA_BGM_OPENING_VARKON);
        }
        if (play->csCtx.curFrame == 960) {
            SFX_PLAY_AT_POS(&this->actor.projectedPos, NA_SE_EV_HORSE_GROAN);
        }
    } else if (type == ENVIEWER_TYPE_6_HORSE_VARKONOS) {
        if (gSaveContext.sceneLayer == 5 || gSaveContext.sceneLayer == 10) {
            Actor_PlaySfx(&this->actor, NA_SE_EV_HORSE_RUN_LEVEL - SFX_FLAG);
        }
    } else if (type == ENVIEWER_TYPE_4_HORSE_VARKONOS) {
        s16 curFrame = this->skin.skelAnime.curFrame;

        if (this->skin.skelAnime.animation == &gHorseVarkonRearingAnim) {
            if (curFrame == 8) {
                Actor_PlaySfx(&this->actor, NA_SE_EV_VARKON_HORSE_NEIGH);
            }
            if (curFrame == 30) {
                Actor_PlaySfx(&this->actor, NA_SE_EV_HORSE_LAND2);
            }
        } else if (this->skin.skelAnime.animation == &gHorseVarkonIdleAnim) {
            if (curFrame == 25) {
                Actor_PlaySfx(&this->actor, NA_SE_EV_HORSE_SANDDUST);
            }
        } else if (this->skin.skelAnime.animation == &gHorseVarkonGallopingAnim) {
            Actor_PlaySfx(&this->actor, NA_SE_EV_HORSE_RUN_LEVEL - SFX_FLAG);
        }
    }

    if (sTimer != 0) {
        sTimer--;
    }

    EnViewer_UpdatePosition(this, play);
    Actor_MoveXZGravity(&this->actor); // has no effect, speed/velocity and gravity are 0

    animFinished = SkelAnime_Update(&this->skin.skelAnime);
    if (type == ENVIEWER_TYPE_3_VARKONOS || type == ENVIEWER_TYPE_4_HORSE_VARKONOS) {
        if (play->csCtx.state != CS_STATE_IDLE && play->csCtx.actorCues[1] != NULL) {
            if (play->csCtx.actorCues[1]->id == 2 && sTimer == 0) {
                if (type == ENVIEWER_TYPE_3_VARKONOS) {
                    if (this->skin.skelAnime.animation != &gYoungVarkonosHorsebackIdleAnim) {
                        Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &gYoungVarkonosHorsebackIdleAnim, 1.0f);
                    }
                } else if (this->skin.skelAnime.animation != &gHorseVarkonIdleAnim) {
                    Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &gHorseVarkonIdleAnim, 1.0f);
                }
            } else if (play->csCtx.actorCues[1]->id == 1) {
                sTimer = 100;
                if (type == ENVIEWER_TYPE_3_VARKONOS) {
                    if (this->skin.skelAnime.animation != &gYoungVarkonosHorsebackRearAnim) {
                        Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &gYoungVarkonosHorsebackRearAnim, 1.0f);
                    }
                } else if (this->skin.skelAnime.animation != &gHorseVarkonRearingAnim) {
                    Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &gHorseVarkonRearingAnim, 1.0f);
                }
            } else if (type == ENVIEWER_TYPE_3_VARKONOS) {
                switch (this->state) {
                    case 0:
                        if (play->csCtx.actorCues[1]->id == 4) {
                            Animation_MorphToPlayOnce(&this->skin.skelAnime,
                                                      &gYoungVarkonosHorsebackLookSidewaysStartAnim, -5.0f);
                            this->state++;
                        }
                        break;
                    case 1:
                        if (animFinished) {
                            Animation_MorphToLoop(&this->skin.skelAnime, &gYoungVarkonosHorsebackLookSidewaysLoopAnim,
                                                  -5.0f);
                            this->state++;
                        }
                        break;
                    case 2:
                        if (play->csCtx.actorCues[1]->id == 5) {
                            Animation_MorphToPlayOnce(&this->skin.skelAnime,
                                                      &gYoungVarkonosHorsebackMagicChargeUpStartAnim, -5.0f);
                            this->state++;
                        }
                        break;
                    case 3:
                        if (animFinished) {
                            Animation_MorphToLoop(&this->skin.skelAnime, &gYoungVarkonosHorsebackMagicChargeUpLoopAnim,
                                                  -5.0f);
                            this->state++;
                        }
                        break;
                    case 4:
                        if (play->csCtx.actorCues[1]->id == 11) {
                            Animation_MorphToLoop(&this->skin.skelAnime, &gYoungVarkonosHorsebackLookSidewaysLoopAnim,
                                                  -20.0f);
                            this->state++;
                        }
                        break;
                    case 5:
                        if (play->csCtx.actorCues[1]->id == 8) {
                            Animation_MorphToLoop(&this->skin.skelAnime, &gYoungVarkonosHorsebackIdleAnim, -15.0f);
                            this->state++;
                        }
                        break;
                    case 6:
                        if (play->csCtx.actorCues[1]->id == 12) {
                            Actor_PlaySfx(&this->actor, NA_SE_EN_VARKON_VOICE_DEMO);
                            Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &gYoungVarkonosHorsebackRideAnim, 3.0f);
                            this->state++;
                        }
                        break;
                    case 7:
                        this->state = 0;
                        break;
                }
            } else if (this->skin.skelAnime.animation != &gHorseVarkonGallopingAnim &&
                       play->csCtx.actorCues[1]->id == 12) {
                Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &gHorseVarkonGallopingAnim, 3.0f);
            }
        }
    } else if (type == ENVIEWER_TYPE_1_ILYA) {
        if (gSaveContext.sceneLayer == 5) {
            if (play->csCtx.curFrame == 845) {
                Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_ITEM_FLUTE, 4.0f, 81.0f, 2600.0f, 0, 0,
                                   0, 0);
            }
        } else {
            if (play->csCtx.curFrame == 195) {
                Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_ITEM_FLUTE, 4.0f, 81.0f, 2035.0f, 0, 0,
                                   0, 1);
            }
        }
        switch (this->state) {
            case 0:
                if (play->csCtx.state != CS_STATE_IDLE && play->csCtx.actorCues[0] != NULL &&
                    play->csCtx.actorCues[0]->id == 6 &&
                    this->skin.skelAnime.animation != &object_opening_demo1_Anim_002574) {
                    Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &object_opening_demo1_Anim_002574, 1.5f);
                    this->state++;
                }
                break;
            case 1:
                if (play->csCtx.state != CS_STATE_IDLE && play->csCtx.actorCues[0] != NULL &&
                    play->csCtx.actorCues[0]->id == 2 &&
                    this->skin.skelAnime.animation != &object_opening_demo1_Anim_0029CC) {
                    Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &object_opening_demo1_Anim_0029CC, 3.0f);
                    this->state++;
                }
                break;
        }
    } else if (type == ENVIEWER_TYPE_2_ELARA) {
        if (play->sceneId == SCENE_ELDORA_FIELD) {
            switch (this->state) {
                case 0:
                    if (play->csCtx.state != CS_STATE_IDLE) {
                        if (play->csCtx.actorCues[0] != NULL && play->csCtx.actorCues[0]->id == 6 &&
                            this->skin.skelAnime.animation != &object_opening_demo1_Anim_001410) {
                            Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &object_opening_demo1_Anim_001410, 1.5f);
                            this->state++;
                        }
                    }
                    break;
                case 1:
                    if (play->csCtx.state != CS_STATE_IDLE) {
                        if (play->csCtx.actorCues[0] != NULL && play->csCtx.actorCues[0]->id == 2 &&
                            this->skin.skelAnime.animation != &object_opening_demo1_Anim_000450) {
                            Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &object_opening_demo1_Anim_000450, 3.0f);
                            this->state++;
                        }
                    }
                    break;
            }
        } else {
            Audio_SetBaseFilter(0);
            switch (this->state) {
                case 0:
                    Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &object_opening_demo1_Anim_00504C, 1.0f);
                    this->state++;
                    break;
                case 1:
                    if (play->csCtx.actorCues[0]->id == 11) {
                        Animation_MorphToPlayOnce(&this->skin.skelAnime, &object_opening_demo1_Anim_00420C, -5.0f);
                        this->state++;
                    }
                    break;
                case 2:
                    if (animFinished) {
                        Animation_MorphToLoop(&this->skin.skelAnime, &object_opening_demo1_Anim_0048FC, -5.0f);
                        this->state++;
                    }
                    break;
                case 3:
                    break;
            }
        }
    } else if (type == ENVIEWER_TYPE_7_VARKONOS) {
        switch (this->state) {
            case 0:
                if ((play->csCtx.state != CS_STATE_IDLE) && (play->csCtx.actorCues[1] != NULL) &&
                    (play->csCtx.actorCues[1]->id == 7)) {
                    SFX_PLAY_CENTERED(NA_SE_EN_VARKON_LAUGH);
                    Animation_MorphToPlayOnce(&this->skin.skelAnime, &gYoungVarkonosLaughStartAnim, -5.0f);
                    this->state++;
                }
                break;
            case 1:
                if (animFinished) {
                    Animation_MorphToLoop(&this->skin.skelAnime, &gYoungVarkonosLaughLoopAnim, -5.0f);
                    this->state++;
                }
                break;
        }
    } else if (type == ENVIEWER_TYPE_8_VARKONOS) {
        switch (this->state) {
            case 0:
                if (play->csCtx.state != CS_STATE_IDLE) {
                    if ((play->csCtx.actorCues[1] != NULL) && (play->csCtx.actorCues[1]->id == 9)) {
                        Animation_PlayLoopSetSpeed(&this->skin.skelAnime, &gYoungVarkonosWalkAnim, 1.0f);
                        this->state++;
                    }
                }
                break;
            case 1:
                if (play->csCtx.actorCues[1]->id == 10) {
                    Animation_MorphToPlayOnce(&this->skin.skelAnime, &gYoungVarkonosKneelStartAnim, -10.0f);
                    this->state++;
                }
                break;
            case 2:
                if (animFinished) {
                    Animation_MorphToLoop(&this->skin.skelAnime, &gYoungVarkonosKneelLoopAnim, -5.0f);
                    this->state++;
                }
                break;
            case 3:
                if (play->csCtx.actorCues[1]->id == 4) {
                    Animation_MorphToPlayOnce(&this->skin.skelAnime, &gYoungVarkonosKneelLookSidewaysAnim, -5.0f);
                    this->state++;
                }
                break;
            default:
                this->state = 0;
                break;
        }
    }
}

void EnViewer_Update(Actor* thisx, PlayState* play) {
    EnViewer* this = (EnViewer*)thisx;

    Asset_SetCpuSegment(6, play->objectCtx.slots[this->animObjectSlot].dsObj);
    this->actionFunc(this, play);
}

s32 EnViewer_Varkonos3OverrideLimbDraw(PlayState* play, s32 limbIndex, DListRef* dList, Vec3f* pos, Vec3s* rot,
                                        void* thisx) {
    if (gSaveContext.sceneLayer == 4) {
        if (play->csCtx.curFrame >= 400) {
            if (limbIndex == YOUNG_VARKONOS_LIMB_LEFT_HAND) {
                *dList = gYoungVarkonosOpenLeftHandDL;
            }
        }
    } else {
        if ((play->csCtx.curFrame >= 1510) && (play->csCtx.curFrame <= 1650)) {
            if (limbIndex == YOUNG_VARKONOS_LIMB_LEFT_HAND) {
                *dList = gYoungVarkonosOpenLeftHandDL;
            }
        }
    }
    return false;
}

void EnViewer_Varkonos9PostLimbDraw(PlayState* play, s32 limbIndex, DListRef* dList, Vec3s* rot, void* thisx) {
    if (limbIndex == VARKONOS_LIMB_JEWEL) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
        GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(Asset_Resolve(gVarkonosEyesDL))));
    }
}

void EnViewer_VarkonosPostLimbDrawUpdateCapeVec(PlayState* play, s32 limbIndex, DListRef* dList, Vec3s* rot, void* thisx) {
    static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };

    if (limbIndex == YOUNG_VARKONOS_LIMB_HEAD) {
        Matrix_MultVec3f(&zeroVec, &sVarkonosNeckWorldPos);
    }
}

void EnViewer_DrawVarkonos(EnViewer* this, PlayState* play) {
    s16 frames = 0;
    s16 type;


    type = PARAMS_GET_NOMASK(this->actor.params, 8);

    if (type == ENVIEWER_TYPE_3_VARKONOS || type == ENVIEWER_TYPE_5_VARKONOS || type == ENVIEWER_TYPE_7_VARKONOS ||
        type == ENVIEWER_TYPE_8_VARKONOS) {
        if (gSaveContext.sceneLayer != 4) {
            frames = 149;
        }

        if (frames + 1127 >= play->csCtx.curFrame) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gYoungVarkonosEyeOpenTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gYoungVarkonosEyeOpenTex))));

        } else if (frames + 1128 >= play->csCtx.curFrame) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gYoungVarkonosEyeHalfTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gYoungVarkonosEyeHalfTex))));

        } else if (frames + 1129 >= play->csCtx.curFrame) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gYoungVarkonosEyeClosedTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gYoungVarkonosEyeClosedTex))));

        } else {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gYoungVarkonosEyeLookingDownTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gYoungVarkonosEyeLookingDownTex))));
        }
    } else if (type == ENVIEWER_TYPE_9_VARKONOS) {
        GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gVarkonosCrazedEyeTex))));
    }

    if (type == ENVIEWER_TYPE_9_VARKONOS) {
        SkelAnime_DrawFlexOpa(play, this->skin.skelAnime.skeleton, this->skin.skelAnime.jointTable,
                              this->skin.skelAnime.dListCount, NULL, EnViewer_Varkonos9PostLimbDraw, this);
    } else if (type == ENVIEWER_TYPE_3_VARKONOS) {
        SkelAnime_DrawFlexOpa(play, this->skin.skelAnime.skeleton, this->skin.skelAnime.jointTable,
                              this->skin.skelAnime.dListCount, EnViewer_Varkonos3OverrideLimbDraw,
                              EnViewer_VarkonosPostLimbDrawUpdateCapeVec, this);
        EnViewer_UpdateVarkonosCape(play, this);
    } else if (type == ENVIEWER_TYPE_3_VARKONOS || type == ENVIEWER_TYPE_5_VARKONOS ||
               type == ENVIEWER_TYPE_7_VARKONOS || type == ENVIEWER_TYPE_8_VARKONOS) {
        if ((play->csCtx.state != CS_STATE_IDLE) && (play->csCtx.actorCues[1] != NULL)) {
            SkelAnime_DrawFlexOpa(play, this->skin.skelAnime.skeleton, this->skin.skelAnime.jointTable,
                                  this->skin.skelAnime.dListCount, NULL, EnViewer_VarkonosPostLimbDrawUpdateCapeVec,
                                  this);
            EnViewer_UpdateVarkonosCape(play, this);
        }
    } else {
        SkelAnime_DrawOpa(play, this->skin.skelAnime.skeleton, this->skin.skelAnime.jointTable, NULL, NULL, this);
    }
}

void EnViewer_DrawHorse(EnViewer* this, PlayState* play) {
    func_800A6330(&this->actor, play, &this->skin, NULL, true);
}

s32 EnViewer_ElaraOverrideLimbDraw(PlayState* play, s32 limbIndex, DListRef* dList, Vec3f* pos, Vec3s* rot, void* thisx) {
    if (play->sceneId == SCENE_ELDORA_FIELD) {
        if (limbIndex == 2) {
            *dList = gChildElaraCutsceneDressDL;
        }
        if (limbIndex == 7) {
            *dList = NULL;
        }
        if (limbIndex == 8) {
            *dList = NULL;
        }
        if (limbIndex == 9) {
            *dList = NULL;
        }
        if (limbIndex == 3) {
            *dList = NULL;
        }
        if (limbIndex == 5) {
            *dList = NULL;
        }
    }
    return false;
}

void EnViewer_ElaraPostLimbDraw(PlayState* play, s32 limbIndex, DListRef* dList, Vec3s* rot, void* thisx) {
    STACK_PAD(s32);

    if (play->sceneId == SCENE_HALL_OF_AGES) {
        if (limbIndex == 16) {
            GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(gChildElaraFluteOfAgeDL)));
        }
    }
}

void EnViewer_DrawElara(EnViewer* this, PlayState* play) {
    if (play->sceneId == SCENE_ELDORA_FIELD) {
        if (play->csCtx.curFrame < 771) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeInTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeOutTex))));
        } else if (play->csCtx.curFrame < 772) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeBlinkTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeBlinkTex))));
        } else if (play->csCtx.curFrame < 773) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeShutTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeShutTex))));
        } else if (play->csCtx.curFrame < 791) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeWideTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeWideTex))));
        } else if (play->csCtx.curFrame < 792) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeBlinkTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeBlinkTex))));
        } else if (play->csCtx.curFrame < 793) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeShutTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeShutTex))));
        } else {
            GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeInTex))));
            GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeOutTex))));
        }

        if (gSaveContext.sceneLayer == 6) {
            GFX_TO(OPA, Gfx3D_SetSegment(0x0A, (const void*)(Asset_Resolve(gChildElaraMouthSurprisedTex))));
        } else {
            if (play->csCtx.curFrame < 758) {
                GFX_TO(OPA, Gfx3D_SetSegment(0x0A, (const void*)(Asset_Resolve(gChildElaraMouthWorriedTex))));
            } else if (play->csCtx.curFrame < 848) {
                GFX_TO(OPA, Gfx3D_SetSegment(0x0A, (const void*)(Asset_Resolve(gChildElaraMouthSurprisedTex))));
            } else {
                GFX_TO(OPA, Gfx3D_SetSegment(0x0A, (const void*)(Asset_Resolve(gChildElaraMouthWorriedTex))));
            }
        }
    } else {
        GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gChildElaraEyeShutTex))));
        GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gChildElaraEyeShutTex))));
        GFX_TO(OPA, Gfx3D_SetSegment(0x0A, (const void*)(Asset_Resolve(gChildElaraMouthWorriedTex))));
    }
    SkelAnime_DrawFlexOpa(play, this->skin.skelAnime.skeleton, this->skin.skelAnime.jointTable,
                          this->skin.skelAnime.dListCount, EnViewer_ElaraOverrideLimbDraw, EnViewer_ElaraPostLimbDraw,
                          this);
}

s32 EnViewer_IlyaOverrideLimbDraw(PlayState* play, s32 limbIndex, DListRef* dList, Vec3f* pos, Vec3s* rot, void* thisx) {
    if (limbIndex == 16) {
        *dList = gIlyaHeadMaskedDL;
    }
    return false;
}

void EnViewer_DrawIlya(EnViewer* this, PlayState* play) {
    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gIlyaEyeOpenTex))));
    GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(gIlyaEyeOpenTex))));
    GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, 255));
    GFX_TO(OPA, Gfx3D_SetSegment(0x0C, (const void*)(ACTOR_SETUP_OPA_DL)));
    SkelAnime_DrawFlexOpa(play, this->skin.skelAnime.skeleton, this->skin.skelAnime.jointTable,
                          this->skin.skelAnime.dListCount, EnViewer_IlyaOverrideLimbDraw, NULL, this);
}

static EnViewerDrawFunc sDrawFuncs[] = {
    EnViewer_DrawVarkonos,
    EnViewer_DrawHorse,
    EnViewer_DrawElara,
    EnViewer_DrawIlya,
};

void EnViewer_Draw(Actor* thisx, PlayState* play) {
    EnViewer* this = (EnViewer*)thisx;
    STACK_PAD(s32);
    s16 type;

    if (this->isVisible) {
        type = PARAMS_GET_NOMASK(this->actor.params, 8);
        if (type <= ENVIEWER_TYPE_2_ELARA) { // elara's horse, ilya and elara
            if (play->csCtx.state != CS_STATE_IDLE && play->csCtx.actorCues[0] != NULL) {
                Gfx_SetupDL_25Opa(play->state.gfxCtx);
                sDrawFuncs[this->drawFuncIndex](this, play);
            }
        } else if ((play->csCtx.state != CS_STATE_IDLE && play->csCtx.actorCues[1] != NULL) ||
                   type == ENVIEWER_TYPE_9_VARKONOS) {
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
            sDrawFuncs[this->drawFuncIndex](this, play);
        }
    }
}

void EnViewer_UpdatePosition(EnViewer* this, PlayState* play) {
    Vec3f startPos;
    Vec3f endPos;
    f32 lerpFactor;
    s16 type = PARAMS_GET_NOMASK(this->actor.params, 8);

    if (type <= ENVIEWER_TYPE_2_ELARA) { // elara's horse, ilya and elara
        if (play->csCtx.state != CS_STATE_IDLE && play->csCtx.actorCues[0] != NULL &&
            play->csCtx.curFrame < play->csCtx.actorCues[0]->endFrame) {
            if (type == ENVIEWER_TYPE_0_HORSE_ELARA) {
                if (!sHorseSfxPlayed) {
                    sHorseSfxPlayed = true;
                    SFX_PLAY_AT_POS(&this->actor.projectedPos, NA_SE_EV_HORSE_NEIGH);
                }
                Actor_PlaySfx(&this->actor, NA_SE_EV_HORSE_RUN_LEVEL - SFX_FLAG);
            }

            startPos.x = play->csCtx.actorCues[0]->startPos.x;
            startPos.y = play->csCtx.actorCues[0]->startPos.y;
            startPos.z = play->csCtx.actorCues[0]->startPos.z;
            endPos.x = play->csCtx.actorCues[0]->endPos.x;
            endPos.y = play->csCtx.actorCues[0]->endPos.y;
            endPos.z = play->csCtx.actorCues[0]->endPos.z;
            lerpFactor = Environment_LerpWeight(play->csCtx.actorCues[0]->endFrame,
                                                play->csCtx.actorCues[0]->startFrame, play->csCtx.curFrame);
            this->actor.world.pos.x = (endPos.x - startPos.x) * lerpFactor + startPos.x;
            this->actor.world.pos.y = (endPos.y - startPos.y) * lerpFactor + startPos.y;
            this->actor.world.pos.z = (endPos.z - startPos.z) * lerpFactor + startPos.z;
        }
    } else { // varkonos and varkonos's horse
        if (play->csCtx.state != CS_STATE_IDLE && play->csCtx.actorCues[1] != NULL &&
            play->csCtx.curFrame < play->csCtx.actorCues[1]->endFrame) {
            startPos.x = play->csCtx.actorCues[1]->startPos.x;
            startPos.y = play->csCtx.actorCues[1]->startPos.y;
            startPos.z = play->csCtx.actorCues[1]->startPos.z;
            endPos.x = play->csCtx.actorCues[1]->endPos.x;
            endPos.y = play->csCtx.actorCues[1]->endPos.y;
            endPos.z = play->csCtx.actorCues[1]->endPos.z;
            lerpFactor = Environment_LerpWeight(play->csCtx.actorCues[1]->endFrame,
                                                play->csCtx.actorCues[1]->startFrame, play->csCtx.curFrame);
            this->actor.world.pos.x = (endPos.x - startPos.x) * lerpFactor + startPos.x;
            this->actor.world.pos.y = (endPos.y - startPos.y) * lerpFactor + startPos.y;
            this->actor.world.pos.z = (endPos.z - startPos.z) * lerpFactor + startPos.z;

            if (play->csCtx.actorCues[1]->id == 12) {
                s16 yaw = Math_Vec3f_Yaw(&startPos, &endPos);

                Math_SmoothStepToS(&this->actor.world.rot.y, yaw, 0xA, 0x3E8, 1);
                Math_SmoothStepToS(&this->actor.shape.rot.y, yaw, 0xA, 0x3E8, 1);
            }

            if (type == ENVIEWER_TYPE_9_VARKONOS) {
                this->actor.world.rot.x = play->csCtx.actorCues[1]->rot.x;
                this->actor.world.rot.y = play->csCtx.actorCues[1]->rot.y;
                this->actor.world.rot.z = play->csCtx.actorCues[1]->rot.z;
                this->actor.shape.rot.x = play->csCtx.actorCues[1]->rot.x;
                this->actor.shape.rot.y = play->csCtx.actorCues[1]->rot.y;
                this->actor.shape.rot.z = play->csCtx.actorCues[1]->rot.z;
            }
        }
        if (type == ENVIEWER_TYPE_5_VARKONOS) {
            SFX_PLAY_CENTERED(NA_SE_EV_BURNING - SFX_FLAG);
            EnViewer_DrawFireEffects(this, play);
        }
    }
}

void EnViewer_InitFireEffect(EnViewer* this, PlayState* play, s16 i) {
    EnViewerFireEffect* eff;

    if ((i % 2) == 0) {
        eff = &this->fireEffects[i];
        eff->startPos.x = 100.0f;
        eff->startPos.y = -420.0f;
        eff->startPos.z = 400.0f;
        eff->endPos.x = 100.0f;
        eff->endPos.y = -420.0f;
        eff->endPos.z = -400.0f;
        eff->scale = (Rand_ZeroOne() * 5.0f + 12.0f) * 0.001f;
    } else {
        eff = &this->fireEffects[i];
        eff->startPos.x = -100.0f;
        eff->startPos.y = -420.0f;
        eff->startPos.z = 400.0f;
        eff->endPos.x = -100.0f;
        eff->endPos.y = -420.0f;
        eff->endPos.z = -400.0f;
        eff->scale = (Rand_ZeroOne() * 5.0f + 12.0f) * 0.001f;
    }
    if (this) {}
}

void EnViewer_DrawFireEffects(EnViewer* this2, PlayState* play) {
    EnViewer* this = this2;
    s16 i;

    for (i = 0; i < ARRAY_COUNT(this->fireEffects); i++) {
        switch (this->fireEffects[i].state) {
            case 0:
                EnViewer_InitFireEffect(this, play, i);
                this->fireEffects[i].lerpFactor = (i >> 1) * 0.1f;
                this->fireEffects[i].lerpFactorSpeed = 0.01f;
                this->fireEffects[i].state++;
                break;
            case 1:
                Math_SmoothStepToF(&this->fireEffects[i].lerpFactor, 1.0f, 1.0f, this->fireEffects[i].lerpFactorSpeed,
                                   this->fireEffects[i].lerpFactorSpeed);
                this->fireEffects[i].pos.x =
                    this->fireEffects[i].startPos.x +
                    (this->fireEffects[i].endPos.x - this->fireEffects[i].startPos.x) * this->fireEffects[i].lerpFactor;
                this->fireEffects[i].pos.y =
                    this->fireEffects[i].startPos.y +
                    (this->fireEffects[i].endPos.y - this->fireEffects[i].startPos.y) * this->fireEffects[i].lerpFactor;
                this->fireEffects[i].pos.z =
                    this->fireEffects[i].startPos.z +
                    (this->fireEffects[i].endPos.z - this->fireEffects[i].startPos.z) * this->fireEffects[i].lerpFactor;
                if (this->fireEffects[i].lerpFactor >= 1.0f) {
                    this->fireEffects[i].state++;
                }
                break;
            case 2:
                EnViewer_InitFireEffect(this, play, i);
                this->fireEffects[i].lerpFactor = 0.0f;
                this->fireEffects[i].lerpFactorSpeed = 0.01f;
                this->fireEffects[i].state--;
                break;
        }

        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        Matrix_Translate(this->fireEffects[i].pos.x, this->fireEffects[i].pos.y, this->fireEffects[i].pos.z,
                         MTXMODE_NEW);
        Matrix_Scale(this->fireEffects[i].scale, this->fireEffects[i].scale, this->fireEffects[i].scale, MTXMODE_APPLY);
        GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0, 0, 32, 64, 1, 0, (10 * i - 20 * play->state.frames) % 512, 32, 128));
        GFX_TO(XLU, Gfx3D_SetPrimColor(255, 255, 170, 255));
        GFX_TO(XLU, Gfx3D_SetEnvColor(255, 50, 00, 255));
        GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
        Matrix_Push(); /* (native) multiply by the billboard matrix (segment 1) */ Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY); GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent())); Matrix_Pop();
        GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(gEffFire1DL)));
    }
}

void EnViewer_UpdateVarkonosCape(PlayState* play, EnViewer* this) {
    static s16 yOscillationPhase = 0;
    Vec3f forearmModelOffset;
    Vec3f forearmWorldOffset;

    if (PARAMS_GET_NOMASK(this->actor.params, 8) != ENVIEWER_TYPE_5_VARKONOS) {
        return;
    }

    sVarkonosCape->backPush = BREG(54) / 10.0f;
    sVarkonosCape->backSwayMagnitude = (BREG(60) + 25) / 100.0f;
    sVarkonosCape->sideSwayMagnitude = (BREG(55) - 45) / 10.0f;
    sVarkonosCape->minY = -10000.0f;
    sVarkonosCape->minDist = 0.0f;
    sVarkonosCape->gravity = (BREG(67) - 10) / 10.0f;
    forearmModelOffset.x = KREG(16) - 13.0f;
    forearmModelOffset.y = KREG(17) + 3.0f + Math_SinS(yOscillationPhase) * KREG(20);
    forearmModelOffset.z = KREG(18) - 10.0f;
    yOscillationPhase += KREG(19) * 0x1000 + 0x2000;

    Matrix_RotateY(BINANG_TO_RAD_ALT(this->actor.shape.rot.y), MTXMODE_NEW);
    Matrix_MultVec3f(&forearmModelOffset, &forearmWorldOffset);
    sVarkonosCape->rightForearmPos.x = sVarkonosNeckWorldPos.x + forearmWorldOffset.x;
    sVarkonosCape->rightForearmPos.y = sVarkonosNeckWorldPos.y + forearmWorldOffset.y;
    sVarkonosCape->rightForearmPos.z = sVarkonosNeckWorldPos.z + forearmWorldOffset.z;
    forearmModelOffset.x = -(KREG(16) - 13.0f);
    Matrix_MultVec3f(&forearmModelOffset, &forearmWorldOffset);
    sVarkonosCape->leftForearmPos.x = sVarkonosNeckWorldPos.x + forearmWorldOffset.x;
    sVarkonosCape->leftForearmPos.y = sVarkonosNeckWorldPos.y + forearmWorldOffset.y;
    sVarkonosCape->leftForearmPos.z = sVarkonosNeckWorldPos.z + forearmWorldOffset.z;
}
