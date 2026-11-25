// adapted from flute-of-age/src/code/z_player_lib.c
#include "libc64/math64.h"
#include "engine/stub.h"
#include "engine/asset.h"
#include "engine/gfx3d.h"
#include "engine/object_native.h"
#include "engine/gfx.h"
#include "exact_gu.h"
#include <stdlib.h>
#include <string.h>
#include "attributes.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "regs.h"
#include "segmented_address.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "versions.h"
#include "z_lib.h"
#include "draw.h"
#include "effect.h"
#include "play_state.h"
#include "player.h"
#include "save.h"
#include "skin_matrix.h"

#include "assets/objects/gameplay_keep/player_pause_joint_tables.h"
#include "assets/objects/gameplay_keep/player_anim_headers.h"
#include "assets/objects/object_aren_boy/object_aren_boy.h"
#include "assets/objects/object_aren_child/object_aren_child.h"


typedef struct BowSlingshotStringData {
    /* 0x00 */ DListRef  dList;
    /* 0x04 */ Vec3f pos;
} BowSlingshotStringData; // size = 0x10

FlexSkeletonHeader* gPlayerSkelHeaders[] = { &gArenAdultSkel, &gArenChildSkel };

s16 sBootData[PLAYER_BOOTS_MAX][17] = {
    // PLAYER_BOOTS_SYLVAN
    {
        200,                         // REG(19)
        FRAMERATE_CONST(1000, 1200), // REG(30)
        FRAMERATE_CONST(300, 360),   // REG(32)
        700,                         // REG(34)
        FRAMERATE_CONST(550, 660),   // REG(35)
        FRAMERATE_CONST(270, 324),   // REG(36)
        600,                         // REG(37)
        FRAMERATE_CONST(350, 420),   // REG(38)
        800,                         // R_DECELERATE_RATE
        600,                         // R_RUN_SPEED_LIMIT
        -100,                        // REG(68)
        600,                         // REG(69)
        590,                         // IREG(66)
        750,                         // IREG(67)
        125,                         // IREG(68)
        200,                         // IREG(69)
        FRAMERATE_CONST(130, 156),   // MREG(95)
    },
    // PLAYER_BOOTS_IRON
    {
        200,                         // REG(19)
        FRAMERATE_CONST(1000, 1200), // REG(30)
        FRAMERATE_CONST(300, 360),   // REG(32)
        700,                         // REG(34)
        FRAMERATE_CONST(550, 660),   // REG(35)
        FRAMERATE_CONST(270, 324),   // REG(36)
        1000,                        // REG(37)
        FRAMERATE_CONST(0, 0),       // REG(38)
        800,                         // R_DECELERATE_RATE
        300,                         // R_RUN_SPEED_LIMIT
        -160,                        // REG(68)
        600,                         // REG(69)
        590,                         // IREG(66)
        750,                         // IREG(67)
        125,                         // IREG(68)
        200,                         // IREG(69)
        FRAMERATE_CONST(130, 156),   // MREG(95)
    },
    // PLAYER_BOOTS_HOVER
    {
        200,                         // REG(19)
        FRAMERATE_CONST(1000, 1200), // REG(30)
        FRAMERATE_CONST(300, 360),   // REG(32)
        700,                         // REG(34)
        FRAMERATE_CONST(550, 660),   // REG(35)
        FRAMERATE_CONST(270, 324),   // REG(36)
        600,                         // REG(37)
        FRAMERATE_CONST(600, 720),   // REG(38)
        800,                         // R_DECELERATE_RATE
        550,                         // R_RUN_SPEED_LIMIT
        -100,                        // REG(68)
        600,                         // REG(69)
        540,                         // IREG(66)
        270,                         // IREG(67)
        25,                          // IREG(68)
        0,                           // IREG(69)
        FRAMERATE_CONST(130, 156),   // MREG(95)
    },
    // PLAYER_BOOTS_INDOOR
    {
        200,                         // REG(19)
        FRAMERATE_CONST(1000, 1200), // REG(30)
        FRAMERATE_CONST(300, 360),   // REG(32)
        700,                         // REG(34)
        FRAMERATE_CONST(380, 456),   // REG(35)
        FRAMERATE_CONST(400, 480),   // REG(36)
        0,                           // REG(37)
        FRAMERATE_CONST(300, 360),   // REG(38)
        800,                         // R_DECELERATE_RATE
        500,                         // R_RUN_SPEED_LIMIT
        -100,                        // REG(68)
        600,                         // REG(69)
        590,                         // IREG(66)
        750,                         // IREG(67)
        125,                         // IREG(68)
        200,                         // IREG(69)
        FRAMERATE_CONST(130, 156),   // MREG(95)
    },
    // PLAYER_BOOTS_IRON_UNDERWATER
    {
        80,                        // REG(19)
        FRAMERATE_CONST(800, 960), // REG(30)
        FRAMERATE_CONST(150, 180), // REG(32)
        700,                       // REG(34)
        FRAMERATE_CONST(480, 576), // REG(35)
        FRAMERATE_CONST(270, 324), // REG(36)
        600,                       // REG(37)
        FRAMERATE_CONST(50, 60),   // REG(38)
        800,                       // R_DECELERATE_RATE
        550,                       // R_RUN_SPEED_LIMIT
        -40,                       // REG(68)
        400,                       // REG(69)
        540,                       // IREG(66)
        270,                       // IREG(67)
        25,                        // IREG(68)
        0,                         // IREG(69)
        FRAMERATE_CONST(80, 96),   // MREG(95)
    },
    // PLAYER_BOOTS_SYLVAN_CHILD
    {
        200,                         // REG(19)
        FRAMERATE_CONST(1000, 1200), // REG(30)
        FRAMERATE_CONST(300, 360),   // REG(32)
        800,                         // REG(34)
        FRAMERATE_CONST(500, 600),   // REG(35)
        FRAMERATE_CONST(400, 480),   // REG(36)
        800,                         // REG(37)
        FRAMERATE_CONST(400, 480),   // REG(38)
        800,                         // R_DECELERATE_RATE
        550,                         // R_RUN_SPEED_LIMIT
        -100,                        // REG(68)
        600,                         // REG(69)
        540,                         // IREG(66)
        750,                         // IREG(67)
        125,                         // IREG(68)
        400,                         // IREG(69)
        FRAMERATE_CONST(200, 240),   // MREG(95)
    },
};

// Used to map item actions to model groups
u8 sActionModelGroups[PLAYER_IA_MAX] = {
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_NONE
    PLAYER_MODELGROUP_SWORD,            // PLAYER_IA_SWORD_CS
    PLAYER_MODELGROUP_10,               // PLAYER_IA_FISHING_POLE
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_IA_SWORD_MASTER
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_IA_SWORD_SYLVAN
    PLAYER_MODELGROUP_BGS,              // PLAYER_IA_SWORD_HIGHCAIRN
    PLAYER_MODELGROUP_10,               // PLAYER_IA_ROOT_STICK
    PLAYER_MODELGROUP_HAMMER,           // PLAYER_IA_HAMMER
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_FIRE
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_ICE
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_LIGHT
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_0C
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_0D
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_BOW_0E
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_SLINGSHOT
    PLAYER_MODELGROUP_GRAPNEL,         // PLAYER_IA_GRAPNEL
    PLAYER_MODELGROUP_GRAPNEL,         // PLAYER_IA_LONGGRAPPLE
    PLAYER_MODELGROUP_EXPLOSIVES,       // PLAYER_IA_BOMB
    PLAYER_MODELGROUP_EXPLOSIVES,       // PLAYER_IA_BOMBMOUSE
    PLAYER_MODELGROUP_BOOMERANG,        // PLAYER_IA_BOOMERANG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MAGIC_SPELL_15
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MAGIC_SPELL_16
    PLAYER_MODELGROUP_BOW_SLINGSHOT,    // PLAYER_IA_MAGIC_SPELL_17
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ZEPHYRS_WIND
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_NERIS_LOVE
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_PYRS_FIRE
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ROOT_NUT
    PLAYER_MODELGROUP_FLUTE,          // PLAYER_IA_FLUTE_FAIRY
    PLAYER_MODELGROUP_OOT,              // PLAYER_IA_FLUTE_OF_AGE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_FISH
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_FIRE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_BUG
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_WRAITH
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_BIG_WRAITH
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_MIRAS_LETTER
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_POTION_RED
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_POTION_BLUE
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_POTION_GREEN
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_MILK_FULL
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_MILK_HALF
    PLAYER_MODELGROUP_BOTTLE,           // PLAYER_IA_BOTTLE_FAIRY
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ELARAS_LETTER
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_WEIRD_EGG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_CHICKEN
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MAGIC_BEAN
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_POCKET_EGG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_POCKET_CLUCKER
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_AZURE
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ODD_MUSHROOM
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_ODD_POTION
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_POACHERS_SAW
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_BROKEN_CAIRNS_SWORD
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_PRESCRIPTION
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_FROG
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_EYEDROPS
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_CLAIM_CHECK
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_FOXLING
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_SKULL
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_SPOOKY
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_BUNNY_HOOD
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_CAIRN
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_NERI
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_DUNEAN
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_MASK_TRUTH
    PLAYER_MODELGROUP_DEFAULT,          // PLAYER_IA_LENS_OF_INSIGHT
};

typedef struct EnvHazardTextTriggerEntry {
    /* 0x0 */ u8 flag;
    /* 0x2 */ u16 textId;
} EnvHazardTextTriggerEntry; // size = 0x4

EnvHazardTextTriggerEntry sEnvHazardTextTriggers[] = {
    { ENV_HAZARD_TEXT_TRIGGER_HOTROOM, 0x3040 },    // PLAYER_ENV_HAZARD_HOTROOM - 1
    { ENV_HAZARD_TEXT_TRIGGER_UNDERWATER, 0x401D }, // PLAYER_ENV_HAZARD_UNDERWATER_FLOOR - 1
    { 0, 0x0000 },                                  // PLAYER_ENV_HAZARD_SWIMMING - 1
    { ENV_HAZARD_TEXT_TRIGGER_UNDERWATER, 0x401D }, // PLAYER_ENV_HAZARD_UNDERWATER_FREE - 1
};

