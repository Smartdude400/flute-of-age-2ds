// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Wisp/z_eff_ss_wisp.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_WISP_H
#define Z_EFF_SS_WISP_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsWispInitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ f32 yPosOffset;
    /* 0x10 */ f32 yPosRandScale;
    /* 0x14 */ f32 xzPosRandScale;
    /* 0x18 */ f32 scale;
} EffectSsWispInitParams; // size = 0x1C

#endif
