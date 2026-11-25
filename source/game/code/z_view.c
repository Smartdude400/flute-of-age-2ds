// adapted from flute-of-age/src/code/z_view.c
#include "exact_math.h"
#include "view.h"
#include "engine/gfx3d.h"
#include "exact_gu.h"
#include "engine/gfx2d.h"

#include "avoid_ub.h"
#include "gfx.h"
#include "letterbox.h"
#include "main.h"
#include "printf.h"
#include "regs.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "translation.h"
#include "z_math.h"

#include "libc64/malloc.h"
#include "libu64/debug.h"
#include "game_types.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

vu32 sLogOnNextViewInit = true;

s32 View_ApplyPerspective(View*);
s32 View_ApplyOrtho(View*);

View* View_New(GraphicsContext* gfxCtx) {
    View* view = SYSTEM_ARENA_MALLOC(sizeof(View), "../z_view.c", 285);

    if (view != NULL) {
        memset(view, 0, sizeof(View));
        View_Init(view, gfxCtx);
    }

    return view;
}

void View_Free(View* view) {
    SYSTEM_ARENA_FREE(view, "../z_view.c", 297);
}

void View_Init(View* view, GraphicsContext* gfxCtx) {
    view->gfxCtx = gfxCtx;

    view->viewport.topY = 0;
    view->viewport.bottomY = SCREEN_HEIGHT;
    view->viewport.leftX = 0;
    view->viewport.rightX = SCREEN_WIDTH;

    view->magic = 0x56494557; // "VIEW"

    if (sLogOnNextViewInit == false) {}

    view->scale = 1.0f;
    view->fovy = 60.0f;
    view->zNear = 10.0f;
    view->zFar = 12800.0f;

    view->eye.x = 0.0f;
    view->eye.y = 0.0f;
    view->eye.z = -1.0f;
    view->at.x = 0.0f;
    view->up.x = 0.0f;
    view->up.y = 1.0f;
    view->up.z = 0.0f;

    if (sLogOnNextViewInit) {
        PRINTF("\nview: initialize ---\n");
        sLogOnNextViewInit = false;
    }

    view->unk_124 = 0;
    view->flags = VIEW_VIEWING | VIEW_VIEWPORT | VIEW_PROJECTION_PERSPECTIVE;
    View_InitDistortion(view);
}

void View_LookAt(View* view, Vec3f* eye, Vec3f* at, Vec3f* up) {
    if (eye->x == at->x && eye->z == at->z) {
        eye->x += 0.1f;
    }

    view->eye = *eye;
    view->at = *at;
    view->up = *up;
    view->flags |= VIEW_VIEWING;
}

/*
 * Unused. View_LookAt is always used instead. This version is similar but
 * is missing the input sanitization and the update to the flags.
 */
void View_LookAtUnsafe(View* view, Vec3f* eye, Vec3f* at, Vec3f* up) {
    view->eye = *eye;
    view->at = *at;
    view->up = *up;
}

void View_SetScale(View* view, f32 scale) {
    view->flags |= VIEW_PROJECTION_PERSPECTIVE;
    view->scale = scale;
}

void View_GetScale(View* view, f32* scale) {
    *scale = view->scale;
}

void View_SetPerspective(View* view, f32 fovy, f32 zNear, f32 zFar) {
    view->fovy = fovy;
    view->zNear = zNear;
    view->zFar = zFar;
    view->flags |= VIEW_PROJECTION_PERSPECTIVE;
}

void View_GetPerspective(View* view, f32* fovy, f32* zNear, f32* zFar) {
    *fovy = view->fovy;
    *zNear = view->zNear;
    *zFar = view->zFar;
}

void View_SetOrtho(View* view, f32 fovy, f32 zNear, f32 zFar) {
    view->fovy = fovy;
    view->zNear = zNear;
    view->zFar = zFar;
    view->flags |= VIEW_PROJECTION_ORTHO;
    view->scale = 1.0f;
}

/*
 * Identical to View_GetPerspective, and never called.
 * Named as it seems to fit the "set, get" pattern.
 */
void View_GetOrtho(View* view, f32* fovy, f32* zNear, f32* zFar) {
    *fovy = view->fovy;
    *zNear = view->zNear;
    *zFar = view->zFar;
}

void View_SetViewport(View* view, Viewport* viewport) {
    view->viewport = *viewport;
    view->flags |= VIEW_VIEWPORT;
}

void View_GetViewport(View* view, Viewport* viewport) {
    *viewport = view->viewport;
}

void View_SetDistortionOrientation(View* view, f32 rotX, f32 rotY, f32 rotZ) {
    view->distortionOrientation.x = rotX;
    view->distortionOrientation.y = rotY;
    view->distortionOrientation.z = rotZ;
}

void View_SetDistortionScale(View* view, f32 scaleX, f32 scaleY, f32 scaleZ) {
    view->distortionScale.x = scaleX;
    view->distortionScale.y = scaleY;
    view->distortionScale.z = scaleZ;
}