// Used to map model groups to model types for [animation, left hand, right hand, back, waist]
u8 gPlayerModelTypes[PLAYER_MODELGROUP_MAX][PLAYER_MODELGROUPENTRY_MAX] = {
    /* PLAYER_MODELGROUP_0 */
    { PLAYER_ANIMTYPE_2, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_SHIELD, PLAYER_MODELTYPE_BACK_SWORD_IN_SHEATH,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_CHILD_ELDRAN_SHIELD */
    { PLAYER_ANIMTYPE_1, PLAYER_MODELTYPE_LH_SWORD, PLAYER_MODELTYPE_RH_CLOSED, PLAYER_MODELTYPE_BACK_SHIELD,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_SWORD_AND_SHIELD */
    { PLAYER_ANIMTYPE_1, PLAYER_MODELTYPE_LH_SWORD, PLAYER_MODELTYPE_RH_SHIELD, PLAYER_MODELTYPE_BACK_SHEATH,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_DEFAULT */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OPEN,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_4 */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OPEN,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BGS */
    { PLAYER_ANIMTYPE_3, PLAYER_MODELTYPE_LH_BGS, PLAYER_MODELTYPE_RH_CLOSED, PLAYER_MODELTYPE_BACK_SHIELD,
      PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BOW_SLINGSHOT */
    { PLAYER_ANIMTYPE_4, PLAYER_MODELTYPE_LH_CLOSED, PLAYER_MODELTYPE_RH_BOW_SLINGSHOT,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_EXPLOSIVES */
    { PLAYER_ANIMTYPE_5, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OPEN,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BOOMERANG */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_BOOMERANG, PLAYER_MODELTYPE_RH_OPEN,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_GRAPNEL */
    { PLAYER_ANIMTYPE_4, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_GRAPNEL,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_10 */
    { PLAYER_ANIMTYPE_3, PLAYER_MODELTYPE_LH_CLOSED, PLAYER_MODELTYPE_RH_CLOSED,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_HAMMER */
    { PLAYER_ANIMTYPE_3, PLAYER_MODELTYPE_LH_HAMMER, PLAYER_MODELTYPE_RH_CLOSED,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_FLUTE */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_FLUTE,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_OOT */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_OPEN, PLAYER_MODELTYPE_RH_OOT,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_BOTTLE */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_BOTTLE, PLAYER_MODELTYPE_RH_OPEN,
      PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH, PLAYER_MODELTYPE_WAIST },
    /* PLAYER_MODELGROUP_SWORD */
    { PLAYER_ANIMTYPE_0, PLAYER_MODELTYPE_LH_SWORD, PLAYER_MODELTYPE_RH_OPEN, PLAYER_MODELTYPE_BACK_SHIELD,
      PLAYER_MODELTYPE_WAIST },
};

DListRef  sPlayerRightHandShieldDLs[PLAYER_SHIELD_MAX * 4] = {
    // PLAYER_SHIELD_NONE
    gArenAdultRightHandClosedNearDL,
    gArenChildRightHandClosedNearDL,
    gArenAdultRightHandClosedFarDL,
    gArenChildRightHandClosedFarDL,
    // PLAYER_SHIELD_ROOT
    gArenAdultRightHandClosedNearDL,
    gArenChildRightFistAndRootShieldNearDL,
    gArenAdultRightHandClosedFarDL,
    gArenChildRightFistAndRootShieldFarDL,
    // PLAYER_SHIELD_ELDRAN
    gArenAdultRightHandHoldingEldranShieldNearDL,
    gArenChildRightHandClosedNearDL,
    gArenAdultRightHandHoldingEldranShieldFarDL,
    gArenChildRightHandClosedFarDL,
    // PLAYER_SHIELD_MIRROR
    gArenAdultRightHandHoldingPrismShieldNearDL,
    gArenChildRightHandClosedNearDL,
    gArenAdultRightHandHoldingPrismShieldFarDL,
    gArenChildRightHandClosedFarDL,
};

DListRef  sPlayerBackShieldSwordInSheathDLs[(PLAYER_SHIELD_MAX + 2) * 4] = {
    // PLAYER_SHIELD_NONE
    gArenAdultBackDawnbladeInSheathNearDL,
    gArenChildBackSylvanSwordInSheathNearDL,
    gArenAdultBackDawnbladeInSheathFarDL,
    gArenChildBackSylvanSwordInSheathFarDL,
    // PLAYER_SHIELD_ROOT
    gArenAdultBackDawnbladeInSheathNearDL,
    gArenChildBackRootShieldSylvanSwordInSheathNearDL,
    gArenAdultBackDawnbladeInSheathFarDL,
    gArenChildBackRootShieldSylvanSwordInSheathFarDL,
    // PLAYER_SHIELD_ELDRAN
    gArenAdultBackEldranShieldSwordInSheathNearDL,
    gArenChildBackEldranShieldSwordInSheathNearDL,
    gArenAdultBackEldranShieldSwordInSheathFarDL,
    gArenChildBackEldranShieldSwordInSheathFarDL,
    // PLAYER_SHIELD_MIRROR
    gArenAdultBackPrismShieldDawnbladeInSheathNearDL,
    gArenChildBackSylvanSwordInSheathNearDL,
    gArenAdultBackPrismShieldDawnbladeInSheathFarDL,
    gArenChildBackSylvanSwordInSheathFarDL,
    // PLAYER_SHIELD_NONE (child, no sword)
    NULL,
    NULL,
    NULL,
    NULL,
    // PLAYER_SHIELD_ROOT (child, no sword)
    NULL,
    gArenChildBackRootShieldDL,
    NULL,
    gArenChildBackRootShieldDL,
};

DListRef  sPlayerBackShieldDLs[(PLAYER_SHIELD_MAX + 2) * 4] = {
    // PLAYER_SHIELD_NONE
    gArenAdultBackMasterSheathNearDL,
    gArenChildBackSylvanSheathNearDL,
    gArenAdultBackMasterSheathFarDL,
    gArenChildBackSylvanSheathFarDL,
    // PLAYER_SHIELD_ROOT
    gArenAdultBackMasterSheathNearDL,
    gArenChildBackRootShieldSylvanSheathNearDL,
    gArenAdultBackMasterSheathFarDL,
    gArenChildBackRootShieldSylvanSheathFarDL,
    // PLAYER_SHIELD_ELDRAN
    gArenAdultBackEldranShieldMasterSheathNearDL,
    gArenChildBackEldranShieldSylvanSheathNearDL,
    gArenAdultBackEldranShieldMasterSheathFarDL,
    gArenChildBackEldranShieldSylvanSheathFarDL,
    // PLAYER_SHIELD_MIRROR
    gArenAdultBackPrismShieldMasterSheathNearDL,
    gArenChildBackSylvanSheathNearDL,
    gArenAdultBackPrismShieldMasterSheathFarDL,
    gArenChildBackSylvanSheathFarDL,
    // PLAYER_SHIELD_NONE (child, no sword)
    NULL,
    NULL,
    NULL,
    NULL,
    // PLAYER_SHIELD_ROOT (child, no sword)
    gArenAdultBackMasterSheathNearDL,
    gArenChildBackRootShieldDL,
    gArenAdultBackMasterSheathNearDL,
    gArenChildBackRootShieldDL,
};

DListRef  gPlayerLeftHandBgsDLs[] = {
    // highcairn sword
    gArenAdultLeftHandHoldingBgsNearDL,
    gArenChildLeftHandHoldingDawnbladeDL,
    gArenAdultLeftHandHoldingBgsFarDL,
    gArenChildLeftHandHoldingDawnbladeDL,
    // broken giant knife
    gArenAdultHandHoldingBrokenGiantsKnifeDL,
    gArenChildLeftHandHoldingDawnbladeDL,
    gArenAdultHandHoldingBrokenGiantsKnifeFarDL,
    gArenChildLeftHandHoldingDawnbladeDL,
};

DListRef  gPlayerLeftHandOpenDLs[] = {
    gArenAdultLeftHandNearDL,
    gArenChildLeftHandNearDL,
    gArenAdultLeftHandFarDL,
    gArenChildLeftHandFarDL,
};

DListRef  gPlayerLeftHandClosedDLs[] = {
    gArenAdultLeftHandClosedNearDL,
    gArenChildLeftFistNearDL,
    gArenAdultLeftHandClosedFarDL,
    gArenChildLeftFistFarDL,
};

// Identical to `sPlayerLeftHandSwordDLs` and unused
DListRef  sPlayerLeftHandSwordDLs2[] = {
    gArenAdultLeftHandHoldingDawnbladeNearDL,
    gArenChildLeftFistAndSylvanSwordNearDL,
    gArenAdultLeftHandHoldingDawnbladeFarDL,
    gArenChildLeftFistAndSylvanSwordFarDL,
};

DListRef  sPlayerLeftHandSwordDLs[] = {
    gArenAdultLeftHandHoldingDawnbladeNearDL,
    gArenChildLeftFistAndSylvanSwordNearDL,
    gArenAdultLeftHandHoldingDawnbladeFarDL,
    gArenChildLeftFistAndSylvanSwordFarDL,
};

DListRef  sPlayerRightHandOpenDLs[] = {
    gArenAdultRightHandNearDL,
    gArenChildRightHandNearDL,
    gArenAdultRightHandFarDL,
    gArenChildRightHandFarDL,
};

DListRef  sPlayerRightHandClosedDLs[] = {
    gArenAdultRightHandClosedNearDL,
    gArenChildRightHandClosedNearDL,
    gArenAdultRightHandClosedFarDL,
    gArenChildRightHandClosedFarDL,
};

DListRef  sPlayerRightHandBowSlingshotDLs[] = {
    gArenAdultRightHandHoldingBowNearDL,
    gArenChildRightHandHoldingSlingshotNearDL,
    gArenAdultRightHandHoldingBowFarDL,
    gArenChildRightHandHoldingSlingshotFarDL,
};

DListRef  sPlayerBackSwordInSheathDLs[] = {
    gArenAdultBackDawnbladeInSheathNearDL,
    gArenChildBackSylvanSwordInSheathNearDL,
    gArenAdultBackDawnbladeInSheathFarDL,
    gArenChildBackSylvanSwordInSheathFarDL,
};

DListRef  sPlayerBackSheathDLs[] = {
    gArenAdultBackMasterSheathNearDL,
    gArenChildBackSylvanSheathNearDL,
    gArenAdultBackMasterSheathFarDL,
    gArenChildBackSylvanSheathFarDL,
};

DListRef  sPlayerWaistDLs[] = {
    gArenAdultWaistNearDL,
    gArenChildWaistNearDL,
    gArenAdultWaistFarDL,
    gArenChildWaistFarDL,
};

// Identical to `sPlayerRightHandBowSlingshotDLs` and unused
DListRef  sPlayerRightHandBowSlingshotDLs2[] = {
    gArenAdultRightHandHoldingBowNearDL,
    gArenChildRightHandHoldingSlingshotNearDL,
    gArenAdultRightHandHoldingBowFarDL,
    gArenChildRightHandHoldingSlingshotFarDL,
};

DListRef  sPlayerRightHandFluteDLs[] = {
    gArenAdultRightHandHoldingOotNearDL,
    gArenChildRightHandHoldingFairyFluteNearDL,
    gArenAdultRightHandHoldingOotFarDL,
    gArenChildRightHandHoldingFairyFluteFarDL,
};

DListRef  sPlayerRightHandOotDLs[] = {
    gArenAdultRightHandHoldingOotNearDL,
    gArenChildRightHandAndOotNearDL,
    gArenAdultRightHandHoldingOotFarDL,
    gArenChildRightHandHoldingOOTFarDL,
};

DListRef  sPlayerRightHandGrapnelDLs[] = {
    gArenAdultRightHandHoldingGrapnelNearDL,
    gArenChildRightHandNearDL,
    gArenAdultRightHandHoldingGrapnelNearDL, // No 'far' display list
    gArenChildRightHandFarDL,
};

DListRef  sPlayerLeftHandHammerDLs[] = {
    gArenAdultLeftHandHoldingHammerNearDL,
    gArenChildLeftHandNearDL,
    gArenAdultLeftHandHoldingHammerFarDL,
    gArenChildLeftHandFarDL,
};

DListRef  gPlayerLeftHandBoomerangDLs[] = {
    gArenAdultLeftHandNearDL,
    gArenChildLeftFistAndBoomerangNearDL,
    gArenAdultLeftHandFarDL,
    gArenChildLeftFistAndBoomerangFarDL,
};

DListRef  sPlayerLeftHandBottleDLs[] = {
    gArenAdultLeftHandOutNearDL,
    gArenChildLeftHandUpNearDL,
    gArenAdultLeftHandOutNearDL,
    gArenChildLeftHandUpNearDL,
};

DListRef  sFirstPersonLeftForearmDLs[] = {
    gArenAdultRightArmOutNearDL,
    NULL,
};

DListRef  sFirstPersonLeftHandDLs[] = {
    gArenAdultRightHandOutNearDL,
    NULL,
};

DListRef  sFirstPersonRightShoulderDLs[] = {
    gArenAdultRightShoulderNearDL,
    gArenChildRightShoulderNearDL,
};

DListRef  sFirstPersonForearmDLs[] = {
    gArenAdultLeftArmOutNearDL,
    NULL,
};

DListRef  sFirstPersonRightHandHoldingWeaponDLs[] = {
    gArenAdultRightHandHoldingBowFirstPersonDL,
    gArenChildRightArmStretchedSlingshotDL,
};

// Indexed by model types (left hand, right hand, back or waist)
DListRef* sPlayerDListGroups[PLAYER_MODELTYPE_MAX] = {
    gPlayerLeftHandOpenDLs,            // PLAYER_MODELTYPE_LH_OPEN
    gPlayerLeftHandClosedDLs,          // PLAYER_MODELTYPE_LH_CLOSED
    sPlayerLeftHandSwordDLs,           // PLAYER_MODELTYPE_LH_SWORD
    sPlayerLeftHandSwordDLs2,          // PLAYER_MODELTYPE_LH_SWORD_2
    gPlayerLeftHandBgsDLs,             // PLAYER_MODELTYPE_LH_BGS
    sPlayerLeftHandHammerDLs,          // PLAYER_MODELTYPE_LH_HAMMER
    gPlayerLeftHandBoomerangDLs,       // PLAYER_MODELTYPE_LH_BOOMERANG
    sPlayerLeftHandBottleDLs,          // PLAYER_MODELTYPE_LH_BOTTLE
    sPlayerRightHandOpenDLs,           // PLAYER_MODELTYPE_RH_OPEN
    sPlayerRightHandClosedDLs,         // PLAYER_MODELTYPE_RH_CLOSED
    sPlayerRightHandShieldDLs,         // PLAYER_MODELTYPE_RH_SHIELD
    sPlayerRightHandBowSlingshotDLs,   // PLAYER_MODELTYPE_RH_BOW_SLINGSHOT
    sPlayerRightHandBowSlingshotDLs2,  // PLAYER_MODELTYPE_RH_BOW_SLINGSHOT_2
    sPlayerRightHandFluteDLs,        // PLAYER_MODELTYPE_RH_FLUTE
    sPlayerRightHandOotDLs,            // PLAYER_MODELTYPE_RH_OOT
    sPlayerRightHandGrapnelDLs,       // PLAYER_MODELTYPE_RH_GRAPNEL
    sPlayerBackSwordInSheathDLs,       // PLAYER_MODELTYPE_BACK_SWORD_IN_SHEATH
    sPlayerBackSheathDLs,              // PLAYER_MODELTYPE_BACK_SHEATH
    sPlayerBackShieldSwordInSheathDLs, // PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH
    sPlayerBackShieldDLs,              // PLAYER_MODELTYPE_BACK_SHIELD
    sPlayerWaistDLs,                   // PLAYER_MODELTYPE_WAIST
};

// native: the original bound one of these two tiny display lists to segment 0x0C to choose the
// culled face for the player model. Here they are sentinel handles; Gfx3D_SetSegment records them
// as a cull-mode override (render-only).
const u8 gCullBackDList[1] = { 0 };
const u8 gCullFrontDList[1] = { 1 };

Vec3f* sCurBodyPartPos;
s32 sDListsLodOffset;
Vec3f sGetItemRefPos;
s32 sLeftHandType;
s32 sRightHandType;

void Player_SetBootData(PlayState* play, Player* this) {
    s32 currentBoots;
    s16* bootRegs;

    REG(27) = 2000;
    REG(48) = 370;

    currentBoots = this->currentBoots;
    if (currentBoots == PLAYER_BOOTS_SYLVAN) {
        if (!AREN_IS_ADULT) {
            currentBoots = PLAYER_BOOTS_SYLVAN_CHILD;
        }
    } else if (currentBoots == PLAYER_BOOTS_IRON) {
        if (this->stateFlags1 & PLAYER_STATE1_27) {
            currentBoots = PLAYER_BOOTS_IRON_UNDERWATER;
        }
        REG(27) = 500;
        REG(48) = 100;
    }

    bootRegs = sBootData[currentBoots];
    REG(19) = bootRegs[0];
    REG(30) = bootRegs[1];
    REG(32) = bootRegs[2];
    REG(34) = bootRegs[3];
    REG(35) = bootRegs[4];
    REG(36) = bootRegs[5];
    REG(37) = bootRegs[6];
    REG(38) = bootRegs[7];
    R_DECELERATE_RATE = bootRegs[8];
    R_RUN_SPEED_LIMIT = bootRegs[9];
    REG(68) = bootRegs[10];
    REG(69) = bootRegs[11];
    IREG(66) = bootRegs[12];
    IREG(67) = bootRegs[13];
    IREG(68) = bootRegs[14];
    IREG(69) = bootRegs[15];
    MREG(95) = bootRegs[16];

    if (play->roomCtx.curRoom.type == ROOM_TYPE_INDOORS) {
        R_RUN_SPEED_LIMIT = 500;
    }
}

int Player_InBlockingCsMode(PlayState* play, Player* this) {
    return (this->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_29)) || (this->csAction != PLAYER_CSACTION_NONE) ||
           (play->transitionTrigger == TRANS_TRIGGER_START) || (this->stateFlags1 & PLAYER_STATE1_0) ||
           (this->stateFlags3 & PLAYER_STATE3_FLYING_WITH_GRAPNEL) ||
           ((gSaveContext.magicState != MAGIC_STATE_IDLE) && (Player_ActionToMagicSpell(this, this->itemAction) >= 0));
}

int Player_InCsMode(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return Player_InBlockingCsMode(play, this) || (this->unk_6AD == 4);
}

/**
 * Checks if Player is currently locked onto a hostile actor.
 * `PLAYER_STATE1_HOSTILE_LOCK_ON` controls Player's "battle" response to hostile actors.
 *
 * Note that within Player, `Player_UpdateHostileLockOn` exists, which updates the flag and also returns the check.
 * Player can use this function instead if the flag should be checked, but not updated.
 */
s32 Player_CheckHostileLockOn(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_HOSTILE_LOCK_ON);
}

int Player_IsChildWithEldranShield(Player* this) {
    return gSaveContext.save.arenAge != AREN_AGE_ADULT && (this->currentShield == PLAYER_SHIELD_ELDRAN);
}

s32 Player_ActionToModelGroup(Player* this, s32 itemAction) {
    s32 modelGroup = sActionModelGroups[itemAction];

    if ((modelGroup == PLAYER_MODELGROUP_SWORD_AND_SHIELD) && Player_IsChildWithEldranShield(this)) {
        // child, using sylvan sword with eldran shield equipped
        return PLAYER_MODELGROUP_CHILD_ELDRAN_SHIELD;
    } else {
        return modelGroup;
    }
}

void Player_SetModelsForHoldingShield(Player* this) {
    if ((this->stateFlags1 & PLAYER_STATE1_SHIELDING) &&
        ((this->itemAction < 0) || (this->itemAction == this->heldItemAction))) {
        if (!Player_HoldsTwoHandedWeapon(this) && !Player_IsChildWithEldranShield(this)) {
            this->rightHandType = PLAYER_MODELTYPE_RH_SHIELD;
            this->rightHandDLists =
                sPlayerDListGroups[PLAYER_MODELTYPE_RH_SHIELD] + ((void)0, gSaveContext.save.arenAge);
            if (this->backType == PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH) {
                this->backType = PLAYER_MODELTYPE_BACK_SWORD_IN_SHEATH;
            } else if (this->backType == PLAYER_MODELTYPE_BACK_SHIELD) {
                this->backType = PLAYER_MODELTYPE_BACK_SHEATH;
            }
            this->backDLists = sPlayerDListGroups[this->backType] + ((void)0, gSaveContext.save.arenAge);
            this->modelAnimType = PLAYER_ANIMTYPE_2;
            this->itemAction = -1;
        }
    }
}

void Player_SetModels(Player* this, s32 modelGroup) {
    this->leftHandType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_LEFT_HAND];
    this->rightHandType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_RIGHT_HAND];
    this->backType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_BACK];

    this->leftHandDLists = sPlayerDListGroups[gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_LEFT_HAND]] +
                           ((void)0, gSaveContext.save.arenAge);
    this->rightHandDLists = sPlayerDListGroups[gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_RIGHT_HAND]] +
                            ((void)0, gSaveContext.save.arenAge);
    this->backDLists = sPlayerDListGroups[gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_BACK]] +
                       ((void)0, gSaveContext.save.arenAge);
    this->waistDLists = sPlayerDListGroups[gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_WAIST]] +
                        ((void)0, gSaveContext.save.arenAge);

    Player_SetModelsForHoldingShield(this);
}

