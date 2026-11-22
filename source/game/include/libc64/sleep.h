// adapted from flute-of-age/include/libc64/sleep.h (ultra64.h -> game_types.h)
#ifndef LIBC64_SLEEP_H
#define LIBC64_SLEEP_H

#include "game_types.h"

void Sleep_Cycles(u64 cycles);
void Sleep_Nsec(u32 nsec);
void Sleep_Usec(u32 usec);
void Sleep_Msec(u32 ms);
void Sleep_Sec(u32 sec);

#endif
