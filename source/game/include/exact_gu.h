// exact_gu.h (adapted numeric code, see game/math/exact_gu.c)
#ifndef EXACT_GU_H
#define EXACT_GU_H
#include "game_types.h"
void ExactGuPerspectiveF(f32 mf[4][4], u16* perspNorm, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale);
void ExactGuLookAtF(f32 mf[4][4], f32 xEye, f32 yEye, f32 zEye, f32 xAt, f32 yAt, f32 zAt, f32 xUp, f32 yUp,
                    f32 zUp);
void ExactQuantizeMtxF(f32 mf[4][4]);
#endif
