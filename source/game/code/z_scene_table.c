// adapted from flute-of-age/src/code/z_scene_table.c
#include "exact_math.h"
#include "libc64/qrand.h"
#include "engine/asset.h"
#include "engine/gfx3d.h"
#include "gfx.h"
#if PLATFORM_N64
#endif
#include "quake.h"
#include "regs.h"
#include "segment_symbols.h"
#include "segmented_address.h"
#include "sfx.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "versions.h"
#include "z_lib.h"
#include "frame_advance.h"
#include "play_state.h"
#include "player.h"
#include "save.h"

#include "assets/scenes/indoors/miharigoya/miharigoya_scene.h"
#include "assets/scenes/indoors/souko/souko_scene.h"

#include "assets/scenes/overworld/spot00/spot00_scene.h"
#include "assets/scenes/overworld/spot00/spot00_room_0.h"
#include "assets/scenes/overworld/spot01/spot01_scene.h"
#include "assets/scenes/overworld/spot07/spot07_scene.h"
#include "assets/scenes/overworld/spot12/spot12_scene.h"
#include "assets/scenes/overworld/spot16/spot16_scene.h"
#include "assets/scenes/overworld/spot16/spot16_room_0.h"
#include "assets/scenes/overworld/spot18/spot18_scene.h"
#include "assets/scenes/overworld/spot20/spot20_scene.h"

#include "assets/scenes/dungeons/Bmori1/Bmori1_scene.h"
#include "assets/scenes/dungeons/MIZUsin/MIZUsin_scene.h"
#include "assets/scenes/dungeons/ddan/ddan_scene.h"
#include "assets/scenes/dungeons/dunepass/dunepass_scene.h"
#include "assets/scenes/dungeons/ice_doukutu/ice_doukutu_scene.h"
#include "assets/scenes/dungeons/jyasinzou/jyasinzou_scene.h"
#include "assets/scenes/dungeons/men/men_scene.h"
#include "assets/scenes/dungeons/ydan/ydan_scene.h"

#include "src/overlays/actors/ovl_Bg_Dodoago/z_bg_dodoago.h"

void Scene_DrawConfigDefault(PlayState* play);
void Scene_DrawConfigEldoraField(PlayState* play);
void Scene_DrawConfigOakridgeVillage(PlayState* play);
void Scene_DrawConfigNerisRiver(PlayState* play);
void Scene_DrawConfigSylvanForest(PlayState* play);
void Scene_DrawConfigLakeElden(PlayState* play);
void Scene_DrawConfigNerisDomain(PlayState* play);
void Scene_DrawConfigNerisFountain(PlayState* play);
void Scene_DrawConfigDuneanValley(PlayState* play);
void Scene_DrawConfigWanderingWoods(PlayState* play);
void Scene_DrawConfigDesertSentinel(PlayState* play);
void Scene_DrawConfigDuneansFortress(PlayState* play);
void Scene_DrawConfigWhisperingSands(PlayState* play);
void Scene_DrawConfigEldoraCastle(PlayState* play);
void Scene_DrawConfigCinderMountainTrail(PlayState* play);
void Scene_DrawConfigCinderMountainCrater(PlayState* play);
void Scene_DrawConfigCairnCity(PlayState* play);
void Scene_DrawConfigSunmeadRanch(PlayState* play);
void Scene_DrawConfigFireTemple(PlayState* play);
void Scene_DrawConfigRootTree(PlayState* play);
void Scene_DrawConfigCragjawsCavern(PlayState* play);
void Scene_DrawConfigTideMaw(PlayState* play);
void Scene_DrawConfigForestTemple(PlayState* play);
void Scene_DrawConfigWaterTemple(PlayState* play);
void Scene_DrawConfigShadowTempleAndWell(PlayState* play);
void Scene_DrawConfigSpiritTemple(PlayState* play);
void Scene_DrawConfigInsideVarkonsCastle(PlayState* play);
void Scene_DrawConfigDuneanTrainingGround(PlayState* play);
void Scene_DrawConfigRootTreeBoss(PlayState* play);
void Scene_DrawConfigWaterTempleBoss(PlayState* play);
void Scene_DrawConfigHallOfAges(PlayState* play);
void Scene_DrawConfigGrottos(PlayState* play);
void Scene_DrawConfigChamberOfElders(PlayState* play);
void Scene_DrawConfigGreatFairyFountain(PlayState* play);
void Scene_DrawConfigShootingGallery(PlayState* play);
void Scene_DrawConfigCastleCourtyardGuards(PlayState* play);
void Scene_DrawConfigOutsideVarkonsCastle(PlayState* play);
void Scene_DrawConfigIceCavern(PlayState* play);
void Scene_DrawConfigVarkonsTowerCollapseExterior(PlayState* play);
void Scene_DrawConfigFairysFountain(PlayState* play);
void Scene_DrawConfigThievesHideout(PlayState* play);
void Scene_DrawConfigBombmouseBowlingAlley(PlayState* play);
void Scene_DrawConfigRoyalFamilysTomb(PlayState* play);
void Scene_DrawConfigLakesideLaboratory(PlayState* play);
void Scene_DrawConfigSunmeadBuildings(PlayState* play);
void Scene_DrawConfigMarketGuardHouse(PlayState* play);
void Scene_DrawConfigPotionShopGranny(PlayState* play);
void Scene_DrawConfigCalmWater(PlayState* play);
void Scene_DrawConfigGraveExitLightShining(PlayState* play);
void Scene_DrawConfigBesitu(PlayState* play);
void Scene_DrawConfigFishingPond(PlayState* play);
void Scene_DrawConfigVarkonsTowerCollapseInterior(PlayState* play);
void Scene_DrawConfigInsideVarkonsCastleCollapse(PlayState* play);

// Entrance Table definition
#define DEFINE_ENTRANCE(_0, sceneId, spawn, continueBgm, displayTitleCard, endTransType, startTransType) \
    { sceneId, spawn,                                                                                    \
      (((continueBgm) ? ENTRANCE_INFO_CONTINUE_BGM_FLAG : 0) |                                           \
       ((displayTitleCard) ? ENTRANCE_INFO_DISPLAY_TITLE_CARD_FLAG : 0) |                                \
       (((endTransType) << ENTRANCE_INFO_END_TRANS_TYPE_SHIFT) & ENTRANCE_INFO_END_TRANS_TYPE_MASK) |    \
       (((startTransType) << ENTRANCE_INFO_START_TRANS_TYPE_SHIFT) & ENTRANCE_INFO_START_TRANS_TYPE_MASK)) },