void Player_SetModelGroup(Player* this, s32 modelGroup) {
    this->modelGroup = modelGroup;

    if (modelGroup == PLAYER_MODELGROUP_CHILD_ELDRAN_SHIELD) {
        this->modelAnimType = PLAYER_ANIMTYPE_0;
    } else {
        this->modelAnimType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_ANIM];
    }

    if ((this->modelAnimType < PLAYER_ANIMTYPE_3) && (this->currentShield == PLAYER_SHIELD_NONE)) {
        this->modelAnimType = PLAYER_ANIMTYPE_0;
    }

    Player_SetModels(this, modelGroup);
}

void func_8008EC70(Player* this) {
    this->itemAction = this->heldItemAction;
    Player_SetModelGroup(this, Player_ActionToModelGroup(this, this->heldItemAction));
    this->unk_6AD = 0;
}

void Player_SetEquipmentData(PlayState* play, Player* this) {
    if (this->csAction != PLAYER_CSACTION_86) {
        this->currentShield = SHIELD_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD));
        this->currentTunic = TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC));
        this->currentBoots = BOOTS_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_BOOTS));
        this->currentSwordItemId = B_BTN_ITEM;
        Player_SetModelGroup(this, Player_ActionToModelGroup(this, this->heldItemAction));
        Player_SetBootData(play, this);
    }
}

void Player_UpdateBottleHeld(PlayState* play, Player* this, s32 item, s32 itemAction) {
    Inventory_UpdateBottleItem(play, item, this->heldItemButton);

    if (item != ITEM_BOTTLE_EMPTY) {
        this->heldItemId = item;
        this->heldItemAction = itemAction;
    }

    this->itemAction = itemAction;
}

void Player_ReleaseLockOn(Player* this) {
    this->focusActor = NULL;
    this->stateFlags2 &= ~PLAYER_STATE2_LOCK_ON_WITH_SWITCH;
}

/**
 * This function aims to clear Z-Target related state when it isn't in use.
 * It also handles setting a specific free fall related state that is interntwined with Z-Targeting.
 * TODO: Learn more about this and give a name to PLAYER_STATE1_19
 */
void Player_ClearZTargeting(Player* this) {
    if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
        (this->stateFlags1 & (PLAYER_STATE1_21 | PLAYER_STATE1_23 | PLAYER_STATE1_27)) ||
        (!(this->stateFlags1 & (PLAYER_STATE1_18 | PLAYER_STATE1_19)) &&
         ((this->actor.world.pos.y - this->actor.floorHeight) < 100.0f))) {
        this->stateFlags1 &= ~(PLAYER_STATE1_Z_TARGETING | PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | PLAYER_STATE1_PARALLEL |
                               PLAYER_STATE1_18 | PLAYER_STATE1_19 | PLAYER_STATE1_LOCK_ON_FORCED_TO_RELEASE);
    } else if (!(this->stateFlags1 & (PLAYER_STATE1_18 | PLAYER_STATE1_19 | PLAYER_STATE1_21))) {
        this->stateFlags1 |= PLAYER_STATE1_19;
    }

    Player_ReleaseLockOn(this);
}

/**
 * Sets the "auto lock-on actor" to lock onto an actor without Player's input.
 * This function will first release any existing lock-on or (try to) release parallel.
 *
 * When using Switch Targeting, it is not possible to carry an auto lock-on actor into a normal
 * lock-on when the auto lock-on is finished.
 * This is because the `PLAYER_STATE2_LOCK_ON_WITH_SWITCH` flag is never set with an auto lock-on.
 * With Hold Targeting it is possible to keep the auto lock-on going by keeping the Z button held down.
 *
 * The auto lock-on is considered "friendly" even if the actor is actually hostile. If the auto lock-on is hostile,
 * Player's battle response will not occur (if he is actionable) and the camera behaves differently.
 * When transitioning from auto lock-on to normal lock-on (with Hold Targeting) there will be a noticeable change
 * when it switches from "friendly" mode to "hostile" mode.
 */
void Player_SetAutoLockOnActor(PlayState* play, Actor* actor) {
    Player* this = GET_PLAYER(play);

    Player_ClearZTargeting(this);
    this->focusActor = actor;
    this->autoLockOnActor = actor;
    this->stateFlags1 |= PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS;
    Camera_SetViewParam(Play_GetCamera(play, CAM_ID_MAIN), CAM_VIEW_TARGET, actor);
    Camera_RequestMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_Z_TARGET_FRIENDLY);
}

s32 func_8008EF30(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (this->stateFlags1 & PLAYER_STATE1_23);
}

s32 func_8008EF44(PlayState* play, s32 ammo) {
    play->shootingGalleryStatus = ammo + 1;
    return 1;
}

int Player_IsBurningStickInRange(PlayState* play, Vec3f* pos, f32 xzRange, f32 yRange) {
    Player* this = GET_PLAYER(play);
    Vec3f diff;
    STACK_PAD(s32);

    if ((this->heldItemAction == PLAYER_IA_ROOT_STICK) && (this->unk_860 != 0)) {
        Math_Vec3f_Diff(MELEE_WEAPON_INFO_TIP(&this->meleeWeaponInfo[0]), pos, &diff);
        return ((SQ(diff.x) + SQ(diff.z)) <= SQ(xzRange)) && (0.0f <= diff.y) && (diff.y <= yRange);
    } else {
        return false;
    }
}

s32 Player_GetStrength(void) {
    s32 strengthUpgrade = CUR_UPG_VALUE(UPG_STRENGTH);

    if (AREN_IS_ADULT) {
        return strengthUpgrade;
    } else if (strengthUpgrade != 0) {
        return PLAYER_STR_BRACELET;
    } else {
        return PLAYER_STR_NONE;
    }
}

u8 Player_GetMask(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return this->currentMask;
}

Player* Player_UnsetMask(PlayState* play) {
    Player* this = GET_PLAYER(play);

    this->currentMask = PLAYER_MASK_NONE;

    return this;
}

s32 Player_HasPrismShieldEquipped(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (this->currentShield == PLAYER_SHIELD_MIRROR);
}

