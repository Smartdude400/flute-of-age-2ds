// adapted from flute-of-age/src/code/z_inventory.c
#include "game_types.h"
#include "printf.h"
#include "stack_pad.h"
#include "translation.h"
#include "play_state.h"
#include "player.h"
#include "save.h"

#include "assets/textures/icon_item_static/icon_item_static.h"
#include "assets/textures/icon_item_24_static/icon_item_24_static.h"
#include "assets/textures/parameter_static/parameter_static.h"

// Bit Flag array in which gBitFlags[n] is literally (1 << n)
u32 gBitFlags[] = {
    (1 << 0),  (1 << 1),  (1 << 2),  (1 << 3),  (1 << 4),  (1 << 5),  (1 << 6),  (1 << 7),
    (1 << 8),  (1 << 9),  (1 << 10), (1 << 11), (1 << 12), (1 << 13), (1 << 14), (1 << 15),
    (1 << 16), (1 << 17), (1 << 18), (1 << 19), (1 << 20), (1 << 21), (1 << 22), (1 << 23),
    (1 << 24), (1 << 25), (1 << 26), (1 << 27), (1 << 28), (1 << 29), (1 << 30), (1 << 31),
};

u16 gEquipMasks[EQUIP_TYPE_MAX] = {
    0xF << (EQUIP_TYPE_SWORD * 4),  // EQUIP_TYPE_SWORD
    0xF << (EQUIP_TYPE_SHIELD * 4), // EQUIP_TYPE_SHIELD
    0xF << (EQUIP_TYPE_TUNIC * 4),  // EQUIP_TYPE_TUNIC
    0xF << (EQUIP_TYPE_BOOTS * 4),  // EQUIP_TYPE_BOOTS
};
u16 gEquipNegMasks[EQUIP_TYPE_MAX] = {
    (u16) ~(0xF << (EQUIP_TYPE_SWORD * 4)),  // EQUIP_TYPE_SWORD
    (u16) ~(0xF << (EQUIP_TYPE_SHIELD * 4)), // EQUIP_TYPE_SHIELD
    (u16) ~(0xF << (EQUIP_TYPE_TUNIC * 4)),  // EQUIP_TYPE_TUNIC
    (u16) ~(0xF << (EQUIP_TYPE_BOOTS * 4)),  // EQUIP_TYPE_BOOTS
};

u32 gUpgradeMasks[UPG_MAX] = {
    0x00000007, // UPG_QUIVER
    0x00000038, // UPG_BOMB_BAG
    0x000001C0, // UPG_STRENGTH
    0x00000E00, // UPG_SCALE
    0x00003000, // UPG_WALLET
    0x0001C000, // UPG_BULLET_BAG
    0x000E0000, // UPG_ROOT_STICKS
    0x00700000, // UPG_ROOT_NUTS
};
u32 gUpgradeNegMasks[UPG_MAX] = {
    ~0x00000007, // UPG_QUIVER
    ~0x00000038, // UPG_BOMB_BAG
    ~0x000001C0, // UPG_STRENGTH
    ~0x00000E00, // UPG_SCALE
    ~0x00003000, // UPG_WALLET
    ~0x0001C000, // UPG_BULLET_BAG
    ~0x000E0000, // UPG_ROOT_STICKS
    ~0x00700000, // UPG_ROOT_NUTS
};

u8 gEquipShifts[EQUIP_TYPE_MAX] = {
    EQUIP_TYPE_SWORD * 4,  // EQUIP_TYPE_SWORD
    EQUIP_TYPE_SHIELD * 4, // EQUIP_TYPE_SHIELD
    EQUIP_TYPE_TUNIC * 4,  // EQUIP_TYPE_TUNIC
    EQUIP_TYPE_BOOTS * 4,  // EQUIP_TYPE_BOOTS
};

u8 gUpgradeShifts[UPG_MAX] = {
    0,  // UPG_QUIVER
    3,  // UPG_BOMB_BAG
    6,  // UPG_STRENGTH
    9,  // UPG_SCALE
    12, // UPG_WALLET
    14, // UPG_BULLET_BAG
    17, // UPG_ROOT_STICKS
    20, // UPG_ROOT_NUTS
};

