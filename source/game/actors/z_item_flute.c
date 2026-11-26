// adapted from flute-of-age/src/overlays/actors/ovl_Item_Flute/z_item_flute.c
/*
 * File: z_item_flute.c
 * Overlay: ovl_Item_Flute
 * Description: Flute of Age
 */

#include "src/overlays/actors/ovl_Item_Flute/z_item_flute.h"
#include "engine/asset.h"

#include "attributes.h"
#include "libu64/debug.h"
#include "segmented_address.h"
#include "sfx.h"
#include "draw.h"
#include "effect.h"
#include "play_state.h"
#include "save.h"

#include "assets/scenes/overworld/spot00/spot00_scene.h"

#define FLAGS ACTOR_FLAG_UPDATE_CULLING_DISABLED

void ItemFlute_Init(Actor* thisx, PlayState* play);
void ItemFlute_Destroy(Actor* thisx, PlayState* play);
void ItemFlute_Update(Actor* thisx, PlayState* play);
void ItemFlute_Draw(Actor* thisx, PlayState* play);

void ItemFlute_GetThrown(ItemFlute* this, PlayState* play);
void ItemFlute_Fly(ItemFlute* this, PlayState* play);
void ItemFlute_WaitInWater(ItemFlute* this, PlayState* play);
void ItemFlute_StartSoTCutscene(ItemFlute* this, PlayState* play);
void func_80B864EC(ItemFlute* this, PlayState* play);
void func_80B865E0(ItemFlute* this, PlayState* play);
void ItemFlute_DoNothing(ItemFlute* this, PlayState* play);

ActorProfile Item_Flute_Profile = {
    /**/ ACTOR_ITEM_FLUTE,
    /**/ ACTORCAT_ITEMACTION,
    /**/ FLAGS,
    /**/ OBJECT_GI_FLUTE,
    /**/ sizeof(ItemFlute),
    /**/ ItemFlute_Init,
    /**/ ItemFlute_Destroy,
    /**/ ItemFlute_Update,
    /**/ ItemFlute_Draw,
};

