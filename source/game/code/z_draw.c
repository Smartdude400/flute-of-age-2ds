// adapted from flute-of-age/src/code/z_draw.c
#include "gfx.h"
#include "engine/gfx3d.h"
#include "gfx_setupdl.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "draw.h"
#include "play_state.h"

#include "assets/objects/object_gi_key/object_gi_key.h"
#include "assets/objects/object_gi_jewel/object_gi_jewel.h"
#include "assets/objects/object_gi_melody/object_gi_melody.h"
#include "assets/objects/object_gi_heart/object_gi_heart.h"
#include "assets/objects/object_gi_compass/object_gi_compass.h"
#include "assets/objects/object_gi_bosskey/object_gi_bosskey.h"
#include "assets/objects/object_gi_medal/object_gi_medal.h"
#include "assets/objects/object_gi_nuts/object_gi_nuts.h"
#include "assets/objects/object_gi_hearts/object_gi_hearts.h"
#include "assets/objects/object_gi_arrowcase/object_gi_arrowcase.h"
#include "assets/objects/object_gi_bombpouch/object_gi_bombpouch.h"
#include "assets/objects/object_gi_bottle/object_gi_bottle.h"
#include "assets/objects/object_gi_stick/object_gi_stick.h"
#include "assets/objects/object_gi_map/object_gi_map.h"
#include "assets/objects/object_gi_shield_1/object_gi_shield_1.h"
#include "assets/objects/object_gi_magicpot/object_gi_magicpot.h"
#include "assets/objects/object_gi_bomb_1/object_gi_bomb_1.h"
#include "assets/objects/object_gi_purse/object_gi_purse.h"
#include "assets/objects/object_gi_dunean/object_gi_dunean.h"
#include "assets/objects/object_gi_arrow/object_gi_arrow.h"
#include "assets/objects/object_gi_bomb_2/object_gi_bomb_2.h"
#include "assets/objects/object_gi_egg/object_gi_egg.h"
#include "assets/objects/object_gi_scale/object_gi_scale.h"
#include "assets/objects/object_gi_shield_2/object_gi_shield_2.h"
#include "assets/objects/object_gi_grapnel/object_gi_grapnel.h"
#include "assets/objects/object_gi_flute/object_gi_flute.h"
#include "assets/objects/object_gi_milk/object_gi_milk.h"
#include "assets/objects/object_gi_pachinko/object_gi_pachinko.h"
#include "assets/objects/object_gi_boomerang/object_gi_boomerang.h"
#include "assets/objects/object_gi_bow/object_gi_bow.h"
#include "assets/objects/object_gi_glasses/object_gi_glasses.h"
#include "assets/objects/object_gi_liquid/object_gi_liquid.h"
#include "assets/objects/object_gi_shield_3/object_gi_shield_3.h"
#include "assets/objects/object_gi_letter/object_gi_letter.h"
#include "assets/objects/object_gi_clothes/object_gi_clothes.h"
#include "assets/objects/object_gi_bean/object_gi_bean.h"
#include "assets/objects/object_gi_fish/object_gi_fish.h"
#include "assets/objects/object_gi_saw/object_gi_saw.h"
#include "assets/objects/object_gi_hammer/object_gi_hammer.h"
#include "assets/objects/object_gi_grass/object_gi_grass.h"
#include "assets/objects/object_gi_longsword/object_gi_longsword.h"
#include "assets/objects/object_gi_niwatori/object_gi_niwatori.h"
#include "assets/objects/object_gi_bottle_letter/object_gi_bottle_letter.h"
#include "assets/objects/object_gi_flute_0/object_gi_flute_0.h"
#include "assets/objects/object_gi_boots_2/object_gi_boots_2.h"
#include "assets/objects/object_gi_seed/object_gi_seed.h"
#include "assets/objects/object_gi_gloves/object_gi_gloves.h"
#include "assets/objects/object_gi_coin/object_gi_coin.h"
#include "assets/objects/object_gi_ki_tan_mask/object_gi_ki_tan_mask.h"
#include "assets/objects/object_gi_dreadbound_mask/object_gi_dreadbound_mask.h"
#include "assets/objects/object_gi_skj_mask/object_gi_skj_mask.h"
#include "assets/objects/object_gi_rabit_mask/object_gi_rabit_mask.h"
#include "assets/objects/object_gi_truth_mask/object_gi_truth_mask.h"
#include "assets/objects/object_gi_eye_lotion/object_gi_eye_lotion.h"
#include "assets/objects/object_gi_powder/object_gi_powder.h"
#include "assets/objects/object_gi_mushroom/object_gi_mushroom.h"
#include "assets/objects/object_gi_ticketstone/object_gi_ticketstone.h"
#include "assets/objects/object_gi_brokensword/object_gi_brokensword.h"
#include "assets/objects/object_gi_prescription/object_gi_prescription.h"
#include "assets/objects/object_gi_bracelet/object_gi_bracelet.h"
#include "assets/objects/object_gi_soldout/object_gi_soldout.h"
#include "assets/objects/object_gi_frog/object_gi_frog.h"
#include "assets/objects/object_gi_golonmask/object_gi_golonmask.h"
#include "assets/objects/object_gi_zoramask/object_gi_zoramask.h"
#include "assets/objects/object_gi_gerudomask/object_gi_gerudomask.h"
#include "assets/objects/object_gi_hoverboots/object_gi_hoverboots.h"
#include "assets/objects/object_gi_m_arrow/object_gi_m_arrow.h"
#include "assets/objects/object_gi_sutaru/object_gi_sutaru.h"
#include "assets/objects/object_gi_goddess/object_gi_goddess.h"
#include "assets/objects/object_gi_fire/object_gi_fire.h"
#include "assets/objects/object_gi_insect/object_gi_insect.h"
#include "assets/objects/object_gi_butterfly/object_gi_butterfly.h"
#include "assets/objects/object_gi_ghost/object_gi_ghost.h"
#include "assets/objects/object_gi_soul/object_gi_soul.h"
#include "assets/objects/object_gi_dekupouch/object_gi_dekupouch.h"
#include "assets/objects/object_gi_rupy/object_gi_rupy.h"
#include "assets/objects/object_gi_sword_1/object_gi_sword_1.h"
#include "assets/objects/object_st/object_st.h"

