// adapted from flute-of-age/src/code/z_parameter.c
#include "array_count.h"
#include "engine/stub.h"
#include "engine/hud.h"
#include "engine/asset.h"
#include "engine/gfx3d.h"
#include "attributes.h"
#include "controller.h"
#include "flag_set.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "language_array.h"
#include "main.h"
#include "map.h"
#include "printf.h"
#include "regs.h"
#include "segment_symbols.h"
#include "segmented_address.h"
#include "sequence.h"
#include "sfx.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "translation.h"
#include "versions.h"
#include "audio.h"
#include "lifemeter.h"
#include "horse.h"
#include "flute.h"
#include "play_state.h"
#include "player.h"
#include "save.h"

#include "assets/textures/parameter_static/parameter_static.h"
#include "assets/textures/do_action_static/do_action_static.h"
#include "assets/textures/icon_item_static/icon_item_static.h"


typedef struct RestrictionFlags {
    /* 0x00 */ u8 sceneId;
    /* 0x01 */ u8 flags1;
    /* 0x02 */ u8 flags2;
    /* 0x03 */ u8 flags3;
} RestrictionFlags; // size = 0x4

static RestrictionFlags sRestrictionFlags[] = {
    { SCENE_ELDORA_FIELD, 0x00, 0x00, 0x10 },
    { SCENE_OAKRIDGE_VILLAGE, 0x00, 0x00, 0x10 },
    { SCENE_GRAVEYARD, 0x00, 0x00, 0x10 },
    { SCENE_NERIS_RIVER, 0x00, 0x00, 0x10 },
    { SCENE_SYLVAN_FOREST, 0x00, 0x00, 0x10 },
    { SCENE_ANCIENT_GROVE, 0x00, 0x00, 0x10 },
    { SCENE_LAKE_ELDEN, 0x00, 0x00, 0x10 },
    { SCENE_NERIS_DOMAIN, 0x00, 0x00, 0x10 },
    { SCENE_NERIS_FOUNTAIN, 0x00, 0x00, 0x10 },
    { SCENE_DUNEAN_VALLEY, 0x00, 0x00, 0x10 },
    { SCENE_WANDERING_WOODS, 0x00, 0x00, 0x10 },
    { SCENE_DESERT_SENTINEL, 0x00, 0x00, 0x10 },
    { SCENE_DUNEANS_FORTRESS, 0x00, 0x00, 0x10 },
    { SCENE_WHISPERING_SANDS, 0x00, 0x00, 0x10 },
    { SCENE_ELDORA_CASTLE, 0x00, 0x00, 0x10 },
    { SCENE_OUTSIDE_VARKONS_CASTLE, 0x00, 0x00, 0x10 },
    { SCENE_CINDER_MOUNTAIN_TRAIL, 0x00, 0x00, 0x10 },
    { SCENE_CINDER_MOUNTAIN_CRATER, 0x00, 0x00, 0x10 },
    { SCENE_CAIRN_CITY, 0x00, 0x00, 0x10 },
    { SCENE_SUNMEAD_RANCH, 0x00, 0x00, 0x10 },
    { SCENE_HALL_OF_AGES, 0x00, 0x10, 0x15 },
    { SCENE_CHAMBER_OF_ELDERS, 0xA2, 0xAA, 0xAA },
    { SCENE_SHOOTING_GALLERY, 0x11, 0x55, 0x55 },
    { SCENE_CASTLE_COURTYARD_GUARDS_DAY, 0x11, 0x55, 0x55 },
    { SCENE_CASTLE_COURTYARD_GUARDS_NIGHT, 0x11, 0x55, 0x55 },
    { SCENE_DREADBOUND_GRAVE, 0x00, 0x00, 0xD0 },
    { SCENE_GRAVE_WITH_FAIRYS_FOUNTAIN, 0x00, 0x00, 0xD0 },
    { SCENE_ROYAL_FAMILYS_TOMB, 0x00, 0x00, 0xD0 },
    { SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC, 0x00, 0x00, 0x10 },
    { SCENE_FAIRYS_FOUNTAIN, 0x00, 0x00, 0xD0 },
    { SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS, 0x00, 0x00, 0x10 },
    { SCENE_VARKONS_TOWER_COLLAPSE_EXTERIOR, 0x00, 0x05, 0x50 },
    { SCENE_CASTLE_COURTYARD_ELARA, 0x00, 0x05, 0x54 },
    { SCENE_FISHING_POND, 0x11, 0x55, 0x55 },
    { SCENE_BOMBMOUSE_BOWLING_ALLEY, 0x11, 0x55, 0x55 },
    { SCENE_SUNMEAD_BUILDINGS, 0x00, 0x10, 0x15 },
    { SCENE_MARKET_GUARD_HOUSE, 0x00, 0x10, 0x14 },
    { SCENE_POTION_SHOP_GRANNY, 0x10, 0x15, 0x55 },
    { SCENE_TREASURE_BOX_SHOP, 0x10, 0x15, 0x55 },
    { SCENE_HOUSE_OF_WEBGUARD, 0x00, 0x10, 0x15 },
    { SCENE_MARKET_ENTRANCE_DAY, 0x00, 0x10, 0x15 },
    { SCENE_MARKET_ENTRANCE_NIGHT, 0x00, 0x10, 0x15 },
    { SCENE_MARKET_ENTRANCE_RUINS, 0x00, 0x10, 0xD5 },
    { SCENE_MARKET_DAY, 0x00, 0x10, 0x15 },
    { SCENE_MARKET_NIGHT, 0x00, 0x10, 0x15 },
    { SCENE_MARKET_RUINS, 0x00, 0x10, 0xD5 },
    { SCENE_BACK_ALLEY_DAY, 0x00, 0x10, 0x15 },
    { SCENE_BACK_ALLEY_NIGHT, 0x00, 0x10, 0x15 },
    { SCENE_HALL_OF_AGES_EXTERIOR_DAY, 0x00, 0x10, 0x15 },
    { SCENE_HALL_OF_AGES_EXTERIOR_NIGHT, 0x00, 0x10, 0x15 },
    { SCENE_HALL_OF_AGES_EXTERIOR_RUINS, 0x00, 0x10, 0xD5 },
    { SCENE_ARENS_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_OAKRIDGE_CENTER_GUEST_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_BACK_ALLEY_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_KNOW_IT_ALL_BROS_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_TWINS_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_MILOS_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_SYLVAS_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_STABLE, 0x10, 0x10, 0x15 },
    { SCENE_GRAVEKEEPERS_HUT, 0x10, 0x10, 0x15 },
    { SCENE_DOG_LADY_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_ILYAS_HOUSE, 0x10, 0x10, 0x15 },
    { SCENE_LAKESIDE_LABORATORY, 0x00, 0x10, 0x15 },
    { SCENE_CARPENTERS_TENT, 0x10, 0x10, 0x15 },
    { SCENE_BAZAAR, 0x10, 0x10, 0x15 },
    { SCENE_SYLVAN_SHOP, 0x10, 0x10, 0x15 },
    { SCENE_CAIRN_SHOP, 0x10, 0x10, 0x15 },
    { SCENE_NERI_SHOP, 0x10, 0x10, 0x15 },
    { SCENE_POTION_SHOP_OAKRIDGE, 0x10, 0x10, 0x15 },
    { SCENE_POTION_SHOP_MARKET, 0x10, 0x10, 0x15 },
    { SCENE_BOMBMOUSE_SHOP, 0x10, 0x10, 0x15 },
    { SCENE_WANDERING_MASK_SHOP, 0x10, 0x10, 0x15 },
    { SCENE_DUNEAN_TRAINING_GROUND, 0x00, 0x03, 0x10 },
    { SCENE_ROOT_TREE, 0x00, 0x00, 0x00 },
    { SCENE_ROOT_TREE_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_CRAGJAWS_CAVERN, 0x00, 0x00, 0x00 },
    { SCENE_CRAGJAWS_CAVERN_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_TIDE_MAW, 0x00, 0x00, 0x00 },
    { SCENE_TIDE_MAW_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_FOREST_TEMPLE, 0x00, 0x00, 0x00 },
    { SCENE_FOREST_TEMPLE_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_BOTTOM_OF_THE_WELL, 0x00, 0x00, 0x00 },
    { SCENE_SHADOW_TEMPLE, 0x00, 0x00, 0x00 },
    { SCENE_SHADOW_TEMPLE_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_FIRE_TEMPLE, 0x00, 0x00, 0x00 },
    { SCENE_FIRE_TEMPLE_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_WATER_TEMPLE, 0x00, 0x00, 0x00 },
    { SCENE_WATER_TEMPLE_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_SPIRIT_TEMPLE, 0x00, 0x00, 0x00 },
    { SCENE_SPIRIT_TEMPLE_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_VARKONS_TOWER, 0x00, 0x00, 0x00 },
    { SCENE_VARKONOS_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_ICE_CAVERN, 0x00, 0x00, 0xC0 },
    { SCENE_WINDMILL_AND_FENNOS_GRAVE, 0x00, 0x03, 0x14 },
    { SCENE_INSIDE_VARKONS_CASTLE, 0x00, 0x03, 0x10 },
    { SCENE_VARKON_BOSS, 0x00, 0x45, 0x50 },
    { SCENE_VARKONS_TOWER_COLLAPSE_INTERIOR, 0x00, 0x05, 0x50 },
    { SCENE_INSIDE_VARKONS_CASTLE_COLLAPSE, 0x00, 0x05, 0x50 },
    { SCENE_THIEVES_HIDEOUT, 0x00, 0x00, 0x10 },
    { SCENE_GROTTOS, 0x00, 0x00, 0xD0 },
    { 0xFF, 0x00, 0x00, 0x00 },
};

static s16 sHBAScoreTier = 0;
static u16 sHBAScoreDigits[] = { 0, 0, 0, 0 };

static u16 sCUpInvisible = 0;
static u16 sCUpTimer = 0;

s16 gSpoilingItems[] = { ITEM_ODD_MUSHROOM, ITEM_EYEBALL_FROG, ITEM_EYE_DROPS };
s16 gSpoilingItemReverts[] = { ITEM_AZURE, ITEM_PRESCRIPTION, ITEM_PRESCRIPTION };

static s16 sMagicBorderR = 255;
static s16 sMagicBorderG = 255;
static s16 sMagicBorderB = 255;

static s16 sExtraItemBases[] = {
    ITEM_ROOT_STICK, // ITEM_ROOT_STICKS_5
    ITEM_ROOT_STICK, // ITEM_ROOT_STICKS_10
    ITEM_ROOT_NUT,   // ITEM_ROOT_NUTS_5
    ITEM_ROOT_NUT,   // ITEM_ROOT_NUTS_10
    ITEM_BOMB,       // ITEM_BOMBS_5
    ITEM_BOMB,       // ITEM_BOMBS_10
    ITEM_BOMB,       // ITEM_BOMBS_20
    ITEM_BOMB,       // ITEM_BOMBS_30
    ITEM_BOW,        // ITEM_ARROWS_5
    ITEM_BOW,        // ITEM_ARROWS_10
    ITEM_BOW,        // ITEM_ARROWS_30
    ITEM_ROOT_SEEDS, // ITEM_ROOT_SEEDS_30
    ITEM_BOMBMOUSE,    // ITEM_BOMBMOUSES_5
    ITEM_BOMBMOUSE,    // ITEM_BOMBMOUSES_20
    ITEM_ROOT_STICK, // ITEM_ROOT_STICK_UPGRADE_20
    ITEM_ROOT_STICK, // ITEM_ROOT_STICK_UPGRADE_30
    ITEM_ROOT_NUT,   // ITEM_ROOT_NUT_UPGRADE_30
    ITEM_ROOT_NUT,   // ITEM_ROOT_NUT_UPGRADE_40
};

static s16 sEnvHazard = PLAYER_ENV_HAZARD_NONE;
static s16 sEnvHazardActive = false;


// original name: "alpha_change"
void Interface_ChangeHudVisibilityMode(u16 hudVisibilityMode) {
    if (hudVisibilityMode != gSaveContext.hudVisibilityMode) {
        PRINTF("ＡＬＰＨＡーＴＹＰＥ＝%d  LAST_TIME_TYPE=%d\n", hudVisibilityMode, gSaveContext.prevHudVisibilityMode);
        gSaveContext.hudVisibilityMode = gSaveContext.nextHudVisibilityMode = hudVisibilityMode;
        gSaveContext.hudVisibilityModeTimer = 1;
    }
}

/**
 * Raise button alphas on the HUD to the requested value
 * Apply a set value of 70 to disabled buttons
 */
void Interface_RaiseButtonAlphas(PlayState* play, s16 risingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (gSaveContext.buttonStatus[0] == BTN_DISABLED) {
        if (interfaceCtx->bAlpha != 70) {
            interfaceCtx->bAlpha = 70;
        }
    } else {
        if (interfaceCtx->bAlpha != 255) {
            interfaceCtx->bAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[1] == BTN_DISABLED) {
        if (interfaceCtx->cLeftAlpha != 70) {
            interfaceCtx->cLeftAlpha = 70;
        }
    } else {
        if (interfaceCtx->cLeftAlpha != 255) {
            interfaceCtx->cLeftAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[2] == BTN_DISABLED) {
        if (interfaceCtx->cDownAlpha != 70) {
            interfaceCtx->cDownAlpha = 70;
        }
    } else {
        if (interfaceCtx->cDownAlpha != 255) {
            interfaceCtx->cDownAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[3] == BTN_DISABLED) {
        if (interfaceCtx->cRightAlpha != 70) {
            interfaceCtx->cRightAlpha = 70;
        }
    } else {
        if (interfaceCtx->cRightAlpha != 255) {
            interfaceCtx->cRightAlpha = risingAlpha;
        }
    }

    if (gSaveContext.buttonStatus[4] == BTN_DISABLED) {
        if (interfaceCtx->aAlpha != 70) {
            interfaceCtx->aAlpha = 70;
        }
    } else {
        if (interfaceCtx->aAlpha != 255) {
            interfaceCtx->aAlpha = risingAlpha;
        }
    }
}

/**
 * Lower button alphas on the HUD to the requested value
 * if forceRisingButtonAlphas is set, then instead raise button alphas
 */
void Interface_DimButtonAlphas(PlayState* play, s16 dimmingAlpha, s16 risingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (gSaveContext.forceRisingButtonAlphas) {
        Interface_RaiseButtonAlphas(play, risingAlpha);
        return;
    }

    if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
        interfaceCtx->bAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
        interfaceCtx->aAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
        interfaceCtx->cLeftAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
        interfaceCtx->cDownAlpha = dimmingAlpha;
    }

    if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
        interfaceCtx->cRightAlpha = dimmingAlpha;
    }
}

void Interface_UpdateHudAlphas(PlayState* play, s16 dimmingAlpha) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 risingAlpha = 255 - dimmingAlpha;

    switch (gSaveContext.nextHudVisibilityMode) {
        case HUD_VISIBILITY_NOTHING:
        case HUD_VISIBILITY_NOTHING_ALT:
        case HUD_VISIBILITY_B:
            PRINTF("a_alpha=%d, c_alpha=%d   →   ", interfaceCtx->aAlpha, interfaceCtx->cLeftAlpha);

            if (gSaveContext.nextHudVisibilityMode == HUD_VISIBILITY_B) {
                if (interfaceCtx->bAlpha != 255) {
                    interfaceCtx->bAlpha = risingAlpha;
                }
            } else {
                if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                    interfaceCtx->bAlpha = dimmingAlpha;
                }
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            PRINTF("a_alpha=%d, c_alpha=%d\n", interfaceCtx->aAlpha, interfaceCtx->cLeftAlpha);

            break;

        case HUD_VISIBILITY_HEARTS_FORCE:
            // aAlpha is immediately overwritten in Interface_DimButtonAlphas
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            Interface_DimButtonAlphas(play, dimmingAlpha, risingAlpha);

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            // aAlpha is immediately overwritten below
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_FORCE:
            Interface_DimButtonAlphas(play, dimmingAlpha, risingAlpha);

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            // aAlpha overwrites the value set in Interface_DimButtonAlphas
            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_FORCE:
            Interface_DimButtonAlphas(play, dimmingAlpha, risingAlpha);

            // aAlpha overwrites the value set in Interface_DimButtonAlphas
            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            switch (play->sceneId) {
                case SCENE_ELDORA_FIELD:
                case SCENE_OAKRIDGE_VILLAGE:
                case SCENE_GRAVEYARD:
                case SCENE_NERIS_RIVER:
                case SCENE_SYLVAN_FOREST:
                case SCENE_ANCIENT_GROVE:
                case SCENE_LAKE_ELDEN:
                case SCENE_NERIS_DOMAIN:
                case SCENE_NERIS_FOUNTAIN:
                case SCENE_DUNEAN_VALLEY:
                case SCENE_WANDERING_WOODS:
                case SCENE_DESERT_SENTINEL:
                case SCENE_DUNEANS_FORTRESS:
                case SCENE_WHISPERING_SANDS:
                case SCENE_ELDORA_CASTLE:
                case SCENE_CINDER_MOUNTAIN_TRAIL:
                case SCENE_CINDER_MOUNTAIN_CRATER:
                case SCENE_CAIRN_CITY:
                case SCENE_SUNMEAD_RANCH:
                case SCENE_OUTSIDE_VARKONS_CASTLE:
                    if (interfaceCtx->minimapAlpha < 170) {
                        interfaceCtx->minimapAlpha = risingAlpha;
                    } else {
                        interfaceCtx->minimapAlpha = 170;
                    }
                    break;

                default:
                    if (interfaceCtx->minimapAlpha != 255) {
                        interfaceCtx->minimapAlpha = risingAlpha;
                    }
                    break;
            }
            break;

        case HUD_VISIBILITY_ALL_NO_MINIMAP_BY_BTN_STATUS:
            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            Interface_RaiseButtonAlphas(play, risingAlpha);

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_B_ALT:
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS:
            if ((interfaceCtx->bAlpha != 0) && (interfaceCtx->bAlpha > dimmingAlpha)) {
                interfaceCtx->bAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            break;

        case HUD_VISIBILITY_A_B_MINIMAP:
            if (interfaceCtx->aAlpha != 255) {
                interfaceCtx->aAlpha = risingAlpha;
            }

            if (interfaceCtx->bAlpha != 255) {
                interfaceCtx->bAlpha = risingAlpha;
            }

            if (interfaceCtx->minimapAlpha != 255) {
                interfaceCtx->minimapAlpha = risingAlpha;
            }

            if ((interfaceCtx->cLeftAlpha != 0) && (interfaceCtx->cLeftAlpha > dimmingAlpha)) {
                interfaceCtx->cLeftAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cDownAlpha != 0) && (interfaceCtx->cDownAlpha > dimmingAlpha)) {
                interfaceCtx->cDownAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->cRightAlpha != 0) && (interfaceCtx->cRightAlpha > dimmingAlpha)) {
                interfaceCtx->cRightAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->magicAlpha != 0) && (interfaceCtx->magicAlpha > dimmingAlpha)) {
                interfaceCtx->magicAlpha = dimmingAlpha;
            }

            if ((interfaceCtx->healthAlpha != 0) && (interfaceCtx->healthAlpha > dimmingAlpha)) {
                interfaceCtx->healthAlpha = dimmingAlpha;
            }

            break;

        case HUD_VISIBILITY_HEARTS_MAGIC_FORCE:
            Interface_DimButtonAlphas(play, dimmingAlpha, risingAlpha);

            if ((interfaceCtx->minimapAlpha != 0) && (interfaceCtx->minimapAlpha > dimmingAlpha)) {
                interfaceCtx->minimapAlpha = dimmingAlpha;
            }

            // aAlpha overwrites the value set in Interface_DimButtonAlphas
            if ((interfaceCtx->aAlpha != 0) && (interfaceCtx->aAlpha > dimmingAlpha)) {
                interfaceCtx->aAlpha = dimmingAlpha;
            }

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            break;
    }

    if ((play->roomCtx.curRoom.type == ROOM_TYPE_DUNGEON) && (interfaceCtx->minimapAlpha >= 255)) {
        interfaceCtx->minimapAlpha = 255;
    }
}