EntranceInfo gEntranceTable[] = {
#include "tables/entrance_table.h"
};

#undef DEFINE_ENTRANCE

// Linker symbol declarations (used in the table below)
#define DEFINE_SCENE(name, title, _2, _3, _4, _5) \
    DECLARE_ROM_SEGMENT(name)                     \
    DECLARE_ROM_SEGMENT(title)

#include "tables/scene_table.h"

#undef DEFINE_SCENE

// Scene Table definition
#define DEFINE_SCENE(name, title, _2, drawConfig, unk_10, unk_12) \
    { ROM_FILE(name), ROM_FILE(title), unk_10, drawConfig, unk_12, 0 },

// Handle `none` as a special case for scenes without a title card
#define _noneSegmentRomStart NULL
#define _noneSegmentRomEnd NULL

SceneTableEntry gSceneTable[] = {
#include "tables/scene_table.h"
};

#undef _noneSegmentRomStart
#undef _noneSegmentRomEnd

#undef DEFINE_SCENE

// native: was the static list sDefaultDisplayList (segments 08-0D -> gEmptyDL, prim/env 128).
static void Scene_DefaultSegments(void) {
    for (int seg = 0x08; seg <= 0x0D; seg++) {
        Gfx3D_SegDLBegin(seg); // empty list == gEmptyDL
        Gfx3D_SegDLEnd();
    }
    Gfx3D_SetPrimColor(128, 128, 128, 128);
    Gfx3D_SetEnvColor(128, 128, 128, 128);
}

#if PLATFORM_N64 // Scene_Draw is at end of file in GC/iQue versions

SceneDrawConfigFunc sSceneDrawConfigs[SDC_MAX] = {
    Scene_DrawConfigDefault,                     // SDC_DEFAULT
    Scene_DrawConfigEldoraField,                 // SDC_ELDORA_FIELD
    Scene_DrawConfigOakridgeVillage,             // SDC_OAKRIDGE_VILLAGE
    Scene_DrawConfigNerisRiver,                  // SDC_NERIS_RIVER
    Scene_DrawConfigSylvanForest,                // SDC_SYLVAN_FOREST
    Scene_DrawConfigLakeElden,                   // SDC_LAKE_ELDEN
    Scene_DrawConfigNerisDomain,                 // SDC_NERIS_DOMAIN
    Scene_DrawConfigNerisFountain,               // SDC_NERIS_FOUNTAIN
    Scene_DrawConfigDuneanValley,                // SDC_DUNEAN_VALLEY
    Scene_DrawConfigWanderingWoods,                   // SDC_WANDERING_WOODS
    Scene_DrawConfigDesertSentinel,              // SDC_DESERT_SENTINEL
    Scene_DrawConfigDuneansFortress,             // SDC_DUNEANS_FORTRESS
    Scene_DrawConfigWhisperingSands,            // SDC_WHISPERING_SANDS
    Scene_DrawConfigEldoraCastle,                // SDC_ELDORA_CASTLE
    Scene_DrawConfigCinderMountainTrail,          // SDC_CINDER_MOUNTAIN_TRAIL
    Scene_DrawConfigCinderMountainCrater,         // SDC_CINDER_MOUNTAIN_CRATER
    Scene_DrawConfigCairnCity,                   // SDC_CAIRN_CITY
    Scene_DrawConfigSunmeadRanch,                 // SDC_SUNMEAD_RANCH
    Scene_DrawConfigFireTemple,                  // SDC_FIRE_TEMPLE
    Scene_DrawConfigRootTree,                    // SDC_ROOT_TREE
    Scene_DrawConfigCragjawsCavern,              // SDC_CRAGJAWS_CAVERN
    Scene_DrawConfigTideMaw,                    // SDC_TIDE_MAW
    Scene_DrawConfigForestTemple,                // SDC_FOREST_TEMPLE
    Scene_DrawConfigWaterTemple,                 // SDC_WATER_TEMPLE
    Scene_DrawConfigShadowTempleAndWell,         // SDC_SHADOW_TEMPLE_AND_WELL
    Scene_DrawConfigSpiritTemple,                // SDC_SPIRIT_TEMPLE
    Scene_DrawConfigInsideVarkonsCastle,          // SDC_INSIDE_VARKONS_CASTLE
    Scene_DrawConfigDuneanTrainingGround,        // SDC_DUNEAN_TRAINING_GROUND
    Scene_DrawConfigRootTreeBoss,                // SDC_ROOT_TREE_BOSS
    Scene_DrawConfigWaterTempleBoss,             // SDC_WATER_TEMPLE_BOSS
    Scene_DrawConfigHallOfAges,                // SDC_HALL_OF_AGES
    Scene_DrawConfigGrottos,                     // SDC_GROTTOS
    Scene_DrawConfigChamberOfElders,           // SDC_CHAMBER_OF_ELDERS
    Scene_DrawConfigGreatFairyFountain,          // SDC_GREAT_FAIRYS_FOUNTAIN
    Scene_DrawConfigShootingGallery,             // SDC_SHOOTING_GALLERY
    Scene_DrawConfigCastleCourtyardGuards,       // SDC_CASTLE_COURTYARD_GUARDS
    Scene_DrawConfigOutsideVarkonsCastle,         // SDC_OUTSIDE_VARKONS_CASTLE
    Scene_DrawConfigIceCavern,                   // SDC_ICE_CAVERN
    Scene_DrawConfigVarkonsTowerCollapseExterior, // SDC_VARKONS_TOWER_COLLAPSE_EXTERIOR
    Scene_DrawConfigFairysFountain,              // SDC_FAIRYS_FOUNTAIN
    Scene_DrawConfigThievesHideout,              // SDC_THIEVES_HIDEOUT
    Scene_DrawConfigBombmouseBowlingAlley,         // SDC_BOMBMOUSE_BOWLING_ALLEY
    Scene_DrawConfigRoyalFamilysTomb,            // SDC_ROYAL_FAMILYS_TOMB
    Scene_DrawConfigLakesideLaboratory,          // SDC_LAKESIDE_LABORATORY
    Scene_DrawConfigSunmeadBuildings,             // SDC_SUNMEAD_BUILDINGS
    Scene_DrawConfigMarketGuardHouse,            // SDC_MARKET_GUARD_HOUSE
    Scene_DrawConfigPotionShopGranny,            // SDC_POTION_SHOP_GRANNY
    Scene_DrawConfigCalmWater,                   // SDC_CALM_WATER
    Scene_DrawConfigGraveExitLightShining,       // SDC_GRAVE_EXIT_LIGHT_SHINING
    Scene_DrawConfigBesitu,                      // SDC_BESITU
    Scene_DrawConfigFishingPond,                 // SDC_FISHING_POND
    Scene_DrawConfigVarkonsTowerCollapseInterior, // SDC_VARKONS_TOWER_COLLAPSE_INTERIOR
    Scene_DrawConfigInsideVarkonsCastleCollapse,  // SDC_INSIDE_VARKONS_CASTLE_COLLAPSE
};