u16 gUpgradeCapacities[UPG_MAX][4] = {
    { 0, 30, 40, 50 },     // UPG_QUIVER
    { 0, 20, 30, 40 },     // UPG_BOMB_BAG
    { 0, 0, 0, 0 },        // UPG_STRENGTH (unused)
    { 0, 0, 0, 0 },        // UPG_SCALE (unused)
    { 99, 200, 500, 500 }, // UPG_WALLET
    { 0, 30, 40, 50 },     // UPG_BULLET_BAG
    { 0, 10, 20, 30 },     // UPG_ROOT_STICKS
    { 0, 20, 30, 40 },     // UPG_ROOT_NUTS
};

u32 gGsFlagsMasks[] = { 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 };
u32 gGsFlagsShifts[] = { 0, 8, 16, 24 };

void* gItemIcons[] = {
    // icon_item_static 32x32 rgba32
    gItemIconRootStickTex,         // ITEM_ROOT_STICK
    gItemIconRootNutTex,           // ITEM_ROOT_NUT
    gItemIconBombTex,              // ITEM_BOMB
    gItemIconBowTex,               // ITEM_BOW
    gItemIconArrowFireTex,         // ITEM_ARROW_FIRE
    gItemIconPyrsFireTex,          // ITEM_PYRS_FIRE
    gItemIconSlingshotTex,         // ITEM_SLINGSHOT
    gItemIconFluteFairyTex,      // ITEM_FLUTE_FAIRY
    gItemIconFluteOfAgeTex,     // ITEM_FLUTE_OF_AGE
    gItemIconBombmouseTex,           // ITEM_BOMBMOUSE
    gItemIconGrapnelTex,          // ITEM_GRAPNEL
    gItemIconLonggrappleTex,          // ITEM_LONGGRAPPLE
    gItemIconArrowIceTex,          // ITEM_ARROW_ICE
    gItemIconZephyrsWindTex,       // ITEM_ZEPHYRS_WIND
    gItemIconBoomerangTex,         // ITEM_BOOMERANG
    gItemIconLensOfInsightTex,       // ITEM_LENS_OF_INSIGHT
    gItemIconMagicBeanTex,         // ITEM_MAGIC_BEAN
    gItemIconHammerTex,            // ITEM_HAMMER
    gItemIconArrowLightTex,        // ITEM_ARROW_LIGHT
    gItemIconNerisLoveTex,        // ITEM_NERIS_LOVE
    gItemIconBottleEmptyTex,       // ITEM_BOTTLE_EMPTY
    gItemIconBottlePotionRedTex,   // ITEM_BOTTLE_POTION_RED
    gItemIconBottlePotionGreenTex, // ITEM_BOTTLE_POTION_GREEN
    gItemIconBottlePotionBlueTex,  // ITEM_BOTTLE_POTION_BLUE
    gItemIconBottleFairyTex,       // ITEM_BOTTLE_FAIRY
    gItemIconBottleFishTex,        // ITEM_BOTTLE_FISH
    gItemIconBottleMilkFullTex,    // ITEM_BOTTLE_MILK_FULL
    gItemIconBottleMirasLetterTex, // ITEM_BOTTLE_MIRAS_LETTER
    gItemIconBottleBlueFireTex,    // ITEM_BOTTLE_BLUE_FIRE
    gItemIconBottleBugTex,         // ITEM_BOTTLE_BUG
    gItemIconBottleBigWraithTex,      // ITEM_BOTTLE_BIG_WRAITH
    gItemIconBottleMilkHalfTex,    // ITEM_BOTTLE_MILK_HALF
    gItemIconBottleWraithTex,         // ITEM_BOTTLE_WRAITH
    gItemIconWeirdEggTex,          // ITEM_WEIRD_EGG
    gItemIconChickenTex,           // ITEM_CHICKEN
    gItemIconElarasLetterTex,      // ITEM_ELARAS_LETTER
    gItemIconMaskFoxlingTex,        // ITEM_MASK_FOXLING
    gItemIconMaskSkullTex,         // ITEM_MASK_SKULL
    gItemIconMaskSpookyTex,        // ITEM_MASK_SPOOKY
    gItemIconMaskBunnyHoodTex,     // ITEM_MASK_BUNNY_HOOD
    gItemIconMaskCairnTex,         // ITEM_MASK_CAIRN
    gItemIconMaskNeriTex,          // ITEM_MASK_NERI
    gItemIconMaskDuneanTex,        // ITEM_MASK_DUNEAN
    gItemIconMaskTruthTex,         // ITEM_MASK_TRUTH
    gItemIconSoldOutTex,           // ITEM_SOLD_OUT
    gItemIconPocketEggTex,         // ITEM_POCKET_EGG
    gItemIconPocketCluckerTex,       // ITEM_POCKET_CLUCKER
    gItemIconAzureTex,            // ITEM_AZURE
    gItemIconOddMushroomTex,       // ITEM_ODD_MUSHROOM
    gItemIconOddPotionTex,         // ITEM_ODD_POTION
    gItemIconPoachersSawTex,       // ITEM_POACHERS_SAW
    gItemIconBrokenCairnsSwordTex, // ITEM_BROKEN_CAIRNS_SWORD
    gItemIconPrescriptionTex,      // ITEM_PRESCRIPTION
    gItemIconEyeballFrogTex,       // ITEM_EYEBALL_FROG
    gItemIconEyeDropsTex,          // ITEM_EYE_DROPS
    gItemIconClaimCheckTex,        // ITEM_CLAIM_CHECK
    gItemIconBowFireTex,           // ITEM_BOW_FIRE
    gItemIconBowIceTex,            // ITEM_BOW_ICE
    gItemIconBowLightTex,          // ITEM_BOW_LIGHT
    gItemIconSwordSylvanTex,       // ITEM_SWORD_SYLVAN
    gItemIconSwordMasterTex,       // ITEM_SWORD_MASTER
    gItemIconSwordHighcairnTex,     // ITEM_SWORD_HIGHCAIRN
    gItemIconShieldRootTex,        // ITEM_SHIELD_ROOT
    gItemIconShieldEldranTex,      // ITEM_SHIELD_ELDRAN
    gItemIconShieldMirrorTex,      // ITEM_SHIELD_MIRROR
    gItemIconTunicSylvanTex,       // ITEM_TUNIC_SYLVAN
    gItemIconTunicCairnTex,        // ITEM_TUNIC_CAIRN
    gItemIconTunicNeriTex,         // ITEM_TUNIC_NERI
    gItemIconBootsSylvanTex,       // ITEM_BOOTS_SYLVAN
    gItemIconBootsIronTex,         // ITEM_BOOTS_IRON
    gItemIconBootsHoverTex,        // ITEM_BOOTS_HOVER
    gItemIconBulletBag30Tex,       // ITEM_BULLET_BAG_30
    gItemIconBulletBag40Tex,       // ITEM_BULLET_BAG_40
    gItemIconBulletBag50Tex,       // ITEM_BULLET_BAG_50
    gItemIconQuiver30Tex,          // ITEM_QUIVER_30
    gItemIconQuiver40Tex,          // ITEM_QUIVER_40
    gItemIconQuiver50Tex,          // ITEM_QUIVER_50
    gItemIconBombBag20Tex,         // ITEM_BOMB_BAG_20
    gItemIconBombBag30Tex,         // ITEM_BOMB_BAG_30
    gItemIconBombBag40Tex,         // ITEM_BOMB_BAG_40
    gItemIconCairnsBraceletTex,    // ITEM_STRENGTH_CAIRNS_BRACELET
    gItemIconSilverGauntletsTex,   // ITEM_STRENGTH_SILVER_GAUNTLETS
    gItemIconGoldenGauntletsTex,   // ITEM_STRENGTH_GOLD_GAUNTLETS
    gItemIconScaleSilverTex,       // ITEM_SCALE_SILVER
    gItemIconScaleGoldenTex,       // ITEM_SCALE_GOLDEN
    gItemIconBrokenGiantsKnifeTex, // ITEM_GIANTS_KNIFE
    gItemIconAdultsWalletTex,      // ITEM_ADULTS_WALLET
    gItemIconGiantsWalletTex,      // ITEM_GIANTS_WALLET
    gItemIconRootSeedsTex,         // ITEM_ROOT_SEEDS
    gItemIconFishingPoleTex,       // ITEM_FISHING_POLE
    // icon_item_static 16x24 ia8
    gSongNoteTex, // ITEM_SONG_MINUET
    gSongNoteTex, // ITEM_SONG_BOLERO
    gSongNoteTex, // ITEM_SONG_SERENADE
    gSongNoteTex, // ITEM_SONG_REQUIEM
    gSongNoteTex, // ITEM_SONG_NOCTURNE
    gSongNoteTex, // ITEM_SONG_PRELUDE
    gSongNoteTex, // ITEM_SONG_LULLABY
    gSongNoteTex, // ITEM_SONG_ASTER
    gSongNoteTex, // ITEM_SONG_SYLVA
    gSongNoteTex, // ITEM_SONG_SUN
    gSongNoteTex, // ITEM_SONG_TIME
    gSongNoteTex, // ITEM_SONG_STORMS
    // icon_item_24static 24x24 rgba32
    gQuestIconMedallionForestTex, // ITEM_MEDALLION_FOREST
    gQuestIconMedallionFireTex,   // ITEM_MEDALLION_FIRE
    gQuestIconMedallionWaterTex,  // ITEM_MEDALLION_WATER
    gQuestIconMedallionSpiritTex, // ITEM_MEDALLION_SPIRIT
    gQuestIconMedallionShadowTex, // ITEM_MEDALLION_SHADOW
    gQuestIconMedallionLightTex,  // ITEM_MEDALLION_LIGHT
    gQuestIconSylvanEmeraldTex,   // ITEM_SYLVAN_EMERALD
    gQuestIconCairnRubyTex,       // ITEM_CAIRN_RUBY
    gQuestIconNeriSapphireTex,    // ITEM_NERI_SAPPHIRE
    gQuestIconStoneOfEchoesTex,    // ITEM_STONE_OF_ECHOES
    gQuestIconDuneansCardTex,     // ITEM_DUNEANS_CARD
    gQuestIconGoldWebguardTex,   // ITEM_SKULL_TOKEN
    gQuestIconHeartContainerTex,  // ITEM_HEART_CONTAINER
    gQuestIconHeartPieceTex,      // ITEM_HEART_PIECE
    gQuestIconDungeonBossKeyTex,  // ITEM_DUNGEON_BOSS_KEY
    gQuestIconDungeonMapTex,      // ITEM_DUNGEON_COMPASS (swapped with map (inconsequential))
    gQuestIconDungeonCompassTex,  // ITEM_DUNGEON_MAP (swapped with compass (inconsequential))
    gQuestIconDungeonBossKeyTex,  // ITEM_SMALL_KEY (but texture is boss key (inconsequential))
    gQuestIconMagicJarSmallTex,   // ITEM_MAGIC_JAR_SMALL
    gQuestIconMagicJarBigTex,     // ITEM_MAGIC_JAR_BIG
    // icon_item_static 48x48 ia8
    gHeartPieceIcon1Tex, // ITEM_HEART_PIECE_2
    gHeartPieceIcon2Tex, // ITEM_INVALID_1
    gHeartPieceIcon3Tex, // ITEM_INVALID_2
    // parameter_static 16x16 ia8
    gFluteBtnIconCUpTex,    // ITEM_INVALID_3
    gFluteBtnIconCDownTex,  // ITEM_INVALID_4
    gFluteBtnIconCLeftTex,  // ITEM_INVALID_5
    gFluteBtnIconCRightTex, // ITEM_INVALID_6
    gFluteBtnIconATex,      // ITEM_INVALID_7
};