int Player_HasPrismShieldSetToDraw(PlayState* play) {
    Player* this = GET_PLAYER(play);

    return (this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) && (this->currentShield == PLAYER_SHIELD_MIRROR);
}

s32 Player_ActionToMagicSpell(UNUSED Player* this, s32 itemAction) {
    s32 magicSpell = itemAction - PLAYER_IA_MAGIC_SPELL_15;

    if ((magicSpell >= 0) && (magicSpell < 6)) {
        return magicSpell;
    } else {
        return -1;
    }
}

int Player_HoldsGrapnel(Player* this) {
    return (this->heldItemAction == PLAYER_IA_GRAPNEL) || (this->heldItemAction == PLAYER_IA_LONGGRAPPLE);
}

int func_8008F128(Player* this) {
    return Player_HoldsGrapnel(this) && (this->heldActor == NULL);
}

s32 Player_ActionToMeleeWeapon(s32 itemAction) {
    s32 meleeWeapon = itemAction - PLAYER_IA_FISHING_POLE;

    if ((meleeWeapon > 0) && (meleeWeapon < 6)) {
        return meleeWeapon;
    } else {
        return 0;
    }
}

s32 Player_GetMeleeWeaponHeld(Player* this) {
    return Player_ActionToMeleeWeapon(this->heldItemAction);
}

s32 Player_HoldsTwoHandedWeapon(Player* this) {
    if ((this->heldItemAction >= PLAYER_IA_SWORD_HIGHCAIRN) && (this->heldItemAction <= PLAYER_IA_HAMMER)) {
        return 1;
    } else {
        return 0;
    }
}

int Player_HoldsBrokenKnife(Player* this) {
    return (this->heldItemAction == PLAYER_IA_SWORD_HIGHCAIRN) &&
           (gSaveContext.save.info.playerData.swordHealth <= 0.0f);
}

s32 Player_ActionToBottle(UNUSED Player* this, s32 itemAction) {
    s32 bottle = itemAction - PLAYER_IA_BOTTLE;

    if ((bottle >= 0) && (bottle < 13)) {
        return bottle;
    } else {
        return -1;
    }
}

s32 Player_GetBottleHeld(Player* this) {
    return Player_ActionToBottle(this, this->heldItemAction);
}

s32 Player_ActionToExplosive(UNUSED Player* this, s32 itemAction) {
    s32 explosive = itemAction - PLAYER_IA_BOMB;

    if ((explosive >= 0) && (explosive < 2)) {
        return explosive;
    } else {
        return -1;
    }
}

s32 Player_GetExplosiveHeld(Player* this) {
    return Player_ActionToExplosive(this, this->heldItemAction);
}

s32 func_8008F2BC(UNUSED Player* this, s32 itemAction) {
    s32 sword = 0;

    if (itemAction != PLAYER_IA_SWORD_CS) {
        sword = itemAction - PLAYER_IA_SWORD_MASTER;
        if ((sword < 0) || (sword >= 3)) {
            goto return_neg;
        }
    }

    return sword;

return_neg:
    return -1;
}

s32 Player_GetEnvironmentalHazard(PlayState* play) {
    Player* this = GET_PLAYER(play);
    EnvHazardTextTriggerEntry* triggerEntry;
    s32 envHazard;

    if (play->roomCtx.curRoom.environmentType == ROOM_ENV_HOT) { // Room is hot
        envHazard = PLAYER_ENV_HAZARD_HOTROOM - 1;
    } else if ((this->underwaterTimer > 80) &&
               ((this->currentBoots == PLAYER_BOOTS_IRON) || (this->underwaterTimer >= 300))) {
        envHazard = ((this->currentBoots == PLAYER_BOOTS_IRON) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))
                        ? (PLAYER_ENV_HAZARD_UNDERWATER_FLOOR - 1)
                        : (PLAYER_ENV_HAZARD_UNDERWATER_FREE - 1);
    } else if (this->stateFlags1 & PLAYER_STATE1_27) { // Swimming
        envHazard = PLAYER_ENV_HAZARD_SWIMMING - 1;
    } else {
        return PLAYER_ENV_HAZARD_NONE;
    }

    triggerEntry = &sEnvHazardTextTriggers[envHazard];
    if (!Player_InCsMode(play)) {
        if ((triggerEntry->flag != 0) && !(gSaveContext.envHazardTextTriggerFlags & triggerEntry->flag) &&
            (((envHazard == (PLAYER_ENV_HAZARD_HOTROOM - 1)) && (this->currentTunic != PLAYER_TUNIC_CAIRN)) ||
             (((envHazard == (PLAYER_ENV_HAZARD_UNDERWATER_FLOOR - 1)) ||
               (envHazard == (PLAYER_ENV_HAZARD_UNDERWATER_FREE - 1))) &&
              (this->currentBoots == PLAYER_BOOTS_IRON) && (this->currentTunic != PLAYER_TUNIC_NERI)))) {
            Message_StartTextbox(play, triggerEntry->textId, NULL);
            gSaveContext.envHazardTextTriggerFlags |= triggerEntry->flag;
        }
    }

    return envHazard + 1;
}

PlayerFaceIndices sPlayerFaces[PLAYER_FACE_MAX] = {
    // The first 6 faces defined must be default blinking faces. See relevant code in `Player_UpdateCommon`.
    { PLAYER_EYES_OPEN, PLAYER_MOUTH_CLOSED },   // PLAYER_FACE_NEUTRAL
    { PLAYER_EYES_HALF, PLAYER_MOUTH_CLOSED },   // PLAYER_FACE_NEUTRAL_BLINKING_HALF
    { PLAYER_EYES_CLOSED, PLAYER_MOUTH_CLOSED }, // PLAYER_FACE_NEUTRAL_BLINKING_CLOSED

    // This duplicate set of blinking faces is defined because Player will choose between the first and second set
    // based on gameplayFrames. See relevant code in `Player_UpdateCommon`.
    // This, in theory, allows for psuedo-random variance in the faces used. But in practice, duplicate faces are used.
    { PLAYER_EYES_OPEN, PLAYER_MOUTH_CLOSED },   // PLAYER_FACE_NEUTRAL_2
    { PLAYER_EYES_HALF, PLAYER_MOUTH_CLOSED },   // PLAYER_FACE_NEUTRAL_BLINKING_HALF_2
    { PLAYER_EYES_CLOSED, PLAYER_MOUTH_CLOSED }, // PLAYER_FACE_NEUTRAL_BLINKING_CLOSED_2

    // The rest of these faces go unused. Face data encoded within animations handles all other faces.
    { PLAYER_EYES_LEFT, PLAYER_MOUTH_CLOSED },  // PLAYER_FACE_LOOK_LEFT
    { PLAYER_EYES_WIDE, PLAYER_MOUTH_HALF },    // PLAYER_FACE_SURPRISED
    { PLAYER_EYES_WINCING, PLAYER_MOUTH_OPEN }, // PLAYER_FACE_HURT
    { PLAYER_EYES_OPEN, PLAYER_MOUTH_OPEN },    // PLAYER_FACE_GASP
    { PLAYER_EYES_RIGHT, PLAYER_MOUTH_CLOSED }, // PLAYER_FACE_LOOK_RIGHT
    { PLAYER_EYES_LEFT, PLAYER_MOUTH_CLOSED },  // PLAYER_FACE_LOOK_LEFT_2
    { PLAYER_EYES_CLOSED, PLAYER_MOUTH_OPEN },  // PLAYER_FACE_EYES_CLOSED_MOUTH_OPEN
    { PLAYER_EYES_HALF, PLAYER_MOUTH_HALF },    // PLAYER_FACE_OPENING
    { PLAYER_EYES_OPEN, PLAYER_MOUTH_OPEN },    // PLAYER_FACE_EYES_AND_MOUTH_OPEN
    { PLAYER_EYES_OPEN, PLAYER_MOUTH_CLOSED },  // PLAYER_FACE_NEUTRAL_3
};

/**
 * Aren's eyes and mouth textures are placed at the exact same place in adult and child Aren's respective object files.
 * This allows the array to only contain the symbols for one file and have it apply to both. This is a problem for
 * shiftability, and changes will need to be made in the code to account for this in a modding scenario. The symbols
 * from adult Aren's object are used here.
 */
#ifndef AVOID_UB
void* sEyeTextures[PLAYER_EYES_MAX] = {
    gArenAdultEyesOpenTex,    // PLAYER_EYES_OPEN
    gArenAdultEyesHalfTex,    // PLAYER_EYES_HALF
    gArenAdultEyesClosedTex,  // PLAYER_EYES_CLOSED
    gArenAdultEyesRightTex,   // PLAYER_EYES_RIGHT
    gArenAdultEyesLeftTex,    // PLAYER_EYES_LEFT
    gArenAdultEyesWideTex,    // PLAYER_EYES_WIDE
    gArenAdultEyesDownTex,    // PLAYER_EYES_DOWN
    gArenAdultEyesWincingTex, // PLAYER_EYES_WINCING
};

void* sMouthTextures[PLAYER_MOUTH_MAX] = {
    gArenAdultMouthClosedTex, // PLAYER_MOUTH_CLOSED
    gArenAdultMouthHalfTex,   // PLAYER_MOUTH_HALF
    gArenAdultMouthOpenTex,   // PLAYER_MOUTH_OPEN
    gArenAdultMouthSmileTex,  // PLAYER_MOUTH_SMILE
};
#else
// Defining `AVOID_UB` will use a 2D array instead and properly use the child link pointers to allow for shifting.
void* sEyeTextures[][PLAYER_EYES_MAX] = {
    {
        gArenAdultEyesOpenTex,    // PLAYER_EYES_OPEN
        gArenAdultEyesHalfTex,    // PLAYER_EYES_HALF
        gArenAdultEyesClosedTex,  // PLAYER_EYES_CLOSED
        gArenAdultEyesRightTex,   // PLAYER_EYES_RIGHT
        gArenAdultEyesLeftTex,    // PLAYER_EYES_LEFT
        gArenAdultEyesWideTex,    // PLAYER_EYES_WIDE
        gArenAdultEyesDownTex,    // PLAYER_EYES_DOWN
        gArenAdultEyesWincingTex, // PLAYER_EYES_WINCING
    },
    {
        gArenChildEyesOpenTex,   // PLAYER_EYES_OPEN
        gArenChildEyesHalfTex,   // PLAYER_EYES_HALF
        gArenChildEyesClosedTex, // PLAYER_EYES_CLOSED
        /*
        Note `PLAYER_EYES_RIGHT` corresponds to the "left" eyes texture, and vice-versa with the "right" eyes textures.
        This is because on the textures Aren appears to look left/right as if facing outwards the screen,
        but the image is mirrored by the child Aren model's UVs, reversing the direction actually looked in,
        which results in-game in the correct eyes direction.
        */
        gArenChildEyesLeftTex,    // PLAYER_EYES_RIGHT
        gArenChildEyesRightTex,   // PLAYER_EYES_LEFT
        gArenChildEyesWideTex,    // PLAYER_EYES_WIDE
        gArenChildEyesDownTex,    // PLAYER_EYES_DOWN
        gArenChildEyesWincingTex, // PLAYER_EYES_WINCING
    },
};

void* sMouthTextures[][PLAYER_MOUTH_MAX] = {
    {
        gArenAdultMouthClosedTex, // PLAYER_MOUTH_CLOSED
        gArenAdultMouthHalfTex,   // PLAYER_MOUTH_HALF
        gArenAdultMouthOpenTex,   // PLAYER_MOUTH_OPEN
        gArenAdultMouthSmileTex,  // PLAYER_MOUTH_SMILE
    },
    {
        gArenChildMouthClosedTex, // PLAYER_MOUTH_CLOSED
        gArenChildMouthHalfTex,   // PLAYER_MOUTH_HALF
        gArenChildMouthOpenTex,   // PLAYER_MOUTH_OPEN
        gArenChildMouthSmileTex,  // PLAYER_MOUTH_SMILE
    },
};
#endif

Color_RGB8 sTunicColors[PLAYER_TUNIC_MAX] = {
    { 30, 105, 27 }, // PLAYER_TUNIC_SYLVAN
    { 100, 20, 0 },  // PLAYER_TUNIC_CAIRN
    { 0, 60, 100 },  // PLAYER_TUNIC_NERI
};

Color_RGB8 sGauntletColors[] = {
    { 255, 255, 255 },
    { 254, 207, 15 },
};

DListRef  sBootDListGroups[][2] = {
    { gArenAdultLeftIronBootDL, gArenAdultRightIronBootDL },   // PLAYER_BOOTS_IRON
    { gArenAdultLeftHoverBootDL, gArenAdultRightHoverBootDL }, // PLAYER_BOOTS_HOVER
};

