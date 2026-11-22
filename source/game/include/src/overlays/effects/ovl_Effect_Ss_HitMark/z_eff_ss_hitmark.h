// adapted from flute-of-age/include/src/overlays/effects/ovl_Effect_Ss_HitMark/z_eff_ss_hitmark.h (ultra64.h -> game_types.h)
#ifndef Z_EFF_SS_HITMARK_H
#define Z_EFF_SS_HITMARK_H

#include "game_types.h"
#include "z_math.h"

typedef struct EffectSsHitMarkInitParams {
    /* 0x00 */ s32 type;
    /* 0x04 */ s16 scale;
    /* 0x08 */ Vec3f pos;
} EffectSsHitMarkInitParams; // size = 0x14

typedef enum EffectSsHitmarkType {
    EFFECT_HITMARK_WHITE,
    EFFECT_HITMARK_DUST,
    EFFECT_HITMARK_RED,
    EFFECT_HITMARK_METAL
} EffectSsHitmarkType;

#endif