// "Get Item" Model Draw Functions
void GetItem_DrawMaskOrBombmouse(PlayState* play, s16 giDrawId);
void GetItem_DrawSoldOut(PlayState* play, s16 giDrawId);
void GetItem_DrawBlueFire(PlayState* play, s16 giDrawId);
void GetItem_DrawWraiths(PlayState* play, s16 giDrawId);
void GetItem_DrawFairy(PlayState* play, s16 giDrawId);
void GetItem_DrawPrismShield(PlayState* play, s16 giDrawId);
void GetItem_DrawSkullToken(PlayState* play, s16 giDrawId);
void GetItem_DrawEggOrMedallion(PlayState* play, s16 giDrawId);
void GetItem_DrawCompass(PlayState* play, s16 giDrawId);
void GetItem_DrawPotion(PlayState* play, s16 giDrawId);
void GetItem_DrawCairnSword(PlayState* play, s16 giDrawId);
void GetItem_DrawRootNuts(PlayState* play, s16 giDrawId);
void GetItem_DrawRecoveryHeart(PlayState* play, s16 giDrawId);
void GetItem_DrawFish(PlayState* play, s16 giDrawId);
void GetItem_DrawOpa0(PlayState* play, s16 giDrawId);
void GetItem_DrawOpa0Xlu1(PlayState* play, s16 giDrawId);
void GetItem_DrawXlu01(PlayState* play, s16 giDrawId);
void GetItem_DrawOpa10Xlu2(PlayState* play, s16 giDrawId);
void GetItem_DrawMagicArrow(PlayState* play, s16 giDrawId);
void GetItem_DrawMagicSpell(PlayState* play, s16 giDrawId);
void GetItem_DrawOpa1023(PlayState* play, s16 giDrawId);
void GetItem_DrawOpa10Xlu32(PlayState* play, s16 giDrawId);
void GetItem_DrawSmallCrown(PlayState* play, s16 giDrawId);
void GetItem_DrawScale(PlayState* play, s16 giDrawId);
void GetItem_DrawBulletBag(PlayState* play, s16 giDrawId);
void GetItem_DrawWallet(PlayState* play, s16 giDrawId);