void func_80083108(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Player* player = GET_PLAYER(play);
    MessageContext* msgCtx = &play->msgCtx;
    s16 i;
    s16 sp28 = false;

    if ((gSaveContext.save.cutsceneIndex < CS_INDEX_0) ||
        ((play->sceneId == SCENE_SUNMEAD_RANCH) && (gSaveContext.save.cutsceneIndex == CS_INDEX_0))) {
        gSaveContext.forceRisingButtonAlphas = false;

        if ((player->stateFlags1 & PLAYER_STATE1_23) || (play->shootingGalleryStatus > 1) ||
            ((play->sceneId == SCENE_BOMBMOUSE_BOWLING_ALLEY) && Flags_GetSwitch(play, 0x38))) {
            if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {
                gSaveContext.forceRisingButtonAlphas = true;

                if (gSaveContext.buttonStatus[0] == BTN_DISABLED) {
                    gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
                        gSaveContext.buttonStatus[3] = BTN_ENABLED;
                }

                if ((gSaveContext.save.info.equips.buttonItems[0] != ITEM_SLINGSHOT) &&
                    (gSaveContext.save.info.equips.buttonItems[0] != ITEM_BOW) &&
                    (gSaveContext.save.info.equips.buttonItems[0] != ITEM_BOMBMOUSE) &&
                    (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE)) {
                    gSaveContext.buttonStatus[0] = gSaveContext.save.info.equips.buttonItems[0];

                    if ((play->sceneId == SCENE_BOMBMOUSE_BOWLING_ALLEY) && Flags_GetSwitch(play, 0x38)) {
                        gSaveContext.save.info.equips.buttonItems[0] = ITEM_BOMBMOUSE;
                        Interface_LoadItemIcon1(play, 0);
                    } else {
                        gSaveContext.save.info.equips.buttonItems[0] = ITEM_BOW;
                        if (play->shootingGalleryStatus > 1) {
                            if (AREN_AGE_IN_YEARS == YEARS_CHILD) {
                                gSaveContext.save.info.equips.buttonItems[0] = ITEM_SLINGSHOT;
                            }

                            Interface_LoadItemIcon1(play, 0);
                        } else {
                            if (gSaveContext.save.info.inventory.items[SLOT_BOW] == ITEM_NONE) {
                                gSaveContext.save.info.equips.buttonItems[0] = ITEM_NONE;
                            } else {
                                Interface_LoadItemIcon1(play, 0);
                            }
                        }
                    }

                    gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] = gSaveContext.buttonStatus[3] =
                        BTN_DISABLED;
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_FORCE);
                }

                if (play->transitionMode != TRANS_MODE_OFF) {
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);
                } else if (gSaveContext.minigameState == 1) {
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_B);
                } else if (play->shootingGalleryStatus > 1) {
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_B);
                } else if ((play->sceneId == SCENE_BOMBMOUSE_BOWLING_ALLEY) && Flags_GetSwitch(play, 0x38)) {
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_B);
                } else if (player->stateFlags1 & PLAYER_STATE1_23) {
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_A_B_MINIMAP);
                }
            } else {
                if (player->stateFlags1 & PLAYER_STATE1_23) {
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_A_B_MINIMAP);
                }
            }
        } else if (play->sceneId == SCENE_CHAMBER_OF_ELDERS) {
            Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);
        } else if (play->sceneId == SCENE_FISHING_POND) {
            // should likely be set to true
            gSaveContext.forceRisingButtonAlphas = 2;
            if (play->interfaceCtx.unk_260 != 0) {
                if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_FISHING_POLE) {
                    gSaveContext.buttonStatus[0] = gSaveContext.save.info.equips.buttonItems[0];
                    gSaveContext.save.info.equips.buttonItems[0] = ITEM_FISHING_POLE;
                    gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
                    Interface_LoadItemIcon1(play, 0);
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_A_B_MINIMAP);
                }

                if (gSaveContext.hudVisibilityMode != HUD_VISIBILITY_A_B_MINIMAP) {
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_A_B_MINIMAP);
                }
            } else if (gSaveContext.save.info.equips.buttonItems[0] == ITEM_FISHING_POLE) {
                gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0];
                gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;

                if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {
                    Interface_LoadItemIcon1(play, 0);
                }

                gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
                    gSaveContext.buttonStatus[3] = BTN_DISABLED;
                Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
            } else {
                if (gSaveContext.buttonStatus[0] == BTN_ENABLED) {
                    gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
                }

                gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
                    gSaveContext.buttonStatus[3] = BTN_DISABLED;
                Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
            }
        } else if (msgCtx->msgMode == MSGMODE_NONE) {
            if ((Player_GetEnvironmentalHazard(play) >= PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) &&
                (Player_GetEnvironmentalHazard(play) <= PLAYER_ENV_HAZARD_UNDERWATER_FREE)) {
                if (gSaveContext.buttonStatus[0] != BTN_DISABLED) {
                    sp28 = true;
                }

                gSaveContext.buttonStatus[0] = BTN_DISABLED;

                for (i = 1; i < 4; i++) {
                    if (Player_GetEnvironmentalHazard(play) == PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] != ITEM_GRAPNEL) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_LONGGRAPPLE)) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        } else {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    } else {
                        if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                            sp28 = true;
                        }

                        gSaveContext.buttonStatus[i] = BTN_DISABLED;
                    }
                }

                if (sp28) {
                    gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
                }

                Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
            } else if ((player->stateFlags1 & PLAYER_STATE1_21) || (player->stateFlags2 & PLAYER_STATE2_CRAWLING)) {
                if (gSaveContext.buttonStatus[0] != BTN_DISABLED) {
                    gSaveContext.buttonStatus[0] = BTN_DISABLED;
                    gSaveContext.buttonStatus[1] = BTN_DISABLED;
                    gSaveContext.buttonStatus[2] = BTN_DISABLED;
                    gSaveContext.buttonStatus[3] = BTN_DISABLED;
                    gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
                    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
                }
            } else if (GET_EVENTINF_OTTO_RACE_STATE() == OTTO_RACE_STATE_HORSE_RENTAL_PERIOD) {
                if (player->stateFlags1 & PLAYER_STATE1_23) {
                    if ((gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) &&
                        (gSaveContext.save.info.equips.buttonItems[0] != ITEM_BOW)) {
                        if (gSaveContext.save.info.inventory.items[SLOT_BOW] == ITEM_NONE) {
                            gSaveContext.save.info.equips.buttonItems[0] = ITEM_NONE;
                        } else {
                            gSaveContext.save.info.equips.buttonItems[0] = ITEM_BOW;
                            sp28 = true;
                        }
                    }
                } else {
                    if ((gSaveContext.save.info.equips.buttonItems[0] == ITEM_NONE) ||
                        (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOW)) {

                        if ((gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_SYLVAN) &&
                            (gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_MASTER) &&
                            (gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_HIGHCAIRN) &&
                            (gSaveContext.save.info.equips.buttonItems[0] != ITEM_GIANTS_KNIFE)) {
                            gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0];
                        } else {
                            gSaveContext.buttonStatus[0] = gSaveContext.save.info.equips.buttonItems[0];
                        }
                    }
                    sp28 = true;
                }

                if (sp28) {
                    Interface_LoadItemIcon1(play, 0);
                    sp28 = false;
                }

                for (i = 1; i < 4; i++) {
                    if ((gSaveContext.save.info.equips.buttonItems[i] != ITEM_FLUTE_FAIRY) &&
                        (gSaveContext.save.info.equips.buttonItems[i] != ITEM_FLUTE_OF_AGE)) {
                        if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                            sp28 = true;
                        }

                        gSaveContext.buttonStatus[i] = BTN_DISABLED;
                    } else {
                        if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                            sp28 = true;
                        }

                        gSaveContext.buttonStatus[i] = BTN_ENABLED;
                    }
                }

                if (sp28) {
                    gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
                }

                Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
            } else {
                if (interfaceCtx->restrictions.bButton == 0) {
                    if ((gSaveContext.save.info.equips.buttonItems[0] == ITEM_SLINGSHOT) ||
                        (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOW) ||
                        (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOMBMOUSE) ||
                        (gSaveContext.save.info.equips.buttonItems[0] == ITEM_NONE)) {
                        if ((gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) ||
                            (gSaveContext.save.info.infTable[INFTABLE_INDEX_1DX] == 0)) {
                            gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0];
                            sp28 = true;

                            if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {
                                Interface_LoadItemIcon1(play, 0);
                            }
                        }
                    } else if ((gSaveContext.buttonStatus[0] & 0xFF) == BTN_DISABLED) {
                        sp28 = true;

                        if (((gSaveContext.buttonStatus[0] & 0xFF) == BTN_DISABLED) ||
                            ((gSaveContext.buttonStatus[0] & 0xFF) == BTN_ENABLED)) {
                            gSaveContext.buttonStatus[0] = BTN_ENABLED;
                        } else {
                            gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0] & 0xFF;
                        }
                    }
                } else if (interfaceCtx->restrictions.bButton == 1) {
                    if ((gSaveContext.save.info.equips.buttonItems[0] == ITEM_SLINGSHOT) ||
                        (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOW) ||
                        (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOMBMOUSE) ||
                        (gSaveContext.save.info.equips.buttonItems[0] == ITEM_NONE)) {
                        if ((gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) ||
                            (gSaveContext.save.info.infTable[INFTABLE_INDEX_1DX] == 0)) {
                            gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0];
                            sp28 = true;

                            if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {
                                Interface_LoadItemIcon1(play, 0);
                            }
                        }
                    } else {
                        if (gSaveContext.buttonStatus[0] == BTN_ENABLED) {
                            sp28 = true;
                        }

                        gSaveContext.buttonStatus[0] = BTN_DISABLED;
                    }
                }

                if (interfaceCtx->restrictions.bottles != 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_BOTTLE_EMPTY) &&
                            (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_BOTTLE_WRAITH)) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    }
                } else if (interfaceCtx->restrictions.bottles == 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_BOTTLE_EMPTY) &&
                            (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_BOTTLE_WRAITH)) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }
                }

                if (interfaceCtx->restrictions.tradeItems != 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_WEIRD_EGG) &&
                            (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_CLAIM_CHECK)) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    }
                } else if (interfaceCtx->restrictions.tradeItems == 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_WEIRD_EGG) &&
                            (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_CLAIM_CHECK)) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }
                }

                if (interfaceCtx->restrictions.grapnel != 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] == ITEM_GRAPNEL) ||
                            (gSaveContext.save.info.equips.buttonItems[i] == ITEM_LONGGRAPPLE)) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    }
                } else if (interfaceCtx->restrictions.grapnel == 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] == ITEM_GRAPNEL) ||
                            (gSaveContext.save.info.equips.buttonItems[i] == ITEM_LONGGRAPPLE)) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }
                }

                if (interfaceCtx->restrictions.flute != 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] == ITEM_FLUTE_FAIRY) ||
                            (gSaveContext.save.info.equips.buttonItems[i] == ITEM_FLUTE_OF_AGE)) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    }
                } else if (interfaceCtx->restrictions.flute == 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] == ITEM_FLUTE_FAIRY) ||
                            (gSaveContext.save.info.equips.buttonItems[i] == ITEM_FLUTE_OF_AGE)) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }
                }

                if (interfaceCtx->restrictions.zephyrs != 0) {
                    for (i = 1; i < 4; i++) {
                        if (gSaveContext.save.info.equips.buttonItems[i] == ITEM_ZEPHYRS_WIND) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                            PRINTF("***(i=%d)***  ", i);
                        }
                    }
                } else if (interfaceCtx->restrictions.zephyrs == 0) {
                    for (i = 1; i < 4; i++) {
                        if (gSaveContext.save.info.equips.buttonItems[i] == ITEM_ZEPHYRS_WIND) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }
                }

                if (interfaceCtx->restrictions.pyrsNeris != 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] == ITEM_PYRS_FIRE) ||
                            (gSaveContext.save.info.equips.buttonItems[i] == ITEM_NERIS_LOVE)) {
                            if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_DISABLED;
                        }
                    }
                } else if (interfaceCtx->restrictions.pyrsNeris == 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] == ITEM_PYRS_FIRE) ||
                            (gSaveContext.save.info.equips.buttonItems[i] == ITEM_NERIS_LOVE)) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }
                }

                if (interfaceCtx->restrictions.all != 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] != ITEM_FLUTE_FAIRY) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_FLUTE_OF_AGE) &&
                            !((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_BOTTLE_EMPTY) &&
                              (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_BOTTLE_WRAITH)) &&
                            !((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_WEIRD_EGG) &&
                              (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_CLAIM_CHECK))) {
                            if ((play->sceneId != SCENE_TREASURE_BOX_SHOP) ||
                                (gSaveContext.save.info.equips.buttonItems[i] != ITEM_LENS_OF_INSIGHT)) {
                                if (gSaveContext.buttonStatus[i] == BTN_ENABLED) {
                                    sp28 = true;
                                }

                                gSaveContext.buttonStatus[i] = BTN_DISABLED;
                            } else {
                                if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                    sp28 = true;
                                }

                                gSaveContext.buttonStatus[i] = BTN_ENABLED;
                            }
                        }
                    }
                } else if (interfaceCtx->restrictions.all == 0) {
                    for (i = 1; i < 4; i++) {
                        if ((gSaveContext.save.info.equips.buttonItems[i] != ITEM_PYRS_FIRE) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_GRAPNEL) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_LONGGRAPPLE) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_ZEPHYRS_WIND) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_NERIS_LOVE) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_FLUTE_FAIRY) &&
                            (gSaveContext.save.info.equips.buttonItems[i] != ITEM_FLUTE_OF_AGE) &&
                            !((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_BOTTLE_EMPTY) &&
                              (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_BOTTLE_WRAITH)) &&
                            !((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_WEIRD_EGG) &&
                              (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_CLAIM_CHECK))) {
                            if (gSaveContext.buttonStatus[i] == BTN_DISABLED) {
                                sp28 = true;
                            }

                            gSaveContext.buttonStatus[i] = BTN_ENABLED;
                        }
                    }
                }
            }
        }
    }

    if (sp28) {
        gSaveContext.hudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
        if ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF)) {
            Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL);
            PRINTF("????????  alpha_change( 50 );  ?????\n");
        } else {
            PRINTF("game_play->fade_direction || game_play->fbdemo_wipe_modem");
        }
    }
}

void Interface_SetSceneRestrictions(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    s16 i = 0;
    u8 sceneId;
#if OOT_VERSION != IQUE_CN
    STACK_PADS(s32, 3);
#endif

    interfaceCtx->restrictions.all = 0;
    interfaceCtx->restrictions.pyrsNeris = 0;
    interfaceCtx->restrictions.zephyrs = 0;
    interfaceCtx->restrictions.sunsSong = 0;
    interfaceCtx->restrictions.warpSongs = 0;
    interfaceCtx->restrictions.flute = 0;
    interfaceCtx->restrictions.grapnel = 0;
    interfaceCtx->restrictions.tradeItems = 0;
    interfaceCtx->restrictions.bottles = 0;
    interfaceCtx->restrictions.aButton = 0;
    interfaceCtx->restrictions.bButton = 0;
    interfaceCtx->restrictions.hGauge = 0;

    PRINTF(
        T("ボタン表示関係データ設定 scene_data_ID=%d\n", "Data settings related to button display scene_data_ID=%d\n"),
        play->sceneId);

    do {
        sceneId = (u8)play->sceneId;
        if (sRestrictionFlags[i].sceneId == sceneId) {
            interfaceCtx->restrictions.hGauge = (sRestrictionFlags[i].flags1 & 0xC0) >> 6;
            interfaceCtx->restrictions.bButton = (sRestrictionFlags[i].flags1 & 0x30) >> 4;
            interfaceCtx->restrictions.aButton = (sRestrictionFlags[i].flags1 & 0x0C) >> 2;
            interfaceCtx->restrictions.bottles = (sRestrictionFlags[i].flags1 & 0x03) >> 0;
            interfaceCtx->restrictions.tradeItems = (sRestrictionFlags[i].flags2 & 0xC0) >> 6;
            interfaceCtx->restrictions.grapnel = (sRestrictionFlags[i].flags2 & 0x30) >> 4;
            interfaceCtx->restrictions.flute = (sRestrictionFlags[i].flags2 & 0x0C) >> 2;
            interfaceCtx->restrictions.warpSongs = (sRestrictionFlags[i].flags2 & 0x03) >> 0;
            interfaceCtx->restrictions.sunsSong = (sRestrictionFlags[i].flags3 & 0xC0) >> 6;
            interfaceCtx->restrictions.zephyrs = (sRestrictionFlags[i].flags3 & 0x30) >> 4;
            interfaceCtx->restrictions.pyrsNeris = (sRestrictionFlags[i].flags3 & 0x0C) >> 2;
            interfaceCtx->restrictions.all = (sRestrictionFlags[i].flags3 & 0x03) >> 0;

            PRINTF_COLOR_YELLOW();
            PRINTF("parameter->button_status = %x,%x,%x\n", sRestrictionFlags[i].flags1, sRestrictionFlags[i].flags2,
                   sRestrictionFlags[i].flags3);
            PRINTF("h_gage=%d, b_button=%d, a_button=%d, c_bottle=%d\n", interfaceCtx->restrictions.hGauge,
                   interfaceCtx->restrictions.bButton, interfaceCtx->restrictions.aButton,
                   interfaceCtx->restrictions.bottles);
            PRINTF("c_warasibe=%d, c_hook=%d, c_flute=%d, c_warp=%d\n", interfaceCtx->restrictions.tradeItems,
                   interfaceCtx->restrictions.grapnel, interfaceCtx->restrictions.flute,
                   interfaceCtx->restrictions.warpSongs);
            PRINTF("c_sunmoon=%d, m_wind=%d, m_magic=%d, another=%d\n", interfaceCtx->restrictions.sunsSong,
                   interfaceCtx->restrictions.zephyrs, interfaceCtx->restrictions.pyrsNeris,
                   interfaceCtx->restrictions.all);
            PRINTF_RST();
            break;
        }
        i++;
    } while (sRestrictionFlags[i].sceneId != 0xFF);
}

void* Gfx_TextureIA8(void* displayListHead, void* texture, s16 textureWidth, s16 textureHeight, s16 rectLeft, s16 rectTop,
                    s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy) {
    PRESENTATION_STUB("Gfx_TextureIA8");
    return 0;
}

void* Gfx_TextureI8(void* displayListHead, void* texture, s16 textureWidth, s16 textureHeight, s16 rectLeft, s16 rectTop,
                   s16 rectWidth, s16 rectHeight, u16 dsdx, u16 dtdy) {
    PRESENTATION_STUB("Gfx_TextureI8");
    return 0;
}