void Scene_Draw(PlayState* play) {
    // native: 64DD scene-draw hook removed (no disk drive)
    sSceneDrawConfigs[play->sceneDrawConfig](play);
}

#endif

void Scene_DrawConfigDefault(PlayState* play) {

    Scene_DefaultSegments();
    Scene_DefaultSegments();

}

void* sRootTreeEntranceTextures[] = {
    gRootTreeDayEntranceTex,
    gRootTreeNightEntranceTex,
};

void Scene_DrawConfigRootTree(PlayState* play) {
    u32 gameplayFrames = play->gameplayFrames;


    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - (gameplayFrames % 128), (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sRootTreeEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));

}

void Scene_DrawConfigRootTreeBoss(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, (gameplayFrames * 2) % 256, 0, 64, 32, 1, 0, (gameplayFrames * 2) % 128, 64, 32));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* gDCEntranceTextures[] = {
    gDCDayEntranceTex,
    gDCNightEntranceTex,
};
void* sDCLavaFloorTextures[] = {
    gDCLavaFloor1Tex, gDCLavaFloor2Tex, gDCLavaFloor3Tex, gDCLavaFloor4Tex,
    gDCLavaFloor5Tex, gDCLavaFloor6Tex, gDCLavaFloor7Tex, gDCLavaFloor8Tex,
};

void Scene_DrawConfigCragjawsCavern(PlayState* play) {
    u32 gameplayFrames;
    STACK_PAD(s32);


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(gDCEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));
    GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(sDCLavaFloorTextures[(s32)(gameplayFrames & 14) >> 1]))));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, (gameplayFrames * 1) % 256, 0, 64, 32, 1, 0, (gameplayFrames * 1) % 128, 64, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0A, 0, 0, (gameplayFrames * 1) % 128, 32, 32, 1, 0, (gameplayFrames * 2) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0B));
    GFX_TO(OPA, Gfx3D_SetEnvColor(255, 255, 255, play->roomCtx.drawParams[BGDODOAGO_EYE_LEFT]));
    Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0C));
    GFX_TO(OPA, Gfx3D_SetEnvColor(255, 255, 255, play->roomCtx.drawParams[BGDODOAGO_EYE_RIGHT]));
    Gfx3D_SegDLEnd();

}

void Scene_DrawConfigHallOfAges(PlayState* play) {
    f32 temp;


    temp = play->roomCtx.drawParams[0] / 255.0f;

    GFX_TO(XLU, Gfx3D_SegDLBegin(0x08));
    GFX_TO(OPA, Gfx3D_SegDLBegin(0x08));
    GFX_TO(OPA, Gfx3D_SetPrimColor(255 - (u8)(185.0f * temp), 255 - (u8)(145.0f * temp), 255 - (u8)(105.0f * temp), 255));
    Gfx3D_SegDLEnd();

    GFX_TO(XLU, Gfx3D_SegDLBegin(0x09));
    GFX_TO(OPA, Gfx3D_SegDLBegin(0x09));
    GFX_TO(OPA, Gfx3D_SetPrimColor(76 + (u8)(6.0f * temp), 76 + (u8)(34.0f * temp), 76 + (u8)(74.0f * temp), 255));
    Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0A));
    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0A));
    GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, play->roomCtx.drawParams[0]));
    Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0B));
    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0B));
    GFX_TO(XLU, Gfx3D_SetPrimColor(89 + (u8)(166.0f * temp), 89 + (u8)(166.0f * temp), 89 + (u8)(166.0f * temp), 255));
    GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, play->roomCtx.drawParams[0]));
    Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0C));
    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0C));
    GFX_TO(XLU, Gfx3D_SetPrimColor(255 + (u8)(179.0f * temp), 255 + (u8)(179.0f * temp), 255 + (u8)(179.0f * temp), 255));
    GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, play->roomCtx.drawParams[0]));
    Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0D));
    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0D));
    GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, play->roomCtx.drawParams[1]));
    Gfx3D_SegDLEnd();


    if (gSaveContext.sceneLayer == 5) {
        gCustomLensFlareOn = true;
        gCustomLensFlarePos.x = -20.0f;
        gCustomLensFlarePos.y = 1220.0f;
        gCustomLensFlarePos.z = -684.0f;
        gLensFlareScale = 10;
        gLensFlareColorIntensity = 8.0f;
        gLensFlareGlareStrength = 200;
    }
}

void Scene_DrawConfigGrottos(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll(0x08, 0, 0, (gameplayFrames * 1) % 64, 256, 16));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - (gameplayFrames % 128), (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0A, 0, 0, 0, 32, 32, 1, 0, 127 - (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x0B, 0, 0, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0C, 0, 0, (gameplayFrames * 50) % 2048, 8, 512, 1, 0, (gameplayFrames * 60) % 2048, 8, 512));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0D, 0, 0, 0, 32, 64, 1, 0, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigChamberOfElders(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll(0x08, 0, 0, (gameplayFrames * 2) % 256, 64, 64));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0A, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - (gameplayFrames * 1) % 128, (gameplayFrames * 1) % 256, 32, 64, 1, 0, 0, 32, 128));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigGreatFairyFountain(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 3) % 256, 32, 64, 1, gameplayFrames % 128, (gameplayFrames * 3) % 256, 32, 64));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigGraveExitLightShining(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll(0x08, 0, 0, gameplayFrames % 64, 256, 16));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigFairysFountain(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll(0x09, 0, 0, gameplayFrames % 64, 256, 16));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigShadowTempleAndWell(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;

    if (play->sceneId == SCENE_SHADOW_TEMPLE_BOSS) {
        GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, (gameplayFrames * 2) % 128, 0, 32, 32, 1, (gameplayFrames * 2) % 128, 0, 32, 32));
    } else {
        GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, (gameplayFrames * 2) % 128, 0, 32, 32, 1, (gameplayFrames * 2) % 128, 0, 32, 32));
    }

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sThievesHideoutEntranceTextures[] = {
    gThievesHideoutDayEntranceTex,
    gThievesHideoutNightEntranceTex,
};

