// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Sibuki/z_eff_ss_sibuki.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_SIBUKI_H
#define Z_EFF_SS_SIBUKI_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsSibukiInitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
    /* 0x24 */ s16 moveDelay;
    /* 0x26 */ s16 direction;
    /* 0x28 */ s16 scale;
} EffectSsSibukiInitParams; // size = 0x2C

#endif
