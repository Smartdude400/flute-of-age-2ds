// exact_math.h (adapted from flute-of-age/src/libultra/gu + src/libc64/fp.s numeric behaviour)
// exact_math.h (adapted numeric code)
// Bit-exact reproductions of the original game's trigonometric helpers. Gameplay results
// (movement, camera, collision) depend on these exact polynomial/table results, which differ from
// newlib's correctly-rounded libm. Call sites in adapted game code are renamed
// sinf->ExactSinF, cosf->ExactCosF, sins->ExactSinS, coss->ExactCosS (tools/import_source.py).
#ifndef EXACT_MATH_H
#define EXACT_MATH_H
#include "game_types.h"
f32 ExactSinF(f32 angle);
f32 ExactCosF(f32 angle);
s16 ExactSinS(u16 angle);
s16 ExactCosS(u16 angle);
// libc64 fp.s rounding helpers (pass through s32 like the original)
f32 ExactFloorF(f32 x); f64 ExactFloor(f64 x); f32 ExactCeilF(f32 x); f64 ExactCeil(f64 x);
f32 ExactTruncF(f32 x); f64 ExactTrunc(f64 x); f32 ExactNearbyIntF(f32 x); f64 ExactNearbyInt(f64 x);
f32 ExactRoundF(f32 x); f64 ExactRound(f64 x);
s32 ExactLFloorF(f32 x); s32 ExactLCeilF(f32 x); s32 ExactLTruncF(f32 x); s32 ExactLNearbyIntF(f32 x);
s32 ExactLRoundF(f32 x);
#endif