void Scene_DrawConfigThievesHideout(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x09, 0, 0, (gameplayFrames * 3) % 128, 32, 32));

    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sThievesHideoutEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));

}

void* sWaterTempleEntranceTextures[] = {
    gWaterTempleDayEntranceTex,
    gWaterTempleNightEntranceTex,
};

void Scene_DrawConfigWaterTemple(PlayState* play) {
    u32 gameplayFrames;
    s32 spB0;
    s32 spAC;


    spB0 = (play->roomCtx.drawParams[1] >> 8) & 0xFF;
    spAC = play->roomCtx.drawParams[1] & 0xFF;
    gameplayFrames = play->gameplayFrames;

#if !OOT_MQ
    GFX_TO(XLU, Gfx3D_SetSegment(0x06, (const void*)(Asset_Resolve(sWaterTempleEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));
#else
    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sWaterTempleEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));
#endif

    if (spB0 == 1) {
        GFX_TO(OPA, Gfx3D_SegDLBegin(0x08)); GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, spAC)); Gfx3D_SegDLEnd();
    } else if (spB0 < 1) {
        GFX_TO(OPA, Gfx3D_SegDLBegin(0x08)); GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, 255)); Gfx3D_SegDLEnd();
    } else {
        GFX_TO(OPA, Gfx3D_SegDLBegin(0x08)); GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, 160)); Gfx3D_SegDLEnd();
    }

    if (spB0 == 2) {
        GFX_TO(OPA, Gfx3D_SegDLBegin(0x09)); GFX_TO(OPA, Gfx3D_SetTexScroll2(0x09, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, spAC)); Gfx3D_SegDLEnd();
    } else if (spB0 < 2) {
        GFX_TO(OPA, Gfx3D_SegDLBegin(0x09)); GFX_TO(OPA, Gfx3D_SetTexScroll2(0x09, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, 255)); Gfx3D_SegDLEnd();
    } else {
        GFX_TO(XLU, Gfx3D_SegDLBegin(0x09)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, 160)); Gfx3D_SegDLEnd();
    }

    if (spB0 != 0) {
        GFX_TO(XLU, Gfx3D_SegDLBegin(0x0A)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, 160)); Gfx3D_SegDLEnd();
        GFX_TO(XLU, Gfx3D_SegDLBegin(0x0B)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0B, 0, gameplayFrames * 3, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, 180)); Gfx3D_SegDLEnd();
    } else {
        GFX_TO(XLU, Gfx3D_SegDLBegin(0x0A)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, (gameplayFrames * 1) % 128, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, 160 + (s32)((spAC / 200.0f) * 95.0f))); Gfx3D_SegDLEnd();
        GFX_TO(XLU, Gfx3D_SegDLBegin(0x0B)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0B, 0, gameplayFrames * 3, 0, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, 185 + (s32)((spAC / 200.0f) * 70.0f))); Gfx3D_SegDLEnd();
    }

    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0C)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0C, 0, gameplayFrames * 1, gameplayFrames * 1, 32, 32, 1, 0, 127 - (gameplayFrames * 1), 32, 32)); GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, 128)); Gfx3D_SegDLEnd();
    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0D)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0D, 0, gameplayFrames * 4, 0, 32, 32, 1, gameplayFrames * 4, 0, 32, 32)); GFX_TO(XLU, Gfx3D_SetEnvColor(0, 0, 0, 128)); Gfx3D_SegDLEnd();

}

void Scene_DrawConfigWaterTempleBoss(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, gameplayFrames * 1, 0, 32, 32, 1, 0, 0, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, play->roomCtx.drawParams[0]));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 145));

}

void Scene_DrawConfigShootingGallery(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x08, 0, 0, gameplayFrames % 64, 4, 16));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigCastleCourtyardGuards(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32));

    if (play->sceneId == SCENE_CASTLE_COURTYARD_GUARDS_DAY) {
        GFX_TO(XLU, Gfx3D_SetTexScroll(0x09, 0, 0, (gameplayFrames * 10) % 256, 32, 64));
    }

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigOutsideVarkonsCastle(PlayState* play) {
    u32 gameplayFrames;
    s8 sp83;

    sp83 = ExactCosS(play->gameplayFrames * 1500) >> 8;


    gameplayFrames = play->gameplayFrames;

    if (play->sceneId == SCENE_OUTSIDE_VARKONS_CASTLE) {
        GFX_TO(XLU, Gfx3D_SetTexScroll(0x09, 0, 0, (gameplayFrames * 1) % 256, 64, 64));
        GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0, 255 - (gameplayFrames * 1) % 256, 64, 64, 1, 0, (gameplayFrames * 1) % 256, 64, 64));
    }

    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0B, 0, 255 - (gameplayFrames * 1) % 128, (gameplayFrames * 1) % 128, 32, 32, 1, (gameplayFrames * 1) % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    sp83 = (sp83 >> 1) + 192;
    GFX_TO(OPA, Gfx3D_SetEnvColor(sp83, sp83, sp83, 128));

}

// Screen Shake for Varkon's Tower Collapse
void func_8009BEEC(PlayState* play) {
    s32 quakeIndex;

    if (play->gameplayFrames % 128 == 13) {
        quakeIndex = Quake_Request(GET_ACTIVE_CAM(play), QUAKE_TYPE_2);
        Quake_SetSpeed(quakeIndex, 10000);
        Quake_SetPerturbations(quakeIndex, 4, 0, 0, 0);
        Quake_SetDuration(quakeIndex, 127);
    }

    if ((play->gameplayFrames % 64 == 0) && (Rand_ZeroOne() > 0.6f)) {
        quakeIndex = Quake_Request(GET_ACTIVE_CAM(play), QUAKE_TYPE_3);
        Quake_SetSpeed(quakeIndex, 32000.0f + (Rand_ZeroOne() * 3000.0f));
        Quake_SetPerturbations(quakeIndex, 10.0f - (Rand_ZeroOne() * 9.0f), 0, 0, 0);
        Quake_SetDuration(quakeIndex, 48.0f - (Rand_ZeroOne() * 15.0f));
    }
}

