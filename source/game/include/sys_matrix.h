// adapted from flute-of-age/include/sys_matrix.h (ultra64.h -> game_types.h)
#ifndef SYS_MATRIX_H
#define SYS_MATRIX_H

#include "z_math.h"

struct GraphicsContext;
struct GameState;

typedef enum MatrixMode {
    /* 0 */ MTXMODE_NEW,  // generates a new matrix
    /* 1 */ MTXMODE_APPLY // applies transformation to the current matrix
} MatrixMode;

extern MtxF gIdentityMtxF;

/* Stack operations */

void Matrix_Init(struct GameState* gameState);
void Matrix_Push(void);
void Matrix_Pop(void);
void Matrix_Get(MtxF* dest);
void Matrix_Put(MtxF* src);

/* Basic operations */

void Matrix_Mult(MtxF* mf, u8 mode);
void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode);
void Matrix_Scale(f32 x, f32 y, f32 z, u8 mode);
void Matrix_RotateX(f32 x, u8 mode);
void Matrix_RotateY(f32 y, u8 mode);
void Matrix_RotateZ(f32 z, u8 mode);

/* Compound operations */

void Matrix_RotateZYX(s16 x, s16 y, s16 z, u8 mode);
void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation);
void Matrix_SetTranslateRotateYXZ(f32 translateX, f32 translateY, f32 translateZ, Vec3s* rot);

/* Conversion operations */

// Game-side s15.16 matrix: reproduces the original float->fixed->float quantization where gameplay
// reads it back (view/projection -> viewProjectionMtxF). Not a platform type: plain integers.
// The original's display-list matrix finalizers are not ported; draw code calls the native
// Gfx3D_LoadMtxF(Matrix_GetCurrent()) instead (engine/gfx3d.h).
typedef struct MtxQ {
    s32 q[4][4]; // q[i][j] == (s32)(mf[i][j] * 0x10000)
} MtxQ;

MtxQ* Matrix_MtxFToMtxQ(MtxF* src, MtxQ* dest);
void Matrix_MtxQToMtxF(MtxQ* src, MtxF* dest);
MtxF* Matrix_GetCurrent(void);

#define MATRIX_CHECK_FLOATS(mtx, file, line) (mtx)

/* Vector operations */

void Matrix_MultVec3f(Vec3f* src, Vec3f* dest);
void Matrix_MultVec3fExt(Vec3f* src, Vec3f* dest, MtxF* mf);

/* Copy and another conversion */

void Matrix_MtxFCopy(MtxF* dest, MtxF* src);

/* Miscellaneous */

void Matrix_Transpose(MtxF* mf);
void Matrix_ReplaceRotation(MtxF* mf);
void Matrix_MtxFToYXZRotS(MtxF* mf, Vec3s* rotDest, s32 flag);
void Matrix_MtxFToZYXRotS(MtxF* mf, Vec3s* rotDest, s32 flag);
void Matrix_RotateAxis(f32 angle, Vec3f* axis, u8 mode);

#endif
