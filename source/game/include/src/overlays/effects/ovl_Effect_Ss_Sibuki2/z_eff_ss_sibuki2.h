// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Sibuki2/z_eff_ss_sibuki2.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_SIBUKI2_H
#define Z_EFF_SS_SIBUKI2_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsSibuki2InitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
    /* 0x24 */ s16 scale;
} EffectSsSibuki2InitParams; // size = 0x28

#endif
