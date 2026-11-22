// engine/actor_native.c (rewritten: native)
#include "engine/actor_native.h"
#include "engine/log.h"

static u32 sLogged[0x200 / 32];

void ActorNative_LogUnported(s32 actorId) {
    if (actorId < 0 || actorId >= 0x200) {
        return;
    }
    if (!(sLogged[actorId / 32] & (1u << (actorId % 32)))) {
        sLogged[actorId / 32] |= 1u << (actorId % 32);
        log_printf("[ACTOR] id 0x%03lx not ported: spawn skipped\n", (unsigned long)actorId);
    }
}

void ActorNative_LogUnportedEffect(s32 type) {
    static u64 logged;
    if (type >= 0 && type < 64 && !(logged & (1ull << type))) {
        logged |= 1ull << type;
        log_printf("[EFFECT] type %ld not ported: spawn skipped\n", (long)type);
    }
}
