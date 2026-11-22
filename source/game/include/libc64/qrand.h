// adapted from flute-of-age/include/libc64/qrand.h (ultra64.h -> game_types.h)
#ifndef LIBC64_QRAND_H
#define LIBC64_QRAND_H

#include "game_types.h"

u32 Rand_Next(void);
void Rand_Seed(u32 seed);
f32 Rand_ZeroOne(void);
void Rand_Seed_Variable(u32* rndNum, u32 seed);
u32 Rand_Next_Variable(u32* rndNum);
f32 Rand_ZeroOne_Variable(u32* rndNum);

#if !PLATFORM_N64
f32 Rand_Centered(void);
f32 Rand_Centered_Variable(u32* rndNum);
#endif

#endif