BAD_RETURN(s32) View_SetDistortionSpeed(View* view, f32 speed) {
    view->distortionSpeed = speed;
}

void View_InitDistortion(View* view) {
    view->distortionOrientation.x = 0.0f;
    view->distortionOrientation.y = 0.0f;
    view->distortionOrientation.z = 0.0f;
    view->distortionScale.x = 1.0f;
    view->distortionScale.y = 1.0f;
    view->distortionScale.z = 1.0f;
    view->curDistortionOrientation = view->distortionOrientation;
    view->curDistortionScale = view->distortionScale;
    view->distortionSpeed = 0.0f;
}

void View_ClearDistortion(View* view) {
    view->distortionOrientation.x = 0.0f;
    view->distortionOrientation.y = 0.0f;
    view->distortionOrientation.z = 0.0f;
    view->distortionScale.x = 1.0f;
    view->distortionScale.y = 1.0f;
    view->distortionScale.z = 1.0f;
    view->distortionSpeed = 1.0f;
}

void View_SetDistortion(View* view, Vec3f orientation, Vec3f scale, f32 speed) {
    view->distortionOrientation = orientation;
    view->distortionScale = scale;
    view->distortionSpeed = speed;
}

/**
 * Apply view to POLY_OPA_DISP, POLY_XLU_DISP (and OVERLAY_DISP if ortho)
 */
s32 View_Apply(View* view, s32 mask) {
    mask = (view->flags & mask) | (mask >> 4);

    if (mask & VIEW_PROJECTION_ORTHO) {
        return View_ApplyOrtho(view);
    } else {
        return View_ApplyPerspective(view);
    }
}

/**
 * Apply scissor, viewport and projection (ortho) to OVERLAY_DISP.
 */
/**
 * Apply scissor, viewport, view and projection (perspective) to OVERLAY_DISP.
 */
/**
 * Just updates view's view matrix from its eye/at/up vectors. Opens disps but doesn't use them.
 */
#if DEBUG_FEATURES
/**
 * Logs an error and returns nonzero if camera is too far from the origin.
 */
s32 View_ErrorCheckEyePosition(f32 eyeX, f32 eyeY, f32 eyeZ) {
    s32 error = 0;

    if (SQ(eyeX) + SQ(eyeY) + SQ(eyeZ) > SQ(32767.0f)) {
        error = 3;
    } else {
        f32 absEyeX = ABS(eyeX);
        f32 absEyeY = ABS(eyeY);
        f32 absEyeZ = ABS(eyeZ);

        if (((18900.0f < absEyeX) || (18900.0f < absEyeY)) || (18900.0f < absEyeZ)) {
            error = 2;
        } else if (((16000.0f < absEyeX) || (16000.0f < absEyeY)) || (16000.0f < absEyeZ)) {
            error = 1;
        }
    }

    if (error != 0) {
        PRINTF_COLOR_RED();
        PRINTF(T("eye が大きすぎます eye=[%8.3f %8.3f %8.3f] error=%d\n",
                 "eye is too large eye=[%8.3f %8.3f %8.3f] error=%d\n"),
               eyeX, eyeY, eyeZ, error);
        PRINTF_RST();
    }

    return error;
}
#endif

// ===== native rewrites of the view application functions =========================================
// Matrix construction keeps the original math exactly (ExactGuPerspectiveF / ExactGuLookAtF) and the
// float->s15.16->float quantization the original pipeline applied, because gameplay reads these
// matrices back (viewProjectionMtxF in Play_Draw, billboards, Play_GetScreenPos). The resulting
// float matrices are stored in view->projection / view->viewing and handed to the DS geometry engine.
// Viewport/scissor: the DS always renders the full 256x192 frame; letterboxing is drawn by the
// native 2D layer (presentation only).