typedef struct DrawItemTableEntry {
    /* 0x00 */ void (*drawFunc)(PlayState*, s16);
    /* 0x04 */ DListRef dlists[8];
} DrawItemTableEntry; // size = 0x24

DrawItemTableEntry sDrawItemTable[] = {
    // GID_BOTTLE_EMPTY
    { GetItem_DrawOpa0Xlu1, { gGiBottleStopperDL, gGiBottleDL } },
    // GID_SMALL_KEY
    { GetItem_DrawOpa0, { gGiSmallKeyDL } },
    // GID_SONG_MINUET
    { GetItem_DrawXlu01, { gGiMinuetColorDL, gGiSongNoteDL } },
    // GID_SONG_BOLERO
    { GetItem_DrawXlu01, { gGiBoleroColorDL, gGiSongNoteDL } },
    // GID_SONG_SERENADE
    { GetItem_DrawXlu01, { gGiSerenadeColorDL, gGiSongNoteDL } },
    // GID_SONG_REQUIEM
    { GetItem_DrawXlu01, { gGiRequiemColorDL, gGiSongNoteDL } },
    // GID_SONG_NOCTURNE
    { GetItem_DrawXlu01, { gGiNocturneColorDL, gGiSongNoteDL } },
    // GID_SONG_PRELUDE
    { GetItem_DrawXlu01, { gGiPreludeColorDL, gGiSongNoteDL } },
    // GID_RECOVERY_HEART
    { GetItem_DrawRecoveryHeart, { gGiRecoveryHeartDL } },
    // GID_BOSS_KEY
    { GetItem_DrawOpa0Xlu1, { gGiBossKeyDL, gGiBossKeyGemDL } },
    // GID_COMPASS
    { GetItem_DrawCompass, { gGiCompassDL, gGiCompassGlassDL } },
    // GID_MEDALLION_FOREST
    { GetItem_DrawEggOrMedallion, { gGiForestMedallionFaceDL, gGiMedallionDL } },
    // GID_MEDALLION_FIRE
    { GetItem_DrawEggOrMedallion, { gGiFireMedallionFaceDL, gGiMedallionDL } },
    // GID_MEDALLION_WATER
    { GetItem_DrawEggOrMedallion, { gGiWaterMedallionFaceDL, gGiMedallionDL } },
    // GID_MEDALLION_SPIRIT
    { GetItem_DrawEggOrMedallion, { gGiSpiritMedallionFaceDL, gGiMedallionDL } },
    // GID_MEDALLION_SHADOW
    { GetItem_DrawEggOrMedallion, { gGiShadowMedallionFaceDL, gGiMedallionDL } },
    // GID_MEDALLION_LIGHT
    { GetItem_DrawEggOrMedallion, { gGiLightMedallionFaceDL, gGiMedallionDL } },
    // GID_ROOT_NUTS
    { GetItem_DrawRootNuts, { gGiNutDL } },
    // GID_HEART_CONTAINER
    { GetItem_DrawXlu01, { gGiHeartBorderDL, gGiHeartContainerDL } },
    // GID_HEART_PIECE
    { GetItem_DrawXlu01, { gGiHeartBorderDL, gGiHeartPieceDL } },
    // GID_QUIVER_30
    { GetItem_DrawOpa1023, { gGiQuiverInnerDL, gGiQuiver30InnerColorDL, gGiQuiver30OuterColorDL, gGiQuiverOuterDL } },
    // GID_QUIVER_40
    { GetItem_DrawOpa1023, { gGiQuiverInnerDL, gGiQuiver40InnerColorDL, gGiQuiver40OuterColorDL, gGiQuiverOuterDL } },
    // GID_QUIVER_50
    { GetItem_DrawOpa1023, { gGiQuiverInnerDL, gGiQuiver50InnerColorDL, gGiQuiver50OuterColorDL, gGiQuiverOuterDL } },
    // GID_BOMB_BAG_20
    { GetItem_DrawOpa1023, { gGiBombBagDL, gGiBombBag20BagColorDL, gGiBombBag20RingColorDL, gGiBombBagRingDL } },
    // GID_BOMB_BAG_30
    { GetItem_DrawOpa1023, { gGiBombBagDL, gGiBombBag30BagColorDL, gGiBombBag30RingColorDL, gGiBombBagRingDL } },
    // GID_BOMB_BAG_40
    { GetItem_DrawOpa1023, { gGiBombBagDL, gGiBombBag40BagColorDL, gGiBombBag40RingColorDL, gGiBombBagRingDL } },
    // GID_ROOT_STICK
    { GetItem_DrawOpa0, { gGiStickDL } },
    // GID_DUNGEON_MAP
    { GetItem_DrawOpa0, { gGiDungeonMapDL } },
    // GID_SHIELD_ROOT
    { GetItem_DrawOpa0, { gGiRootShieldDL } },
    // GID_MAGIC_JAR_SMALL
    { GetItem_DrawOpa0, { gGiMagicJarSmallDL } },
    // GID_MAGIC_JAR_LARGE
    { GetItem_DrawOpa0, { gGiMagicJarLargeDL } },
    // GID_BOMB
    { GetItem_DrawOpa0, { gGiBombDL } },
    // GID_STONE_OF_ECHOES
    { GetItem_DrawOpa0, { gGiStoneOfEchoesDL } },
    // GID_WALLET_ADULT
    { GetItem_DrawWallet,
      { gGiWalletDL, gGiAdultWalletColorDL, gGiAdultWalletCrownOuterColorDL, gGiWalletCrownOuterDL,
        gGiAdultWalletStringColorDL, gGiWalletStringDL, gGiAdultWalletCrownInnerColorDL, gGiWalletCrownInnerDL } },
    // GID_WALLET_GIANT
    { GetItem_DrawWallet,
      { gGiWalletDL, gGiGiantsWalletColorDL, gGiGiantsWalletCrownOuterColorDL, gGiWalletCrownOuterDL,
        gGiGiantsWalletStringColorDL, gGiWalletStringDL, gGiGiantsWalletCrownInnerColorDL, gGiWalletCrownInnerDL } },
    // GID_DUNEANS_CARD
    { GetItem_DrawOpa0, { gGiDuneanCardDL } },
    // GID_ARROWS_5
    { GetItem_DrawOpa0, { gGiArrowSmallDL } },
    // GID_ARROWS_10
    { GetItem_DrawOpa0, { gGiArrowMediumDL } },
    // GID_ARROWS_30
    { GetItem_DrawOpa0, { gGiArrowLargeDL } },
    // GID_BOMBMOUSE
    { GetItem_DrawMaskOrBombmouse, { gGiBombmouseDL } },
    // GID_EGG
    { GetItem_DrawEggOrMedallion, { gGiEggMaterialDL, gGiEggDL } },
    // GID_SCALE_SILVER
    { GetItem_DrawScale, { gGiScaleWaterDL, gGiSilverScaleWaterColorDL, gGiSilverScaleColorDL, gGiScaleDL } },
    // GID_SCALE_GOLDEN
    { GetItem_DrawScale, { gGiScaleWaterDL, gGiGoldenScaleWaterColorDL, gGiGoldenScaleColorDL, gGiScaleDL } },
    // GID_SHIELD_ELDRAN
    { GetItem_DrawOpa0, { gGiEldranShieldDL } },
    // GID_GRAPNEL
    { GetItem_DrawOpa0, { gGiGrapnelDL } },
    // GID_LONGGRAPPLE
    { GetItem_DrawOpa0, { gGiLonggrappleDL } },
    // GID_FLUTE_OF_AGE
    { GetItem_DrawOpa0Xlu1, { gGiFluteTimeDL, gGiFluteTimeHolesDL } },
    // GID_BOTTLE_MILK_FULL
    { GetItem_DrawOpa0Xlu1, { gGiMilkBottleContentsDL, gGiMilkBottleDL } },
    // GID_MASK_FOXLING
    { GetItem_DrawOpa0Xlu1, { gGiFoxlingMaskDL, gGiFoxlingMaskEyesDL } },
    // GID_MASK_SPOOKY
    { GetItem_DrawOpa0, { gGiSpookyMaskDL } },
    // GID_SLINGSHOT
    { GetItem_DrawOpa0, { gGiSlingshotDL } },
    // GID_BOOMERANG
    { GetItem_DrawOpa0, { gGiBoomerangDL } },
    // GID_BOW
    { GetItem_DrawOpa0, { gGiBowDL } },
    // GID_LENS_OF_INSIGHT
    { GetItem_DrawOpa0Xlu1, { gGiLensDL, gGiLensGlassDL } },
    // GID_BOTTLE_POTION_GREEN
    { GetItem_DrawPotion,
      { gGiPotionPotDL, gGiGreenPotColorDL, gGiGreenLiquidColorDL, gGiPotionLiquidDL, gGiGreenPatternColorDL,
        gGiPotionPatternDL } },
    // GID_BOTTLE_POTION_RED
    { GetItem_DrawPotion,
      { gGiPotionPotDL, gGiRedPotColorDL, gGiRedLiquidColorDL, gGiPotionLiquidDL, gGiRedPatternColorDL,
        gGiPotionPatternDL } },
    // GID_BOTTLE_POTION_BLUE
    { GetItem_DrawPotion,
      { gGiPotionPotDL, gGiBluePotColorDL, gGiBlueLiquidColorDL, gGiPotionLiquidDL, gGiBluePatternColorDL,
        gGiPotionPatternDL } },
    // GID_SHIELD_MIRROR
    { GetItem_DrawPrismShield, { gGiPrismShieldDL, gGiPrismShieldSymbolDL } },
    // GID_ELARAS_LETTER
    { GetItem_DrawOpa0Xlu1, { gGiLetterDL, gGiLetterWritingDL } },
    // GID_TUNIC_CAIRN
    { GetItem_DrawOpa1023, { gGiTunicCollarDL, gGiCairnCollarColorDL, gGiCairnTunicColorDL, gGiTunicDL } },
    // GID_TUNIC_NERI
    { GetItem_DrawOpa1023, { gGiTunicCollarDL, gGiNeriCollarColorDL, gGiNeriTunicColorDL, gGiTunicDL } },
    // GID_MAGIC_BEAN
    { GetItem_DrawOpa0, { gGiBeanDL } },
    // GID_FISH
    { GetItem_DrawFish, { gGiFishDL } },
    // GID_POACHERS_SAW
    { GetItem_DrawOpa0, { gGiSawDL } },
    // GID_HAMMER
    { GetItem_DrawOpa0, { gGiHammerDL } },
    // GID_GRASS
    { GetItem_DrawOpa0, { gGiGrassDL } },
    // GID_SWORD_HIGHCAIRN
    { GetItem_DrawCairnSword, { gGiHighcairnSwordDL } },
    // GID_CLUCKER
    { GetItem_DrawOpa10Xlu2, { gGiChickenDL, gGiChickenColorDL, gGiChickenEyesDL } },
    // GID_BOTTLE_MIRAS_LETTER
    { GetItem_DrawOpa0Xlu1, { gGiLetterBottleContentsDL, gGiLetterBottleDL } },
    // GID_FLUTE_FAIRY
    { GetItem_DrawOpa0Xlu1, { gGiFluteFairyDL, gGiFluteFairyHolesDL } },
    // GID_BOOTS_IRON
    { GetItem_DrawOpa0Xlu1, { gGiIronBootsDL, gGiIronBootsRivetsDL } },
    // GID_ROOT_SEEDS
    { GetItem_DrawOpa0, { gGiSeedDL } },
    // GID_SILVER_GAUNTLETS
    { GetItem_DrawOpa10Xlu32,
      { gGiGauntletsDL, gGiSilverGauntletsColorDL, gGiGauntletsPlateDL, gGiSilverGauntletsPlateColorDL } },
    // GID_GOLD_GAUNTLETS
    { GetItem_DrawOpa10Xlu32,
      { gGiGauntletsDL, gGiGoldenGauntletsColorDL, gGiGauntletsPlateDL, gGiGoldenGauntletsPlateColorDL } },
    // GID_NCOIN_YELLOW
    { GetItem_DrawOpa10Xlu2, { gGiCoinDL, gGiYellowCoinColorDL, gGiNDL } },
    // GID_NCOIN_RED
    { GetItem_DrawOpa10Xlu2, { gGiCoinDL, gGiRedCoinColorDL, gGiNDL } },
    // GID_NCOIN_GREEN
    { GetItem_DrawOpa10Xlu2, { gGiCoinDL, gGiGreenCoinColorDL, gGiNDL } },
    // GID_NCOIN_BLUE
    { GetItem_DrawOpa10Xlu2, { gGiCoinDL, gGiBlueCoinColorDL, gGiNDL } },
    // GID_MASK_SKULL
    { GetItem_DrawOpa0, { gGiSkullMaskDL } },
    // GID_MASK_BUNNY_HOOD
    { GetItem_DrawOpa0Xlu1, { gGiBunnyHoodDL, gGiBunnyHoodEyesDL } },
    // GID_MASK_TRUTH
    { GetItem_DrawOpa0Xlu1, { gGiMaskOfTruthDL, gGiMaskOfTruthAccentsDL } },
    // GID_EYE_DROPS
    { GetItem_DrawOpa0Xlu1, { gGiEyeDropsCapDL, gGiEyeDropsBottleDL } },
    // GID_ODD_POTION
    { GetItem_DrawOpa0, { gGiOddPotionDL } },
    // GID_ODD_MUSHROOM
    { GetItem_DrawOpa0, { gGiOddMushroomDL } },
    // GID_CLAIM_CHECK
    { GetItem_DrawOpa0Xlu1, { gGiClaimCheckDL, gGiClaimCheckWritingDL } },
    // GID_BROKEN_CAIRNS_SWORD
    { GetItem_DrawCairnSword, { gGiBrokenCairnSwordDL } },
    // GID_PRESCRIPTION
    { GetItem_DrawOpa0Xlu1, { gGiPrescriptionDL, gGiPrescriptionWritingDL } },
    // GID_CAIRNS_BRACELET
    { GetItem_DrawOpa0, { gGiCairnBraceletDL } },
    // GID_SOLDOUT
    { GetItem_DrawSoldOut, { gGiSoldOutDL } },
    // GID_EYEBALL_FROG
    { GetItem_DrawOpa0Xlu1, { gGiFrogDL, gGiFrogEyesDL } },
    // GID_MASK_CAIRN
    { GetItem_DrawMaskOrBombmouse, { gGiCairnMaskDL } },
    // GID_MASK_NERI
    { GetItem_DrawMaskOrBombmouse, { gGiNeriMaskDL } },
    // GID_MASK_DUNEAN
    { GetItem_DrawMaskOrBombmouse, { gGiDuneanMaskDL } },
    // GID_AZURE
    { GetItem_DrawOpa10Xlu2, { gGiChickenDL, gGiAzureColorDL, gGiChickenEyesDL } },
    // GID_BOOTS_HOVER
    { GetItem_DrawOpa0, { gGiHoverBootsDL } },
    // GID_ARROW_FIRE
    { GetItem_DrawMagicArrow, { gGiMagicArrowDL, gGiFireArrowColorDL, gGiArrowMagicDL } },
    // GID_ARROW_ICE
    { GetItem_DrawMagicArrow, { gGiMagicArrowDL, gGiIceArrowColorDL, gGiArrowMagicDL } },
    // GID_ARROW_LIGHT
    { GetItem_DrawMagicArrow, { gGiMagicArrowDL, gGiLightArrowColorDL, gGiArrowMagicDL } },
    // GID_SKULL_TOKEN
    { GetItem_DrawSkullToken, { gGiWebguardTokenDL, gGiWebguardTokenFlameDL } },
    // GID_PYRS_FIRE
    { GetItem_DrawMagicSpell, { gGiMagicSpellDiamondDL, gGiPyrsFireColorDL, gGiMagicSpellOrbDL } },
    // GID_ZEPHYRS_WIND
    { GetItem_DrawMagicSpell, { gGiMagicSpellDiamondDL, gGiZephyrsWindColorDL, gGiMagicSpellOrbDL } },
    // GID_NERIS_LOVE
    { GetItem_DrawMagicSpell, { gGiMagicSpellDiamondDL, gGiNerisLoveColorDL, gGiMagicSpellOrbDL } },
    // GID_BLUE_FIRE
    { GetItem_DrawBlueFire, { gGiBlueFireChamberstickDL, gGiBlueFireFlameDL } },
    // GID_BUG
    { GetItem_DrawOpa0Xlu1, { gGiBugsContainerDL, gGiBugsGlassDL } },
    // GID_BUTTERFLY
    { GetItem_DrawOpa0Xlu1, { gGiButterflyContainerDL, gGiButterflyGlassDL } },
    // GID_WRAITH
    { GetItem_DrawWraiths,
      { gGiGhostContainerLidDL, gGiGhostContainerGlassDL, gGiGhostContainerContentsDL, gGiWraithColorDL } },
    // GID_FAIRY
    { GetItem_DrawFairy, { gGiFairyContainerBaseCapDL, gGiFairyContainerGlassDL, gGiFairyContainerContentsDL } },
    // GID_BULLET_BAG
    { GetItem_DrawBulletBag,
      { gGiBulletBagDL, gGiBulletBagColorDL, gGiBulletBagStringDL, gGiBulletBagStringColorDL, gGiBulletBagWritingDL } },
    // GID_CROWN_GREEN
    { GetItem_DrawSmallCrown,
      { gGiCrownInnerDL, gGiGreenCrownInnerColorDL, gGiCrownOuterDL, gGiGreenCrownOuterColorDL } },
    // GID_CROWN_BLUE
    { GetItem_DrawSmallCrown,
      { gGiCrownInnerDL, gGiBlueCrownInnerColorDL, gGiCrownOuterDL, gGiBlueCrownOuterColorDL } },
    // GID_CROWN_RED
    { GetItem_DrawSmallCrown, { gGiCrownInnerDL, gGiRedCrownInnerColorDL, gGiCrownOuterDL, gGiRedCrownOuterColorDL } },
    // GID_BIG_WRAITH
    { GetItem_DrawWraiths,
      { gGiGhostContainerLidDL, gGiGhostContainerGlassDL, gGiGhostContainerContentsDL, gGiBigWraithColorDL } },
    // GID_CROWN_PURPLE
    { GetItem_DrawOpa10Xlu32,
      { gGiCrownInnerDL, gGiPurpleCrownInnerColorDL, gGiCrownOuterDL, gGiPurpleCrownOuterColorDL } },
    // GID_CROWN_GOLD
    { GetItem_DrawOpa10Xlu32,
      { gGiCrownInnerDL, gGiGoldCrownInnerColorDL, gGiCrownOuterDL, gGiGoldCrownOuterColorDL } },
    // GID_BULLET_BAG_50
    { GetItem_DrawBulletBag,
      { gGiBulletBagDL, gGiBulletBag50ColorDL, gGiBulletBagStringDL, gGiBulletBag50StringColorDL,
        gGiBulletBagWritingDL } },
    // GID_SWORD_SYLVAN
    { GetItem_DrawOpa0, { gGiSylvanSwordDL } },
    // GID_SKULL_TOKEN_2
    { GetItem_DrawSkullToken, { gWebguardTokenDL, gWebguardTokenFlameDL } },
};

