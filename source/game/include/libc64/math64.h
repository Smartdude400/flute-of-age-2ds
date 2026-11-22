// adapted from flute-of-age/include/libc64/math64.h (ultra64.h -> game_types.h)
#ifndef LIBC64_MATH64_H
#define LIBC64_MATH64_H

#include "game_types.h"

f32 Math_FTanF(f32 angle);
f32 Math_FFloorF(f32 x);
f32 Math_FCeilF(f32 x);
f32 Math_FRoundF(f32 x);
f32 Math_FNearbyIntF(f32 x);
f32 Math_FTruncF(f32 x);
f32 Math_FAtanF(f32 x);
f32 Math_FAtan2F(f32 y, f32 x);
f32 Math_FAsinF(f32 x);
f32 Math_FAcosF(f32 x);

#endif
