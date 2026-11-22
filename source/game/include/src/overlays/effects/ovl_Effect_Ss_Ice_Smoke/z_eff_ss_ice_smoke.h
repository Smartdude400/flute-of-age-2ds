// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Ice_Smoke/z_eff_ss_ice_smoke.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_ICE_SMOKE_H
#define Z_EFF_SS_ICE_SMOKE_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsIceSmokeInitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
    /* 0x24 */ s16 scale;
} EffectSsIceSmokeInitParams; // size = 0x28

#endif