/**
 * Draw "Get Item" Model
 * Calls the corresponding draw function for the given draw ID
 */
void GetItem_Draw(PlayState* play, s16 giDrawId) {
    sDrawItemTable[giDrawId].drawFunc(play, giDrawId);
}

// All remaining functions in this file are draw functions referenced in the table and called by the function above

void GetItem_DrawMaskOrBombmouse(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 26);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawSoldOut(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_XLU, 5);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawBlueFire(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0 * (play->state.frames * 0), 0 * (play->state.frames * 0), 16, 32, 1, 1 * (play->state.frames * 1), 1 * -(play->state.frames * 8), 16, 32));
    Matrix_Push();
    Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    Matrix_Pop();

}

void GetItem_DrawWraiths(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0 * (play->state.frames * 0), 0 * (play->state.frames * 0), 16, 32, 1, 1 * (play->state.frames * 1), 1 * -(play->state.frames * 6), 16, 32));
    Matrix_Push();
    Matrix_ReplaceRotation(&play->billboardMtxF);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[3])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));
    Matrix_Pop();

}

void GetItem_DrawFairy(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0 * (play->state.frames * 0), 0 * (play->state.frames * 0), 32, 32, 1, 1 * (play->state.frames * 1), 1 * -(play->state.frames * 6), 32, 32));
    Matrix_Push();
    Matrix_ReplaceRotation(&play->billboardMtxF);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));
    Matrix_Pop();

}

