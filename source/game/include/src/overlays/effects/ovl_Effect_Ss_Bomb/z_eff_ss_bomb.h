// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Bomb/z_eff_ss_bomb.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_BOMB_H
#define Z_EFF_SS_BOMB_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsBombInitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
} EffectSsBombInitParams; // size = 0x24

#endif
