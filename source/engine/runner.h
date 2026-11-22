// (rewritten: native)
#pragma once
#include "engine/types.h"
void Runner_Run(void) __attribute__((noreturn));
u32 Runner_VBlankCount(void); // retraces applied to the game since boot
// Native osGetTime equivalent (CPU count at 46.875 MHz, derived from the applied retrace count so it
// is deterministic under replay). Only the low 32 bits are used by game code (RNG seeding).
u64 Runner_OsTime(void);
void Runner_MarkFault(void);
