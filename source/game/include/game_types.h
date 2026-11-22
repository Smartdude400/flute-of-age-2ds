// game_types.h (rewritten: native)
// Scalar types used by the translated game code. Replaces the original platform umbrella
// header. It defines ONLY plain C types and a few language helpers, with no platform APIs.
// Integer names come from libctru (u8..s64, vu*), the 3DS-native spellings of the same widths.
// f32/f64 are IEEE binary32/binary64, computed by the ARM11 VFPv2 unit (see 3dsport/ARCHITECTURE.md).
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "engine/types.h"

typedef float f32;
typedef double f64;

typedef float MtxF_t[4][4];
typedef union MtxF {
    MtxF_t mf;
    struct {
        float xx, yx, zx, wx,
              xy, yy, zy, wy,
              xz, yz, zz, wz,
              xw, yw, zw, ww;
    };
} MtxF;

// from flute-of-age/include/libc/math.h (game constants)
#define SHT_MAX 32767.0f
#define SHT_MINV (1.0f / SHT_MAX)
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#undef M_SQRT2
#define M_SQRT2 1.41421356237309504880f
#undef MAXFLOAT
#define MAXFLOAT 3.40282347e+38f

// from flute-of-age/include/libc/assert.h: retail builds define NDEBUG, so ASSERT is a no-op
#define ASSERT(cond, msg, file, line) ((void)0)

// Display-list reference: an asset handle naming a converted mesh by its original segmented
// address (see asset_token.h). Replaces the platform display-list pointer type in game data.
typedef const void* DListRef;

// Generic bit-field helpers used by game data encodings (e.g. quest hint commands)
#define _SHIFTL(v, s, w) ((u32)(((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w) ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#include "versions.h"

#endif