void Scene_DrawConfigVarkonsTowerCollapseExterior(PlayState* play) {
    u32 gameplayFrames;
    s8 sp7B;

    sp7B = ExactCosS((play->gameplayFrames * 1500) & 0xFFFF) >> 8;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0, (gameplayFrames * 1) % 512, 64, 128, 1, 0, 511 - (gameplayFrames * 1) % 512, 64, 128));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x09, 0, 0, (gameplayFrames * 1) % 256, 32, 64, 1, 0, 255 - (gameplayFrames * 1) % 256, 32, 64));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, 0, (gameplayFrames * 20) % 2048, 16, 512, 1, 0, (gameplayFrames * 30) % 2048, 16, 512));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    sp7B = (sp7B >> 1) + 192;
    GFX_TO(OPA, Gfx3D_SetEnvColor(sp7B, sp7B, sp7B, 128));


    if (Flags_GetSwitch(play, 0x37)) {
        if ((play->sceneId == SCENE_VARKON_BOSS) || (play->sceneId == SCENE_VARKONS_TOWER_COLLAPSE_EXTERIOR) ||
            (play->sceneId == SCENE_VARKONS_TOWER_COLLAPSE_INTERIOR) ||
            (play->sceneId == SCENE_INSIDE_VARKONS_CASTLE_COLLAPSE)) {
            func_8009BEEC(play);
        }
    }
}

void* sIceCavernEntranceTextures[] = {
    gIceCavernDayEntranceTex,
    gIceCavernNightEntranceTex,
};

void Scene_DrawConfigIceCavern(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sIceCavernEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigRoyalFamilysTomb(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll(0x08, 0, 0, (gameplayFrames * 1) % 64, 256, 16));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 0, (gameplayFrames * 60) % 2048, 8, 512, 1, 0, (gameplayFrames * 50) % 2048, 8, 512));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0A, 0, 127 - (gameplayFrames * 1) % 128, 0, 32, 32, 1, (gameplayFrames * 1) % 128, 0, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0B, 0, 0, 1023 - (gameplayFrames * 6) % 1024, 16, 256, 1, 0, 1023 - (gameplayFrames * 3) % 1024, 16, 256));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigLakesideLaboratory(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0, 0, 32, 32, 1, 0, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll(0x09, 0, 0, 255 - (gameplayFrames * 10) % 256, 32, 64));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigCalmWater(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sGTGEntranceTextures[] = {
    gGTGDayEntranceTex,
    gGTGNightEntranceTex,
};

void Scene_DrawConfigDuneanTrainingGround(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sGTGEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

// native: Gfx_TwoTexScrollPrimColor (runtime tile-scroll list builder) translated at call sites
void Scene_DrawConfigFishingPond(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SegDLBegin(0x08)); GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32)); GFX_TO(XLU, Gfx3D_SetPrimColor(255, 255, 255, play->roomCtx.drawParams[0] + 127)); Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigBombmouseBowlingAlley(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll(0x08, 0, 127 - (gameplayFrames * 4) % 128, 0, 32, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x09, 0, 0, (gameplayFrames * 5) % 64, 16, 16));
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x0A, 0, 0, 63 - (gameplayFrames * 2) % 64, 16, 16));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0B, 0, 0, 127 - (gameplayFrames * 3) % 128, 32, 32, 1, 0, 0, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sSunmeadHouseEntranceTextures[] = {
    gSunmeadHouseDayEntranceTex,
    gSunmeadHouseNightEntranceTex,
};

void Scene_DrawConfigSunmeadBuildings(PlayState* play) {

    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sSunmeadHouseEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sGuardHouseView1Textures[] = {
    gGuardHouseOutSideView1DayTex,
    gGuardHouseOutSideView1NightTex,
};
void* sGuardHouseView2Textures[] = {
    gGuardHouseOutSideView2DayTex,
    gGuardHouseOutSideView2NightTex,
};

void Scene_DrawConfigMarketGuardHouse(PlayState* play) {
    s32 var;


    if (AREN_IS_ADULT) {
        var = 1;
    } else {
        var = gSaveContext.save.nightFlag;
    }

    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sGuardHouseView2Textures[var]))));
    GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(sGuardHouseView1Textures[var]))));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigPotionShopGranny(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x08, 0, 0, (gameplayFrames * 3) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 0, 1023 - (gameplayFrames * 3) % 1024, 16, 256, 1, 0, 1023 - (gameplayFrames * 6) % 1024, 16, 256));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sForestTempleEntranceTextures[] = {
    gForestTempleDayEntranceTex,
    gForestTempleNightEntranceTex,
};

void Scene_DrawConfigForestTemple(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sForestTempleEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0A, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sSpiritTempleEntranceTextures[] = {
    gSpiritTempleDayEntranceTex,
    gSpiritTempleNightEntranceTex,
};

void Scene_DrawConfigSpiritTemple(PlayState* play) {

    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sSpiritTempleEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));

}

void Scene_DrawConfigEldoraField(PlayState* play) {
    u32 gameplayFrames;



    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 10) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 10) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0A));

    if ((gSaveContext.save.dayTime > CLOCK_TIME(7, 0)) && (gSaveContext.save.dayTime <= CLOCK_TIME(18, 30))) {
        Gfx3D_SegDLEnd();
    } else {
        if (gSaveContext.save.dayTime > CLOCK_TIME(18, 30)) {
            if (play->roomCtx.drawParams[0] != 255) {
                Math_StepToS(&play->roomCtx.drawParams[0], 255, 5);
            }
        } else if (gSaveContext.save.dayTime >= CLOCK_TIME(6, 0)) {
            if (play->roomCtx.drawParams[0] != 0) {
                Math_StepToS(&play->roomCtx.drawParams[0], 0, 10);
            }
        }

        GFX_TO(XLU, Gfx3D_SetPrimColor(255, 255, 255, play->roomCtx.drawParams[0]));
        GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(spot00_room_0DL_012B20)));
        Gfx3D_SegDLEnd();
    }

}

void* sOakridgeWindowTextures[] = {
    gOakridgeVillageDayWindowTex,
    gOakridgeVillageNightWindowTex,
};