void Inventory_SwapAgeEquipment(void) {
    s16 i;
    u16 shieldEquipValue;

    if (AREN_AGE_IN_YEARS == YEARS_CHILD) {
        for (i = 0; i < 4; i++) {
            if (i != 0) {
                gSaveContext.save.info.playerData.childEquips.buttonItems[i] =
                    gSaveContext.save.info.equips.buttonItems[i];
            } else {
                gSaveContext.save.info.playerData.childEquips.buttonItems[i] = ITEM_SWORD_SYLVAN;
            }

            if (i != 0) {
                gSaveContext.save.info.playerData.childEquips.cButtonSlots[i - 1] =
                    gSaveContext.save.info.equips.cButtonSlots[i - 1];
            }
        }

        gSaveContext.save.info.playerData.childEquips.equipment = gSaveContext.save.info.equips.equipment;

        if (gSaveContext.save.info.playerData.adultEquips.buttonItems[0] == ITEM_NONE) {
            gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_MASTER;

            if (gSaveContext.save.info.inventory.items[SLOT_ROOT_NUT] != ITEM_NONE) {
                gSaveContext.save.info.equips.buttonItems[1] = ITEM_ROOT_NUT;
                gSaveContext.save.info.equips.cButtonSlots[0] = SLOT_ROOT_NUT;
            } else {
                gSaveContext.save.info.equips.buttonItems[1] = gSaveContext.save.info.equips.cButtonSlots[0] =
                    ITEM_NONE;
            }

            gSaveContext.save.info.equips.buttonItems[2] = ITEM_BOMB;
            gSaveContext.save.info.equips.buttonItems[3] = gSaveContext.save.info.inventory.items[SLOT_FLUTE];
            gSaveContext.save.info.equips.cButtonSlots[1] = SLOT_BOMB;
            gSaveContext.save.info.equips.cButtonSlots[2] = SLOT_FLUTE;
            gSaveContext.save.info.equips.equipment = (EQUIP_VALUE_SWORD_MASTER << (EQUIP_TYPE_SWORD * 4)) |
                                                      (EQUIP_VALUE_SHIELD_ELDRAN << (EQUIP_TYPE_SHIELD * 4)) |
                                                      (EQUIP_VALUE_TUNIC_SYLVAN << (EQUIP_TYPE_TUNIC * 4)) |
                                                      (EQUIP_VALUE_BOOTS_SYLVAN << (EQUIP_TYPE_BOOTS * 4));
        } else {
            for (i = 0; i < 4; i++) {
                gSaveContext.save.info.equips.buttonItems[i] =
                    gSaveContext.save.info.playerData.adultEquips.buttonItems[i];

                if (i != 0) {
                    gSaveContext.save.info.equips.cButtonSlots[i - 1] =
                        gSaveContext.save.info.playerData.adultEquips.cButtonSlots[i - 1];
                }

                if (((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_BOTTLE_EMPTY) &&
                     (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_BOTTLE_WRAITH)) ||
                    ((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_WEIRD_EGG) &&
                     (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_CLAIM_CHECK))) {
                    PRINTF("Register_Item_Pt(%d)=%d\n", i, gSaveContext.save.info.equips.cButtonSlots[i - 1]);
                    gSaveContext.save.info.equips.buttonItems[i] =
                        gSaveContext.save.info.inventory.items[gSaveContext.save.info.equips.cButtonSlots[i - 1]];
                }
            }

            gSaveContext.save.info.equips.equipment = gSaveContext.save.info.playerData.adultEquips.equipment;
        }
    } else {
        for (i = 0; i < 4; i++) {
            gSaveContext.save.info.playerData.adultEquips.buttonItems[i] = gSaveContext.save.info.equips.buttonItems[i];

            if (i != 0) {
                gSaveContext.save.info.playerData.adultEquips.cButtonSlots[i - 1] =
                    gSaveContext.save.info.equips.cButtonSlots[i - 1];
            }
        }

        gSaveContext.save.info.playerData.adultEquips.equipment = gSaveContext.save.info.equips.equipment;

        if (gSaveContext.save.info.playerData.childEquips.buttonItems[0] != ITEM_NONE) {
            for (i = 0; i < 4; i++) {
                gSaveContext.save.info.equips.buttonItems[i] =
                    gSaveContext.save.info.playerData.childEquips.buttonItems[i];

                if (i != 0) {
                    gSaveContext.save.info.equips.cButtonSlots[i - 1] =
                        gSaveContext.save.info.playerData.childEquips.cButtonSlots[i - 1];
                }

                if (((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_BOTTLE_EMPTY) &&
                     (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_BOTTLE_WRAITH)) ||
                    ((gSaveContext.save.info.equips.buttonItems[i] >= ITEM_WEIRD_EGG) &&
                     (gSaveContext.save.info.equips.buttonItems[i] <= ITEM_CLAIM_CHECK))) {
                    PRINTF("Register_Item_Pt(%d)=%d\n", i, gSaveContext.save.info.equips.cButtonSlots[i - 1]);
                    gSaveContext.save.info.equips.buttonItems[i] =
                        gSaveContext.save.info.inventory.items[gSaveContext.save.info.equips.cButtonSlots[i - 1]];
                }
            }

            gSaveContext.save.info.equips.equipment = gSaveContext.save.info.playerData.childEquips.equipment;
            gSaveContext.save.info.equips.equipment &= (u16) ~(0xF << (EQUIP_TYPE_SWORD * 4));
            gSaveContext.save.info.equips.equipment |= EQUIP_VALUE_SWORD_SYLVAN << (EQUIP_TYPE_SWORD * 4);
        }
    }

    shieldEquipValue = gSaveContext.save.info.equips.equipment & gEquipMasks[EQUIP_TYPE_SHIELD];
    if (shieldEquipValue) {
        shieldEquipValue >>= gEquipShifts[EQUIP_TYPE_SHIELD];
        if (!CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SHIELD, shieldEquipValue - 1)) {
            gSaveContext.save.info.equips.equipment &= gEquipNegMasks[EQUIP_TYPE_SHIELD];
        }
    }
}

void Interface_InitHorsebackArchery(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    gSaveContext.minigameState = 1;
    interfaceCtx->unk_23C = interfaceCtx->unk_240 = interfaceCtx->unk_242 = 0;
    gSaveContext.minigameScore = sHBAScoreTier = 0;
    interfaceCtx->hbaAmmo = 20;
}

void func_800849EC(PlayState* play) {
    gSaveContext.save.info.inventory.equipment |= OWNED_EQUIP_FLAG(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_HIGHCAIRN);
    gSaveContext.save.info.inventory.equipment ^=
        OWNED_EQUIP_FLAG_ALT(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BROKENGIANTKNIFE);

    if (CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BROKENGIANTKNIFE)) {
        gSaveContext.save.info.equips.buttonItems[0] = ITEM_GIANTS_KNIFE;
    } else {
        gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_HIGHCAIRN;
    }

    Interface_LoadItemIcon1(play, 0);
}

void Interface_LoadItemIcon1(PlayState* play, u16 button) {
    PRESENTATION_STUB("Interface_LoadItemIcon1");
}

void Interface_LoadItemIcon2(PlayState* play, u16 button) {
    PRESENTATION_STUB("Interface_LoadItemIcon2");
}

void func_80084BF4(PlayState* play, u16 flag) {
    if (flag) {
        if ((gSaveContext.save.info.equips.buttonItems[0] == ITEM_SLINGSHOT) ||
            (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOW) ||
            (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOMBMOUSE) ||
            (gSaveContext.save.info.equips.buttonItems[0] == ITEM_FISHING_POLE) ||
            (gSaveContext.buttonStatus[0] == BTN_DISABLED)) {
            if ((gSaveContext.save.info.equips.buttonItems[0] == ITEM_SLINGSHOT) ||
                (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOW) ||
                (gSaveContext.save.info.equips.buttonItems[0] == ITEM_BOMBMOUSE) ||
                (gSaveContext.save.info.equips.buttonItems[0] == ITEM_FISHING_POLE)) {
                gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0];
                Interface_LoadItemIcon1(play, 0);
            }
        } else if (gSaveContext.save.info.equips.buttonItems[0] == ITEM_NONE) {
            if ((gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) ||
                (gSaveContext.save.info.infTable[INFTABLE_INDEX_1DX] == 0)) {
                gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0];
                Interface_LoadItemIcon1(play, 0);
            }
        }

        gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
            gSaveContext.buttonStatus[3] = BTN_ENABLED;
        Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_ALL_NO_MINIMAP_BY_BTN_STATUS);
    } else {
        gSaveContext.buttonStatus[0] = gSaveContext.buttonStatus[1] = gSaveContext.buttonStatus[2] =
            gSaveContext.buttonStatus[3] = BTN_ENABLED;
        func_80083108(play);
    }
}