s32 View_StepDistortion(View* view, MtxF* projectionMtx) {
    if (view->distortionSpeed == 0.0f) {
        return false;
    } else if (view->distortionSpeed == 1.0f) {
        view->curDistortionOrientation = view->distortionOrientation;
        view->curDistortionScale = view->distortionScale;
        view->distortionSpeed = 0.0f;
    } else {
        view->curDistortionOrientation.x =
            F32_LERPIMP(view->curDistortionOrientation.x, view->distortionOrientation.x, view->distortionSpeed);
        view->curDistortionOrientation.y =
            F32_LERPIMP(view->curDistortionOrientation.y, view->distortionOrientation.y, view->distortionSpeed);
        view->curDistortionOrientation.z =
            F32_LERPIMP(view->curDistortionOrientation.z, view->distortionOrientation.z, view->distortionSpeed);
        view->curDistortionScale.x =
            F32_LERPIMP(view->curDistortionScale.x, view->distortionScale.x, view->distortionSpeed);
        view->curDistortionScale.y =
            F32_LERPIMP(view->curDistortionScale.y, view->distortionScale.y, view->distortionSpeed);
        view->curDistortionScale.z =
            F32_LERPIMP(view->curDistortionScale.z, view->distortionScale.z, view->distortionSpeed);
    }

    Matrix_Put(projectionMtx);
    Matrix_RotateX(view->curDistortionOrientation.x, MTXMODE_APPLY);
    Matrix_RotateY(view->curDistortionOrientation.y, MTXMODE_APPLY);
    Matrix_RotateZ(view->curDistortionOrientation.z, MTXMODE_APPLY);
    Matrix_Scale(view->curDistortionScale.x, view->curDistortionScale.y, view->curDistortionScale.z, MTXMODE_APPLY);
    Matrix_RotateZ(-view->curDistortionOrientation.z, MTXMODE_APPLY);
    Matrix_RotateY(-view->curDistortionOrientation.y, MTXMODE_APPLY);
    Matrix_RotateX(-view->curDistortionOrientation.x, MTXMODE_APPLY);
    Matrix_Get(projectionMtx);
    ExactQuantizeMtxF(projectionMtx->mf); // MATRIX_TO_MTX(projectionMtx)
    return true;
}

static void View_BuildPerspective(View* view) {
    s32 width = view->viewport.rightX - view->viewport.leftX;
    s32 height = view->viewport.bottomY - view->viewport.topY;
    f32 aspect = (f32)width / (f32)height;

    ExactGuPerspectiveF(view->projection.mf, &view->normal, view->fovy, aspect, view->zNear, view->zFar, view->scale);
    ExactQuantizeMtxF(view->projection.mf);
    view->projectionPtr = &view->projection;
}

static void View_BuildViewing(View* view) {
    if (view->eye.x == view->at.x && view->eye.y == view->at.y && view->eye.z == view->at.z) {
        view->eye.x += 1.0f;
        view->eye.y += 1.0f;
        view->eye.z += 1.0f;
    }
    ExactGuLookAtF(view->viewing.mf, view->eye.x, view->eye.y, view->eye.z, view->at.x, view->at.y, view->at.z,
                   view->up.x, view->up.y, view->up.z);
    ExactQuantizeMtxF(view->viewing.mf);
    view->viewingPtr = &view->viewing;
}

s32 View_ApplyPerspective(View* view) {
    View_BuildPerspective(view);
    // The original distorts a copy that is drawn with; view->projection keeps the undistorted matrix.
    MtxF drawProjection = view->projection;
    View_StepDistortion(view, &drawProjection);
    View_BuildViewing(view);
    Gfx3D_SetCamera(&drawProjection, &view->viewing);
    return 1;
}

s32 View_ApplyPerspectiveToOverlay(View* view) {
    // (native) the original applied scissor, viewport, perspective and viewing to OVERLAY_DISP only
    View_BuildPerspective(view);
    View_BuildViewing(view);
    Gfx3D_SetOverlayCamera(&view->projection, &view->viewing, view->viewport.leftX, view->viewport.topY,
                           view->viewport.rightX, view->viewport.bottomY);
    return 1;
}

s32 View_UpdateViewingMatrix(View* view) {
    View_BuildViewing(view);
    return 1;
}

s32 View_ApplyOrtho(View* view) {
    // Orthographic views are only used for 2D/menu rendering, which the native 2D layer handles.
    Gfx2D_Begin3DOrtho();
    return 1;
}

s32 View_ApplyOrthoToOverlay(View* view) {
    // (native) the original applied scissor, viewport and an orthographic projection (guOrtho over the
    // screen, centred) to OVERLAY_DISP; the model matrix is loaded by the draws themselves
    f32 l = -(f32)SCREEN_WIDTH * 0.5f, r = (f32)SCREEN_WIDTH * 0.5f;
    f32 b = -(f32)SCREEN_HEIGHT * 0.5f, t = (f32)SCREEN_HEIGHT * 0.5f;
    f32 n = view->zNear, f = view->zFar;
    MtxF* mf = &view->projection;
    memset(mf, 0, sizeof(MtxF));
    mf->mf[0][0] = 2.0f / (r - l);
    mf->mf[1][1] = 2.0f / (t - b);
    mf->mf[2][2] = -2.0f / (f - n);
    mf->mf[3][0] = -(r + l) / (r - l);
    mf->mf[3][1] = -(t + b) / (t - b);
    mf->mf[3][2] = -(f + n) / (f - n);
    mf->mf[3][3] = 1.0f;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mf->mf[i][j] *= view->scale;
        }
    }
    ExactQuantizeMtxF(mf->mf);
    view->projectionPtr = &view->projection;
    Gfx3D_SetOverlayCamera(&view->projection, NULL, view->viewport.leftX, view->viewport.topY,
                           view->viewport.rightX, view->viewport.bottomY);
    return 1;
}