void Scene_DrawConfigOakridgeVillage(PlayState* play) {

    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sOakridgeWindowTextures[((void)0, gSaveContext.save.nightFlag)]))));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigNerisRiver(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 6) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 6) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigSylvanForest(PlayState* play) {
    u32 gameplayFrames;
    u8 spA3;
    u16 spA0;

    spA3 = 128;
    spA0 = 500;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 10) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 10) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    if (gSaveContext.sceneLayer == 4) {
        spA3 = 255 - (u8)play->roomCtx.drawParams[0];
    } else if (gSaveContext.sceneLayer == 6) {
        spA0 = play->roomCtx.drawParams[0] + 500;
    } else if (
#if OOT_VERSION < PAL_1_0
        !IS_CUTSCENE_LAYER && GET_EVENTCHKINF(EVENTCHKINF_07)
#else
        (!IS_CUTSCENE_LAYER || AREN_IS_ADULT) && GET_EVENTCHKINF(EVENTCHKINF_07)
#endif
    ) {
        spA0 = 2150;
    }

    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0A));
    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, spA3));
    Gfx3D_SegDLEnd();

    GFX_TO(XLU, Gfx3D_SegDLBegin(0x0B));
    GFX_TO(OPA, Gfx3D_SegDLBegin(0x0B));
    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, spA0 * 0.1f));
    Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0C, 0, 0, (s16)(-play->roomCtx.drawParams[0] * 0.02f), 32, 16, 1, 0, (s16)(-play->roomCtx.drawParams[0] * 0.02f), 32, 16));

}

void Scene_DrawConfigLakeElden(PlayState* play) {
    u32 gameplayFrames;


    if (IS_CUTSCENE_LAYER || (AREN_IS_ADULT && !GET_EVENTCHKINF(EVENTCHKINF_RESTORED_LAKE_ELDEN))) {
        play->roomCtx.drawParams[0] = 87;
    }

    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SegDLBegin(0x08)); GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, gameplayFrames, gameplayFrames, 32, 32, 1, 0, 0, 32, 32)); GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, play->roomCtx.drawParams[0] + 168)); Gfx3D_SegDLEnd();
    GFX_TO(OPA, Gfx3D_SegDLBegin(0x09)); GFX_TO(OPA, Gfx3D_SetTexScroll2(0x09, 0, -gameplayFrames, -gameplayFrames, 32, 32, 1, 0, 0, 16, 64)); GFX_TO(OPA, Gfx3D_SetEnvColor(0, 0, 0, play->roomCtx.drawParams[0] + 168)); Gfx3D_SegDLEnd();

    GFX_TO(OPA, Gfx3D_SetEnvColor(255, 255, 255, 128));

}

void* sNerisDomainEntranceTextures[] = {
    gNerisDomainDayEntranceTex,
    gNerisDomainNightEntranceTex,
};

void Scene_DrawConfigNerisDomain(PlayState* play) {
    u32 gameplayFrames;
    u32 var;


    gameplayFrames = play->gameplayFrames;
    var = 127 - (gameplayFrames * 1) % 128;
    if (AREN_IS_ADULT) {
        var = 0;
    }
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0C, 0, 0, 0, 64, 32, 1, 0, var, 64, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sNerisDomainEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));

}

void Scene_DrawConfigNerisFountain(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, (gameplayFrames * 1) % 128, 0, 32, 32, 1, 0, 0, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 0, 255 - (gameplayFrames * 2) % 256, 64, 64, 1, 0, 255 - (gameplayFrames * 2) % 256, 64, 64));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, 0, (gameplayFrames * 1) % 128, 32, 32, 1, 0, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigDuneanValley(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0, (gameplayFrames * 3) % 1024, 32, 256, 1, 0, (gameplayFrames * 3) % 1024, 32, 256));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 0, (gameplayFrames * 1) % 256, 64, 64, 1, 0, (gameplayFrames * 1) % 256, 64, 64));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0A, 0, 0, (gameplayFrames * 2) % 128, 32, 32, 1, 0, (gameplayFrames * 2) % 128, 32, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0B, 0, 0, 0, 32, 32, 1, 0, 127 - (gameplayFrames * 3) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0C, 0, 0, (gameplayFrames * 1) % 128, 32, 32, 1, 0, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x0D, 0, 0, (gameplayFrames * 1) % 64, 16, 16, 1, 0, (gameplayFrames * 1) % 64, 16, 16));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigWanderingWoods(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, gameplayFrames % 128, 0, 32, 16, 1, gameplayFrames % 128, 0, 32, 16));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, gameplayFrames % 128, 32, 32, 1, gameplayFrames % 128, gameplayFrames % 128, 32, 32));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    if ((play->roomCtx.drawParams[0] == 0) && (INV_CONTENT(ITEM_AZURE) == ITEM_AZURE)) {
        if (play->roomCtx.drawParams[1] == 50) {
            Player_PlaySfx(GET_PLAYER(play), NA_SE_EV_CHICKEN_CRY_M);
            play->roomCtx.drawParams[0] = 1;
        }
        play->roomCtx.drawParams[1]++;
    }

}

void Scene_DrawConfigDesertSentinel(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0, 0, 32, 32, 1, 0, 127 - gameplayFrames % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sDuneanFortressWallTextures[] = {
    gDuneanFortressNightWallTex,
    gDuneanFortressDayWallTex,
};

void Scene_DrawConfigDuneansFortress(PlayState* play) {

    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sDuneanFortressWallTextures[((void)0, gSaveContext.save.nightFlag)]))));

}