void GetItem_DrawPrismShield(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 0 * (play->state.frames * 0) % 256, 1 * (play->state.frames * 2) % 256, 64, 64, 1, 0 * (play->state.frames * 0) % 128, 1 * (play->state.frames * 1) % 128, 32, 32));
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));

}

void GetItem_DrawSkullToken(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0 * (play->state.frames * 0), 1 * -(play->state.frames * 5), 32, 32, 1, 0 * (play->state.frames * 0), 0 * (play->state.frames * 0), 32, 64));
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));

}

void GetItem_DrawEggOrMedallion(PlayState* play, s16 giDrawId) {
    DrawItemTableEntry* entry = &sDrawItemTable[giDrawId];


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 26);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[0])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[1])));

}

void GetItem_DrawCompass(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 5);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));

}

void GetItem_DrawPotion(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, -1 * (play->state.frames * 1), 1 * (play->state.frames * 1), 32, 32, 1, -1 * (play->state.frames * 1), 1 * (play->state.frames * 1), 32, 32));
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[3])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[4])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[5])));

}

void GetItem_DrawCairnSword(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 1 * (play->state.frames * 1), 0 * (play->state.frames * 1), 32, 32, 1, 0 * (play->state.frames * 1), 0 * (play->state.frames * 1), 32, 32));
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawRootNuts(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_SetTexScroll2(0x08, 0, 1 * (play->state.frames * 6), 1 * (play->state.frames * 6), 32, 32, 1, 1 * (play->state.frames * 6), 1 * (play->state.frames * 6), 32, 32));
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawRecoveryHeart(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0 * (play->state.frames * 1), 1 * -(play->state.frames * 3), 32, 32, 1, 0 * (play->state.frames * 1), 1 * -(play->state.frames * 2), 32, 32));
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawFish(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 0 * (play->state.frames * 0), 1 * (play->state.frames * 1), 32, 32, 1, 0 * (play->state.frames * 0), 1 * (play->state.frames * 1), 32, 32));
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawOpa0(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawOpa0Xlu1(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));

}