u8 Item_Give(PlayState* play, u8 item) {
    static s16 sAmmoRefillCounts[] = { 5, 10, 20, 30 }; // Sticks, nuts, bombs
    static s16 sArrowRefillCounts[] = { 5, 10, 30 };
    static s16 sBombmouseRefillCounts[] = { 5, 20 };
    static s16 sCrownRefillCounts[] = { 1, 5, 20, 50, 200, 10 };
    s16 i;
    s16 slot;
    s16 temp;

    //! @bug UB: For items that are not inventory items, the SLOT macro indexes gItemSlots out of bounds.
    slot = SLOT(item);
    if (item >= ITEM_ROOT_STICKS_5) {
        slot = SLOT(sExtraItemBases[item - ITEM_ROOT_STICKS_5]);
    }

    PRINTF_COLOR_YELLOW();
    PRINTF("item_get_setting=%d  pt=%d  z=%x\n", item, slot, gSaveContext.save.info.inventory.items[slot]);
    PRINTF_RST();

    if ((item >= ITEM_MEDALLION_FOREST) && (item <= ITEM_MEDALLION_LIGHT)) {
        gSaveContext.save.info.inventory.questItems |= gBitFlags[item - ITEM_MEDALLION_FOREST + QUEST_MEDALLION_FOREST];

        PRINTF_COLOR_YELLOW();
        PRINTF(T("封印 = %x\n", "Seals = %x\n"), gSaveContext.save.info.inventory.questItems);
        PRINTF_RST();

        if (item == ITEM_MEDALLION_WATER) {
            Horse_FixLakeEldenPosition(play);
        }

        return ITEM_NONE;
    } else if ((item >= ITEM_SONG_MINUET) && (item <= ITEM_SONG_STORMS)) {
        gSaveContext.save.info.inventory.questItems |= gBitFlags[item - ITEM_SONG_MINUET + QUEST_SONG_MINUET];

        PRINTF_COLOR_YELLOW();
        PRINTF(T("楽譜 = %x\n", "Musical scores = %x\n"), gSaveContext.save.info.inventory.questItems);
        PRINTF(T("楽譜 = %x (%x) (%x)\n", "Musical scores = %x (%x) (%x)\n"),
               gSaveContext.save.info.inventory.questItems, gBitFlags[item - ITEM_SONG_MINUET + QUEST_SONG_MINUET],
               gBitFlags[item - ITEM_SONG_MINUET]);
        PRINTF_RST();

        return ITEM_NONE;
    } else if ((item >= ITEM_SYLVAN_EMERALD) && (item <= ITEM_NERI_SAPPHIRE)) {
        gSaveContext.save.info.inventory.questItems |= gBitFlags[item - ITEM_SYLVAN_EMERALD + QUEST_SYLVAN_EMERALD];

        PRINTF_COLOR_YELLOW();
        PRINTF(T("精霊石 = %x\n", "Primal Stones = %x\n"), gSaveContext.save.info.inventory.questItems);
        PRINTF_RST();

        return ITEM_NONE;
    } else if ((item == ITEM_STONE_OF_ECHOES) || (item == ITEM_DUNEANS_CARD)) {
        gSaveContext.save.info.inventory.questItems |= gBitFlags[item - ITEM_STONE_OF_ECHOES + QUEST_STONE_OF_ECHOES];

        PRINTF_COLOR_YELLOW();
        PRINTF(T("アイテム = %x\n", "Items = %x\n"), gSaveContext.save.info.inventory.questItems);
        PRINTF_RST();

        return ITEM_NONE;
    } else if (item == ITEM_SKULL_TOKEN) {
        gSaveContext.save.info.inventory.questItems |= gBitFlags[item - ITEM_SKULL_TOKEN + QUEST_SKULL_TOKEN];
        gSaveContext.save.info.inventory.gsTokens++;

        PRINTF_COLOR_YELLOW();
        PRINTF(T("Ｎコイン = %x(%d)\n", "N Coins = %x(%d)\n"), gSaveContext.save.info.inventory.questItems,
               gSaveContext.save.info.inventory.gsTokens);
        PRINTF_RST();

        return ITEM_NONE;
    } else if ((item >= ITEM_SWORD_SYLVAN) && (item <= ITEM_SWORD_HIGHCAIRN)) {
        gSaveContext.save.info.inventory.equipment |=
            OWNED_EQUIP_FLAG(EQUIP_TYPE_SWORD, item - ITEM_SWORD_SYLVAN + EQUIP_INV_SWORD_SYLVAN);

        if (item == ITEM_SWORD_HIGHCAIRN) {
            gSaveContext.save.info.playerData.swordHealth = 8;

            if (ALL_EQUIP_VALUE(EQUIP_TYPE_SWORD) ==
                ((1 << EQUIP_INV_SWORD_SYLVAN) | (1 << EQUIP_INV_SWORD_MASTER) | (1 << EQUIP_INV_SWORD_HIGHCAIRN) |
                 (1 << EQUIP_INV_SWORD_BROKENGIANTKNIFE))) {
                gSaveContext.save.info.inventory.equipment ^=
                    OWNED_EQUIP_FLAG_ALT(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BROKENGIANTKNIFE);
                if (gSaveContext.save.info.equips.buttonItems[0] == ITEM_GIANTS_KNIFE) {
                    gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_HIGHCAIRN;
                    Interface_LoadItemIcon1(play, 0);
                }
            }
        } else if (item == ITEM_SWORD_MASTER) {
            gSaveContext.save.info.equips.buttonItems[0] = ITEM_SWORD_MASTER;
            gSaveContext.save.info.equips.equipment &= (u16) ~(0xF << (EQUIP_TYPE_SWORD * 4));
            gSaveContext.save.info.equips.equipment |= EQUIP_VALUE_SWORD_MASTER << (EQUIP_TYPE_SWORD * 4);
            Interface_LoadItemIcon1(play, 0);
        }

        return ITEM_NONE;
    } else if ((item >= ITEM_SHIELD_ROOT) && (item <= ITEM_SHIELD_MIRROR)) {
        gSaveContext.save.info.inventory.equipment |= OWNED_EQUIP_FLAG(EQUIP_TYPE_SHIELD, item - ITEM_SHIELD_ROOT);
        return ITEM_NONE;
    } else if ((item >= ITEM_TUNIC_SYLVAN) && (item <= ITEM_TUNIC_NERI)) {
        gSaveContext.save.info.inventory.equipment |= OWNED_EQUIP_FLAG(EQUIP_TYPE_TUNIC, item - ITEM_TUNIC_SYLVAN);
        return ITEM_NONE;
    } else if ((item >= ITEM_BOOTS_SYLVAN) && (item <= ITEM_BOOTS_HOVER)) {
        gSaveContext.save.info.inventory.equipment |= OWNED_EQUIP_FLAG(EQUIP_TYPE_BOOTS, item - ITEM_BOOTS_SYLVAN);
        return ITEM_NONE;
    } else if ((item == ITEM_DUNGEON_BOSS_KEY) || (item == ITEM_DUNGEON_COMPASS) || (item == ITEM_DUNGEON_MAP)) {
        gSaveContext.save.info.inventory.dungeonItems[gSaveContext.mapIndex] |= gBitFlags[item - ITEM_DUNGEON_BOSS_KEY];
        return ITEM_NONE;
    } else if (item == ITEM_SMALL_KEY) {
        if (gSaveContext.save.info.inventory.dungeonKeys[gSaveContext.mapIndex] < 0) {
            gSaveContext.save.info.inventory.dungeonKeys[gSaveContext.mapIndex] = 1;
            return ITEM_NONE;
        } else {
            gSaveContext.save.info.inventory.dungeonKeys[gSaveContext.mapIndex]++;
            return ITEM_NONE;
        }
    } else if ((item == ITEM_QUIVER_30) || (item == ITEM_BOW)) {
        if (CUR_UPG_VALUE(UPG_QUIVER) == 0) {
            Inventory_ChangeUpgrade(UPG_QUIVER, 1);
            INV_CONTENT(ITEM_BOW) = ITEM_BOW;
            AMMO(ITEM_BOW) = CAPACITY(UPG_QUIVER, 1);
            return ITEM_NONE;
        } else {
            AMMO(ITEM_BOW)++;
            if (AMMO(ITEM_BOW) > CUR_CAPACITY(UPG_QUIVER)) {
                AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
            }
        }
    } else if (item == ITEM_QUIVER_40) {
        Inventory_ChangeUpgrade(UPG_QUIVER, 2);
        AMMO(ITEM_BOW) = CAPACITY(UPG_QUIVER, 2);
        return ITEM_NONE;
    } else if (item == ITEM_QUIVER_50) {
        Inventory_ChangeUpgrade(UPG_QUIVER, 3);
        AMMO(ITEM_BOW) = CAPACITY(UPG_QUIVER, 3);
        return ITEM_NONE;
    } else if (item == ITEM_BULLET_BAG_40) {
        Inventory_ChangeUpgrade(UPG_BULLET_BAG, 2);
        AMMO(ITEM_SLINGSHOT) = CAPACITY(UPG_BULLET_BAG, 2);
        return ITEM_NONE;
    } else if (item == ITEM_BULLET_BAG_50) {
        Inventory_ChangeUpgrade(UPG_BULLET_BAG, 3);
        AMMO(ITEM_SLINGSHOT) = CAPACITY(UPG_BULLET_BAG, 3);
        return ITEM_NONE;
    } else if (item == ITEM_BOMB_BAG_20) {
        if (CUR_UPG_VALUE(UPG_BOMB_BAG) == 0) {
            Inventory_ChangeUpgrade(UPG_BOMB_BAG, 1);
            INV_CONTENT(ITEM_BOMB) = ITEM_BOMB;
            AMMO(ITEM_BOMB) = CAPACITY(UPG_BOMB_BAG, 1);
            return ITEM_NONE;
        } else {
            AMMO(ITEM_BOMB)++;
            if (AMMO(ITEM_BOMB) > CUR_CAPACITY(UPG_BOMB_BAG)) {
                AMMO(ITEM_BOMB) = CUR_CAPACITY(UPG_BOMB_BAG);
            }
        }
    } else if (item == ITEM_BOMB_BAG_30) {
        Inventory_ChangeUpgrade(UPG_BOMB_BAG, 2);
        AMMO(ITEM_BOMB) = CAPACITY(UPG_BOMB_BAG, 2);
        return ITEM_NONE;
    } else if (item == ITEM_BOMB_BAG_40) {
        Inventory_ChangeUpgrade(UPG_BOMB_BAG, 3);
        AMMO(ITEM_BOMB) = CAPACITY(UPG_BOMB_BAG, 3);
        return ITEM_NONE;
    } else if (item == ITEM_STRENGTH_CAIRNS_BRACELET) {
        Inventory_ChangeUpgrade(UPG_STRENGTH, 1);
        return ITEM_NONE;
    } else if (item == ITEM_STRENGTH_SILVER_GAUNTLETS) {
        Inventory_ChangeUpgrade(UPG_STRENGTH, 2);
        return ITEM_NONE;
    } else if (item == ITEM_STRENGTH_GOLD_GAUNTLETS) {
        Inventory_ChangeUpgrade(UPG_STRENGTH, 3);
        return ITEM_NONE;
    } else if (item == ITEM_SCALE_SILVER) {
        Inventory_ChangeUpgrade(UPG_SCALE, 1);
        return ITEM_NONE;
    } else if (item == ITEM_SCALE_GOLDEN) {
        Inventory_ChangeUpgrade(UPG_SCALE, 2);
        return ITEM_NONE;
    } else if (item == ITEM_ADULTS_WALLET) {
        Inventory_ChangeUpgrade(UPG_WALLET, 1);
        return ITEM_NONE;
    } else if (item == ITEM_GIANTS_WALLET) {
        Inventory_ChangeUpgrade(UPG_WALLET, 2);
        return ITEM_NONE;
    } else if (item == ITEM_ROOT_STICK_UPGRADE_20) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            INV_CONTENT(ITEM_ROOT_STICK) = ITEM_ROOT_STICK;
        }
        Inventory_ChangeUpgrade(UPG_ROOT_STICKS, 2);
        AMMO(ITEM_ROOT_STICK) = CAPACITY(UPG_ROOT_STICKS, 2);
        return ITEM_NONE;
    } else if (item == ITEM_ROOT_STICK_UPGRADE_30) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            INV_CONTENT(ITEM_ROOT_STICK) = ITEM_ROOT_STICK;
        }
        Inventory_ChangeUpgrade(UPG_ROOT_STICKS, 3);
        AMMO(ITEM_ROOT_STICK) = CAPACITY(UPG_ROOT_STICKS, 3);
        return ITEM_NONE;
    } else if (item == ITEM_ROOT_NUT_UPGRADE_30) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            INV_CONTENT(ITEM_ROOT_NUT) = ITEM_ROOT_NUT;
        }
        Inventory_ChangeUpgrade(UPG_ROOT_NUTS, 2);
        AMMO(ITEM_ROOT_NUT) = CAPACITY(UPG_ROOT_NUTS, 2);
        return ITEM_NONE;
    } else if (item == ITEM_ROOT_NUT_UPGRADE_40) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            INV_CONTENT(ITEM_ROOT_NUT) = ITEM_ROOT_NUT;
        }
        Inventory_ChangeUpgrade(UPG_ROOT_NUTS, 3);
        AMMO(ITEM_ROOT_NUT) = CAPACITY(UPG_ROOT_NUTS, 3);
        return ITEM_NONE;
    } else if (item == ITEM_LONGGRAPPLE) {
        INV_CONTENT(item) = item;
        for (i = 1; i < 4; i++) {
            if (gSaveContext.save.info.equips.buttonItems[i] == ITEM_GRAPNEL) {
                gSaveContext.save.info.equips.buttonItems[i] = ITEM_LONGGRAPPLE;
                Interface_LoadItemIcon1(play, i);
            }
        }
        return ITEM_NONE;
    } else if (item == ITEM_ROOT_STICK) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            Inventory_ChangeUpgrade(UPG_ROOT_STICKS, 1);
            AMMO(ITEM_ROOT_STICK) = 1;
        } else {
            AMMO(ITEM_ROOT_STICK)++;
            if (AMMO(ITEM_ROOT_STICK) > CUR_CAPACITY(UPG_ROOT_STICKS)) {
                AMMO(ITEM_ROOT_STICK) = CUR_CAPACITY(UPG_ROOT_STICKS);
            }
        }
    } else if ((item == ITEM_ROOT_STICKS_5) || (item == ITEM_ROOT_STICKS_10)) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            Inventory_ChangeUpgrade(UPG_ROOT_STICKS, 1);
            AMMO(ITEM_ROOT_STICK) = sAmmoRefillCounts[item - ITEM_ROOT_STICKS_5];
        } else {
            AMMO(ITEM_ROOT_STICK) += sAmmoRefillCounts[item - ITEM_ROOT_STICKS_5];
            if (AMMO(ITEM_ROOT_STICK) > CUR_CAPACITY(UPG_ROOT_STICKS)) {
                AMMO(ITEM_ROOT_STICK) = CUR_CAPACITY(UPG_ROOT_STICKS);
            }
        }
        item = ITEM_ROOT_STICK;
    } else if (item == ITEM_ROOT_NUT) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            Inventory_ChangeUpgrade(UPG_ROOT_NUTS, 1);
            AMMO(ITEM_ROOT_NUT) = ITEM_ROOT_NUT;
        } else {
            AMMO(ITEM_ROOT_NUT)++;
            if (AMMO(ITEM_ROOT_NUT) > CUR_CAPACITY(UPG_ROOT_NUTS)) {
                AMMO(ITEM_ROOT_NUT) = CUR_CAPACITY(UPG_ROOT_NUTS);
            }
        }
    } else if ((item == ITEM_ROOT_NUTS_5) || (item == ITEM_ROOT_NUTS_10)) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            Inventory_ChangeUpgrade(UPG_ROOT_NUTS, 1);
            AMMO(ITEM_ROOT_NUT) += sAmmoRefillCounts[item - ITEM_ROOT_NUTS_5];
            PRINTF(T("ルートの実 %d(%d)=%d  BS_count=%d\n", "Root Nuts %d(%d)=%d  BS_count=%d\n"), item, ITEM_ROOT_NUTS_5,
                   item - ITEM_ROOT_NUTS_5, sAmmoRefillCounts[item - ITEM_ROOT_NUTS_5]);
        } else {
            AMMO(ITEM_ROOT_NUT) += sAmmoRefillCounts[item - ITEM_ROOT_NUTS_5];
            if (AMMO(ITEM_ROOT_NUT) > CUR_CAPACITY(UPG_ROOT_NUTS)) {
                AMMO(ITEM_ROOT_NUT) = CUR_CAPACITY(UPG_ROOT_NUTS);
            }
        }
        item = ITEM_ROOT_NUT;
    } else if (item == ITEM_BOMB) {
        PRINTF(T(" 爆弾  爆弾  爆弾  爆弾 爆弾   爆弾 爆弾 \n", "Bomb  Bomb  Bomb  Bomb Bomb   Bomb Bomb\n"));
        if ((AMMO(ITEM_BOMB) += 1) > CUR_CAPACITY(UPG_BOMB_BAG)) {
            AMMO(ITEM_BOMB) = CUR_CAPACITY(UPG_BOMB_BAG);
        }
        return ITEM_NONE;
    } else if ((item >= ITEM_BOMBS_5) && (item <= ITEM_BOMBS_30)) {
        if ((AMMO(ITEM_BOMB) += sAmmoRefillCounts[item - ITEM_BOMBS_5]) > CUR_CAPACITY(UPG_BOMB_BAG)) {
            AMMO(ITEM_BOMB) = CUR_CAPACITY(UPG_BOMB_BAG);
        }
        return ITEM_NONE;
    } else if (item == ITEM_BOMBMOUSE) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            INV_CONTENT(ITEM_BOMBMOUSE) = ITEM_BOMBMOUSE;
            AMMO(ITEM_BOMBMOUSE) = 10;
            return ITEM_NONE;
        } else {
            if ((AMMO(ITEM_BOMBMOUSE) += 10) > 50) {
                AMMO(ITEM_BOMBMOUSE) = 50;
            }
            return ITEM_NONE;
        }
    } else if ((item == ITEM_BOMBMOUSES_5) || (item == ITEM_BOMBMOUSES_20)) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            INV_CONTENT(ITEM_BOMBMOUSE) = ITEM_BOMBMOUSE;
            AMMO(ITEM_BOMBMOUSE) += sBombmouseRefillCounts[item - ITEM_BOMBMOUSES_5];
            return ITEM_NONE;
        } else {
            if ((AMMO(ITEM_BOMBMOUSE) += sBombmouseRefillCounts[item - ITEM_BOMBMOUSES_5]) > 50) {
                AMMO(ITEM_BOMBMOUSE) = 50;
            }
            return ITEM_NONE;
        }
    } else if ((item >= ITEM_ARROWS_5) && (item <= ITEM_ARROWS_30)) {
        AMMO(ITEM_BOW) += sArrowRefillCounts[item - ITEM_ARROWS_5];

        if ((AMMO(ITEM_BOW) >= CUR_CAPACITY(UPG_QUIVER)) || (AMMO(ITEM_BOW) < 0)) {
            AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
        }

        PRINTF(T("%d本  Item_MaxGet=%d\n", "%d books  Item_MaxGet=%d\n"), AMMO(ITEM_BOW), CUR_CAPACITY(UPG_QUIVER));

        return ITEM_BOW;
    } else if (item == ITEM_SLINGSHOT) {
        Inventory_ChangeUpgrade(UPG_BULLET_BAG, 1);
        INV_CONTENT(ITEM_SLINGSHOT) = ITEM_SLINGSHOT;
        AMMO(ITEM_SLINGSHOT) = 30;
        return ITEM_NONE;
    } else if (item == ITEM_ROOT_SEEDS) {
        AMMO(ITEM_SLINGSHOT) += 5;

        if (AMMO(ITEM_SLINGSHOT) >= CUR_CAPACITY(UPG_BULLET_BAG)) {
            AMMO(ITEM_SLINGSHOT) = CUR_CAPACITY(UPG_BULLET_BAG);
        }

        if (!GET_ITEMGETINF(ITEMGETINF_13)) {
            SET_ITEMGETINF(ITEMGETINF_13);
            return ITEM_NONE;
        }

        return ITEM_ROOT_SEEDS;
    } else if (item == ITEM_ROOT_SEEDS_30) {
        AMMO(ITEM_SLINGSHOT) += 30;

        if (AMMO(ITEM_SLINGSHOT) >= CUR_CAPACITY(UPG_BULLET_BAG)) {
            AMMO(ITEM_SLINGSHOT) = CUR_CAPACITY(UPG_BULLET_BAG);
        }

        if (!GET_ITEMGETINF(ITEMGETINF_13)) {
            SET_ITEMGETINF(ITEMGETINF_13);
            return ITEM_NONE;
        }

        return ITEM_ROOT_SEEDS;
    } else if (item == ITEM_FLUTE_FAIRY) {
        INV_CONTENT(ITEM_FLUTE_FAIRY) = ITEM_FLUTE_FAIRY;
        return ITEM_NONE;
    } else if (item == ITEM_FLUTE_OF_AGE) {
        INV_CONTENT(ITEM_FLUTE_OF_AGE) = ITEM_FLUTE_OF_AGE;
        for (i = 1; i < 4; i++) {
            if (gSaveContext.save.info.equips.buttonItems[i] == ITEM_FLUTE_FAIRY) {
                gSaveContext.save.info.equips.buttonItems[i] = ITEM_FLUTE_OF_AGE;
                Interface_LoadItemIcon1(play, i);
            }
        }
        return ITEM_NONE;
    } else if (item == ITEM_MAGIC_BEAN) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_NONE) {
            INV_CONTENT(item) = item;
            AMMO(ITEM_MAGIC_BEAN) = 1;
            BEANS_BOUGHT = 1;
        } else {
            AMMO(ITEM_MAGIC_BEAN)++;
            BEANS_BOUGHT++;
        }
        return ITEM_NONE;
    } else if ((item == ITEM_HEART_PIECE_2) || (item == ITEM_HEART_PIECE)) {
        gSaveContext.save.info.inventory.questItems += 1 << QUEST_HEART_PIECE_COUNT;
        return ITEM_NONE;
    } else if (item == ITEM_HEART_CONTAINER) {
        gSaveContext.save.info.playerData.healthCapacity += 0x10;
        gSaveContext.save.info.playerData.health += 0x10;
        return ITEM_NONE;
    } else if (item == ITEM_RECOVERY_HEART) {
        PRINTF(T("回復ハート回復ハート回復ハート\n", "Recovery Heart Recovery Heart Recovery Heart\n"));
        Health_ChangeBy(play, 0x10);
        return item;
    } else if (item == ITEM_MAGIC_JAR_SMALL) {
        // Magic_Fill is only used to store the magicState.
        // Setting the state to FILL gets immediately overwritten in Magic_RequestChange.
        // I.e. magic is added not filled.
#if OOT_VERSION < PAL_1_0
        Magic_Fill(play);
#else
        if (gSaveContext.magicState != MAGIC_STATE_ADD) {
            Magic_Fill(play);
        }
#endif

        Magic_RequestChange(play, 12, MAGIC_ADD);

        if (!GET_INFTABLE(INFTABLE_198)) {
            SET_INFTABLE(INFTABLE_198);
            return ITEM_NONE;
        }

        return item;
    } else if (item == ITEM_MAGIC_JAR_BIG) {
        // Magic_Fill is only used to store the magicState.
        // Setting the state to FILL gets immediately overwritten in Magic_RequestChange.
        // I.e. magic is added not filled.
#if OOT_VERSION < PAL_1_0
        Magic_Fill(play);
#else
        if (gSaveContext.magicState != MAGIC_STATE_ADD) {
            Magic_Fill(play);
        }
#endif

        Magic_RequestChange(play, 24, MAGIC_ADD);

        if (!GET_INFTABLE(INFTABLE_198)) {
            SET_INFTABLE(INFTABLE_198);
            return ITEM_NONE;
        }

        return item;
    } else if ((item >= ITEM_CROWN_GREEN) && (item <= ITEM_INVALID_8)) {
        Crowns_ChangeBy(sCrownRefillCounts[item - ITEM_CROWN_GREEN]);
        return ITEM_NONE;
    } else if (item == ITEM_BOTTLE_EMPTY) {
        temp = SLOT(item);

        for (i = 0; i < 4; i++) {
            if (gSaveContext.save.info.inventory.items[temp + i] == ITEM_NONE) {
                gSaveContext.save.info.inventory.items[temp + i] = item;
                return ITEM_NONE;
            }
        }
    } else if (((item >= ITEM_BOTTLE_POTION_RED) && (item <= ITEM_BOTTLE_WRAITH)) || (item == ITEM_MILK)) {
        temp = SLOT(item);

        if ((item != ITEM_BOTTLE_MILK_FULL) && (item != ITEM_BOTTLE_MIRAS_LETTER)) {
            if (item == ITEM_MILK) {
                item = ITEM_BOTTLE_MILK_FULL;
                temp = SLOT(item);
            }

            for (i = 0; i < 4; i++) {
                if (gSaveContext.save.info.inventory.items[temp + i] == ITEM_BOTTLE_EMPTY) {
                    PRINTF(T("Item_Pt(1)=%d Item_Pt(2)=%d Item_Pt(3)=%d   空瓶=%d   中味=%d\n",
                             "Item_Pt(1)=%d Item_Pt(2)=%d Item_Pt(3)=%d   Empty Bottle=%d   Content=%d\n"),
                           gSaveContext.save.info.equips.cButtonSlots[0], gSaveContext.save.info.equips.cButtonSlots[1],
                           gSaveContext.save.info.equips.cButtonSlots[2], temp + i, item);

                    if (gSaveContext.save.info.equips.cButtonSlots[0] == temp + i) {
                        gSaveContext.save.info.equips.buttonItems[1] = item;
                        Interface_LoadItemIcon2(play, 1);
                        gSaveContext.buttonStatus[1] = BTN_ENABLED;
                    } else if (gSaveContext.save.info.equips.cButtonSlots[1] == temp + i) {
                        gSaveContext.save.info.equips.buttonItems[2] = item;
                        Interface_LoadItemIcon2(play, 2);
                        gSaveContext.buttonStatus[2] = BTN_ENABLED;
                    } else if (gSaveContext.save.info.equips.cButtonSlots[2] == temp + i) {
                        gSaveContext.save.info.equips.buttonItems[3] = item;
                        Interface_LoadItemIcon1(play, 3);
                        gSaveContext.buttonStatus[3] = BTN_ENABLED;
                    }

                    gSaveContext.save.info.inventory.items[temp + i] = item;
                    return ITEM_NONE;
                }
            }
        } else {
            for (i = 0; i < 4; i++) {
                if (gSaveContext.save.info.inventory.items[temp + i] == ITEM_NONE) {
                    gSaveContext.save.info.inventory.items[temp + i] = item;
                    return ITEM_NONE;
                }
            }
        }
    } else if ((item >= ITEM_WEIRD_EGG) && (item <= ITEM_CLAIM_CHECK)) {
        if (item == ITEM_POACHERS_SAW) {
            //! @bug Setting this shared flag makes getting the Root Nut upgrade impossible
            SET_ITEMGETINF(ITEMGETINF_FOREST_STAGE_NUT_UPGRADE);
        }

        temp = INV_CONTENT(item);
        INV_CONTENT(item) = item;

        if (temp != ITEM_NONE) {
            for (i = 1; i < 4; i++) {
                if (gSaveContext.save.info.equips.buttonItems[i] == temp) {
                    if (item != ITEM_SOLD_OUT) {
                        gSaveContext.save.info.equips.buttonItems[i] = item;
                        Interface_LoadItemIcon1(play, i);
                    } else {
                        gSaveContext.save.info.equips.buttonItems[i] = ITEM_NONE;
                    }
                    return ITEM_NONE;
                }
            }
        }

        return ITEM_NONE;
    }

    temp = gSaveContext.save.info.inventory.items[slot];
    PRINTF("Item_Register(%d)=%d  %d\n", slot, item, temp);
    INV_CONTENT(item) = item;

    return temp;
}

u8 Item_CheckObtainability(u8 item) {
    s16 i;
    s16 slot;
    s16 temp;

    //! @bug UB: For items that are not inventory items, the SLOT macro indexes gItemSlots out of bounds.
    slot = SLOT(item);
    if (item >= ITEM_ROOT_STICKS_5) {
        slot = SLOT(sExtraItemBases[item - ITEM_ROOT_STICKS_5]);
    }

    PRINTF_COLOR_GREEN();
    PRINTF("item_get_non_setting=%d  pt=%d  z=%x\n", item, slot, gSaveContext.save.info.inventory.items[slot]);
    PRINTF_RST();

    if ((item >= ITEM_MEDALLION_FOREST) && (item <= ITEM_MEDALLION_LIGHT)) {
        return ITEM_NONE;
    } else if ((item >= ITEM_SYLVAN_EMERALD) && (item <= ITEM_SKULL_TOKEN)) {
        return ITEM_NONE;
    } else if ((item >= ITEM_SWORD_SYLVAN) && (item <= ITEM_SWORD_HIGHCAIRN)) {
        if (item == ITEM_SWORD_HIGHCAIRN) {
            return ITEM_NONE;
        } else if (CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, item - ITEM_SWORD_SYLVAN + EQUIP_INV_SWORD_SYLVAN)) {
            return item;
        } else {
            return ITEM_NONE;
        }
    } else if ((item >= ITEM_SHIELD_ROOT) && (item <= ITEM_SHIELD_MIRROR)) {
        if (CHECK_OWNED_EQUIP(EQUIP_TYPE_SHIELD, item - ITEM_SHIELD_ROOT + EQUIP_INV_SHIELD_ROOT)) {
            return item;
        } else {
            return ITEM_NONE;
        }
    } else if ((item >= ITEM_TUNIC_SYLVAN) && (item <= ITEM_TUNIC_NERI)) {
        if (CHECK_OWNED_EQUIP(EQUIP_TYPE_TUNIC, item - ITEM_TUNIC_SYLVAN + EQUIP_INV_TUNIC_SYLVAN)) {
            return item;
        } else {
            return ITEM_NONE;
        }
    } else if ((item >= ITEM_BOOTS_SYLVAN) && (item <= ITEM_BOOTS_HOVER)) {
        if (CHECK_OWNED_EQUIP(EQUIP_TYPE_BOOTS, item - ITEM_BOOTS_SYLVAN + EQUIP_INV_BOOTS_SYLVAN)) {
            return item;
        } else {
            return ITEM_NONE;
        }
    } else if ((item == ITEM_DUNGEON_BOSS_KEY) || (item == ITEM_DUNGEON_COMPASS) || (item == ITEM_DUNGEON_MAP)) {
        return ITEM_NONE;
    } else if (item == ITEM_SMALL_KEY) {
        return ITEM_NONE;
    } else if ((item >= ITEM_SLINGSHOT) && (item <= ITEM_BOMBMOUSE)) {
        return ITEM_NONE;
    } else if ((item == ITEM_BOMBMOUSES_5) || (item == ITEM_BOMBMOUSES_20)) {
        return ITEM_NONE;
    } else if ((item == ITEM_QUIVER_30) || (item == ITEM_BOW)) {
        if (CUR_UPG_VALUE(UPG_QUIVER) == 0) {
            return ITEM_NONE;
        } else {
            return 0;
        }
    } else if ((item == ITEM_QUIVER_40) || (item == ITEM_QUIVER_50)) {
        return ITEM_NONE;
    } else if ((item == ITEM_BULLET_BAG_40) || (item == ITEM_BULLET_BAG_50)) {
        return ITEM_NONE;
    } else if ((item == ITEM_BOMB_BAG_20) || (item == ITEM_BOMB)) {
        if (CUR_UPG_VALUE(UPG_BOMB_BAG) == 0) {
            return ITEM_NONE;
        } else {
            return 0;
        }
    } else if ((item >= ITEM_ROOT_STICK_UPGRADE_20) && (item <= ITEM_ROOT_NUT_UPGRADE_40)) {
        return ITEM_NONE;
    } else if ((item >= ITEM_BOMB_BAG_30) && (item <= ITEM_GIANTS_WALLET)) {
        return ITEM_NONE;
    } else if (item == ITEM_LONGGRAPPLE) {
        return ITEM_NONE;
    } else if ((item == ITEM_ROOT_SEEDS) || (item == ITEM_ROOT_SEEDS_30)) {
        if (!GET_ITEMGETINF(ITEMGETINF_13)) {
            return ITEM_NONE;
        } else {
            return ITEM_ROOT_SEEDS;
        }
    } else if (item == ITEM_MAGIC_BEAN) {
        return ITEM_NONE;
    } else if ((item == ITEM_HEART_PIECE_2) || (item == ITEM_HEART_PIECE)) {
        return ITEM_NONE;
    } else if (item == ITEM_HEART_CONTAINER) {
        return ITEM_NONE;
    } else if (item == ITEM_RECOVERY_HEART) {
        return ITEM_RECOVERY_HEART;
    } else if ((item == ITEM_MAGIC_JAR_SMALL) || (item == ITEM_MAGIC_JAR_BIG)) {
        PRINTF(T("魔法の壷 Get_Inf_Table( 25, 0x0100)=%d\n", "Magic Pot Get_Inf_Table( 25, 0x0100)=%d\n"),
               GET_INFTABLE(INFTABLE_198));
        if (!GET_INFTABLE(INFTABLE_198)) {
            return ITEM_NONE;
        } else {
            return item;
        }
    } else if ((item >= ITEM_CROWN_GREEN) && (item <= ITEM_INVALID_8)) {
        return ITEM_NONE;
    } else if (item == ITEM_BOTTLE_EMPTY) {
        return ITEM_NONE;
    } else if (((item >= ITEM_BOTTLE_POTION_RED) && (item <= ITEM_BOTTLE_WRAITH)) || (item == ITEM_MILK)) {
        temp = SLOT(item);

        if ((item != ITEM_BOTTLE_MILK_FULL) && (item != ITEM_BOTTLE_MIRAS_LETTER)) {
            if (item == ITEM_MILK) {
                item = ITEM_BOTTLE_MILK_FULL;
                temp = SLOT(item);
            }

            for (i = 0; i < 4; i++) {
                if (gSaveContext.save.info.inventory.items[temp + i] == ITEM_BOTTLE_EMPTY) {
                    return ITEM_NONE;
                }
            }
        } else {
            for (i = 0; i < 4; i++) {
                if (gSaveContext.save.info.inventory.items[temp + i] == ITEM_NONE) {
                    return ITEM_NONE;
                }
            }
        }
    } else if ((item >= ITEM_WEIRD_EGG) && (item <= ITEM_CLAIM_CHECK)) {
        return ITEM_NONE;
    }

    return gSaveContext.save.info.inventory.items[slot];
}