void Player_DrawImpl(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount, s32 lod, s32 tunic, s32 boots,
                     s32 face, OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw, void* data) {
    Color_RGB8* color;
    s32 eyesIndex;
    s32 mouthIndex;

    // Player's animation data includes eyes and mouth indices for which texture to use on a given frame.
    // Despite being accessed as "the x component of the 22nd limb", the eyes and mouth indices are stored in 2
    // additional bytes tacked onto the end of the limb rotation data for a given animation frame.
    eyesIndex = (jointTable[22].x & 0xF) - 1;
    mouthIndex = (jointTable[22].x >> 4) - 1;


    // If the eyes index provided by the animation is negative, use the value provided by the `face` argument instead
    if (eyesIndex < 0) {
        eyesIndex = sPlayerFaces[face].eyeIndex;
    }

#ifndef AVOID_UB
    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sEyeTextures[eyesIndex]))));
#else
    GFX_TO(OPA, Gfx3D_SetSegment(0x08, (const void*)(Asset_Resolve(sEyeTextures[gSaveContext.save.arenAge][eyesIndex]))));
#endif

    // If the mouth index provided by the animation is negative, use the value provided by the `face` argument instead
    if (mouthIndex < 0) {
        mouthIndex = sPlayerFaces[face].mouthIndex;
    }

#ifndef AVOID_UB
    GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(sMouthTextures[mouthIndex]))));
#else
    GFX_TO(OPA, Gfx3D_SetSegment(0x09, (const void*)(Asset_Resolve(sMouthTextures[gSaveContext.save.arenAge][mouthIndex]))));
#endif

    color = &sTunicColors[tunic];
    GFX_TO(OPA, Gfx3D_SetEnvColor(color->r, color->g, color->b, 0));

    sDListsLodOffset = lod * 2;

    GFX_TO(OPA, SkelAnime_DrawFlexLod(play, skeleton, jointTable, dListCount, overrideLimbDraw, postLimbDraw, data, lod));

    if ((overrideLimbDraw != Player_OverrideLimbDrawGameplayFirstPerson) &&
        (overrideLimbDraw != Player_OverrideLimbDrawGameplayCrawling) &&
        (gSaveContext.gameMode != GAMEMODE_END_CREDITS)) {
        if (AREN_IS_ADULT) {
            s32 strengthUpgrade = CUR_UPG_VALUE(UPG_STRENGTH);

            if (strengthUpgrade >= 2) { // silver or gold gauntlets

                color = &sGauntletColors[strengthUpgrade - 2];
                GFX_TO(OPA, Gfx3D_SetEnvColor(color->r, color->g, color->b, 0));

                GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(gArenAdultLeftGauntletPlate1DL)));
                GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(gArenAdultRightGauntletPlate1DL)));
                GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)((sLeftHandType == PLAYER_MODELTYPE_LH_OPEN)
                                                    ? gArenAdultLeftGauntletPlate2DL
                                                    : gArenAdultLeftGauntletPlate3DL)));
                GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)((sRightHandType == PLAYER_MODELTYPE_RH_OPEN)
                                                    ? gArenAdultRightGauntletPlate2DL
                                                    : gArenAdultRightGauntletPlate3DL)));
            }

            if (boots != PLAYER_BOOTS_SYLVAN) {
                DListRef* bootDLists = sBootDListGroups[boots - PLAYER_BOOTS_IRON];

                GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(bootDLists[0])));
                GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(bootDLists[1])));
            }
        } else {
            if (Player_GetStrength() > PLAYER_STR_NONE) {
                GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(gArenChildCairnBraceletDL)));
            }
        }
    }

}

Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };

Vec3f D_80126038[] = {
    { 1304.0f, 0.0f, 0.0f },
    { 695.0f, 0.0f, 0.0f },
};

f32 D_80126050[] = { 1265.0f, 826.0f };
f32 D_80126058[] = { SQ(13.04f), SQ(6.95f) };
f32 D_80126060[] = { 10.019104f, -19.925102f };
f32 D_80126068[] = { 5.0f, 3.0f };

Vec3f D_80126070 = { 0.0f, -300.0f, 0.0f };

void func_8008F87C(PlayState* play, Player* this, SkelAnime* skelAnime, Vec3f* pos, Vec3s* rot, s32 thighLimbIndex,
                   s32 shinLimbIndex, s32 footLimbIndex) {
    Vec3f spA4;
    Vec3f sp98;
    Vec3f footprintPos;
    CollisionPoly* sp88;
    s32 sp84;
    f32 sp80;
    f32 sp7C;
    f32 sp78;
    f32 sp74;
    f32 sp70;
    f32 sp6C;
    f32 sp68;
    f32 sp64;
    f32 sp60;
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    f32 sp50;
    s16 temp1;
    s16 temp2;
    s32 temp3;

    if ((this->actor.scale.y >= 0.0f) && !(this->stateFlags1 & PLAYER_STATE1_DEAD) &&
        (Player_ActionToMagicSpell(this, this->itemAction) < 0)) {
        STACK_PAD(s32);

        sp7C = D_80126058[(void)0, gSaveContext.save.arenAge];
        sp78 = D_80126060[(void)0, gSaveContext.save.arenAge];
        sp74 = D_80126068[(void)0, gSaveContext.save.arenAge] - this->unk_6C4;

        Matrix_Push();
        Matrix_TranslateRotateZYX(pos, rot);
        Matrix_MultVec3f(&sZeroVec, &spA4);
        Matrix_TranslateRotateZYX(&D_80126038[(void)0, gSaveContext.save.arenAge],
                                  &skelAnime->jointTable[shinLimbIndex]);
        Matrix_Translate(D_80126050[(void)0, gSaveContext.save.arenAge], 0.0f, 0.0f, MTXMODE_APPLY);
        Matrix_MultVec3f(&sZeroVec, &sp98);
        Matrix_MultVec3f(&D_80126070, &footprintPos);
        Matrix_Pop();

        footprintPos.y += 15.0f;

        sp80 = BgCheck_EntityRaycastDown4(&play->colCtx, &sp88, &sp84, &this->actor, &footprintPos) + sp74;

        if (sp98.y < sp80) {
            sp70 = sp98.x - spA4.x;
            sp6C = sp98.y - spA4.y;
            sp68 = sp98.z - spA4.z;

            sp64 = sqrtf(SQ(sp70) + SQ(sp6C) + SQ(sp68));
            sp60 = (SQ(sp64) + sp78) / (2.0f * sp64);

            sp58 = sp7C - SQ(sp60);
            sp58 = (sp7C < SQ(sp60)) ? 0.0f : sqrtf(sp58);

            sp54 = Math_FAtan2F(sp58, sp60);

            sp6C = sp80 - spA4.y;

            sp64 = sqrtf(SQ(sp70) + SQ(sp6C) + SQ(sp68));
            sp60 = (SQ(sp64) + sp78) / (2.0f * sp64);
            sp5C = sp64 - sp60;

            sp58 = sp7C - SQ(sp60);
            sp58 = (sp7C < SQ(sp60)) ? 0.0f : sqrtf(sp58);

            sp50 = Math_FAtan2F(sp58, sp60);

            temp1 = RAD_TO_BINANG(M_PI - (Math_FAtan2F(sp5C, sp58) + ((M_PI / 2) - sp50)));
            temp1 -= skelAnime->jointTable[shinLimbIndex].z;

            if ((s16)(ABS(skelAnime->jointTable[shinLimbIndex].x) + ABS(skelAnime->jointTable[shinLimbIndex].y)) < 0) {
                temp1 += 0x8000;
            }

            temp2 = RAD_TO_BINANG(sp50 - sp54);
            rot->z -= temp2;

            skelAnime->jointTable[thighLimbIndex].z = skelAnime->jointTable[thighLimbIndex].z - temp2;
            skelAnime->jointTable[shinLimbIndex].z = skelAnime->jointTable[shinLimbIndex].z + temp1;
            skelAnime->jointTable[footLimbIndex].z = skelAnime->jointTable[footLimbIndex].z + temp2 - temp1;

            temp3 = SurfaceType_GetFloorType(&play->colCtx, sp88, sp84);

            if ((temp3 >= FLOOR_TYPE_2) && (temp3 <= FLOOR_TYPE_3) && !func_80042108(&play->colCtx, sp88, sp84)) {
                footprintPos.y = sp80;
                EffectSsGFire_Spawn(play, &footprintPos);
            }
        }
    }
}

s32 Player_OverrideLimbDrawGameplayCommon(PlayState* play, s32 limbIndex, DListRef* dList, Vec3f* pos, Vec3s* rot,
                                          void* thisx) {
    Player* this = (Player*)thisx;

    if (limbIndex == PLAYER_LIMB_ROOT) {
        sLeftHandType = this->leftHandType;
        sRightHandType = this->rightHandType;

        // `sCurBodyPartPos` is incremented before each usage, so it starts at index -1.
        // What determines if a limb corresponds to a body part is whether or not it has a non-NULL `dList`.
        // Note: The increment would not be done for the root limb, even if it had a non-NULL `dList`.
        //       So if the root limb had a non-NULL `dList` (which is not the case in vanilla),
        //       an out-of-bounds write to `bodyPartsPos` would occur.
        sCurBodyPartPos = &this->bodyPartsPos[0] - 1;

        if (!AREN_IS_ADULT) {
            if (!(this->skelAnime.movementFlags & ANIM_FLAG_DISABLE_CHILD_ROOT_ADJUSTMENT) ||
                (this->skelAnime.movementFlags & ANIM_FLAG_UPDATE_XZ)) {
                pos->x *= 0.64f;
                pos->z *= 0.64f;
            }

            if (!(this->skelAnime.movementFlags & ANIM_FLAG_DISABLE_CHILD_ROOT_ADJUSTMENT) ||
                (this->skelAnime.movementFlags & ANIM_FLAG_UPDATE_Y)) {
                pos->y *= 0.64f;
            }
        }

        pos->y -= this->unk_6C4;

        if (this->unk_6C2 != 0) {
            Matrix_Translate(pos->x, ((Math_CosS(this->unk_6C2) - 1.0f) * 200.0f) + pos->y, pos->z, MTXMODE_APPLY);
            Matrix_RotateX(BINANG_TO_RAD(this->unk_6C2), MTXMODE_APPLY);
            Matrix_RotateZYX(rot->x, rot->y, rot->z, MTXMODE_APPLY);
            pos->x = pos->y = pos->z = 0.0f;
            rot->x = rot->y = rot->z = 0;
        }
    } else {
        if (*dList != NULL) {
            sCurBodyPartPos++;
        }

        if (limbIndex == PLAYER_LIMB_HEAD) {
            rot->x += this->headLimbRot.z;
            rot->y -= this->headLimbRot.y;
            rot->z += this->headLimbRot.x;
        } else if (limbIndex == PLAYER_LIMB_UPPER) {
            if (this->upperLimbYawSecondary != 0) {
                Matrix_RotateZ(BINANG_TO_RAD(0x44C), MTXMODE_APPLY);
                Matrix_RotateY(BINANG_TO_RAD(this->upperLimbYawSecondary), MTXMODE_APPLY);
            }
            if (this->upperLimbRot.y != 0) {
                Matrix_RotateY(BINANG_TO_RAD(this->upperLimbRot.y), MTXMODE_APPLY);
            }
            if (this->upperLimbRot.x != 0) {
                Matrix_RotateX(BINANG_TO_RAD(this->upperLimbRot.x), MTXMODE_APPLY);
            }
            if (this->upperLimbRot.z != 0) {
                Matrix_RotateZ(BINANG_TO_RAD(this->upperLimbRot.z), MTXMODE_APPLY);
            }
        } else if (limbIndex == PLAYER_LIMB_L_THIGH) {
            STACK_PAD(s32);

            func_8008F87C(play, this, &this->skelAnime, pos, rot, PLAYER_LIMB_L_THIGH, PLAYER_LIMB_L_SHIN,
                          PLAYER_LIMB_L_FOOT);
        } else if (limbIndex == PLAYER_LIMB_R_THIGH) {
            STACK_PAD(s32);

            func_8008F87C(play, this, &this->skelAnime, pos, rot, PLAYER_LIMB_R_THIGH, PLAYER_LIMB_R_SHIN,
                          PLAYER_LIMB_R_FOOT);
        } else {
            return false;
        }
    }

    return false;
}