void GetItem_DrawXlu01(PlayState* play, s16 giDrawId) {
    DrawItemTableEntry* entry = &sDrawItemTable[giDrawId];


    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[0])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[1])));

}

void GetItem_DrawOpa10Xlu2(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));

}

void GetItem_DrawMagicArrow(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));

}

void GetItem_DrawMagicSpell(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 1 * (play->state.frames * 2), 1 * -(play->state.frames * 6), 32, 32, 1, 1 * (play->state.frames * 1), -1 * (play->state.frames * 2), 32, 32));
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));

}

void GetItem_DrawOpa1023(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[3])));

}

void GetItem_DrawOpa10Xlu32(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[3])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));

}

void GetItem_DrawSmallCrown(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Matrix_Scale(0.7f, 0.7f, 0.7f, MTXMODE_APPLY);

    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[3])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));

}

void GetItem_DrawScale(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_SetTexScroll2(0x08, 0, 1 * (play->state.frames * 2), -1 * (play->state.frames * 2), 64, 64, 1, 1 * (play->state.frames * 4), 1 * -(play->state.frames * 4), 32, 32));
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[3])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));

}

void GetItem_DrawBulletBag(PlayState* play, s16 giDrawId) {
    DrawItemTableEntry* entry = &sDrawItemTable[giDrawId];


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[1])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[0])));

    Gfx3D_SetupDLTo(GFX_BUF_XLU, 25);
    GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[2])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[3])));
    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(entry->dlists[4])));

}

void GetItem_DrawWallet(PlayState* play, s16 giDrawId) {
    STACK_PAD(s32);


    Gfx3D_SetupDLTo(GFX_BUF_OPA, 25);
    GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[1])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[0])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[2])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[3])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[4])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[5])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[6])));
    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(sDrawItemTable[giDrawId].dlists[7])));

}
