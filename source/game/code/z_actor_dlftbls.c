// adapted from flute-of-age/src/code/z_actor_dlftbls.c
// Native actor table. Actor code is statically linked on DS, so:
//  - profiles are WEAK references: an actor that is not ported yet resolves to NULL and cannot spawn
//    (Actor_Spawn logs it), instead of failing the link;
//  - vramStart/vramEnd are synthetic: their difference is the ORIGINAL overlay memory size
//    (text+data+rodata+bss, extracted from the ROM by tools/build_actor_table.py). Actor_Spawn
//    still reserves that much actor-heap memory when the "overlay" loads, so heap exhaustion
//    behaves exactly as in the original.
// The fault-screen client of the original is not ported (native fault screen).
#include "z_actor_dlftbls.h"
#include "actor.h"
#include "printf.h"

#define NATIVE_OVL_BASE 0x10000000u

static const u32 sOverlaySizes[ACTOR_ID_MAX] = {
#include "actor_overlay_sizes.inc.h"
};

// Weak profile declarations: resolve to NULL if the actor is not linked
#define DEFINE_ACTOR(name, _1, _2, _3) extern ActorProfile name##_Profile __attribute__((weak));
#define DEFINE_ACTOR_INTERNAL(name, _1, _2, _3) extern ActorProfile name##_Profile __attribute__((weak));
#define DEFINE_ACTOR_UNSET(_0)
#include "tables/actor_table.h"
#undef DEFINE_ACTOR
#undef DEFINE_ACTOR_INTERNAL
#undef DEFINE_ACTOR_UNSET

#define DEFINE_ACTOR(name, enumValue, allocType, _3) \
    [enumValue] = { ROM_FILE_UNSET, (void*)NATIVE_OVL_BASE, NULL, NULL, &name##_Profile, NULL, allocType, 0 },
#define DEFINE_ACTOR_INTERNAL(name, enumValue, allocType, _3) \
    [enumValue] = { ROM_FILE_UNSET, NULL, NULL, NULL, &name##_Profile, NULL, allocType, 0 },
#define DEFINE_ACTOR_UNSET(enumValue) [enumValue] = { 0 },

ActorOverlay gActorOverlayTable[] = {
#include "tables/actor_table.h"
};

#undef DEFINE_ACTOR
#undef DEFINE_ACTOR_INTERNAL
#undef DEFINE_ACTOR_UNSET

s32 gMaxActorId = 0;

void ActorOverlayTable_LogPrint(void) {
}

void EffectSsTable_InitNative(void);

void ActorOverlayTable_Init(void) {
    gMaxActorId = ACTOR_ID_MAX;
    EffectSsTable_InitNative();
    for (s32 i = 0; i < ACTOR_ID_MAX; i++) {
        if (gActorOverlayTable[i].vramStart != NULL) {
            gActorOverlayTable[i].vramEnd = (void*)(NATIVE_OVL_BASE + sOverlaySizes[i]);
        }
    }
}

void ActorOverlayTable_Cleanup(void) {
    gMaxActorId = 0;
}
