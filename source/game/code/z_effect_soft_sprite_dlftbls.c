// adapted from flute-of-age/src/code/z_effect_soft_sprite_dlftbls.c
// Native effect table, same scheme as z_actor_dlftbls.c: weak profile references (unported effects
// resolve to NULL and are not spawned) and synthetic vram ranges whose size equals the ORIGINAL
// overlay memory size, so EffectSs_Spawn reserves identical actor-heap memory.
#include "effect.h"

#define NATIVE_OVL_BASE 0x18000000u

static const u32 sEffectOverlaySizes[EFFECT_SS_TYPE_MAX] = {
#include "effect_ss_overlay_sizes.inc.h"
};

#define DEFINE_EFFECT_SS(name, _1) extern EffectSsProfile name##_Profile __attribute__((weak));
#define DEFINE_EFFECT_SS_UNSET(_0)
#include "tables/effect_ss_table.h"
#undef DEFINE_EFFECT_SS
#undef DEFINE_EFFECT_SS_UNSET

#define DEFINE_EFFECT_SS(name, enumValue) \
    [enumValue] = { ROM_FILE_UNSET, (void*)NATIVE_OVL_BASE, NULL, NULL, &name##_Profile, 1 },
#define DEFINE_EFFECT_SS_UNSET(enumValue) [enumValue] = { ROM_FILE_UNSET, NULL, NULL, NULL, NULL, 0 },

EffectSsOverlay gEffectSsOverlayTable[] = {
#include "tables/effect_ss_table.h"
};

#undef DEFINE_EFFECT_SS
#undef DEFINE_EFFECT_SS_UNSET

void EffectSsTable_InitNative(void) {
    for (s32 i = 0; i < EFFECT_SS_TYPE_MAX; i++) {
        if (gEffectSsOverlayTable[i].vramStart != NULL) {
            gEffectSsOverlayTable[i].vramEnd = (void*)(NATIVE_OVL_BASE + sEffectOverlaySizes[i]);
        }
    }
}
