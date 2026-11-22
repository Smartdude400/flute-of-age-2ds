// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_En_Fire/z_eff_ss_en_fire.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_EN_FIRE_H
#define Z_EFF_SS_EN_FIRE_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsEnFireInitParams {
    /* 0x00 */ struct Actor* actor;
    /* 0x04 */ Vec3f pos;
    /* 0x10 */ s16 scale;
    /* 0x12 */ s16 unk_12;
    /* 0x14 */ s16 flags;
    /* 0x16 */ s16 bodyPart;
} EffectSsEnFireInitParams; // size = 0x18

#endif