void Scene_DrawConfigWhisperingSands(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0, gameplayFrames % 128, 32, 32, 1, 0, gameplayFrames % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 0, gameplayFrames % 128, 32, 32, 1, 0, gameplayFrames % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigEldoraCastle(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 10) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 10) % 128, 32, 32));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 3) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigCinderMountainTrail(PlayState* play) {


    GFX_TO(XLU, Gfx3D_SegDLBegin(0x08));

    if ((gSaveContext.save.dayTime > CLOCK_TIME(7, 0)) && (gSaveContext.save.dayTime <= CLOCK_TIME(18, 0))) {
        Gfx3D_SegDLEnd();
    } else {
        if (gSaveContext.save.dayTime > CLOCK_TIME(18, 0)) {
            if (play->roomCtx.drawParams[0] != 255) {
                Math_StepToS(&play->roomCtx.drawParams[0], 255, 5);
            }
        } else if (gSaveContext.save.dayTime >= CLOCK_TIME(6, 0)) {
            if (play->roomCtx.drawParams[0] != 0) {
                Math_StepToS(&play->roomCtx.drawParams[0], 0, 10);
            }
        }

        GFX_TO(XLU, Gfx3D_SetPrimColor(255, 255, 255, play->roomCtx.drawParams[0]));
        GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(spot16_room_0DL_00AA48)));
        Gfx3D_SegDLEnd();
    }

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigCinderMountainCrater(PlayState* play) {
    s8 sp6F = ExactCosS((play->gameplayFrames * 1500) & 0xFFFF) >> 8;
    s8 sp6E = ExactCosS((play->gameplayFrames * 1500) & 0xFFFF) >> 8;
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    sp6F = (sp6F >> 1) + 192;
    sp6E = (sp6E >> 1) + 192;

    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0, gameplayFrames % 128, 32, 32, 1, 0, gameplayFrames % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(sp6F, sp6E, 255, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void* sCairnCityEntranceTextures[] = {
    gCairnCityDayEntranceTex,
    gCairnCityNightEntranceTex,
};

void Scene_DrawConfigCairnCity(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0, 127 - gameplayFrames % 128, 32, 32, 1, gameplayFrames % 128, 0, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sCairnCityEntranceTextures[((void)0, gSaveContext.save.nightFlag)]))));

}

void* sSunmeadRanchWindowTextures[] = {
    gSunmeadRanchDayWindowTex,
    gSunmeadRangeNightWindowsTex,
};

void Scene_DrawConfigSunmeadRanch(PlayState* play) {

    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sSunmeadRanchWindowTextures[((void)0, gSaveContext.save.nightFlag)]))));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigFireTemple(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0, 127 - gameplayFrames % 128, 32, 32, 1, 127 - gameplayFrames % 128, 0, 32, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x09, 0, (gameplayFrames * 3) % 128, 127 - (gameplayFrames * 6) % 128, 32, 32, 1, (gameplayFrames * 6) % 128, 127 - (gameplayFrames * 3) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 64));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 64));

}

f32 D_8012A398 = 0.0f;

void Scene_DrawConfigTideMaw(PlayState* play) {
    static s16 D_8012A39C = 538;
    static s16 D_8012A3A0 = 4272;
    u32 gameplayFrames;
    f32 temp;


    gameplayFrames = play->gameplayFrames;
    if (play->sceneId == SCENE_TIDE_MAW) {
        GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, gameplayFrames % 128, (gameplayFrames * 2) % 128, 32, 32, 1, 127 - gameplayFrames % 128, (gameplayFrames * 2) % 128, 32, 32));
        GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0B, 0, 0, 255 - (gameplayFrames * 4) % 256, 32, 64, 1, 0, 255 - (gameplayFrames * 4) % 256, 32, 64));
    } else {
        GFX_TO(OPA, Gfx3D_SetTexScroll(0x08, 0, (127 - (gameplayFrames * 1)) % 128, (gameplayFrames * 1) % 128, 32, 32));
    }

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

    if (FrameAdvance_IsEnabled(play) != true) {

        D_8012A39C += 1820;
        D_8012A3A0 += 1820;

        temp = 0.020000001f;
        View_SetDistortionOrientation(&play->view,
                                      ((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_CosS(D_8012A39C),
                                      ((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_SinS(D_8012A39C),
                                      ((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_SinS(D_8012A3A0));
        View_SetDistortionScale(&play->view, 1.f + (0.79999995f * temp * Math_SinS(D_8012A3A0)),
                                1.f + (0.39999998f * temp * Math_CosS(D_8012A3A0)),
                                1.f + (1 * temp * Math_CosS(D_8012A39C)));
        View_SetDistortionSpeed(&play->view, 0.95f);

        switch (play->roomCtx.drawParams[0]) {
            case 0:
                break;
            case 1:
                if (play->roomCtx.drawParams[1] < 1200) {
                    play->roomCtx.drawParams[1] += 200;
                } else {
                    play->roomCtx.drawParams[0]++;
                }
                break;
            case 2:
                if (play->roomCtx.drawParams[1] > 0) {
                    play->roomCtx.drawParams[1] -= 30;
                } else {
                    play->roomCtx.drawParams[1] = 0;
                    play->roomCtx.drawParams[0] = 0;
                }
                break;
        }

        D_8012A398 += 0.15f + (play->roomCtx.drawParams[1] * 0.001f);
    }

    if (play->roomCtx.curRoom.num == 2) {
        Matrix_Scale(1.0f, ExactSinF(D_8012A398) * 0.8f, 1.0f, MTXMODE_NEW);
    } else {
        Matrix_Scale(1.005f, ExactSinF(D_8012A398) * 0.8f, 1.005f, MTXMODE_NEW);
    }

    GFX_TO(OPA, Gfx3D_SetSegmentMtx(0x0D, Matrix_GetCurrent()));

}

void Scene_DrawConfigInsideVarkonsCastle(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;

    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 512, 32, 128, 1, gameplayFrames % 128, (gameplayFrames * 1) % 512, 32, 128));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x09, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x0A, 0, 127 - gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128, 32, 32));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

void Scene_DrawConfigInsideVarkonsCastleCollapse(PlayState* play) {
    func_8009BEEC(play);
}

void Scene_DrawConfigVarkonsTowerCollapseInterior(PlayState* play) {
    func_8009BEEC(play);
}

void Scene_DrawConfigBesitu(PlayState* play) {
    u32 gameplayFrames;


    gameplayFrames = play->gameplayFrames;
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x08, 0, 127 - (gameplayFrames * 2) % 128, 0, 32, 64));
    GFX_TO(OPA, Gfx3D_SetTexScroll(0x09, 0, 0, (gameplayFrames * 2) % 512, 128, 128));

    GFX_TO(OPA, Gfx3D_SetEnvColor(128, 128, 128, 128));

    GFX_TO(XLU, Gfx3D_SetEnvColor(128, 128, 128, 128));

}

#if !PLATFORM_N64 // Scene_Draw is at beginning of file in N64 versions

