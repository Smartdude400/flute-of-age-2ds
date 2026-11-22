// adapted from flute-of-age/src/libultra/gu/{perspective.c,lookat.c,mtxutil.c}
// Exact reproductions of the perspective / look-at matrix construction. Gameplay reads the
// resulting (s15.16-quantized) view and projection back (actor culling, screen positions), so the
// float math must match bit for bit. Same operations, same order, same double/float promotions.
// The platform matrix output is replaced by the game-side MtxF/MtxQ types (sys_matrix.h).
#include "exact_math.h"
#include "exact_gu.h"
#include "z_math.h"

#define GU_PI 3.1415926

static void IdentF(f32 mf[4][4]) {
    s32 i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (i == j) {
                mf[i][j] = 1.0;
            } else {
                mf[i][j] = 0.0;
            }
        }
    }
}

void ExactGuPerspectiveF(f32 mf[4][4], u16* perspNorm, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale) {
    f32 yscale;
    s32 row;
    s32 col;

    IdentF(mf);
    fovy *= GU_PI / 180.0;
    yscale = ExactCosF(fovy / 2) / ExactSinF(fovy / 2);
    mf[0][0] = yscale / aspect;
    mf[1][1] = yscale;
    mf[2][2] = (near + far) / (near - far);
    mf[2][3] = -1;
    mf[3][2] = 2 * near * far / (near - far);
    mf[3][3] = 0.0f;
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            mf[row][col] *= scale;
        }
    }
    if (perspNorm != NULL) {
        if (near + far <= 2.0) {
            *perspNorm = 65535;
        } else {
            *perspNorm = (f64)(1 << 17) / (near + far);
            if (*perspNorm <= 0) {
                *perspNorm = 1;
            }
        }
    }
}

void ExactGuLookAtF(f32 mf[4][4], f32 xEye, f32 yEye, f32 zEye, f32 xAt, f32 yAt, f32 zAt, f32 xUp, f32 yUp,
                    f32 zUp) {
    f32 length;
    f32 xLook;
    f32 yLook;
    f32 zLook;
    f32 xRight;
    f32 yRight;
    f32 zRight;

    IdentF(mf);
    xLook = xAt - xEye;
    yLook = yAt - yEye;
    zLook = zAt - zEye;
    length = -1.0 / sqrtf(SQ(xLook) + SQ(yLook) + SQ(zLook));
    xLook *= length;
    yLook *= length;
    zLook *= length;
    xRight = yUp * zLook - zUp * yLook;
    yRight = zUp * xLook - xUp * zLook;
    zRight = xUp * yLook - yUp * xLook;
    length = 1.0 / sqrtf(SQ(xRight) + SQ(yRight) + SQ(zRight));
    xRight *= length;
    yRight *= length;
    zRight *= length;
    xUp = yLook * zRight - zLook * yRight;
    yUp = zLook * xRight - xLook * zRight;
    zUp = xLook * yRight - yLook * xRight;
    length = 1.0 / sqrtf(SQ(xUp) + SQ(yUp) + SQ(zUp));
    xUp *= length;
    yUp *= length;
    zUp *= length;
    mf[0][0] = xRight;
    mf[1][0] = yRight;
    mf[2][0] = zRight;
    mf[3][0] = -(xEye * xRight + yEye * yRight + zEye * zRight);
    mf[0][1] = xUp;
    mf[1][1] = yUp;
    mf[2][1] = zUp;
    mf[3][1] = -(xEye * xUp + yEye * yUp + zEye * zUp);
    mf[0][2] = xLook;
    mf[1][2] = yLook;
    mf[2][2] = zLook;
    mf[3][2] = -(xEye * xLook + yEye * yLook + zEye * zLook);
    mf[0][3] = 0;
    mf[1][3] = 0;
    mf[2][3] = 0;
    mf[3][3] = 1;
}

// Float -> s15.16 -> float round trip (FTOFIX32 then FIX32TOF), element-wise, as the original
// matrix pipeline did between guPerspective/guLookAt and the later read-back.
void ExactQuantizeMtxF(f32 mf[4][4]) {
    for (s32 i = 0; i < 4; i++) {
        for (s32 j = 0; j < 4; j++) {
            s32 q = (s32)(mf[i][j] * (f32)0x00010000);
            mf[i][j] = (f32)q * (1.0f / (f32)0x00010000);
        }
    }
}
