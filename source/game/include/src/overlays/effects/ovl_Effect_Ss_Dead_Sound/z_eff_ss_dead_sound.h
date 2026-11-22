// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_Dead_Sound/z_eff_ss_dead_sound.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_DEAD_SOUND_H
#define Z_EFF_SS_DEAD_SOUND_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsDeadSoundInitParams {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
    /* 0x24 */ u16 sfxId;
    /* 0x26 */ s16 lowerPriority;
    /* 0x28 */ s16 repeatMode;
    /* 0x26 */ s16 unused2;
    /* 0x2C */ s32 life;
} EffectSsDeadSoundInitParams; // size = 0x30

#define DEADSOUND_REPEAT_MODE_OFF 1
#define DEADSOUND_REPEAT_MODE_ON 2

#endif