s32 Player_OverrideLimbDrawGameplayDefault(PlayState* play, s32 limbIndex, DListRef* dList, Vec3f* pos, Vec3s* rot,
                                           void* thisx) {
    Player* this = (Player*)thisx;

    if (!Player_OverrideLimbDrawGameplayCommon(play, limbIndex, dList, pos, rot, thisx)) {
        if (limbIndex == PLAYER_LIMB_L_HAND) {
            DListRef* dLists = this->leftHandDLists;

            if ((sLeftHandType == PLAYER_MODELTYPE_LH_BGS) && (gSaveContext.save.info.playerData.swordHealth <= 0.0f)) {
                dLists += 4;
            } else if ((sLeftHandType == PLAYER_MODELTYPE_LH_BOOMERANG) &&
                       (this->stateFlags1 & PLAYER_STATE1_BOOMERANG_THROWN)) {
                dLists = gPlayerLeftHandOpenDLs + gSaveContext.save.arenAge;
                sLeftHandType = PLAYER_MODELTYPE_LH_OPEN;
            } else if ((this->leftHandType == PLAYER_MODELTYPE_LH_OPEN) && (this->actor.speed > 2.0f) &&
                       !(this->stateFlags1 & PLAYER_STATE1_27)) {
                dLists = gPlayerLeftHandClosedDLs + gSaveContext.save.arenAge;
                sLeftHandType = PLAYER_MODELTYPE_LH_CLOSED;
            }

            *dList = *(dLists + sDListsLodOffset);
        } else if (limbIndex == PLAYER_LIMB_R_HAND) {
            DListRef* dLists = this->rightHandDLists;

            if (sRightHandType == PLAYER_MODELTYPE_RH_SHIELD) {
                dLists += this->currentShield * 4;
            } else if ((this->rightHandType == PLAYER_MODELTYPE_RH_OPEN) && (this->actor.speed > 2.0f) &&
                       !(this->stateFlags1 & PLAYER_STATE1_27)) {
                dLists = sPlayerRightHandClosedDLs + gSaveContext.save.arenAge;
                sRightHandType = PLAYER_MODELTYPE_RH_CLOSED;
            }

            *dList = *(dLists + sDListsLodOffset);
        } else if (limbIndex == PLAYER_LIMB_BACK) {
            DListRef* dLists = this->backDLists;

            if ((this->backType == PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH) ||
                (this->backType == PLAYER_MODELTYPE_BACK_SHIELD)) {
                dLists += this->currentShield * 4;
                if (!AREN_IS_ADULT && (this->currentShield < PLAYER_SHIELD_ELDRAN) &&
                    (gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_SYLVAN)) {
                    dLists += PLAYER_SHIELD_MAX * 4;
                }
            } else if (!AREN_IS_ADULT &&
                       ((this->backType == PLAYER_MODELTYPE_BACK_SWORD_IN_SHEATH) ||
                        (this->backType == PLAYER_MODELTYPE_BACK_SHEATH)) &&
                       (gSaveContext.save.info.equips.buttonItems[0] != ITEM_SWORD_SYLVAN)) {
                dLists = sPlayerBackShieldSwordInSheathDLs + PLAYER_SHIELD_MAX * 4;
            }

            *dList = *(dLists + sDListsLodOffset);
        } else if (limbIndex == PLAYER_LIMB_WAIST) {
            *dList = *(this->waistDLists + sDListsLodOffset);
        }
    }

    return false;
}

s32 Player_OverrideLimbDrawGameplayFirstPerson(PlayState* play, s32 limbIndex, DListRef* dList, Vec3f* pos, Vec3s* rot,
                                               void* thisx) {
    Player* this = (Player*)thisx;

    if (!Player_OverrideLimbDrawGameplayCommon(play, limbIndex, dList, pos, rot, thisx)) {
        if (this->unk_6AD != 2) {
            *dList = NULL;
        } else if (limbIndex == PLAYER_LIMB_L_FOREARM) {
            *dList = sFirstPersonLeftForearmDLs[(void)0, gSaveContext.save.arenAge];
        } else if (limbIndex == PLAYER_LIMB_L_HAND) {
            *dList = sFirstPersonLeftHandDLs[(void)0, gSaveContext.save.arenAge];
        } else if (limbIndex == PLAYER_LIMB_R_SHOULDER) {
            *dList = sFirstPersonRightShoulderDLs[(void)0, gSaveContext.save.arenAge];
        } else if (limbIndex == PLAYER_LIMB_R_FOREARM) {
            *dList = sFirstPersonForearmDLs[(void)0, gSaveContext.save.arenAge];
        } else if (limbIndex == PLAYER_LIMB_R_HAND) {
            *dList = Player_HoldsGrapnel(this)
                         ? gArenAdultRightHandHoldingGrapnelFirstPersonDL
                         : sFirstPersonRightHandHoldingWeaponDLs[(void)0, gSaveContext.save.arenAge];
        } else {
            *dList = NULL;
        }
    }

    return false;
}

s32 Player_OverrideLimbDrawGameplayCrawling(PlayState* play, s32 limbIndex, DListRef* dList, Vec3f* pos, Vec3s* rot,
                                            void* thisx) {
    if (!Player_OverrideLimbDrawGameplayCommon(play, limbIndex, dList, pos, rot, thisx)) {
        *dList = NULL;
    }

    return false;
}

/**
 * Handles colliders for player weapons, by creating a quad collider each frame between the weapon's previous position
 * and its new position.
 * This position is given as a pair, `newPosA` and `newPosB`, representing two ends of a line that can be thought of as
 * the active part of the weapon. Note that this line is not necessarily following the weapon's shape: for example
 * arrows use a line perpendicular to the shaft.
 * @param collider The quad collider to use for the weapon.
 * @param newPosA One end of the line. For melee weapons, this is the tip.
 * @param newPosB One end of the line. For melee weapons, this is the base.
 * @return true if the weapon is active at a new position.
 */
u8 Player_UpdateWeaponInfo(PlayState* play, ColliderQuad* collider, WeaponInfo* weaponInfo, Vec3f* newPosA,
                           Vec3f* newPosB) {
    if (!weaponInfo->active) {
        if (collider != NULL) {
            Collider_ResetQuadAT(play, &collider->base);
        }
        Math_Vec3f_Copy(&weaponInfo->posA, newPosA);
        Math_Vec3f_Copy(&weaponInfo->posB, newPosB);
        weaponInfo->active = true;
        return true;
    } else if ((weaponInfo->posA.x == newPosA->x) && (weaponInfo->posA.y == newPosA->y) &&
               (weaponInfo->posA.z == newPosA->z) && (weaponInfo->posB.x == newPosB->x) &&
               (weaponInfo->posB.y == newPosB->y) && (weaponInfo->posB.z == newPosB->z)) {
        if (collider != NULL) {
            Collider_ResetQuadAT(play, &collider->base);
        }
        return false;
    } else {
        if (collider != NULL) {
            Collider_SetQuadVertices(collider, newPosB, newPosA, &weaponInfo->posB, &weaponInfo->posA);
            CollisionCheck_SetAT(play, &play->colChkCtx, &collider->base);
        }
        Math_Vec3f_Copy(&weaponInfo->posB, newPosB);
        Math_Vec3f_Copy(&weaponInfo->posA, newPosA);
        weaponInfo->active = true;
        return true;
    }
}

void Player_UpdateShieldCollider(PlayState* play, Player* this, ColliderQuad* collider, Vec3f* quadSrc) {
    static u8 shieldColMaterials[PLAYER_SHIELD_MAX] = {
        COL_MATERIAL_METAL,
        COL_MATERIAL_WOOD,
        COL_MATERIAL_METAL,
        COL_MATERIAL_METAL,
    };

    if (this->stateFlags1 & PLAYER_STATE1_SHIELDING) {
        Vec3f quadDest[4];

        this->shieldQuad.base.colMaterial = shieldColMaterials[this->currentShield];

        Matrix_MultVec3f(&quadSrc[0], &quadDest[0]);
        Matrix_MultVec3f(&quadSrc[1], &quadDest[1]);
        Matrix_MultVec3f(&quadSrc[2], &quadDest[2]);
        Matrix_MultVec3f(&quadSrc[3], &quadDest[3]);
        Collider_SetQuadVertices(collider, &quadDest[0], &quadDest[1], &quadDest[2], &quadDest[3]);

        CollisionCheck_SetAC(play, &play->colChkCtx, &collider->base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &collider->base);
    }
}

// Positions for the tip of melee weapons, in the left hand limb's own model space.
Vec3f sMeleeWeaponTipOffsetFromLeftHand0 = { 5000.0f, 400.0f, 0.0f };
Vec3f sMeleeWeaponTipOffsetFromLeftHand1 = { 5000.0f, -400.0f, 1000.0f };
Vec3f sMeleeWeaponTipOffsetFromLeftHand2 = { 5000.0f, 1400.0f, -1000.0f };

// Positions for the base of melee weapons, in the left hand limb's own model space.
Vec3f sMeleeWeaponBaseOffsetFromLeftHand0 = { 0.0f, 400.0f, 0.0f };
Vec3f sMeleeWeaponBaseOffsetFromLeftHand1 = { 0.0f, 1400.0f, -1000.0f };
Vec3f sMeleeWeaponBaseOffsetFromLeftHand2 = { 0.0f, -400.0f, 1000.0f };

void Player_UpdateMeleeWeaponInfo(PlayState* play, Player* this, Vec3f* newTipPositions) {
    Vec3f newBasePositions[3];

    Matrix_MultVec3f(&sMeleeWeaponBaseOffsetFromLeftHand0, &newBasePositions[0]);
    Matrix_MultVec3f(&sMeleeWeaponBaseOffsetFromLeftHand1, &newBasePositions[1]);
    Matrix_MultVec3f(&sMeleeWeaponBaseOffsetFromLeftHand2, &newBasePositions[2]);

    if (Player_UpdateWeaponInfo(play, NULL, &this->meleeWeaponInfo[0], &newTipPositions[0], &newBasePositions[0]) &&
        !(this->stateFlags1 & PLAYER_STATE1_SHIELDING)) {
        EffectBlure_AddVertex(Effect_GetByIndex(this->meleeWeaponEffectIndex), &this->meleeWeaponInfo[0].posA,
                              &this->meleeWeaponInfo[0].posB);
    }

    if ((this->meleeWeaponState > 0) &&
        ((this->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) || (this->stateFlags2 & PLAYER_STATE2_17))) {
        Player_UpdateWeaponInfo(play, &this->meleeWeaponQuads[0], &this->meleeWeaponInfo[1], &newTipPositions[1],
                                &newBasePositions[1]);
        Player_UpdateWeaponInfo(play, &this->meleeWeaponQuads[1], &this->meleeWeaponInfo[2], &newTipPositions[2],
                                &newBasePositions[2]);
    }
}

void Player_DrawGetItemImpl(PlayState* play, Player* this, Vec3f* refPos, s32 drawIdPlusOne) {
    f32 height = (this->exchangeItemId != EXCH_ITEM_NONE) ? 6.0f : 14.0f;



    Asset_SetSegment(6, this->giDsObj);

    Matrix_Translate(refPos->x + (3.3f * Math_SinS(this->actor.shape.rot.y)), refPos->y + height,
                     refPos->z + ((3.3f + (IREG(90) / 10.0f)) * Math_CosS(this->actor.shape.rot.y)), MTXMODE_NEW);
    Matrix_RotateZYX(0, play->gameplayFrames * 1000, 0, MTXMODE_APPLY);
    Matrix_Scale(0.2f, 0.2f, 0.2f, MTXMODE_APPLY);

    GetItem_Draw(play, drawIdPlusOne - 1);

}

void Player_DrawGetItem(PlayState* play, Player* this) {
    // native: the converted GI object was loaded synchronously when requested (draw-only flag)
    if (!this->giObjectLoading || this->giObjectLoadPending) {
        this->giObjectLoadPending = 0;
        this->giObjectLoading = false;
        Player_DrawGetItemImpl(play, this, &sGetItemRefPos, ABS(this->unk_862));
    }
}

void Player_CalcMeleeWeaponTipPositions(Player* this, Vec3f* tipPositions) {
    sMeleeWeaponTipOffsetFromLeftHand1.x = sMeleeWeaponTipOffsetFromLeftHand0.x;

    if (this->unk_845 >= 3) {
        this->unk_845++;
        sMeleeWeaponTipOffsetFromLeftHand1.x *= 1.0f + ((9 - this->unk_845) * 0.1f);
    }

    sMeleeWeaponTipOffsetFromLeftHand1.x += 1200.0f;
    sMeleeWeaponTipOffsetFromLeftHand2.x = sMeleeWeaponTipOffsetFromLeftHand1.x;

    Matrix_MultVec3f(&sMeleeWeaponTipOffsetFromLeftHand0, &tipPositions[0]);
    Matrix_MultVec3f(&sMeleeWeaponTipOffsetFromLeftHand1, &tipPositions[1]);
    Matrix_MultVec3f(&sMeleeWeaponTipOffsetFromLeftHand2, &tipPositions[2]);
}