// Used to map item IDs to inventory slots
u8 gItemSlots[] = {
    SLOT_ROOT_STICK,    // ITEM_ROOT_STICK
    SLOT_ROOT_NUT,      // ITEM_ROOT_NUT
    SLOT_BOMB,          // ITEM_BOMB
    SLOT_BOW,           // ITEM_BOW
    SLOT_ARROW_FIRE,    // ITEM_ARROW_FIRE
    SLOT_PYRS_FIRE,     // ITEM_PYRS_FIRE
    SLOT_SLINGSHOT,     // ITEM_SLINGSHOT
    SLOT_FLUTE,       // ITEM_FLUTE_FAIRY
    SLOT_FLUTE,       // ITEM_FLUTE_OF_AGE
    SLOT_BOMBMOUSE,       // ITEM_BOMBMOUSE
    SLOT_GRAPNEL,      // ITEM_GRAPNEL
    SLOT_GRAPNEL,      // ITEM_LONGGRAPPLE
    SLOT_ARROW_ICE,     // ITEM_ARROW_ICE
    SLOT_ZEPHYRS_WIND,  // ITEM_ZEPHYRS_WIND
    SLOT_BOOMERANG,     // ITEM_BOOMERANG
    SLOT_LENS_OF_INSIGHT, // ITEM_LENS_OF_INSIGHT
    SLOT_MAGIC_BEAN,    // ITEM_MAGIC_BEAN
    SLOT_HAMMER,        // ITEM_HAMMER
    SLOT_ARROW_LIGHT,   // ITEM_ARROW_LIGHT
    SLOT_NERIS_LOVE,   // ITEM_NERIS_LOVE
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_EMPTY
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_POTION_RED
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_POTION_GREEN
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_POTION_BLUE
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_FAIRY
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_FISH
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_MILK_FULL
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_MIRAS_LETTER
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_BLUE_FIRE
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_BUG
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_BIG_WRAITH
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_MILK_HALF
    SLOT_BOTTLE_1,      // ITEM_BOTTLE_WRAITH
    SLOT_TRADE_CHILD,   // ITEM_WEIRD_EGG
    SLOT_TRADE_CHILD,   // ITEM_CHICKEN
    SLOT_TRADE_CHILD,   // ITEM_ELARAS_LETTER
    SLOT_TRADE_CHILD,   // ITEM_MASK_FOXLING
    SLOT_TRADE_CHILD,   // ITEM_MASK_SKULL
    SLOT_TRADE_CHILD,   // ITEM_MASK_SPOOKY
    SLOT_TRADE_CHILD,   // ITEM_MASK_BUNNY_HOOD
    SLOT_TRADE_CHILD,   // ITEM_MASK_CAIRN
    SLOT_TRADE_CHILD,   // ITEM_MASK_NERI
    SLOT_TRADE_CHILD,   // ITEM_MASK_DUNEAN
    SLOT_TRADE_CHILD,   // ITEM_MASK_TRUTH
    SLOT_TRADE_CHILD,   // ITEM_SOLD_OUT
    SLOT_TRADE_ADULT,   // ITEM_POCKET_EGG
    SLOT_TRADE_ADULT,   // ITEM_POCKET_CLUCKER
    SLOT_TRADE_ADULT,   // ITEM_AZURE
    SLOT_TRADE_ADULT,   // ITEM_ODD_MUSHROOM
    SLOT_TRADE_ADULT,   // ITEM_ODD_POTION
    SLOT_TRADE_ADULT,   // ITEM_POACHERS_SAW
    SLOT_TRADE_ADULT,   // ITEM_BROKEN_CAIRNS_SWORD
    SLOT_TRADE_ADULT,   // ITEM_PRESCRIPTION
    SLOT_TRADE_ADULT,   // ITEM_EYEBALL_FROG
    SLOT_TRADE_ADULT,   // ITEM_EYE_DROPS
    SLOT_TRADE_ADULT,   // ITEM_CLAIM_CHECK
};

