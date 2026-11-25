// engine/sleep_native.c (rewritten: native) - the game's sleep helpers (original: src/code/sleep.c), blocking
// the game thread for the same time as on the original.
#include <3ds.h>
#include "engine/types.h"

void Sleep_Usec(u32 usec) {
    svcSleepThread((s64)usec * 1000);
}

void Sleep_Msec(u32 ms) {
    svcSleepThread((s64)ms * 1000000);
}