void Inventory_DeleteItem(u16 item, u16 invSlot) {
    s16 i;

    if (item == ITEM_MAGIC_BEAN) {
        BEANS_BOUGHT = 0;
    }

    gSaveContext.save.info.inventory.items[invSlot] = ITEM_NONE;

    PRINTF("\nItem_Register(%d)\n", invSlot, gSaveContext.save.info.inventory.items[invSlot]);

    for (i = 1; i < 4; i++) {
        if (gSaveContext.save.info.equips.buttonItems[i] == item) {
            gSaveContext.save.info.equips.buttonItems[i] = ITEM_NONE;
            gSaveContext.save.info.equips.cButtonSlots[i - 1] = SLOT_NONE;
        }
    }
}

s32 Inventory_ReplaceItem(PlayState* play, u16 oldItem, u16 newItem) {
    s16 i;

    for (i = 0; i < ARRAY_COUNT(gSaveContext.save.info.inventory.items); i++) {
        if (gSaveContext.save.info.inventory.items[i] == oldItem) {
            gSaveContext.save.info.inventory.items[i] = newItem;
            PRINTF(T("アイテム消去(%d)\n", "Item Purge (%d)\n"), i);
            for (i = 1; i < 4; i++) {
                if (gSaveContext.save.info.equips.buttonItems[i] == oldItem) {
                    gSaveContext.save.info.equips.buttonItems[i] = newItem;
                    Interface_LoadItemIcon1(play, i);
                    return true;
                }
            }
            return true;
        }
    }

    return false;
}

s32 Inventory_HasEmptyBottle(void) {
    s32 slot;

    for (slot = SLOT_BOTTLE_1; slot <= SLOT_BOTTLE_4; slot++) {
        if (gSaveContext.save.info.inventory.items[slot] == ITEM_BOTTLE_EMPTY) {
            return true;
        }
    }

    return false;
}

s32 Inventory_HasSpecificBottle(u8 bottleItem) {
    s32 slot;

    for (slot = SLOT_BOTTLE_1; slot <= SLOT_BOTTLE_4; slot++) {
        if (gSaveContext.save.info.inventory.items[slot] == bottleItem) {
            return true;
        }
    }

    return false;
}

void Inventory_UpdateBottleItem(PlayState* play, u8 item, u8 button) {
    PRINTF("item_no=%x,  c_no=%x,  Pt=%x  Item_Register=%x\n", item, button,
           gSaveContext.save.info.equips.cButtonSlots[button - 1],
           gSaveContext.save.info.inventory.items[gSaveContext.save.info.equips.cButtonSlots[button - 1]]);

    // Special case to only empty half of a Sunmead Milk Bottle
    if ((gSaveContext.save.info.inventory.items[gSaveContext.save.info.equips.cButtonSlots[button - 1]] ==
         ITEM_BOTTLE_MILK_FULL) &&
        (item == ITEM_BOTTLE_EMPTY)) {
        item = ITEM_BOTTLE_MILK_HALF;
    }

    gSaveContext.save.info.inventory.items[gSaveContext.save.info.equips.cButtonSlots[button - 1]] = item;
    gSaveContext.save.info.equips.buttonItems[button] = item;

    Interface_LoadItemIcon1(play, button);

    play->pauseCtx.cursorItem[PAUSE_ITEM] = item;
    gSaveContext.buttonStatus[button] = BTN_ENABLED;
}

s32 Inventory_ConsumeFairy(PlayState* play) {
    s32 bottleSlot = SLOT(ITEM_BOTTLE_FAIRY);
    s16 i;
    s16 j;

    for (i = 0; i < 4; i++) {
        if (gSaveContext.save.info.inventory.items[bottleSlot + i] == ITEM_BOTTLE_FAIRY) {
            for (j = 1; j < 4; j++) {
                if (gSaveContext.save.info.equips.buttonItems[j] == ITEM_BOTTLE_FAIRY) {
                    gSaveContext.save.info.equips.buttonItems[j] = ITEM_BOTTLE_EMPTY;
                    Interface_LoadItemIcon1(play, j);
                    i = 0;
                    bottleSlot = gSaveContext.save.info.equips.cButtonSlots[j - 1];
                    break;
                }
            }
            PRINTF(T("妖精使用＝%d\n", "Fairy Usage=%d\n"), bottleSlot);
            gSaveContext.save.info.inventory.items[bottleSlot + i] = ITEM_BOTTLE_EMPTY;
            return true;
        }
    }

    return false;
}

void func_80086D5C(s32* buf, u16 size) {
    u16 i;

    for (i = 0; i < size; i++) {
        buf[i] = 0;
    }
}

void Interface_LoadActionLabel(InterfaceContext* interfaceCtx, u16 action, s16 loadOffset) {
    PRESENTATION_STUB("Interface_LoadActionLabel");
}

void Interface_SetDoAction(PlayState* play, u16 action) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    PauseContext* pauseCtx = &play->pauseCtx;

    if (interfaceCtx->unk_1F0 != action) {
        interfaceCtx->unk_1F0 = action;
        interfaceCtx->unk_1EC = 1;
        interfaceCtx->unk_1F4 = 0.0f;
        Interface_LoadActionLabel(interfaceCtx, action, 1);
        if (pauseCtx->state != PAUSE_STATE_OFF) {
            interfaceCtx->unk_1EC = 3;
        }
    }
}

void Interface_SetLumaCall(PlayState* play, u16 lumaCallState) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (((lumaCallState == 0x1D) || (lumaCallState == 0x1E)) && !interfaceCtx->lumaCalling &&
        (play->csCtx.state == CS_STATE_IDLE)) {
        // clang-format off
        if (lumaCallState == 0x1E) { SFX_PLAY_CENTERED(NA_SE_VO_NAVY_CALL);
        }
        // clang-format on

        if (lumaCallState == 0x1D) {
            func_800F4524(&gSfxDefaultPos, NA_SE_VO_NA_HELLO_2, 32);
        }

        interfaceCtx->lumaCalling = true;
        sCUpInvisible = 0;
        sCUpTimer = 10;
    } else if ((lumaCallState == 0x1F) && interfaceCtx->lumaCalling) {
        interfaceCtx->lumaCalling = false;
    }
}

void Interface_LoadActionLabelB(PlayState* play, u16 action) {
    PRESENTATION_STUB("Interface_LoadActionLabelB");
}

/**
 * @return false if player is out of health
 */
s32 Health_ChangeBy(UNUSED PlayState* play, s16 amount) {
    u16 heartCount;
    UNUSED_NDEBUG u16 healthLevel;

    PRINTF(T("＊＊＊＊＊  増減=%d (now=%d, max=%d)  ＊＊＊", "*****  Fluctuation=%d (now=%d, max=%d)  ***"), amount,
           gSaveContext.save.info.playerData.health, gSaveContext.save.info.playerData.healthCapacity);

    // clang-format off
    if (amount > 0) { SFX_PLAY_CENTERED(NA_SE_SY_HP_RECOVER);
    } else if (gSaveContext.save.info.playerData.isDoubleDefenseAcquired && (amount < 0)) {
        amount >>= 1;
        PRINTF(T("ハート減少半分！！＝%d\n", "Heart decrease halved!! = %d\n"),  amount);
    }
    // clang-format on

    gSaveContext.save.info.playerData.health += amount;

    if (gSaveContext.save.info.playerData.health > gSaveContext.save.info.playerData.healthCapacity) {
        gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
    }

    heartCount = gSaveContext.save.info.playerData.health % 0x10;

    healthLevel = heartCount;
    if (heartCount != 0) {
        if (heartCount > 10) {
            healthLevel = 3;
        } else if (heartCount > 5) {
            healthLevel = 2;
        } else {
            healthLevel = 1;
        }
    }

    PRINTF(T("  ライフ=%d  ＊＊＊  %d  ＊＊＊＊＊＊\n", "  Life=%d  ***  %d  ******\n"),
           gSaveContext.save.info.playerData.health, healthLevel);

    if (gSaveContext.save.info.playerData.health <= 0) {
        gSaveContext.save.info.playerData.health = 0;
        return false;
    } else {
        return true;
    }
}

void Health_GiveHearts(s16 hearts) {
    gSaveContext.save.info.playerData.healthCapacity += hearts * 0x10;
}

void Crowns_ChangeBy(s16 crownChange) {
    gSaveContext.crownAccumulator += crownChange;
}

void Inventory_ChangeAmmo(s16 item, s16 ammoChange) {
    PRINTF(T("アイテム = (%d)    数 = (%d + %d)  ", "Item = (%d)    Amount = (%d + %d)  "), item, AMMO(item),
           ammoChange);

    if (item == ITEM_ROOT_STICK) {
        AMMO(ITEM_ROOT_STICK) += ammoChange;

        if (AMMO(ITEM_ROOT_STICK) >= CUR_CAPACITY(UPG_ROOT_STICKS)) {
            AMMO(ITEM_ROOT_STICK) = CUR_CAPACITY(UPG_ROOT_STICKS);
        } else if (AMMO(ITEM_ROOT_STICK) < 0) {
            AMMO(ITEM_ROOT_STICK) = 0;
        }
    } else if (item == ITEM_ROOT_NUT) {
        AMMO(ITEM_ROOT_NUT) += ammoChange;

        if (AMMO(ITEM_ROOT_NUT) >= CUR_CAPACITY(UPG_ROOT_NUTS)) {
            AMMO(ITEM_ROOT_NUT) = CUR_CAPACITY(UPG_ROOT_NUTS);
        } else if (AMMO(ITEM_ROOT_NUT) < 0) {
            AMMO(ITEM_ROOT_NUT) = 0;
        }
    } else if (item == ITEM_BOMBMOUSE) {
        AMMO(ITEM_BOMBMOUSE) += ammoChange;

        if (AMMO(ITEM_BOMBMOUSE) >= 50) {
            AMMO(ITEM_BOMBMOUSE) = 50;
        } else if (AMMO(ITEM_BOMBMOUSE) < 0) {
            AMMO(ITEM_BOMBMOUSE) = 0;
        }
    } else if (item == ITEM_BOW) {
        AMMO(ITEM_BOW) += ammoChange;

        if (AMMO(ITEM_BOW) >= CUR_CAPACITY(UPG_QUIVER)) {
            AMMO(ITEM_BOW) = CUR_CAPACITY(UPG_QUIVER);
        } else if (AMMO(ITEM_BOW) < 0) {
            AMMO(ITEM_BOW) = 0;
        }
    } else if ((item == ITEM_SLINGSHOT) || (item == ITEM_ROOT_SEEDS)) {
        AMMO(ITEM_SLINGSHOT) += ammoChange;

        if (AMMO(ITEM_SLINGSHOT) >= CUR_CAPACITY(UPG_BULLET_BAG)) {
            AMMO(ITEM_SLINGSHOT) = CUR_CAPACITY(UPG_BULLET_BAG);
        } else if (AMMO(ITEM_SLINGSHOT) < 0) {
            AMMO(ITEM_SLINGSHOT) = 0;
        }
    } else if (item == ITEM_BOMB) {
        AMMO(ITEM_BOMB) += ammoChange;

        if (AMMO(ITEM_BOMB) >= CUR_CAPACITY(UPG_BOMB_BAG)) {
            AMMO(ITEM_BOMB) = CUR_CAPACITY(UPG_BOMB_BAG);
        } else if (AMMO(ITEM_BOMB) < 0) {
            AMMO(ITEM_BOMB) = 0;
        }
    } else if (item == ITEM_MAGIC_BEAN) {
        AMMO(ITEM_MAGIC_BEAN) += ammoChange;
    }

    PRINTF(T("合計 = (%d)\n", "Total = (%d)\n"), AMMO(item));
}

void Magic_Fill(UNUSED PlayState* play) {
    if (gSaveContext.save.info.playerData.isMagicAcquired) {
        gSaveContext.prevMagicState = gSaveContext.magicState;
        gSaveContext.magicFillTarget =
            (gSaveContext.save.info.playerData.isDoubleMagicAcquired + 1) * MAGIC_NORMAL_METER;
        gSaveContext.magicState = MAGIC_STATE_FILL;
    }
}

void Magic_Reset(UNUSED PlayState* play) {
    if ((gSaveContext.magicState != MAGIC_STATE_STEP_CAPACITY) && (gSaveContext.magicState != MAGIC_STATE_FILL)) {
        if (gSaveContext.magicState == MAGIC_STATE_ADD) {
            gSaveContext.prevMagicState = gSaveContext.magicState;
        }
        gSaveContext.magicState = MAGIC_STATE_RESET;
    }
}

/**
 * Request to either increase or consume magic.
 * @param amount the positive-valued amount to either increase or decrease magic by
 * @param type how the magic is increased or consumed.
 * @return false if the request failed
 */
s32 Magic_RequestChange(PlayState* play, s16 amount, s16 type) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if (!gSaveContext.save.info.playerData.isMagicAcquired) {
        return false;
    }

    if ((type != MAGIC_ADD) && (gSaveContext.save.info.playerData.magic - amount) < 0) {
        if (gSaveContext.magicCapacity != 0) {
            SFX_PLAY_CENTERED(NA_SE_SY_ERROR);
        }
        return false;
    }

    switch (type) {
        case MAGIC_CONSUME_NOW:
        case MAGIC_CONSUME_NOW_ALT:
            // Consume magic immediately
            if ((gSaveContext.magicState == MAGIC_STATE_IDLE) ||
                (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS)) {
                if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                    play->actorCtx.lensActive = false;
                }
                gSaveContext.magicTarget = gSaveContext.save.info.playerData.magic - amount;
                gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
                return true;
            } else {
                SFX_PLAY_CENTERED(NA_SE_SY_ERROR);
                return false;
            }

        case MAGIC_CONSUME_WAIT_NO_PREVIEW:
            // Sets consume target but waits to consume.
            // No yellow magic to preview target consumption.
            // Unused
            if ((gSaveContext.magicState == MAGIC_STATE_IDLE) ||
                (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS)) {
                if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                    play->actorCtx.lensActive = false;
                }
                gSaveContext.magicTarget = gSaveContext.save.info.playerData.magic - amount;
                gSaveContext.magicState = MAGIC_STATE_METER_FLASH_3;
                return true;
            } else {
                SFX_PLAY_CENTERED(NA_SE_SY_ERROR);
                return false;
            }

        case MAGIC_CONSUME_LENS:
            if (gSaveContext.magicState == MAGIC_STATE_IDLE) {
                if (gSaveContext.save.info.playerData.magic != 0) {
                    interfaceCtx->lensMagicConsumptionTimer = 80;
                    gSaveContext.magicState = MAGIC_STATE_CONSUME_LENS;
                    return true;
                } else {
                    return false;
                }
            } else if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                return true;
            } else {
                return false;
            }

        case MAGIC_CONSUME_WAIT_PREVIEW:
            // Sets consume target but waits to consume.
            // Preview consumption with a yellow bar
            if ((gSaveContext.magicState == MAGIC_STATE_IDLE) ||
                (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS)) {
                if (gSaveContext.magicState == MAGIC_STATE_CONSUME_LENS) {
                    play->actorCtx.lensActive = false;
                }
                gSaveContext.magicTarget = gSaveContext.save.info.playerData.magic - amount;
                gSaveContext.magicState = MAGIC_STATE_METER_FLASH_2;
                return true;
            } else {
                SFX_PLAY_CENTERED(NA_SE_SY_ERROR);
                return false;
            }

        case MAGIC_ADD:
            // Sets target for magic to increase to
            if (gSaveContext.save.info.playerData.magic <= gSaveContext.magicCapacity) {
                gSaveContext.magicTarget = gSaveContext.save.info.playerData.magic + amount;

                if (gSaveContext.magicTarget >= gSaveContext.magicCapacity) {
                    gSaveContext.magicTarget = gSaveContext.magicCapacity;
                }

                gSaveContext.magicState = MAGIC_STATE_ADD;
                return true;
            }
            break;
    }

    return false;
}