void ItemFlute_SetupAction(ItemFlute* this, ItemFluteActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void ItemFlute_Init(Actor* thisx, PlayState* play) {
    ItemFlute* this = (ItemFlute*)thisx;
    s32 params = thisx->params;

    ActorShape_Init(&this->actor.shape, 0, NULL, 0);
    Actor_SetScale(&this->actor, 0.1f);

    switch (params) {
        case 0:
            ItemFlute_SetupAction(this, ItemFlute_GetThrown);
            break;
        case 1:
            ItemFlute_SetupAction(this, func_80B865E0);
            break;
        case 2:
            ItemFlute_SetupAction(this, ItemFlute_DoNothing);
            break;
        case 3:
            ItemFlute_SetupAction(this, ItemFlute_WaitInWater);
            if (!GET_EVENTCHKINF(EVENTCHKINF_ELARA_FLED_CASTLE) || GET_EVENTCHKINF(EVENTCHKINF_43)) {
                Actor_Kill(thisx);
                return;
            }
            Actor_Spawn(&play->actorCtx, play, ACTOR_ELF_MSG2, 299.0f, -140.0f, 884.0f, 0, 4, 1, 0x3800);
            Actor_SetScale(thisx, 0.2f);
            break;
        default:
            Actor_Kill(thisx);
            return;
    }

    LOG_NUM("no", params, "../z_item_flute.c", 210);
    this->spinRotOffset = 0x400;
}

void ItemFlute_Destroy(Actor* thisx, PlayState* play) {
}

void ItemFlute_Fly(ItemFlute* this, PlayState* play) {
    Vec3f ripplePos;

    Actor_UpdatePos(&this->actor);
    this->actor.shape.rot.x += this->spinRotOffset * 2;
    this->actor.shape.rot.y += this->spinRotOffset * 3;

    if (this->actor.minVelocityY < this->actor.velocity.y) {
        this->actor.velocity.y += this->actor.gravity;
        if (this->actor.velocity.y < this->actor.minVelocityY) {
            this->actor.velocity.y = this->actor.minVelocityY;
        }
    }

    if (play->csCtx.curFrame == 881) {
        this->actor.world.pos.x = 250.0f;
        this->actor.world.pos.y = 60.0f;
        this->actor.world.pos.z = 1075.0f;
        this->actor.velocity.x = 1.0f;
        this->actor.velocity.y = -5.0f;
        this->actor.velocity.z = -7.0f;
    }

    if (play->csCtx.curFrame == 897) {
        EffectSsGRipple_Spawn(play, &this->actor.world.pos, 100, 500, 0);
        EffectSsGSplash_Spawn(play, &this->actor.world.pos, NULL, NULL, 1, 0);
        this->actor.velocity.x = 0.0f;
        this->actor.velocity.y = 0.0f;
        this->actor.velocity.z = 0.0f;
        this->actor.gravity = -0.1f;
        this->actor.minVelocityY = -0.5f;
        this->spinRotOffset = 0;
        Actor_PlaySfx(&this->actor, NA_SE_EV_BOMB_DROP_WATER);
    }

    // landed in water
    if (play->csCtx.curFrame == 906) {
        ripplePos.x = 274.0f;
        ripplePos.y = -60.0f;
        ripplePos.z = 907.0f;
        EffectSsGRipple_Spawn(play, &ripplePos, 100, 500, 0);
    }
}

void ItemFlute_GetThrown(ItemFlute* this, PlayState* play) {
    this->actor.gravity = -0.3f;
    this->actor.minVelocityY = -5.0f;
    this->actor.velocity.x = 0.0f;
    this->actor.velocity.y = 6.0f;
    this->actor.velocity.z = 0.0f;
    ItemFlute_SetupAction(this, ItemFlute_Fly);
}

void func_80B864EC(ItemFlute* this, PlayState* play) {
    Actor_UpdatePos(&this->actor);
    this->actor.shape.rot.x += this->spinRotOffset * 2;
    this->actor.shape.rot.y += this->spinRotOffset * 3;

    if (this->actor.minVelocityY < this->actor.velocity.y) {
        this->actor.velocity.y += this->actor.gravity;

        if (this->actor.velocity.y < this->actor.minVelocityY) {
            this->actor.velocity.y = this->actor.minVelocityY;
        }
    }

    if (play->csCtx.curFrame == 220) {
        this->actor.world.pos.x = 144.0f;
        this->actor.world.pos.y = 80.0f;
        this->actor.world.pos.z = 1686.0f;
        this->actor.velocity.x = 1.0f;
        this->actor.velocity.y = 2.0f;
        this->actor.velocity.z = -7.0f;
        this->actor.gravity = -0.15f;
        this->actor.minVelocityY = -5.0f;
    }
}

void func_80B865E0(ItemFlute* this, PlayState* play) {
    this->actor.gravity = -0.3f;
    this->actor.minVelocityY = -5.0f;
    this->actor.velocity.x = 0.0f;
    this->actor.velocity.y = 4.0f;
    this->actor.velocity.z = 6.0f;
    ItemFlute_SetupAction(this, func_80B864EC);
}

void ItemFlute_DoNothing(ItemFlute* this, PlayState* play) {
}

void ItemFlute_StartSoTCutscene(ItemFlute* this, PlayState* play) {
    if (Actor_TextboxIsClosing(&this->actor, play)) {
        play->csCtx.script = Asset_Resolve(gEldoraFieldElaraSongOfAgesCs);
        gSaveContext.cutsceneTrigger = 1;
    }
}

void ItemFlute_WaitInWater(ItemFlute* this, PlayState* play) {
    if (Actor_HasParent(&this->actor, play)) {
        SET_EVENTCHKINF(EVENTCHKINF_43);
        Flags_SetSwitch(play, 3);
        this->actionFunc = ItemFlute_StartSoTCutscene;
        this->actor.draw = NULL;
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_FLUTE_OF_AGE, 30.0f, 50.0f);

        if ((play->gameplayFrames & 13) == 0) {
            EffectSsWisp_Spawn(play, &this->actor.world.pos, 0.0f, 0.0f, 10.0f, 0.13f);
        }
    }
}

void ItemFlute_Update(Actor* thisx, PlayState* play) {
    ItemFlute* this = (ItemFlute*)thisx;

    this->actionFunc(this, play);
}

void ItemFlute_Draw(Actor* thisx, PlayState* play) {
    UNUSED ItemFlute* this = (ItemFlute*)thisx;

    func_8002EBCC(thisx, play, 0);
    func_8002ED80(thisx, play, 0);
    GetItem_Draw(play, GID_FLUTE_OF_AGE);
}