void Inventory_ChangeEquipment(s16 equipment, u16 value) {
    gSaveContext.save.info.equips.equipment &= gEquipNegMasks[equipment];
    gSaveContext.save.info.equips.equipment |= value << gEquipShifts[equipment];
}

u8 Inventory_DeleteEquipment(PlayState* play, s16 equipment) {
    Player* player = GET_PLAYER(play);
    STACK_PAD(s32);
    u16 equipValue = gSaveContext.save.info.equips.equipment & gEquipMasks[equipment];

    PRINTF(T("装備アイテム抹消 = %d  zzz=%d\n", "Erasing equipment item = %d  zzz=%d\n"), equipment, equipValue);

    if (equipValue) {
        equipValue >>= gEquipShifts[equipment];

        gSaveContext.save.info.equips.equipment &= gEquipNegMasks[equipment];
        gSaveContext.save.info.inventory.equipment ^= OWNED_EQUIP_FLAG(equipment, equipValue - 1);

        if (equipment == EQUIP_TYPE_TUNIC) {
            gSaveContext.save.info.equips.equipment |= EQUIP_VALUE_TUNIC_SYLVAN << (EQUIP_TYPE_TUNIC * 4);
        }

        if (equipment == EQUIP_TYPE_SWORD) {
            gSaveContext.save.info.equips.buttonItems[0] = ITEM_NONE;
            gSaveContext.save.info.infTable[INFTABLE_INDEX_1DX] = 1;
        }

        Player_SetEquipmentData(play, player);
        play->pauseCtx.cursorSpecialPos = PAUSE_CURSOR_PAGE_LEFT;
    }

    return equipValue;
}

void Inventory_ChangeUpgrade(s16 upgrade, s16 value) {
    gSaveContext.save.info.inventory.upgrades &= gUpgradeNegMasks[upgrade];
    gSaveContext.save.info.inventory.upgrades |= value << gUpgradeShifts[upgrade];
}