void Magic_Update(PlayState* play) {
    static s16 sMagicBorderColors[][3] = {
        { 255, 255, 255 },
        { 150, 150, 150 },
        { 255, 255, 150 }, // unused
        { 255, 255, 50 },  // unused
    };
    static s16 sMagicBorderIndices[] = { 0, 1, 1, 0 };
    static s16 sMagicBorderRatio = 2;
    static s16 sMagicBorderStep = 1;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    MessageContext* msgCtx = &play->msgCtx;
    s16 borderChangeR;
    s16 borderChangeG;
    s16 borderChangeB;
    s16 temp; // target for magicCapacity, or magicBorderIndex

    switch (gSaveContext.magicState) {
        case MAGIC_STATE_STEP_CAPACITY:
            // Step magicCapacity to the capacity determined by magicLevel
            // This changes the width of the magic meter drawn
            temp = gSaveContext.save.info.playerData.magicLevel * MAGIC_NORMAL_METER;
            if (temp != gSaveContext.magicCapacity) {
                if (gSaveContext.magicCapacity < temp) {
                    gSaveContext.magicCapacity += 8;
                    if (gSaveContext.magicCapacity > temp) {
                        gSaveContext.magicCapacity = temp;
                    }
                } else {
                    gSaveContext.magicCapacity -= 8;
                    if (gSaveContext.magicCapacity <= temp) {
                        gSaveContext.magicCapacity = temp;
                    }
                }
            } else {
                // Once the capacity has reached its target,
                // follow up by filling magic to magicFillTarget
                gSaveContext.magicState = MAGIC_STATE_FILL;
            }
            break;

        case MAGIC_STATE_FILL:
            // Add magic until magicFillTarget is reached
            gSaveContext.save.info.playerData.magic += 4;

            if (gSaveContext.gameMode == GAMEMODE_NORMAL && !IS_CUTSCENE_LAYER) {
                SFX_PLAY_CENTERED(NA_SE_SY_GAUGE_UP - SFX_FLAG);
            }

            PRINTF(T("蓄電  MAGIC_NOW=%d (%d)\n", "Storage  MAGIC_NOW=%d (%d)\n"),
                   gSaveContext.save.info.playerData.magic, gSaveContext.magicFillTarget);

            if (gSaveContext.save.info.playerData.magic >= gSaveContext.magicFillTarget) {
                gSaveContext.save.info.playerData.magic = gSaveContext.magicFillTarget;
                gSaveContext.magicState = gSaveContext.prevMagicState;
                gSaveContext.prevMagicState = MAGIC_STATE_IDLE;
            }
            break;

        case MAGIC_STATE_CONSUME_SETUP:
            // Sets the speed at which magic border flashes
            sMagicBorderRatio = 2;
            gSaveContext.magicState = MAGIC_STATE_CONSUME;
            break;

        case MAGIC_STATE_CONSUME:
            // Consume magic until target is reached or no more magic is available
            gSaveContext.save.info.playerData.magic -= 2;
            if (gSaveContext.save.info.playerData.magic <= 0) {
                gSaveContext.save.info.playerData.magic = 0;
                gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
                sMagicBorderR = sMagicBorderG = sMagicBorderB = 255;
            } else if (gSaveContext.save.info.playerData.magic == gSaveContext.magicTarget) {
                gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
                sMagicBorderR = sMagicBorderG = sMagicBorderB = 255;
            }
            FALLTHROUGH; // Flash border while magic is being consumed
        case MAGIC_STATE_METER_FLASH_1:
        case MAGIC_STATE_METER_FLASH_2:
        case MAGIC_STATE_METER_FLASH_3:
            temp = sMagicBorderIndices[sMagicBorderStep];
            borderChangeR = ABS(sMagicBorderR - sMagicBorderColors[temp][0]) / sMagicBorderRatio;
            borderChangeG = ABS(sMagicBorderG - sMagicBorderColors[temp][1]) / sMagicBorderRatio;
            borderChangeB = ABS(sMagicBorderB - sMagicBorderColors[temp][2]) / sMagicBorderRatio;

            if (sMagicBorderR >= sMagicBorderColors[temp][0]) {
                sMagicBorderR -= borderChangeR;
            } else {
                sMagicBorderR += borderChangeR;
            }

            if (sMagicBorderG >= sMagicBorderColors[temp][1]) {
                sMagicBorderG -= borderChangeG;
            } else {
                sMagicBorderG += borderChangeG;
            }

            if (sMagicBorderB >= sMagicBorderColors[temp][2]) {
                sMagicBorderB -= borderChangeB;
            } else {
                sMagicBorderB += borderChangeB;
            }

            sMagicBorderRatio--;
            if (sMagicBorderRatio == 0) {
                sMagicBorderR = sMagicBorderColors[temp][0];
                sMagicBorderG = sMagicBorderColors[temp][1];
                sMagicBorderB = sMagicBorderColors[temp][2];
                sMagicBorderRatio = YREG(40 + sMagicBorderStep);
                sMagicBorderStep++;
                if (sMagicBorderStep >= 4) {
                    sMagicBorderStep = 0;
                }
            }
            break;

        case MAGIC_STATE_RESET:
            sMagicBorderR = sMagicBorderG = sMagicBorderB = 255;
            gSaveContext.magicState = MAGIC_STATE_IDLE;
            break;

        case MAGIC_STATE_CONSUME_LENS:
            // Slowly consume magic while lens is on
            if (!IS_PAUSED(&play->pauseCtx) && (msgCtx->msgMode == MSGMODE_NONE) &&
                (play->gameOverCtx.state == GAMEOVER_INACTIVE) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
                (play->transitionMode == TRANS_MODE_OFF) && !Play_InCsMode(play)) {
                if ((gSaveContext.save.info.playerData.magic == 0) ||

                    ((Player_GetEnvironmentalHazard(play) >= PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) &&
                     (Player_GetEnvironmentalHazard(play) <= PLAYER_ENV_HAZARD_UNDERWATER_FREE)) ||
                    ((gSaveContext.save.info.equips.buttonItems[1] != ITEM_LENS_OF_INSIGHT) &&
                     (gSaveContext.save.info.equips.buttonItems[2] != ITEM_LENS_OF_INSIGHT) &&
                     (gSaveContext.save.info.equips.buttonItems[3] != ITEM_LENS_OF_INSIGHT)) ||
                    !play->actorCtx.lensActive) {
                    // Force lens off and set magic meter state to idle
                    play->actorCtx.lensActive = false;
                    SFX_PLAY_CENTERED(NA_SE_SY_GLASSMODE_OFF);
                    gSaveContext.magicState = MAGIC_STATE_IDLE;
                    sMagicBorderR = sMagicBorderG = sMagicBorderB = 255;
                    break;
                }

                interfaceCtx->lensMagicConsumptionTimer--;
                if (interfaceCtx->lensMagicConsumptionTimer == 0) {
                    gSaveContext.save.info.playerData.magic--;
                    interfaceCtx->lensMagicConsumptionTimer = 80;
                }
            }

            temp = sMagicBorderIndices[sMagicBorderStep];
            borderChangeR = ABS(sMagicBorderR - sMagicBorderColors[temp][0]) / sMagicBorderRatio;
            borderChangeG = ABS(sMagicBorderG - sMagicBorderColors[temp][1]) / sMagicBorderRatio;
            borderChangeB = ABS(sMagicBorderB - sMagicBorderColors[temp][2]) / sMagicBorderRatio;

            if (sMagicBorderR >= sMagicBorderColors[temp][0]) {
                sMagicBorderR -= borderChangeR;
            } else {
                sMagicBorderR += borderChangeR;
            }

            if (sMagicBorderG >= sMagicBorderColors[temp][1]) {
                sMagicBorderG -= borderChangeG;
            } else {
                sMagicBorderG += borderChangeG;
            }

            if (sMagicBorderB >= sMagicBorderColors[temp][2]) {
                sMagicBorderB -= borderChangeB;
            } else {
                sMagicBorderB += borderChangeB;
            }

            sMagicBorderRatio--;
            if (sMagicBorderRatio == 0) {
                sMagicBorderR = sMagicBorderColors[temp][0];
                sMagicBorderG = sMagicBorderColors[temp][1];
                sMagicBorderB = sMagicBorderColors[temp][2];
                sMagicBorderRatio = YREG(40 + sMagicBorderStep);
                sMagicBorderStep++;
                if (sMagicBorderStep >= 4) {
                    sMagicBorderStep = 0;
                }
            }
            break;

        case MAGIC_STATE_ADD:
            // Add magic until target is reached
            gSaveContext.save.info.playerData.magic += 4;
            SFX_PLAY_CENTERED(NA_SE_SY_GAUGE_UP - SFX_FLAG);
            if (gSaveContext.save.info.playerData.magic >= gSaveContext.magicTarget) {
                gSaveContext.save.info.playerData.magic = gSaveContext.magicTarget;
                gSaveContext.magicState = gSaveContext.prevMagicState;
                gSaveContext.prevMagicState = MAGIC_STATE_IDLE;
            }
            break;

        default:
            gSaveContext.magicState = MAGIC_STATE_IDLE;
            break;
    }
}

void Magic_DrawMeter(PlayState* play) {
    PRESENTATION_STUB("Magic_DrawMeter");
}

void Interface_SetSubTimer(s16 seconds) {
    gSaveContext.timerX[TIMER_ID_SUB] = 140;
    gSaveContext.timerY[TIMER_ID_SUB] = 80;
    sEnvHazardActive = false;
    gSaveContext.subTimerSeconds = seconds;

    if (seconds != 0) {
        // count down
        gSaveContext.subTimerState = SUBTIMER_STATE_DOWN_INIT;
    } else {
        // count up
        gSaveContext.subTimerState = SUBTIMER_STATE_UP_INIT;
    }
}

/**
 * Set the subTimer to 1 second left
 */
void Interface_SetSubTimerToFinalSecond(UNUSED PlayState* play) {
    if (gSaveContext.subTimerState != SUBTIMER_STATE_OFF) {
        if (GET_EVENTINF(EVENTINF_MARATHON_ACTIVE)) {
            // The running-man race counts up and finished at MARATHON_TIME_LIMIT
            gSaveContext.subTimerSeconds = MARATHON_TIME_LIMIT - 1;
        } else {
            gSaveContext.subTimerSeconds = 1;
        }
    }
}

void Interface_SetTimer(s16 seconds) {
    gSaveContext.timerX[TIMER_ID_MAIN] = 140;
    gSaveContext.timerY[TIMER_ID_MAIN] = 80;
    sEnvHazardActive = false;
    gSaveContext.timerSeconds = seconds;

    if (seconds != 0) {
        // count down
        gSaveContext.timerState = TIMER_STATE_DOWN_INIT;
    } else {
        // count up
        gSaveContext.timerState = TIMER_STATE_UP_INIT;
    }
}

void Interface_DrawActionLabel(GraphicsContext* gfxCtx, void* texture) {
    PRESENTATION_STUB("Interface_DrawActionLabel");
}

void Interface_DrawItemButtons(PlayState* play) {
    PRESENTATION_STUB("Interface_DrawItemButtons");
}

void Interface_DrawItemIconTexture(PlayState* play, void* texture, s16 button) {
    PRESENTATION_STUB("Interface_DrawItemIconTexture");
}

void Interface_DrawAmmoCount(PlayState* play, s16 button, s16 alpha) {
    PRESENTATION_STUB("Interface_DrawAmmoCount");
}

void Interface_DrawActionButton(PlayState* play) {
    PRESENTATION_STUB("Interface_DrawActionButton");
}

void Interface_InitVertices(PlayState* play) {
    PRESENTATION_STUB("Interface_InitVertices");
}

void func_8008A8B8(PlayState* play, s32 topY, s32 bottomY, s32 leftX, s32 rightX) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    Vec3f eye;
    Vec3f lookAt;
    Vec3f up;

    eye.x = eye.y = eye.z = 0.0f;
    lookAt.x = lookAt.y = 0.0f;
    lookAt.z = -1.0f;
    up.x = up.z = 0.0f;
    up.y = 1.0f;

    View_LookAt(&interfaceCtx->view, &eye, &lookAt, &up);

    interfaceCtx->viewport.topY = topY;
    interfaceCtx->viewport.bottomY = bottomY;
    interfaceCtx->viewport.leftX = leftX;
    interfaceCtx->viewport.rightX = rightX;
    View_SetViewport(&interfaceCtx->view, &interfaceCtx->viewport);

    View_SetPerspective(&interfaceCtx->view, 60.0f, 10.0f, 60.0f);
    View_ApplyPerspectiveToOverlay(&interfaceCtx->view);
}

void func_8008A994(InterfaceContext* interfaceCtx) {
    SET_FULLSCREEN_VIEWPORT(&interfaceCtx->view);
    View_ApplyOrthoToOverlay(&interfaceCtx->view);
}

