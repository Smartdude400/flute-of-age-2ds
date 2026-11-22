// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Hahen/z_eff_ss_hahen.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_HAHEN_H
#define Z_EFF_SS_HAHEN_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsHahenInitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
    /* 0x24 */ DListRef  dList;
    /* 0x28 */ s16 unused;
    /* 0x2A */ s16 scale;
    /* 0x2C */ s16 objId;
    /* 0x2C */ s16 life;
} EffectSsHahenInitParams; // size = 0x30

#define HAHEN_OBJECT_DEFAULT -1

#endif