SceneDrawConfigFunc sSceneDrawConfigs[SDC_MAX] = {
    Scene_DrawConfigDefault,                     // SDC_DEFAULT
    Scene_DrawConfigEldoraField,                 // SDC_ELDORA_FIELD
    Scene_DrawConfigOakridgeVillage,             // SDC_OAKRIDGE_VILLAGE
    Scene_DrawConfigNerisRiver,                  // SDC_NERIS_RIVER
    Scene_DrawConfigSylvanForest,                // SDC_SYLVAN_FOREST
    Scene_DrawConfigLakeElden,                   // SDC_LAKE_ELDEN
    Scene_DrawConfigNerisDomain,                 // SDC_NERIS_DOMAIN
    Scene_DrawConfigNerisFountain,               // SDC_NERIS_FOUNTAIN
    Scene_DrawConfigDuneanValley,                // SDC_DUNEAN_VALLEY
    Scene_DrawConfigWanderingWoods,                   // SDC_WANDERING_WOODS
    Scene_DrawConfigDesertSentinel,              // SDC_DESERT_SENTINEL
    Scene_DrawConfigDuneansFortress,             // SDC_DUNEANS_FORTRESS
    Scene_DrawConfigWhisperingSands,            // SDC_WHISPERING_SANDS
    Scene_DrawConfigEldoraCastle,                // SDC_ELDORA_CASTLE
    Scene_DrawConfigCinderMountainTrail,          // SDC_CINDER_MOUNTAIN_TRAIL
    Scene_DrawConfigCinderMountainCrater,         // SDC_CINDER_MOUNTAIN_CRATER
    Scene_DrawConfigCairnCity,                   // SDC_CAIRN_CITY
    Scene_DrawConfigSunmeadRanch,                 // SDC_SUNMEAD_RANCH
    Scene_DrawConfigFireTemple,                  // SDC_FIRE_TEMPLE
    Scene_DrawConfigRootTree,                    // SDC_ROOT_TREE
    Scene_DrawConfigCragjawsCavern,              // SDC_CRAGJAWS_CAVERN
    Scene_DrawConfigTideMaw,                    // SDC_TIDE_MAW
    Scene_DrawConfigForestTemple,                // SDC_FOREST_TEMPLE
    Scene_DrawConfigWaterTemple,                 // SDC_WATER_TEMPLE
    Scene_DrawConfigShadowTempleAndWell,         // SDC_SHADOW_TEMPLE_AND_WELL
    Scene_DrawConfigSpiritTemple,                // SDC_SPIRIT_TEMPLE
    Scene_DrawConfigInsideVarkonsCastle,          // SDC_INSIDE_VARKONS_CASTLE
    Scene_DrawConfigDuneanTrainingGround,        // SDC_DUNEAN_TRAINING_GROUND
    Scene_DrawConfigRootTreeBoss,                // SDC_ROOT_TREE_BOSS
    Scene_DrawConfigWaterTempleBoss,             // SDC_WATER_TEMPLE_BOSS
    Scene_DrawConfigHallOfAges,                // SDC_HALL_OF_AGES
    Scene_DrawConfigGrottos,                     // SDC_GROTTOS
    Scene_DrawConfigChamberOfElders,           // SDC_CHAMBER_OF_ELDERS
    Scene_DrawConfigGreatFairyFountain,          // SDC_GREAT_FAIRYS_FOUNTAIN
    Scene_DrawConfigShootingGallery,             // SDC_SHOOTING_GALLERY
    Scene_DrawConfigCastleCourtyardGuards,       // SDC_CASTLE_COURTYARD_GUARDS
    Scene_DrawConfigOutsideVarkonsCastle,         // SDC_OUTSIDE_VARKONS_CASTLE
    Scene_DrawConfigIceCavern,                   // SDC_ICE_CAVERN
    Scene_DrawConfigVarkonsTowerCollapseExterior, // SDC_VARKONS_TOWER_COLLAPSE_EXTERIOR
    Scene_DrawConfigFairysFountain,              // SDC_FAIRYS_FOUNTAIN
    Scene_DrawConfigThievesHideout,              // SDC_THIEVES_HIDEOUT
    Scene_DrawConfigBombmouseBowlingAlley,         // SDC_BOMBMOUSE_BOWLING_ALLEY
    Scene_DrawConfigRoyalFamilysTomb,            // SDC_ROYAL_FAMILYS_TOMB
    Scene_DrawConfigLakesideLaboratory,          // SDC_LAKESIDE_LABORATORY
    Scene_DrawConfigSunmeadBuildings,             // SDC_SUNMEAD_BUILDINGS
    Scene_DrawConfigMarketGuardHouse,            // SDC_MARKET_GUARD_HOUSE
    Scene_DrawConfigPotionShopGranny,            // SDC_POTION_SHOP_GRANNY
    Scene_DrawConfigCalmWater,                   // SDC_CALM_WATER
    Scene_DrawConfigGraveExitLightShining,       // SDC_GRAVE_EXIT_LIGHT_SHINING
    Scene_DrawConfigBesitu,                      // SDC_BESITU
    Scene_DrawConfigFishingPond,                 // SDC_FISHING_POND
    Scene_DrawConfigVarkonsTowerCollapseInterior, // SDC_VARKONS_TOWER_COLLAPSE_INTERIOR
    Scene_DrawConfigInsideVarkonsCastleCollapse,  // SDC_INSIDE_VARKONS_CASTLE_COLLAPSE
};

void Scene_Draw(PlayState* play) {
#if DEBUG_FEATURES
    if (R_HREG_MODE == HREG_MODE_SCENE_CONFIG) {
        if (R_SCENE_CONFIG_INIT != HREG_MODE_SCENE_CONFIG) {
            R_SCENE_CONFIG_INIT = HREG_MODE_SCENE_CONFIG;
            R_SCENE_CONFIG_DRAW_DEFAULT_DLIST = 1;
            R_SCENE_CONFIG_DRAW_SCENE_CONFIG = 1;

            // these regs are not used in this mode
            HREG(83) = 0;
            HREG(84) = 0;
            HREG(85) = 0;
            HREG(86) = 0;
            HREG(87) = 0;
            HREG(88) = 0;
            HREG(89) = 0;
            HREG(91) = 0;
            HREG(92) = 0;
            HREG(93) = 0;
            HREG(94) = 0;
        }


        if (R_SCENE_CONFIG_DRAW_DEFAULT_DLIST == 1) {
            Scene_DefaultSegments();
            Scene_DefaultSegments();
        }


        if (R_SCENE_CONFIG_DRAW_SCENE_CONFIG == 1) {
            sSceneDrawConfigs[play->sceneDrawConfig](play);
        }
    } else {
        sSceneDrawConfigs[play->sceneDrawConfig](play);
    }
#else
    sSceneDrawConfigs[play->sceneDrawConfig](play);
#endif
}

#endif