void Interface_Draw(PlayState* play) {
    // native: HUD graphics are drawn by Hud_Draw (engine/hud.c); this function keeps the original
    // logic (timers, respawns, button items) with only display-list statements removed.
    Hud_Draw(play);
    static s16 magicArrowEffectsR[] = { 255, 100, 255 };
    static s16 magicArrowEffectsG[] = { 0, 100, 255 };
    static s16 magicArrowEffectsB[] = { 0, 255, 100 };
    static s16 timerDigitLeftPos[] = { 16, 25, 34, 42, 51 };
    static s16 sDigitWidths[] = { 9, 9, 8, 9, 9 };
    // unused, most likely colors
    UNUSED static s16 D_80125B1C[][3] = {
        { 0, 150, 0 }, { 100, 255, 0 }, { 255, 255, 255 }, { 0, 0, 0 }, { 255, 255, 255 },
    };
    static s16 crownDigitsFirst[] = { 1, 0, 0 };
    static s16 crownDigitsCount[] = { 2, 3, 3 };
    static s16 spoilingItemEntrances[] = { ENTR_WANDERING_WOODS_2, ENTR_NERIS_DOMAIN_3, ENTR_NERIS_DOMAIN_3 };
    UNUSED static f32 D_80125B54[] = { -40.0f, -35.0f };
    UNUSED static s16 D_80125B5C[] = { 91, 91 };
    static s16 sTimerNextSecondTimer;
    static s16 sTimerStateTimer;
    static s16 sSubTimerNextSecondTimer;
    static s16 sSubTimerStateTimer;
    static s16 sTimerDigits[5];
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    PauseContext* pauseCtx = &play->pauseCtx;
    MessageContext* msgCtx = &play->msgCtx;
    Player* player = GET_PLAYER(play);
    s16 svar1;
    s16 svar2;
    s16 svar3;
    s16 svar4;
    s16 svar5;
    s16 timerId;


    GFX_TO(OVL, Gfx3D_SetSegment(0x02, (const void*)(interfaceCtx->parameterSegment)));
    GFX_TO(OVL, Gfx3D_SetSegment(0x07, (const void*)(interfaceCtx->doActionSegment)));
    GFX_TO(OVL, Gfx3D_SetSegment(0x08, (const void*)(interfaceCtx->iconItemSegment)));
    GFX_TO(OVL, Gfx3D_SetSegment(0x0B, (const void*)(interfaceCtx->mapSegment)));

    if (pauseCtx->debugState == PAUSE_DEBUG_STATE_CLOSED) {

        Gfx3D_SetupDLTo(GFX_BUF_OVL, 39);

        // Crown Icon
        GFX_TO(OVL, Gfx3D_SetPrimColor(200, 255, 100, interfaceCtx->magicAlpha));
        GFX_TO(OVL, Gfx3D_SetEnvColor(0, 80, 0, 255));

        switch (play->sceneId) {
            case SCENE_FOREST_TEMPLE:
            case SCENE_FIRE_TEMPLE:
            case SCENE_WATER_TEMPLE:
            case SCENE_SPIRIT_TEMPLE:
            case SCENE_SHADOW_TEMPLE:
            case SCENE_BOTTOM_OF_THE_WELL:
            case SCENE_ICE_CAVERN:
            case SCENE_VARKONS_TOWER:
            case SCENE_DUNEAN_TRAINING_GROUND:
            case SCENE_THIEVES_HIDEOUT:
            case SCENE_INSIDE_VARKONS_CASTLE:
            case SCENE_VARKONS_TOWER_COLLAPSE_INTERIOR:
            case SCENE_INSIDE_VARKONS_CASTLE_COLLAPSE:
            case SCENE_TREASURE_BOX_SHOP:
                if (gSaveContext.save.info.inventory.dungeonKeys[gSaveContext.mapIndex] >= 0) {
                    // Small Key Icon
                    GFX_TO(OVL, Gfx3D_SetPrimColor(200, 230, 255, interfaceCtx->magicAlpha));
                    GFX_TO(OVL, Gfx3D_SetEnvColor(0, 0, 20, 255));

                    // Small Key Counter
                    GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->magicAlpha));

                    interfaceCtx->counterDigits[2] = 0;
                    interfaceCtx->counterDigits[3] =
                        gSaveContext.save.info.inventory.dungeonKeys[gSaveContext.mapIndex];

                    while (interfaceCtx->counterDigits[3] >= 10) {
                        interfaceCtx->counterDigits[2]++;
                        interfaceCtx->counterDigits[3] -= 10;
                    }

                    svar3 = 42;

                    if (interfaceCtx->counterDigits[2] != 0) {
                        svar3 += 8;
                    }

                }
                break;
            default:
                break;
        }

        // Crown Counter

        if (gSaveContext.save.info.playerData.crowns == CUR_CAPACITY(UPG_WALLET)) {
            GFX_TO(OVL, Gfx3D_SetPrimColor(120, 255, 0, interfaceCtx->magicAlpha));
        } else if (gSaveContext.save.info.playerData.crowns != 0) {
            GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->magicAlpha));
        } else {
            GFX_TO(OVL, Gfx3D_SetPrimColor(100, 100, 100, interfaceCtx->magicAlpha));
        }


        interfaceCtx->counterDigits[0] = interfaceCtx->counterDigits[1] = 0;
        interfaceCtx->counterDigits[2] = gSaveContext.save.info.playerData.crowns;

        if ((interfaceCtx->counterDigits[2] > 9999) || (interfaceCtx->counterDigits[2] < 0)) {
            interfaceCtx->counterDigits[2] &= 0xDDD;
        }

        while (interfaceCtx->counterDigits[2] >= 100) {
            interfaceCtx->counterDigits[0]++;
            interfaceCtx->counterDigits[2] -= 100;
        }

        while (interfaceCtx->counterDigits[2] >= 10) {
            interfaceCtx->counterDigits[1]++;
            interfaceCtx->counterDigits[2] -= 10;
        }

        svar2 = crownDigitsFirst[CUR_UPG_VALUE(UPG_WALLET)];
        svar4 = crownDigitsCount[CUR_UPG_VALUE(UPG_WALLET)];

        for (svar1 = 0, svar3 = 42; svar1 < svar4; svar1++, svar2++, svar3 += 8) {
        }


        if ((R_PAUSE_BG_PRERENDER_STATE != PAUSE_BG_PRERENDER_PROCESS) &&
            (R_PAUSE_BG_PRERENDER_STATE != PAUSE_BG_PRERENDER_READY)) {
            Attention_Draw(&play->actorCtx.attention, play);
        }

        Gfx3D_SetupDLTo(GFX_BUF_OVL, 39);


        GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->bAlpha));

        if (!(interfaceCtx->unk_1FA)) {
            // B Button Icon & Ammo Count
            if (gSaveContext.save.info.equips.buttonItems[0] != ITEM_NONE) {

                if ((player->stateFlags1 & PLAYER_STATE1_23) || (play->shootingGalleryStatus > 1) ||
                    ((play->sceneId == SCENE_BOMBMOUSE_BOWLING_ALLEY) && Flags_GetSwitch(play, 0x38))) {
                }
            }
        } else {
            // B Button Do Action Label
            GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->bAlpha));


            R_B_LABEL_DD = (1 << 10) / (R_B_LABEL_SCALE(gSaveContext.language) / 100.0f);
        }


        // C-Left Button Icon & Ammo Count
        if (gSaveContext.save.info.equips.buttonItems[1] < 0xF0) {
            GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->cLeftAlpha));
        }


        // C-Down Button Icon & Ammo Count
        if (gSaveContext.save.info.equips.buttonItems[2] < 0xF0) {
            GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->cDownAlpha));
        }


        // C-Right Button Icon & Ammo Count
        if (gSaveContext.save.info.equips.buttonItems[3] < 0xF0) {
            GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->cRightAlpha));
        }

        // A Button
        Gfx3D_SetupDLTo(GFX_BUF_OVL, 42);
        func_8008A8B8(play, R_A_BTN_Y, R_A_BTN_Y + 45, R_A_BTN_X, R_A_BTN_X + 45);
        GFX_TO(OVL, Gfx3D_SetPrimColor(R_A_BTN_COLOR(0), R_A_BTN_COLOR(1), R_A_BTN_COLOR(2), interfaceCtx->aAlpha));
        func_8008A8B8(play, R_A_ICON_Y, R_A_ICON_Y + 45, R_A_ICON_X, R_A_ICON_X + 45);
        GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->aAlpha));
        GFX_TO(OVL, Gfx3D_SetEnvColor(0, 0, 0, 0));
        Matrix_Translate(0.0f, 0.0f, R_A_LABEL_Z(gSaveContext.language) / 10.0f, MTXMODE_NEW);
        Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
        Matrix_RotateX(interfaceCtx->unk_1F4 / 10000.0f, MTXMODE_APPLY);
        GFX_TO(OVL, Gfx3D_LoadMtxF(Matrix_GetCurrent()));

        if ((interfaceCtx->unk_1EC < 2) || (interfaceCtx->unk_1EC == 3)) {
        } else {
        }



        if ((pauseCtx->state == PAUSE_STATE_MAIN) && (pauseCtx->mainState == PAUSE_MAIN_STATE_3)) {
            // Inventory Equip Effects
            GFX_TO(OVL, Gfx3D_SetSegment(0x08, (const void*)(pauseCtx->iconItemSegment)));
            Gfx3D_SetupDLTo(GFX_BUF_OVL, 42);

            // PAUSE_CURSOR_QUAD_4

            if (pauseCtx->equipTargetItem < 0xBF) {
                // Normal Equip (icon goes from the inventory slot to the C button when equipping it)
                GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, pauseCtx->equipAnimAlpha));

            } else {
                // Magic Arrow Equip Effect
                svar1 = pauseCtx->equipTargetItem - 0xBF;
                GFX_TO(OVL, Gfx3D_SetPrimColor(magicArrowEffectsR[svar1], magicArrowEffectsG[svar1], magicArrowEffectsB[svar1], pauseCtx->equipAnimAlpha));

                if ((pauseCtx->equipAnimAlpha > 0) && (pauseCtx->equipAnimAlpha < 255)) {
                    svar1 = (pauseCtx->equipAnimAlpha / 8) / 2;
                    // PAUSE_CURSOR_QUAD_4
                }

            }

        }

        Gfx3D_SetupDLTo(GFX_BUF_OVL, 39);

        if (!IS_PAUSED(&play->pauseCtx)) {
            if (gSaveContext.minigameState != 1) {
                // Carrots rendering if the action corresponds to riding a horse
                if (interfaceCtx->unk_1EE == 8) {
                    // Load Carrot Icon

                    // Draw 6 carrots
                    for (svar1 = 1, svar5 = ZREG(14); svar1 < 7; svar1++, svar5 += 16) {
                        // Carrot Color (based on availability)
                        if ((interfaceCtx->numHorseBoosts == 0) || (interfaceCtx->numHorseBoosts < svar1)) {
                            GFX_TO(OVL, Gfx3D_SetPrimColor(0, 150, 255, interfaceCtx->aAlpha));
                        } else {
                            GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->aAlpha));
                        }

                    }
                }
            } else {
                // Score for the Horseback Archery
                svar5 = WREG(32);
                GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, interfaceCtx->bAlpha));

                // Target Icon


                // Score Counter

                svar5 = WREG(32) + 6 * 9;

                for (svar1 = svar2 = 0; svar1 < 4; svar1++) {
                    if (sHBAScoreDigits[svar1] != 0 || (svar2 != 0) || (svar1 >= 3)) {
                        svar5 += 9;
                        svar2++;
                    }
                }

            }
        }

        if ((gSaveContext.subTimerState == SUBTIMER_STATE_RESPAWN) &&
            (Message_GetState(&play->msgCtx) == TEXT_STATE_EVENT)) {
            // Trade quest timer reached 0
            sSubTimerStateTimer = 40;
            gSaveContext.save.cutsceneIndex = CS_INDEX_NONE;
            play->transitionTrigger = TRANS_TRIGGER_START;
            play->transitionType = TRANS_TYPE_FADE_WHITE;
            gSaveContext.subTimerState = SUBTIMER_STATE_OFF;

            if ((gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_SYLVAN) &&
                (gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_MASTER) &&
                (gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_HIGHCAIRN) &&
                (gSaveContext.save.info.equips.buttonItems[0] != ITEM_GIANTS_KNIFE)) {
                if (gSaveContext.buttonStatus[0] != BTN_ENABLED) {
                    gSaveContext.save.info.equips.buttonItems[0] = gSaveContext.buttonStatus[0];
                } else {
                    gSaveContext.save.info.equips.buttonItems[0] = ITEM_NONE;
                }
            }

            // Revert any spoiling trade quest items
            for (svar1 = 0; svar1 < ARRAY_COUNT(gSpoilingItems); svar1++) {
                if (INV_CONTENT(ITEM_TRADE_ADULT) == gSpoilingItems[svar1]) {
#if OOT_VERSION >= NTSC_1_1
                    gSaveContext.eventInf[EVENTINF_INDEX_HORSES] &=
                        (u16) ~(EVENTINF_OTTO_RACE_STATE_MASK | EVENTINF_MASK(EVENTINF_OTTO_RACE_HORSETYPE) |
                                EVENTINF_MASK(EVENTINF_OTTO_RACE_LOST_ONCE) |
                                EVENTINF_MASK(EVENTINF_OTTO_RACE_SECOND_RACE) | EVENTINF_MASK(EVENTINF_OTTO_RACE_0F));
                    PRINTF("EVENT_INF=%x\n", gSaveContext.eventInf[EVENTINF_INDEX_HORSES]);
#endif
                    play->nextEntranceIndex = spoilingItemEntrances[svar1];
                    INV_CONTENT(gSpoilingItemReverts[svar1]) = gSpoilingItemReverts[svar1];

                    for (svar2 = 1; svar2 < 4; svar2++) {
                        if (gSaveContext.save.info.equips.buttonItems[svar2] == gSpoilingItems[svar1]) {
                            gSaveContext.save.info.equips.buttonItems[svar2] = gSpoilingItemReverts[svar1];
                            Interface_LoadItemIcon1(play, svar2);
                        }
                    }
                }
            }
        }

        if (!IS_PAUSED(&play->pauseCtx) && (play->gameOverCtx.state == GAMEOVER_INACTIVE) &&
            (msgCtx->msgMode == MSGMODE_NONE) && !(player->stateFlags2 & PLAYER_STATE2_24) &&
            (play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF) &&
            !Play_InCsMode(play) && (gSaveContext.minigameState != 1) && (play->shootingGalleryStatus <= 1) &&
            !((play->sceneId == SCENE_BOMBMOUSE_BOWLING_ALLEY) && Flags_GetSwitch(play, 0x38))) {

            timerId = TIMER_ID_MAIN;

            switch (gSaveContext.timerState) {
                case TIMER_STATE_ENV_HAZARD_INIT:
                    sTimerStateTimer = 20;
                    sTimerNextSecondTimer = 20;
                    gSaveContext.timerSeconds = gSaveContext.save.info.playerData.health >> 1;
                    gSaveContext.timerState = TIMER_STATE_ENV_HAZARD_PREVIEW;
                    break;

                case TIMER_STATE_ENV_HAZARD_PREVIEW:
                    sTimerStateTimer--;
                    if (sTimerStateTimer == 0) {
                        sTimerStateTimer = 20;
                        gSaveContext.timerState = TIMER_STATE_ENV_HAZARD_MOVE;
                    }
                    break;

                case TIMER_STATE_DOWN_INIT:
                case TIMER_STATE_UP_INIT:
                    sTimerStateTimer = 20;
                    sTimerNextSecondTimer = 20;
                    if (gSaveContext.timerState == TIMER_STATE_DOWN_INIT) {
                        gSaveContext.timerState = TIMER_STATE_DOWN_PREVIEW;
                    } else {
                        gSaveContext.timerState = TIMER_STATE_UP_PREVIEW;
                    }
                    break;

                case TIMER_STATE_DOWN_PREVIEW:
                case TIMER_STATE_UP_PREVIEW:
                    sTimerStateTimer--;
                    if (sTimerStateTimer == 0) {
                        sTimerStateTimer = 20;
                        if (gSaveContext.timerState == TIMER_STATE_DOWN_PREVIEW) {
                            gSaveContext.timerState = TIMER_STATE_DOWN_MOVE;
                        } else {
                            gSaveContext.timerState = TIMER_STATE_UP_MOVE;
                        }
                    }
                    break;

                case TIMER_STATE_ENV_HAZARD_MOVE:
                case TIMER_STATE_DOWN_MOVE:
                    svar1 = (gSaveContext.timerX[TIMER_ID_MAIN] - 26) / sTimerStateTimer;
                    gSaveContext.timerX[TIMER_ID_MAIN] -= svar1;

                    if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                        svar1 = (gSaveContext.timerY[TIMER_ID_MAIN] - 54) / sTimerStateTimer; // two rows of hearts
                    } else {
                        svar1 = (gSaveContext.timerY[TIMER_ID_MAIN] - 46) / sTimerStateTimer; // one row of hearts
                    }
                    gSaveContext.timerY[TIMER_ID_MAIN] -= svar1;

                    sTimerStateTimer--;
                    if (sTimerStateTimer == 0) {
                        sTimerStateTimer = 20;
                        gSaveContext.timerX[TIMER_ID_MAIN] = 26;

                        if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 54; // two rows of hearts
                        } else {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 46; // one row of hearts
                        }

                        if (gSaveContext.timerState == TIMER_STATE_ENV_HAZARD_MOVE) {
                            gSaveContext.timerState = TIMER_STATE_ENV_HAZARD_TICK;
                        } else {
                            gSaveContext.timerState = TIMER_STATE_DOWN_TICK;
                        }
                    }
                    FALLTHROUGH;
                case TIMER_STATE_ENV_HAZARD_TICK:
                case TIMER_STATE_DOWN_TICK:
                    if ((gSaveContext.timerState == TIMER_STATE_ENV_HAZARD_TICK) ||
                        (gSaveContext.timerState == TIMER_STATE_DOWN_TICK)) {
                        if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 54; // two rows of hearts
                        } else {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 46; // one row of hearts
                        }
                    }

                    if ((gSaveContext.timerState >= TIMER_STATE_ENV_HAZARD_MOVE) && (msgCtx->msgLength == 0)) {
                        if (--sTimerNextSecondTimer == 0) {
                            if (gSaveContext.timerSeconds != 0) {
                                gSaveContext.timerSeconds--;
                            }

                            sTimerNextSecondTimer = 20;

                            if (gSaveContext.timerSeconds == 0) {
                                // Out of time
                                gSaveContext.timerState = TIMER_STATE_STOP;
                                if (sEnvHazardActive) {
                                    gSaveContext.save.info.playerData.health = 0;
                                    play->damagePlayer(play, -(gSaveContext.save.info.playerData.health + 2));
                                }
                                sEnvHazardActive = false;
                            } else if (gSaveContext.timerSeconds > 60) {
                                // Beep at "xx:x1" (every 10 seconds)
                                if (sTimerDigits[4] == 1) {
                                    SFX_PLAY_CENTERED(NA_SE_SY_MESSAGE_WOMAN);
                                }
                            } else if (gSaveContext.timerSeconds > 10) {
                                // Beep on alternating seconds
                                if ((sTimerDigits[4] % 2) != 0) {
                                    SFX_PLAY_CENTERED(NA_SE_SY_WARNING_COUNT_N);
                                }
                            } else {
                                // Beep every second
                                SFX_PLAY_CENTERED(NA_SE_SY_WARNING_COUNT_E);
                            }
                        }
                    }
                    break;

                case TIMER_STATE_UP_MOVE:
                    svar1 = (gSaveContext.timerX[TIMER_ID_MAIN] - 26) / sTimerStateTimer;
                    gSaveContext.timerX[TIMER_ID_MAIN] -= svar1;

                    if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                        svar1 = (gSaveContext.timerY[TIMER_ID_MAIN] - 54) / sTimerStateTimer; // two rows of hearts
                    } else {
                        svar1 = (gSaveContext.timerY[TIMER_ID_MAIN] - 46) / sTimerStateTimer; // one row of hearts
                    }
                    gSaveContext.timerY[TIMER_ID_MAIN] -= svar1;

                    sTimerStateTimer--;
                    if (sTimerStateTimer == 0) {
                        sTimerStateTimer = 20;
                        gSaveContext.timerX[TIMER_ID_MAIN] = 26;
                        if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 54; // two rows of hearts
                        } else {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 46; // one row of hearts
                        }

                        gSaveContext.timerState = TIMER_STATE_UP_TICK;
                    }
                    FALLTHROUGH;
                case TIMER_STATE_UP_TICK:
                    if (gSaveContext.timerState == TIMER_STATE_UP_TICK) {
                        if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 54; // two rows of hearts
                        } else {
                            gSaveContext.timerY[TIMER_ID_MAIN] = 46; // one row of hearts
                        }
                    }

                    if (gSaveContext.timerState >= TIMER_STATE_ENV_HAZARD_MOVE) {
                        sTimerNextSecondTimer--;
                        if (sTimerNextSecondTimer == 0) {
                            gSaveContext.timerSeconds++;
                            sTimerNextSecondTimer = 20;

                            if (gSaveContext.timerSeconds == 3599) { // 59 minutes, 59 seconds
                                sTimerStateTimer = 40;
                                gSaveContext.timerState = TIMER_STATE_UP_FREEZE;
                            } else {
                                SFX_PLAY_CENTERED(NA_SE_SY_WARNING_COUNT_N);
                            }
                        }
                    }
                    break;

                case TIMER_STATE_STOP:
                    if (gSaveContext.subTimerState != SUBTIMER_STATE_OFF) {
                        sSubTimerStateTimer = 20;
                        sSubTimerNextSecondTimer = 20;
                        gSaveContext.timerX[TIMER_ID_SUB] = 140;
                        gSaveContext.timerY[TIMER_ID_SUB] = 80;

                        if (gSaveContext.subTimerState <= SUBTIMER_STATE_STOP) {
                            gSaveContext.subTimerState = SUBTIMER_STATE_DOWN_PREVIEW;
                        } else {
                            gSaveContext.subTimerState = SUBTIMER_STATE_UP_PREVIEW;
                        }

                        gSaveContext.timerState = TIMER_STATE_OFF;
                    } else {
                        gSaveContext.timerState = TIMER_STATE_OFF;
                    }
                    FALLTHROUGH;
                case TIMER_STATE_UP_FREEZE:
                    break;

                default: // TIMER_STATE_OFF
                    // Process the subTimer only if the main timer is off
                    timerId = TIMER_ID_SUB;

                    switch (gSaveContext.subTimerState) {
                        case SUBTIMER_STATE_DOWN_INIT:
                        case SUBTIMER_STATE_UP_INIT:
                            sSubTimerStateTimer = 20;
                            sSubTimerNextSecondTimer = 20;
                            gSaveContext.timerX[TIMER_ID_SUB] = 140;
                            gSaveContext.timerY[TIMER_ID_SUB] = 80;
                            if (gSaveContext.subTimerState == SUBTIMER_STATE_DOWN_INIT) {
                                gSaveContext.subTimerState = SUBTIMER_STATE_DOWN_PREVIEW;
                            } else {
                                gSaveContext.subTimerState = SUBTIMER_STATE_UP_PREVIEW;
                            }
                            break;

                        case SUBTIMER_STATE_DOWN_PREVIEW:
                        case SUBTIMER_STATE_UP_PREVIEW:
                            sSubTimerStateTimer--;
                            if (sSubTimerStateTimer == 0) {
                                sSubTimerStateTimer = 20;
                                if (gSaveContext.subTimerState == SUBTIMER_STATE_DOWN_PREVIEW) {
                                    gSaveContext.subTimerState = SUBTIMER_STATE_DOWN_MOVE;
                                } else {
                                    gSaveContext.subTimerState = SUBTIMER_STATE_UP_MOVE;
                                }
                            }
                            break;

                        case SUBTIMER_STATE_DOWN_MOVE:
                        case SUBTIMER_STATE_UP_MOVE:
                            PRINTF("event_xp[1]=%d,  event_yp[1]=%d  TOTAL_EVENT_TM=%d\n",
                                   ((void)0, gSaveContext.timerX[TIMER_ID_SUB]),
                                   ((void)0, gSaveContext.timerY[TIMER_ID_SUB]), gSaveContext.subTimerSeconds);
                            svar1 = (gSaveContext.timerX[TIMER_ID_SUB] - 26) / sSubTimerStateTimer;
                            gSaveContext.timerX[TIMER_ID_SUB] -= svar1;
                            if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                                // two rows of hearts
                                svar1 = (gSaveContext.timerY[TIMER_ID_SUB] - 54) / sSubTimerStateTimer;
                            } else {
                                // one row of hearts
                                svar1 = (gSaveContext.timerY[TIMER_ID_SUB] - 46) / sSubTimerStateTimer;
                            }
                            gSaveContext.timerY[TIMER_ID_SUB] -= svar1;

                            sSubTimerStateTimer--;
                            if (sSubTimerStateTimer == 0) {
                                sSubTimerStateTimer = 20;
                                gSaveContext.timerX[TIMER_ID_SUB] = 26;

                                if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                                    gSaveContext.timerY[TIMER_ID_SUB] = 54; // two rows of hearts
                                } else {
                                    gSaveContext.timerY[TIMER_ID_SUB] = 46; // one row of hearts
                                }

                                if (gSaveContext.subTimerState == SUBTIMER_STATE_DOWN_MOVE) {
                                    gSaveContext.subTimerState = SUBTIMER_STATE_DOWN_TICK;
                                } else {
                                    gSaveContext.subTimerState = SUBTIMER_STATE_UP_TICK;
                                }
                            }
                            FALLTHROUGH;
                        case SUBTIMER_STATE_DOWN_TICK:
                        case SUBTIMER_STATE_UP_TICK:
                            if ((gSaveContext.subTimerState == SUBTIMER_STATE_DOWN_TICK) ||
                                (gSaveContext.subTimerState == SUBTIMER_STATE_UP_TICK)) {
                                if (gSaveContext.save.info.playerData.healthCapacity > 0xA0) {
                                    gSaveContext.timerY[TIMER_ID_SUB] = 54; // two rows of hearts
                                } else {
                                    gSaveContext.timerY[TIMER_ID_SUB] = 46; // one row of hearts
                                }
                            }

                            if (gSaveContext.subTimerState >= SUBTIMER_STATE_DOWN_MOVE) {
                                sSubTimerNextSecondTimer--;
                                if (sSubTimerNextSecondTimer == 0) {
                                    sSubTimerNextSecondTimer = 20;
                                    if (gSaveContext.subTimerState == SUBTIMER_STATE_DOWN_TICK) {
                                        gSaveContext.subTimerSeconds--;
                                        PRINTF("TOTAL_EVENT_TM=%d\n", gSaveContext.subTimerSeconds);

#if OOT_VERSION < PAL_1_0
                                        if (gSaveContext.subTimerSeconds == 0)
#else
                                        if (gSaveContext.subTimerSeconds <= 0)
#endif
                                        {
                                            // Out of time
                                            if (!Flags_GetSwitch(play, 0x37) ||
                                                ((play->sceneId != SCENE_VARKON_BOSS) &&
                                                 (play->sceneId != SCENE_VARKONS_TOWER_COLLAPSE_EXTERIOR) &&
                                                 (play->sceneId != SCENE_VARKONS_TOWER_COLLAPSE_INTERIOR) &&
                                                 (play->sceneId != SCENE_INSIDE_VARKONS_CASTLE_COLLAPSE))) {
                                                sSubTimerStateTimer = 40;
                                                gSaveContext.subTimerState = SUBTIMER_STATE_RESPAWN;
                                                gSaveContext.save.cutsceneIndex = CS_INDEX_NONE;
                                                Message_StartTextbox(play, 0x71B0, NULL);
                                                Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_8);
                                            } else {
                                                sSubTimerStateTimer = 40;
                                                gSaveContext.subTimerState = SUBTIMER_STATE_STOP;
                                            }
                                        } else if (gSaveContext.subTimerSeconds > 60) {
                                            // Beep at "xx:x1" (every 10 seconds)
                                            if (sTimerDigits[4] == 1) {
                                                SFX_PLAY_CENTERED(NA_SE_SY_MESSAGE_WOMAN);
                                            }
                                        } else if (gSaveContext.subTimerSeconds > 10) {
                                            // Beep on alternating seconds
                                            if ((sTimerDigits[4] % 2) != 0) {
                                                SFX_PLAY_CENTERED(NA_SE_SY_WARNING_COUNT_N);
                                            }
                                        } else {
                                            // Beep every second
                                            SFX_PLAY_CENTERED(NA_SE_SY_WARNING_COUNT_E);
                                        }
                                    } else { // SUBTIMER_STATE_UP_TICK
                                        gSaveContext.subTimerSeconds++;

                                        // Special case for the running-man race
                                        if (GET_EVENTINF(EVENTINF_MARATHON_ACTIVE) &&
                                            (gSaveContext.subTimerSeconds == MARATHON_TIME_LIMIT)) {
                                            // After 4 minutes, cancel the timer
                                            Message_StartTextbox(play, 0x6083, NULL);
                                            CLEAR_EVENTINF(EVENTINF_MARATHON_ACTIVE);
                                            gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
                                        }
                                    }

                                    // Beep at the minute mark
                                    if ((gSaveContext.subTimerSeconds % 60) == 0) {
                                        SFX_PLAY_CENTERED(NA_SE_SY_WARNING_COUNT_N);
                                    }
                                }
                            }
                            break;

                        case SUBTIMER_STATE_STOP:
                            sSubTimerStateTimer--;
                            if (sSubTimerStateTimer == 0) {
                                gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
                            }
                            break;
                    }
                    break;
            }

            if (((gSaveContext.timerState != TIMER_STATE_OFF) && (gSaveContext.timerState != TIMER_STATE_STOP)) ||
                (gSaveContext.subTimerState != SUBTIMER_STATE_OFF)) {
                sTimerDigits[0] = sTimerDigits[1] = sTimerDigits[3] = 0;
                sTimerDigits[2] = 10; // digit 10 is used as ':' (colon)

                if (gSaveContext.timerState != TIMER_STATE_OFF) {
                    sTimerDigits[4] = gSaveContext.timerSeconds;
                } else {
                    sTimerDigits[4] = gSaveContext.subTimerSeconds;
                }

                while (sTimerDigits[4] >= 60) {
                    sTimerDigits[1]++;
                    if (sTimerDigits[1] >= 10) {
                        sTimerDigits[0]++;
                        sTimerDigits[1] -= 10;
                    }
                    sTimerDigits[4] -= 60;
                }

                while (sTimerDigits[4] >= 10) {
                    sTimerDigits[3]++;
                    sTimerDigits[4] -= 10;
                }

                // Clock Icon
                GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, 255));
                GFX_TO(OVL, Gfx3D_SetEnvColor(0, 0, 0, 0));

                // Timer Counter

                if (gSaveContext.timerState != TIMER_STATE_OFF) {
                    // TIMER_ID_MAIN
                    if ((gSaveContext.timerSeconds < 10) && (gSaveContext.timerState <= TIMER_STATE_STOP)) {
                        GFX_TO(OVL, Gfx3D_SetPrimColor(255, 50, 0, 255));
                    } else {
                        GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 255, 255));
                    }
                } else {
                    // TIMER_ID_SUB
                    if ((gSaveContext.subTimerSeconds < 10) && (gSaveContext.subTimerState <= SUBTIMER_STATE_RESPAWN)) {
                        GFX_TO(OVL, Gfx3D_SetPrimColor(255, 50, 0, 255));
                    } else {
                        GFX_TO(OVL, Gfx3D_SetPrimColor(255, 255, 0, 255));
                    }
                }

                for (svar1 = 0; svar1 < ARRAY_COUNT(sTimerDigits); svar1++) {
                }
            }
        }
    }