void Player_DrawGrapnelReticle(PlayState* play, Player* this, f32 arg2) {
    static Vec3f D_801260C8 = { -500.0f, -100.0f, 0.0f };
    CollisionPoly* sp9C;
    s32 bgId;
    Vec3f sp8C;
    Vec3f sp80;
    Vec3f sp74;
    Vec3f sp68;
    f32 sp64;
    f32 sp60;

    D_801260C8.z = 0.0f;
    Matrix_MultVec3f(&D_801260C8, &sp8C);
    D_801260C8.z = arg2;
    Matrix_MultVec3f(&D_801260C8, &sp80);

    if (1) {}

    if (BgCheck_AnyLineTest3(&play->colCtx, &sp8C, &sp80, &sp74, &sp9C, true, true, true, true, &bgId)) {

        Gfx3D_SetupDLTo(GFX_BUF_OVL, 7);

        SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, &sp74, &sp68, &sp64);

        sp60 = (sp64 < 200.0f) ? 0.08f : (sp64 / 200.0f) * 0.08f;

        Matrix_Translate(sp74.x, sp74.y, sp74.z, MTXMODE_NEW);
        Matrix_Scale(sp60, sp60, sp60, MTXMODE_APPLY);

        GFX_TO(OVL, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
        GFX_TO(OVL, Gfx3D_SetSegment(0x06, (const void*)(play->objectCtx.slots[this->actor.objectSlot].segment)));
        GFX_TO(OVL, Gfx3D_DrawDL((u32)(uintptr_t)(gArenAdultGrapnelReticleDL)));

    }
}

// Coordinates of the player focus position, in the head limb's own model space.
Vec3f sPlayerFocusOffsetFromHead = { 1100.0f, -700.0f, 0.0f };

f32 sMeleeWeaponLengths[] = {
    0.0f,    // not a melee weapon
    4000.0f, // Dawnblade
    3000.0f, // Sylvan Sword
    5500.0f, // Highcairn's Sword
    0.0f,    // Root Stick
    2500.0f, // Hammer
};

DListRef  sBottleDLists[] = { gArenAdultBottleDL, gArenChildBottleDL };

Color_RGB8 sBottleColors[] = {
    { 255, 255, 255 }, // Empty
    { 80, 80, 255 },   // Fish
    { 255, 100, 255 }, // Fire
    { 0, 0, 255 },     // Bug
    { 255, 0, 255 },   // Wraith
    { 255, 0, 255 },   // Big Wraith
    { 200, 200, 100 }, // Letter
    { 255, 0, 0 },     // Red Potion
    { 0, 0, 255 },     // Blue Potion
    { 0, 255, 0 },     // Green Potion
    { 255, 255, 255 }, // Milk
    { 255, 255, 255 }, // Half Milk
    { 80, 80, 255 },   // Fairy
};

void Player_PostLimbDrawGameplay(PlayState* play, s32 limbIndex, DListRef* dList, UNUSED Vec3s* rot, void* thisx) {
    Player* this = (Player*)thisx;

    if (*dList != NULL) {
        Matrix_MultVec3f(&sZeroVec, sCurBodyPartPos);
    }

    if (limbIndex == PLAYER_LIMB_L_HAND) {
        MtxF leftHandMtx;
        Actor* heldActor;

        Math_Vec3f_Copy(&this->leftHandPos, sCurBodyPartPos);

        if (this->itemAction == PLAYER_IA_ROOT_STICK) {
            Vec3f tipPositions[3];


            if (this->actor.scale.y >= 0.0f) {
                sMeleeWeaponTipOffsetFromLeftHand0.x = this->unk_85C * 5000.0f;
                Player_CalcMeleeWeaponTipPositions(this, tipPositions);
                if (this->meleeWeaponState != 0) {
                    Player_UpdateMeleeWeaponInfo(play, this, tipPositions);
                } else {
                    Math_Vec3f_Copy(MELEE_WEAPON_INFO_TIP(&this->meleeWeaponInfo[0]), &tipPositions[0]);
                }
            }

            Matrix_Translate(-428.26f, 267.2f, -33.82f, MTXMODE_APPLY);
            Matrix_RotateZYX(-0x8000, 0, 0x4000, MTXMODE_APPLY);
            Matrix_Scale(1.0f, this->unk_85C, 1.0f, MTXMODE_APPLY);

            GFX_TO(OPA, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
            GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(gArenChildArenRootStickDL)));

        } else if ((this->actor.scale.y >= 0.0f) && (this->meleeWeaponState != 0)) {
            Vec3f tipPositions[3];

            if (Player_HoldsBrokenKnife(this)) {
                sMeleeWeaponTipOffsetFromLeftHand0.x = 1500.0f;
            } else {
                sMeleeWeaponTipOffsetFromLeftHand0.x = sMeleeWeaponLengths[Player_GetMeleeWeaponHeld(this)];
            }

            Player_CalcMeleeWeaponTipPositions(this, tipPositions);
            Player_UpdateMeleeWeaponInfo(play, this, tipPositions);
        } else if ((*dList != NULL) && (this->leftHandType == PLAYER_MODELTYPE_LH_BOTTLE)) {
            //! @bug When Player is actively using shield, the `itemAction` value will be set to -1.
            //! If shield is used at the same time a bottle is in hand, `Player_ActionToBottle` will
            //! return -1, which results in an out of bounds access behind the `sBottleColors` array.
            //! A value of -1 happens to access `gArenChildBottleDL` (0x06018478). The last 3 bytes of
            //! this pointer are read as a color, which results in a dark teal color used for the bottle.
            Color_RGB8* bottleColor = &sBottleColors[Player_ActionToBottle(this, this->itemAction)];


            GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
            GFX_TO(XLU, Gfx3D_SetEnvColor(bottleColor->r, bottleColor->g, bottleColor->b, 0));
            GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(sBottleDLists[((void)0, gSaveContext.save.arenAge)])));

        }

        if (this->actor.scale.y >= 0.0f) {
            if (!Player_HoldsGrapnel(this) && ((heldActor = this->heldActor) != NULL)) {
                if (this->stateFlags1 & PLAYER_STATE1_9) {
                    static Vec3f D_80126128 = { 398.0f, 1419.0f, 244.0f };

                    Matrix_MultVec3f(&D_80126128, &heldActor->world.pos);
                    Matrix_RotateZYX(0x69E8, -0x5708, 0x458E, MTXMODE_APPLY);
                    Matrix_Get(&leftHandMtx);
                    Matrix_MtxFToYXZRotS(&leftHandMtx, &heldActor->world.rot, 0);
                    heldActor->shape.rot = heldActor->world.rot;
                } else if (this->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) {
                    Vec3s leftHandRot;

                    Matrix_Get(&leftHandMtx);
                    Matrix_MtxFToYXZRotS(&leftHandMtx, &leftHandRot, 0);

                    if (heldActor->flags & ACTOR_FLAG_CARRY_X_ROT_INFLUENCE) {
                        heldActor->world.rot.x = heldActor->shape.rot.x = leftHandRot.x - this->unk_3BC.x;
                    } else {
                        heldActor->world.rot.y = heldActor->shape.rot.y = this->actor.shape.rot.y + this->unk_3BC.y;
                    }
                }
            } else {
                Matrix_Get(&this->mf_9E0);
                Matrix_MtxFToYXZRotS(&this->mf_9E0, &this->unk_3BC, 0);
            }
        }
    } else if (limbIndex == PLAYER_LIMB_R_HAND) {
        Actor* heldActor = this->heldActor;

        if (this->rightHandType == PLAYER_MODELTYPE_RH_FF) {
            Matrix_Get(&this->shieldMf);
        } else if ((this->rightHandType == PLAYER_MODELTYPE_RH_BOW_SLINGSHOT) ||
                   (this->rightHandType == PLAYER_MODELTYPE_RH_BOW_SLINGSHOT_2)) {
            static BowSlingshotStringData sBowSlingshotStringData[] = {
                { gArenAdultBowStringDL, { 0.0f, -360.4f, 0.0f } },        // Bow
                { gArenChildSlingshotStringDL, { 606.0f, 236.0f, 0.0f } }, // Slingshot
            };
            BowSlingshotStringData* stringData = &sBowSlingshotStringData[gSaveContext.save.arenAge];


            Matrix_Push();
            Matrix_Translate(stringData->pos.x, stringData->pos.y, stringData->pos.z, MTXMODE_APPLY);

            if ((this->stateFlags1 & PLAYER_STATE1_9) && (this->unk_860 >= 0) && (this->unk_834 <= 10)) {
                Vec3f sp90;
                f32 distXYZ;

                Matrix_MultVec3f(&sZeroVec, &sp90);
                distXYZ = Math_Vec3f_DistXYZ(sCurBodyPartPos, &sp90);

                this->unk_858 = distXYZ - 3.0f;
                if (distXYZ < 3.0f) {
                    this->unk_858 = 0.0f;
                } else {
                    this->unk_858 *= 1.6f;
                    if (this->unk_858 > 1.0f) {
                        this->unk_858 = 1.0f;
                    }
                }

                this->unk_85C = -0.5f;
            }

            Matrix_Scale(1.0f, this->unk_858, 1.0f, MTXMODE_APPLY);

            if (!AREN_IS_ADULT) {
                Matrix_RotateZ(this->unk_858 * -0.2f, MTXMODE_APPLY);
            }

            GFX_TO(XLU, Gfx3D_LoadMtxF(Matrix_GetCurrent()));
            GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(stringData->dList)));

            Matrix_Pop();

        } else if ((this->actor.scale.y >= 0.0f) && (this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD)) {
            // Coordinates of the shield quad collider vertices, in the right hand limb's own model space.
            static Vec3f sRightHandLimbModelShieldQuadVertices[] = {
                { -4500.0f, -3000.0f, -600.0f },
                { 1500.0f, -3000.0f, -600.0f },
                { -4500.0f, 3000.0f, -600.0f },
                { 1500.0f, 3000.0f, -600.0f },
            };

            Matrix_Get(&this->shieldMf);
            Player_UpdateShieldCollider(play, this, &this->shieldQuad, sRightHandLimbModelShieldQuadVertices);
        }

        if (this->actor.scale.y >= 0.0f) {
            if ((this->heldItemAction == PLAYER_IA_GRAPNEL) || (this->heldItemAction == PLAYER_IA_LONGGRAPPLE)) {
                static Vec3f D_80126184 = { 100.0f, 1500.0f, 0.0f };

                Matrix_MultVec3f(&D_80126184, &this->unk_3C8);

                if (heldActor != NULL) {
                    static Vec3f D_80126190 = { 100.0f, 1640.0f, 0.0f };
                    MtxF sp44;
                    STACK_PAD(s32);

                    Matrix_MultVec3f(&D_80126190, &heldActor->world.pos);
                    Matrix_RotateZYX(0, -0x4000, -0x4000, MTXMODE_APPLY);
                    Matrix_Get(&sp44);
                    Matrix_MtxFToYXZRotS(&sp44, &heldActor->world.rot, 0);
                    heldActor->shape.rot = heldActor->world.rot;

                    if (func_8002DD78(this)) {
                        Matrix_Translate(500.0f, 300.0f, 0.0f, MTXMODE_APPLY);
                        Player_DrawGrapnelReticle(play, this,
                                                   (this->heldItemAction == PLAYER_IA_GRAPNEL) ? 38600.0f : 77600.0f);
                    }
                }
            }

            if ((this->unk_862 != 0) || ((func_8002DD6C(this) == 0) && (heldActor != NULL))) {
                if (!(this->stateFlags1 & PLAYER_STATE1_10) && (this->unk_862 != 0) &&
                    (this->exchangeItemId != EXCH_ITEM_NONE)) {
                    Math_Vec3f_Copy(&sGetItemRefPos, &this->leftHandPos);
                } else {
                    sGetItemRefPos.x = (this->bodyPartsPos[PLAYER_BODYPART_R_HAND].x + this->leftHandPos.x) * 0.5f;
                    sGetItemRefPos.y = (this->bodyPartsPos[PLAYER_BODYPART_R_HAND].y + this->leftHandPos.y) * 0.5f;
                    sGetItemRefPos.z = (this->bodyPartsPos[PLAYER_BODYPART_R_HAND].z + this->leftHandPos.z) * 0.5f;
                }

                if (this->unk_862 == 0) {
                    Math_Vec3f_Copy(&heldActor->world.pos, &sGetItemRefPos);
                }
            }
        }
    } else if (this->actor.scale.y >= 0.0f) {
        if (limbIndex == PLAYER_LIMB_BACK) {
            if ((this->rightHandType != PLAYER_MODELTYPE_RH_SHIELD) &&
                (this->rightHandType != PLAYER_MODELTYPE_RH_FF)) {
                // Coordinates of the shield quad collider vertices, in the back limb's own model space.
                static Vec3f sBackLimbModelShieldQuadVertices[] = {
                    { -3000.0f, -3000.0f, -900.0f },
                    { 3000.0f, -3000.0f, -900.0f },
                    { -3000.0f, 3000.0f, -900.0f },
                    { 3000.0f, 3000.0f, -900.0f },
                };
                // Position and rotation of the shield on Aren's back, in the back limb's own model space.
                static Vec3f sBackLimbModelShieldOnBackPos = { 630.0f, 100.0f, -30.0f };
                static Vec3s sBackLimbModelShieldOnBackZyxRot = { 0, 0, 0x7FFF };

                if (Player_IsChildWithEldranShield(this)) {
                    Player_UpdateShieldCollider(play, this, &this->shieldQuad, sBackLimbModelShieldQuadVertices);
                }

                Matrix_TranslateRotateZYX(&sBackLimbModelShieldOnBackPos, &sBackLimbModelShieldOnBackZyxRot);
                Matrix_Get(&this->shieldMf);
            }
        } else if (limbIndex == PLAYER_LIMB_HEAD) {
            Matrix_MultVec3f(&sPlayerFocusOffsetFromHead, &this->actor.focus.pos);
        } else {
            // Position of Aren's foot, in the foot limb's own model space.
            static Vec3f sLeftRightFootLimbModelFootPos[] = {
                { 200.0f, 300.0f, 0.0f },
                { 200.0f, 200.0f, 0.0f },
            };
            Vec3f* footPos = &sLeftRightFootLimbModelFootPos[((void)0, gSaveContext.save.arenAge)];

            // The same model position is used for both feet,
            // but the resulting world position will be different for each limb.
            Actor_SetFeetPos(&this->actor, limbIndex, PLAYER_LIMB_L_FOOT, footPos, PLAYER_LIMB_R_FOOT, footPos);
        }
    }
}

