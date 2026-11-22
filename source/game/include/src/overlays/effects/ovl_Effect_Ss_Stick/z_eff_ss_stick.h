// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Stick/z_eff_ss_stick.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_STICK_H
#define Z_EFF_SS_STICK_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsStickInitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ s16 yaw;
} EffectSsStickInitParams; // size = 0x10

#endif