#if DEBUG_FEATURES
    if (pauseCtx->debugState == PAUSE_DEBUG_STATE_FLAG_SET_OPEN) {
        FlagSet_Update(play);
    }
#endif

    if (interfaceCtx->unk_244 != 0) {
        Gfx3D_SetupDLTo(GFX_BUF_OVL, 25); // native: was the HUD render-state setup list
        GFX_TO(OVL, Gfx3D_SetPrimColor(0, 0, 0, interfaceCtx->unk_244));
    }

}

void Interface_Update(PlayState* play) {
    static u8 D_80125B60 = false;
    static s16 sPrevTimeSpeed = 0;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    MessageContext* msgCtx = &play->msgCtx;
    Player* player = GET_PLAYER(play);
    s16 dimmingAlpha;
    s16 risingAlpha;
    u16 action;

#if OOT_PAL && DEBUG_FEATURES
    {
        Input* debugInput = &play->state.input[2];

        if (CHECK_BTN_ALL(debugInput->press.button, BTN_DLEFT)) {
            gSaveContext.language = LANGUAGE_ENG;
            PRINTF("J_N=%x J_N=%x\n", gSaveContext.language, &gSaveContext.language);
        } else if (CHECK_BTN_ALL(debugInput->press.button, BTN_DUP)) {
            gSaveContext.language = LANGUAGE_GER;
            PRINTF("J_N=%x J_N=%x\n", gSaveContext.language, &gSaveContext.language);
        } else if (CHECK_BTN_ALL(debugInput->press.button, BTN_DRIGHT)) {
            gSaveContext.language = LANGUAGE_FRA;
            PRINTF("J_N=%x J_N=%x\n", gSaveContext.language, &gSaveContext.language);
        }
    }
#endif

    if (!IS_PAUSED(&play->pauseCtx)) {
        if ((gSaveContext.minigameState == 1) || !IS_CUTSCENE_LAYER ||
            ((play->sceneId == SCENE_SUNMEAD_RANCH) && (gSaveContext.sceneLayer == 4))) {
            if ((msgCtx->msgMode == MSGMODE_NONE) ||
                ((msgCtx->msgMode != MSGMODE_NONE) && (play->sceneId == SCENE_BOMBMOUSE_BOWLING_ALLEY))) {
                if (play->gameOverCtx.state == GAMEOVER_INACTIVE) {
                    func_80083108(play);
                }
            }
        }
    }

    // Update hud visibility mode
    switch (gSaveContext.nextHudVisibilityMode) {
        case HUD_VISIBILITY_NOTHING:
        case HUD_VISIBILITY_NOTHING_ALT:
        case HUD_VISIBILITY_HEARTS_FORCE:
        case HUD_VISIBILITY_A:
        case HUD_VISIBILITY_A_HEARTS_MAGIC_FORCE:
        case HUD_VISIBILITY_A_HEARTS_MAGIC_MINIMAP_FORCE:
        case HUD_VISIBILITY_ALL_NO_MINIMAP_BY_BTN_STATUS:
        case HUD_VISIBILITY_B:
        case HUD_VISIBILITY_HEARTS_MAGIC:
        case HUD_VISIBILITY_B_ALT:
        case HUD_VISIBILITY_HEARTS:
        case HUD_VISIBILITY_A_B_MINIMAP:
        case HUD_VISIBILITY_HEARTS_MAGIC_FORCE:
            dimmingAlpha = 255 - (32 * gSaveContext.hudVisibilityModeTimer);
            if (dimmingAlpha < 0) {
                dimmingAlpha = 0;
            }

            Interface_UpdateHudAlphas(play, dimmingAlpha);
            gSaveContext.hudVisibilityModeTimer++;

            if (dimmingAlpha == 0) {
                gSaveContext.nextHudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
            }
            break;

        case HUD_VISIBILITY_ALL:
            dimmingAlpha = 255 - (32 * gSaveContext.hudVisibilityModeTimer);
            if (dimmingAlpha < 0) {
                dimmingAlpha = 0;
            }

            risingAlpha = 255 - dimmingAlpha;
            if (risingAlpha >= 255) {
                risingAlpha = 255;
            }

            PRINTF("case 50 : alpha=%d  alpha1=%d\n", dimmingAlpha, risingAlpha);

            Interface_RaiseButtonAlphas(play, risingAlpha);

            if (interfaceCtx->healthAlpha != 255) {
                interfaceCtx->healthAlpha = risingAlpha;
            }

            if (interfaceCtx->magicAlpha != 255) {
                interfaceCtx->magicAlpha = risingAlpha;
            }

            switch (play->sceneId) {
                case SCENE_ELDORA_FIELD:
                case SCENE_OAKRIDGE_VILLAGE:
                case SCENE_GRAVEYARD:
                case SCENE_NERIS_RIVER:
                case SCENE_SYLVAN_FOREST:
                case SCENE_ANCIENT_GROVE:
                case SCENE_LAKE_ELDEN:
                case SCENE_NERIS_DOMAIN:
                case SCENE_NERIS_FOUNTAIN:
                case SCENE_DUNEAN_VALLEY:
                case SCENE_WANDERING_WOODS:
                case SCENE_DESERT_SENTINEL:
                case SCENE_DUNEANS_FORTRESS:
                case SCENE_WHISPERING_SANDS:
                case SCENE_ELDORA_CASTLE:
                case SCENE_CINDER_MOUNTAIN_TRAIL:
                case SCENE_CINDER_MOUNTAIN_CRATER:
                case SCENE_CAIRN_CITY:
                case SCENE_SUNMEAD_RANCH:
                case SCENE_OUTSIDE_VARKONS_CASTLE:
                    if (interfaceCtx->minimapAlpha < 170) {
                        interfaceCtx->minimapAlpha = risingAlpha;
                    } else {
                        interfaceCtx->minimapAlpha = 170;
                    }
                    break;

                default:
                    if (interfaceCtx->minimapAlpha != 255) {
                        interfaceCtx->minimapAlpha = risingAlpha;
                    }
                    break;
            }

            gSaveContext.hudVisibilityModeTimer++;
            if (risingAlpha == 255) {
                gSaveContext.nextHudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
            }

            break;

        case HUD_VISIBILITY_NOTHING_INSTANT:
            // Hide the HUD immediately
            gSaveContext.nextHudVisibilityMode = HUD_VISIBILITY_NOTHING;
            Interface_UpdateHudAlphas(play, 0);
            gSaveContext.nextHudVisibilityMode = HUD_VISIBILITY_NO_CHANGE;
            FALLTHROUGH;
        default:
            break;
    }

    Map_Update(play);

    if (gSaveContext.healthAccumulator != 0) {
        gSaveContext.healthAccumulator -= 4;
        gSaveContext.save.info.playerData.health += 4;

        if ((gSaveContext.save.info.playerData.health & 0xF) < 4) {
            SFX_PLAY_CENTERED(NA_SE_SY_HP_RECOVER);
        }

        PRINTF("now_life=%d  max_life=%d\n", gSaveContext.save.info.playerData.health,
               gSaveContext.save.info.playerData.healthCapacity);

        if (gSaveContext.save.info.playerData.health >= gSaveContext.save.info.playerData.healthCapacity) {
            gSaveContext.save.info.playerData.health = gSaveContext.save.info.playerData.healthCapacity;
            PRINTF("S_Private.now_life=%d  S_Private.max_life=%d\n", gSaveContext.save.info.playerData.health,
                   gSaveContext.save.info.playerData.healthCapacity);
            gSaveContext.healthAccumulator = 0;
        }
    }

    Health_UpdateBeatingHeart(play);
    sEnvHazard = Player_GetEnvironmentalHazard(play);

    if (sEnvHazard == PLAYER_ENV_HAZARD_HOTROOM) {
        if (CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC) == EQUIP_VALUE_TUNIC_CAIRN) {
            sEnvHazard = PLAYER_ENV_HAZARD_NONE;
        }
    } else if ((Player_GetEnvironmentalHazard(play) >= PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) &&
               (Player_GetEnvironmentalHazard(play) <= PLAYER_ENV_HAZARD_UNDERWATER_FREE)) {
        if (CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC) == EQUIP_VALUE_TUNIC_NERI) {
            sEnvHazard = PLAYER_ENV_HAZARD_NONE;
        }
    }

    Health_UpdateMeter(play);

    if ((gSaveContext.timerState >= TIMER_STATE_ENV_HAZARD_MOVE) && !IS_PAUSED(&play->pauseCtx) &&
        (msgCtx->msgMode == MSGMODE_NONE) && !(player->stateFlags2 & PLAYER_STATE2_24) &&
        (play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF) &&
        !Play_InCsMode(play)) {}

    if (gSaveContext.crownAccumulator != 0) {
        if (gSaveContext.crownAccumulator > 0) {
            if (gSaveContext.save.info.playerData.crowns < CUR_CAPACITY(UPG_WALLET)) {
                gSaveContext.crownAccumulator--;
                gSaveContext.save.info.playerData.crowns++;
                SFX_PLAY_CENTERED(NA_SE_SY_RUPY_COUNT);
            } else {
                PRINTF(T("ルピー数ＭＡＸ = %d\n", "Crown Amount MAX = %d\n"), CUR_CAPACITY(UPG_WALLET));
                gSaveContext.save.info.playerData.crowns = CUR_CAPACITY(UPG_WALLET);
                gSaveContext.crownAccumulator = 0;
            }
        } else if (gSaveContext.save.info.playerData.crowns != 0) {
            if (gSaveContext.crownAccumulator <= -50) {
                gSaveContext.crownAccumulator += 10;
                gSaveContext.save.info.playerData.crowns -= 10;

                if (gSaveContext.save.info.playerData.crowns < 0) {
                    gSaveContext.save.info.playerData.crowns = 0;
                }

                SFX_PLAY_CENTERED(NA_SE_SY_RUPY_COUNT);
            } else {
                gSaveContext.crownAccumulator++;
                gSaveContext.save.info.playerData.crowns--;
                SFX_PLAY_CENTERED(NA_SE_SY_RUPY_COUNT);
            }
        } else {
            gSaveContext.crownAccumulator = 0;
        }
    }

    switch (interfaceCtx->unk_1EC) {
        case 1:
            interfaceCtx->unk_1F4 += 31400.0f / WREG(5);
            if (interfaceCtx->unk_1F4 >= 15700.0f) {
                interfaceCtx->unk_1F4 = -15700.0f;
                interfaceCtx->unk_1EC = 2;
            }
            break;
        case 2:
            interfaceCtx->unk_1F4 += 31400.0f / WREG(5);
            if (interfaceCtx->unk_1F4 >= 0.0f) {
                interfaceCtx->unk_1F4 = 0.0f;
                interfaceCtx->unk_1EC = 0;
                interfaceCtx->unk_1EE = interfaceCtx->unk_1F0;
                action = interfaceCtx->unk_1EE;
                if ((action == DO_ACTION_MAX) || (action == DO_ACTION_MAX + 1)) {
                    action = DO_ACTION_NONE;
                }
                Interface_LoadActionLabel(interfaceCtx, action, 0);
            }
            break;
        case 3:
            interfaceCtx->unk_1F4 += 31400.0f / WREG(5);
            if (interfaceCtx->unk_1F4 >= 15700.0f) {
                interfaceCtx->unk_1F4 = -15700.0f;
                interfaceCtx->unk_1EC = 2;
            }
            break;
        case 4:
            interfaceCtx->unk_1F4 += 31400.0f / WREG(5);
            if (interfaceCtx->unk_1F4 >= 0.0f) {
                interfaceCtx->unk_1F4 = 0.0f;
                interfaceCtx->unk_1EC = 0;
                interfaceCtx->unk_1EE = interfaceCtx->unk_1F0;
                action = interfaceCtx->unk_1EE;
                if ((action == DO_ACTION_MAX) || (action == DO_ACTION_MAX + 1)) {
                    action = DO_ACTION_NONE;
                }
                Interface_LoadActionLabel(interfaceCtx, action, 0);
            }
            break;
    }

    WREG(7) = interfaceCtx->unk_1F4;

    // Update Magic
    if (!IS_PAUSED(&play->pauseCtx) && (msgCtx->msgMode == MSGMODE_NONE) &&
        (play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->gameOverCtx.state == GAMEOVER_INACTIVE) &&
        (play->transitionMode == TRANS_MODE_OFF) && ((play->csCtx.state == CS_STATE_IDLE) || !Player_InCsMode(play))) {

        if (gSaveContext.save.info.playerData.isMagicAcquired && (gSaveContext.save.info.playerData.magicLevel == 0)) {
            gSaveContext.save.info.playerData.magicLevel = gSaveContext.save.info.playerData.isDoubleMagicAcquired + 1;
            gSaveContext.magicState = MAGIC_STATE_STEP_CAPACITY;
            PRINTF_COLOR_YELLOW();
            PRINTF(T("魔法スター─────ト！！！！！！！！！\n", "Magic Start!!!!!!!!!\n"));
            PRINTF("MAGIC_MAX=%d\n", gSaveContext.save.info.playerData.magicLevel);
            PRINTF("MAGIC_NOW=%d\n", gSaveContext.save.info.playerData.magic);
            PRINTF("Z_MAGIC_NOW_NOW=%d\n", gSaveContext.magicFillTarget);
            PRINTF("Z_MAGIC_NOW_MAX=%d\n", gSaveContext.magicCapacity);
            PRINTF_RST();
        }

        Magic_Update(play);
    }

    if (gSaveContext.timerState == TIMER_STATE_OFF) {
        if (((sEnvHazard == PLAYER_ENV_HAZARD_HOTROOM) || (sEnvHazard == PLAYER_ENV_HAZARD_UNDERWATER_FLOOR) ||
             (sEnvHazard == PLAYER_ENV_HAZARD_UNDERWATER_FREE)) &&

            ((gSaveContext.save.info.playerData.health >> 1) != 0)) {
            gSaveContext.timerState = TIMER_STATE_ENV_HAZARD_INIT;
            gSaveContext.timerX[TIMER_ID_MAIN] = 140;
            gSaveContext.timerY[TIMER_ID_MAIN] = 80;
            sEnvHazardActive = true;
        }
    } else {
        if (((sEnvHazard == PLAYER_ENV_HAZARD_NONE) || (sEnvHazard == PLAYER_ENV_HAZARD_SWIMMING)) &&
            (gSaveContext.timerState <= TIMER_STATE_ENV_HAZARD_TICK)) {
            gSaveContext.timerState = TIMER_STATE_OFF;
        }
    }

    if (gSaveContext.minigameState == 1) {
        gSaveContext.minigameScore += interfaceCtx->unk_23C;
        interfaceCtx->unk_23C = 0;

        if (sHBAScoreTier == 0) {
            if (gSaveContext.minigameScore >= 1000) {
                sHBAScoreTier++;
            }
        } else if (sHBAScoreTier == 1) {
            if (gSaveContext.minigameScore >= 1500) {
                sHBAScoreTier++;
            }
        }

        sHBAScoreDigits[0] = sHBAScoreDigits[1] = 0;
        sHBAScoreDigits[2] = 0;
        sHBAScoreDigits[3] = gSaveContext.minigameScore;

        while (sHBAScoreDigits[3] >= 1000) {
            sHBAScoreDigits[0]++;
            sHBAScoreDigits[3] -= 1000;
        }

        while (sHBAScoreDigits[3] >= 100) {
            sHBAScoreDigits[1]++;
            sHBAScoreDigits[3] -= 100;
        }

        while (sHBAScoreDigits[3] >= 10) {
            sHBAScoreDigits[2]++;
            sHBAScoreDigits[3] -= 10;
        }
    }

    if (gSaveContext.sunsSongState != DAWNSSONG_INACTIVE) {
        // exit out of flute mode after dawns song finishes playing
        if ((msgCtx->fluteAction != FLUTE_ACTION_CHECK_NOWARP_DONE) &&
            (gSaveContext.sunsSongState == DAWNSSONG_START)) {
            play->msgCtx.fluteMode = FLUTE_MODE_04;
        }

        // handle dawns song in areas where time moves
        if (play->envCtx.sceneTimeSpeed != 0) {
            if (gSaveContext.sunsSongState != DAWNSSONG_SPEED_TIME) {
                D_80125B60 = false;
                if ((gSaveContext.save.dayTime >= CLOCK_TIME(6, 30)) &&
                    (gSaveContext.save.dayTime <= CLOCK_TIME(18, 0) + 1)) {
                    D_80125B60 = true;
                }

                gSaveContext.sunsSongState = DAWNSSONG_SPEED_TIME;
                sPrevTimeSpeed = gTimeSpeed;
                gTimeSpeed = 400;
            } else if (!D_80125B60) {
                if ((gSaveContext.save.dayTime >= CLOCK_TIME(6, 30)) &&
                    (gSaveContext.save.dayTime <= CLOCK_TIME(18, 0) + 1)) {
                    gSaveContext.sunsSongState = DAWNSSONG_INACTIVE;
                    gTimeSpeed = sPrevTimeSpeed;
                    play->msgCtx.fluteMode = FLUTE_MODE_04;
                }
            } else if (gSaveContext.save.dayTime > CLOCK_TIME(18, 0) + 1) {
                gSaveContext.sunsSongState = DAWNSSONG_INACTIVE;
                gTimeSpeed = sPrevTimeSpeed;
                play->msgCtx.fluteMode = FLUTE_MODE_04;
            }
        } else if ((play->roomCtx.curRoom.type != ROOM_TYPE_DUNGEON) && (interfaceCtx->restrictions.sunsSong != 3)) {
            if ((gSaveContext.save.dayTime >= CLOCK_TIME(6, 30)) &&
                (gSaveContext.save.dayTime < CLOCK_TIME(18, 0) + 1)) {
                gSaveContext.nextDayTime = NEXT_TIME_NIGHT;
                play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
                gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
                play->haltAllActors = true;
            } else {
                gSaveContext.nextDayTime = NEXT_TIME_DAY;
                play->transitionType = TRANS_TYPE_FADE_WHITE_FAST;
                gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
                play->haltAllActors = true;
            }

            if (play->sceneId == SCENE_WHISPERING_SANDS) {
                play->transitionType = TRANS_TYPE_SANDSTORM_PERSIST;
                gSaveContext.nextTransitionType = TRANS_TYPE_SANDSTORM_PERSIST;
            }

            gSaveContext.respawnFlag = -2;
            play->nextEntranceIndex = gSaveContext.save.entranceIndex;
            play->transitionTrigger = TRANS_TRIGGER_START;
            gSaveContext.sunsSongState = DAWNSSONG_INACTIVE;
#if OOT_VERSION >= PAL_1_0
            func_800F6964(30);
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
#endif
        } else {
            gSaveContext.sunsSongState = DAWNSSONG_SPECIAL;
        }
    }
}
