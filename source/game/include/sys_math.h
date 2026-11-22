// adapted from flute-of-age/include/sys_math.h (ultra64.h -> game_types.h)
#ifndef SYS_MATH_H
#define SYS_MATH_H

#include "game_types.h"

f32 Math_FactorialF(f32 n);
f32 Math_Factorial(s32 n);
f32 Math_PowF(f32 base, s32 exp);
f32 Math_SinF(f32 angle);
f32 Math_CosF(f32 angle);
s16 Math_Atan2S(f32 x, f32 y);
f32 Math_Atan2F(f32 x, f32 y);

#endif