// native: the pause menu reuses the object space, where the converted objects are placed
// (engine/object_native.c) as the original placed the object files. As the original, the player object and
// the first chunk of gameplay_keep are copied into the pause segment first: the converted player object is
// loaded there (its textures already loaded by the live copy are shared), and the chunk holds the native
// layout of gameplay_keep's plain records below PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE (the pause
// joint tables, player animation headers) at their original offsets.
static DsObj* sPauseArenObject;

static void Player_BindPauseSegments(u8* segment) {
    // gSegments[4] / gSegments[6]: the copies in the pause segment
    Asset_SetSegment(6, sPauseArenObject);
    Asset_SetCpuSegmentRaw(4, PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_START(segment));
    Gfx3D_SetSegmentObj(4, NULL); // the converted gameplay_keep is overwritten by the pause menu
}

u32 Player_InitPauseDrawData(PlayState* play, u8* segment, SkelAnime* skelAnime) {
    s16 arenObjectId = gArenObjectIds[(void)0, gSaveContext.save.arenAge];
    DsObj* keep = play->objectCtx.slots[play->objectCtx.mainKeepSlot].dsObj;
    s32 slot = Object_GetSlot(&play->objectCtx, arenObjectId);
    u8* chunk = PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_START(segment);
    u32 size = 0;
    void* ptr;

    // the chunk overlaps the converted gameplay_keep: build it aside first
    u8* tmp = malloc(PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE);
    if (tmp != NULL) {
        memset(tmp, 0, PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE);
        if (keep != NULL) {
            Asset_CopyRecords(keep, tmp, PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE);
        }
    }

    if (sPauseArenObject != NULL) {
        DsObj_Free(sPauseArenObject);
    }
    ptr = PAUSE_PLAYER_SEGMENT_AREN_OBJECT(segment);
    sPauseArenObject = Vrom_LoadInto(gObjectTable[arenObjectId].vromStart, ptr,
                                     (u32)((u8*)play->objectCtx.spaceEnd - (u8*)ptr), &size);
    if (sPauseArenObject != NULL && slot >= 0) {
        sPauseArenObject->texOwner = play->objectCtx.slots[slot].dsObj;
    }
    if (tmp != NULL) {
        memcpy(chunk, tmp, PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_BUFFER_SIZE);
        free(tmp);
    }

    // Joint tables are placed after the link object
    ptr = (void*)ALIGN16((uintptr_t)ptr + size);

    Player_BindPauseSegments(segment);

    SkelAnime_InitAren(play, skelAnime, gPlayerSkelHeaders[(void)0, gSaveContext.save.arenAge],
                       gPlayerAnim_aren_normal_wait, 9, ptr, ptr, PLAYER_LIMB_MAX);

    return PAUSE_PLAYER_SEGMENT_TOTAL_SIZE(size);
}

u8 sPauseModelGroupBySword[] = {
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_SWORD_SYLVAN
    PLAYER_MODELGROUP_SWORD_AND_SHIELD, // PLAYER_SWORD_MASTER
    PLAYER_MODELGROUP_BGS,              // PLAYER_SWORD_HIGHCAIRN
};

s32 Player_OverrideLimbDrawPause(UNUSED PlayState* play, s32 limbIndex, DListRef* dList, UNUSED Vec3f* pos,
                                 UNUSED Vec3s* rot, void* arg) {
    u8* playerSwordAndShield = arg;
    //! @bug `playerSwordAndShield[0]` can be 0 (`PLAYER_SWORD_NONE`), which indexes `sPauseModelGroupBySword[-1]`.
    //! The result happens to be 0 (`PLAYER_MODELGROUP_0`) in vanilla, but weird values are likely to cause a crash.
    u8 modelGroup = sPauseModelGroupBySword[playerSwordAndShield[0] - PLAYER_SWORD_SYLVAN];
    s32 type;
    s32 dListOffset = 0;
    DListRef* dLists;

    if ((modelGroup == PLAYER_MODELGROUP_SWORD_AND_SHIELD) && !AREN_IS_ADULT &&
        (playerSwordAndShield[1] == PLAYER_SHIELD_ELDRAN)) {
        modelGroup = PLAYER_MODELGROUP_CHILD_ELDRAN_SHIELD;
    }

    if (limbIndex == PLAYER_LIMB_L_HAND) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_LEFT_HAND];
        sLeftHandType = type;
        if ((type == PLAYER_MODELTYPE_LH_BGS) && (gSaveContext.save.info.playerData.swordHealth <= 0.0f)) {
            dListOffset = 4;
        }
    } else if (limbIndex == PLAYER_LIMB_R_HAND) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_RIGHT_HAND];
        sRightHandType = type;
        if (type == PLAYER_MODELTYPE_RH_SHIELD) {
            dListOffset = playerSwordAndShield[1] * 4;
        }
    } else if (limbIndex == PLAYER_LIMB_BACK) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_BACK];
        if ((type == PLAYER_MODELTYPE_BACK_SHIELD_SWORD_IN_SHEATH) || (type == PLAYER_MODELTYPE_BACK_SHIELD)) {
            dListOffset = playerSwordAndShield[1] * 4;
        }
    } else if (limbIndex == PLAYER_LIMB_WAIST) {
        type = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_WAIST];
    } else {
        return false;
    }

    dLists = sPlayerDListGroups[type] + ((void)0, gSaveContext.save.arenAge);
    *dList = *(dLists + dListOffset);

    return false;
}

void Player_DrawPauseImpl(PlayState* play, void* gameplayKeep, void* arenObject, SkelAnime* skelAnime, Vec3f* pos,
                          Vec3s* rot, f32 scale, s32 sword, s32 tunic, s32 shield, s32 boots, s32 width, s32 height,
                          Vec3f* eye, Vec3f* at, f32 fovy, void* colorFrameBuffer, void* depthFrameBuffer) {
    // gdSPDefLights1(80, 80, 80, 255, 255, 255, 84, 84, -84)
    static const u8 sAmbient[3] = { 80, 80, 80 };
    static const s8 sLightDir[1][3] = { { 84, 84, -84 } };
    static const u8 sLightColor[1][3] = { { 255, 255, 255 } };
    static Vec3f lightDir = { 89.8f, 0.0f, 89.8f };
    u8 playerSwordAndShield[2];
    u16 perspNorm;
    MtxF perspMtx;
    MtxF lookAtMtx;

    (void)gameplayKeep;
    (void)arenObject;
    (void)colorFrameBuffer;
    (void)depthFrameBuffer;

    // native: the original drew from a list of its own (called from the work list) into the colour image
    // colorFrameBuffer with depth image depthFrameBuffer, both filled first (depth: farthest, colour: opaque
    // black), viewport width x height; the offscreen pass of the renderer does the same (engine/gfx3d.h).
    // Its setup commands before func_80093C80 (fill mode, geometry mode) are replaced by that setup.
    ExactGuPerspectiveF(perspMtx.mf, &perspNorm, fovy, (f32)width / (f32)height, 10.0f, 4000.0f, 1.0f);
    ExactQuantizeMtxF(perspMtx.mf);
    ExactGuLookAtF(lookAtMtx.mf, eye->x, eye->y, eye->z, at->x, at->y, at->z, 0.0f, 1.0f, 0.0f);
    ExactQuantizeMtxF(lookAtMtx.mf);

    GFX_TO(OPA, Gfx3D_OffscreenBegin(width, height, &perspMtx, &lookAtMtx));

    playerSwordAndShield[0] = sword;
    playerSwordAndShield[1] = shield;

    Matrix_SetTranslateRotateYXZ(pos->x, pos->y, pos->z, rot);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);

    GFX_TO(OPA, Gfx3D_SetLightsHere(sAmbient, 1, sLightDir, sLightColor));

    func_80093C80(play);

    GFX_TO(OPA, Gfx3D_SetFog(0, 0, 0, 0, 997, 1000));

    func_8002EABC(pos, &play->view.eye, &lightDir, play->state.gfxCtx);

    GFX_TO(OPA, Gfx3D_SetSegment(0x0C, (const void*)gCullBackDList));

    Player_DrawImpl(play, skelAnime->skeleton, skelAnime->jointTable, skelAnime->dListCount, 0, tunic, boots,
                    PLAYER_FACE_NEUTRAL, Player_OverrideLimbDrawPause, NULL, &playerSwordAndShield);

    GFX_TO(OPA, Gfx3D_OffscreenEnd());
}

void Player_DrawPause(PlayState* play, u8* segment, SkelAnime* skelAnime, Vec3f* pos, Vec3s* rot, f32 scale, s32 sword,
                      s32 tunic, s32 shield, s32 boots) {
    static Vec3f eye = { 0.0f, 0.0f, -400.0f };
    static Vec3f at = { 0.0f, 0.0f, 0.0f };
    Vec3s* destTable;
    Vec3s* srcTable;
    s32 i;

    Player_BindPauseSegments(segment);

    if (!AREN_IS_ADULT) {
        if (shield == PLAYER_SHIELD_ROOT) {
            srcTable = gArenPauseChildRootShieldJointTable;
        } else {
            srcTable = gArenPauseChildJointTable;
        }
    } else {
        if (sword == PLAYER_SWORD_HIGHCAIRN) {
            srcTable = gArenPauseAdultBgsJointTable;
        } else if (shield != PLAYER_SHIELD_NONE) {
            srcTable = gArenPauseAdultShieldJointTable;
        } else {
            srcTable = gArenPauseAdultJointTable;
        }
    }

    srcTable = Asset_Resolve(srcTable);
    destTable = skelAnime->jointTable;
    for (i = 0; i < skelAnime->limbCount; i++) {
        *destTable++ = *srcTable++;
    }

    Player_DrawPauseImpl(play, PAUSE_PLAYER_SEGMENT_GAMEPLAY_KEEP_START(segment),
                         PAUSE_PLAYER_SEGMENT_AREN_OBJECT(segment), skelAnime, pos, rot, scale, sword, tunic, shield,
                         boots, PAUSE_EQUIP_PLAYER_WIDTH, PAUSE_EQUIP_PLAYER_HEIGHT, &eye, &at, 60.0f,
                         play->state.gfxCtx->curFrameBuffer,
                         (u16*)play->state.gfxCtx->curFrameBuffer + (PAUSE_EQUIP_PLAYER_WIDTH * PAUSE_EQUIP_PLAYER_HEIGHT));
}

