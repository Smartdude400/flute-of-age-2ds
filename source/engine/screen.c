// engine/screen.c (rewritten: native) - see screen.h
#include <3ds.h>
#include <citro3d.h>
#include "engine/screen.h"

#define DISPLAY_TRANSFER_FLAGS                                                                     \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |             \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static C3D_RenderTarget* sTop;
static C3D_RenderTarget* sBottom;
static int sCur = SCREEN_TOP;

void Screen_Init(void) {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 4);
    // The framebuffers are portrait (240 wide, 400 / 320 tall): the LCDs are mounted rotated.
    sTop = C3D_RenderTargetCreate(SCREEN_FB_H, SCREEN_FB_W, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(sTop, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    // The bottom screen is 320 x 240, the size of the original frame: the menu and HUD are drawn there 1:1.
    sBottom = C3D_RenderTargetCreate(SCREEN_GAME_H, SCREEN_GAME_W, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(sBottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
}

void Screen_SetGameViewport(void) {
    // viewport in framebuffer space: x along the 240 axis, y along the 400 (320) axis
    if (sCur == SCREEN_BOTTOM) {
        C3D_SetViewport(0, 0, SCREEN_GAME_H, SCREEN_GAME_W);
    } else {
        C3D_SetViewport(0, (SCREEN_FB_W - SCREEN_GAME_W) / 2, SCREEN_FB_H, SCREEN_GAME_W);
    }
}

void Screen_Select(int screen) {
    sCur = screen;
    C3D_FrameDrawOn(screen == SCREEN_BOTTOM ? sBottom : sTop);
    Screen_SetGameViewport();
}

int Screen_Current(void) {
    return sCur;
}

void Screen_DrawOnScreen(void) {
    Screen_Select(sCur);
}

void Screen_BeginFrame(void) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(sTop, C3D_CLEAR_ALL, 0x000000FF, 0);
    C3D_RenderTargetClear(sBottom, C3D_CLEAR_ALL, 0x000000FF, 0);
    Screen_Select(SCREEN_TOP);
}

void Screen_EndFrame(void) {
    C3D_FrameEnd(0);
}

// ---- frame capture (pause background). The display-transfer unit converts the framebuffer (tiled) into
// linear pixels after the GPU work submitted so far; the next frame (the GPU is idle again by then) the
// game-area pixels are swizzled into a texture in the game's orientation: u along the 320-pixel logical x,
// v along the 240-pixel logical y (512 x 256 texture, first stored row = logical y 0).
static u32* sCaptureLinear;
static C3D_Tex sCapture;
static bool sCaptureInit, sCapturePending;

void Screen_CaptureFrame(void) {
    if (sCaptureLinear == NULL) {
        sCaptureLinear = linearAlloc(SCREEN_FB_W * SCREEN_FB_H * 4);
        if (sCaptureLinear == NULL) {
            return;
        }
    }
    C3D_FrameSplit(0); // submit what is drawn so far
    GX_DisplayTransfer((u32*)sTop->frameBuf.colorBuf, GX_BUFFER_DIM(SCREEN_FB_H, SCREEN_FB_W), sCaptureLinear,
                       GX_BUFFER_DIM(SCREEN_FB_H, SCREEN_FB_W),
                       GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
                           GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
                           GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    sCapturePending = true;
}

const C3D_Tex* Screen_CaptureTexture(void) {
    if (sCapturePending) {
        sCapturePending = false;
        if (!sCaptureInit) {
            if (!C3D_TexInit(&sCapture, 512, 256, GPU_RGBA8)) {
                return NULL;
            }
            sCaptureInit = true;
        }
        GSPGPU_InvalidateDataCache(sCaptureLinear, SCREEN_FB_W * SCREEN_FB_H * 4);
        u32* dst = (u32*)sCapture.data;
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 512; x++) {
                // logical (x, y) -> linear framebuffer pixel: rows along the 400 axis, 240 pixels per row
                u32 c = 0xFF;
                if (x < SCREEN_GAME_W && y < SCREEN_GAME_H) {
                    int fy = (SCREEN_FB_W - SCREEN_GAME_W) / 2 + x;
                    int fx = SCREEN_FB_H - 1 - y;
                    c = sCaptureLinear[fy * SCREEN_FB_H + fx];
                }
                u32 m = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
                dst[((y >> 3) * (512 >> 3) + (x >> 3)) * 64 + m] = c;
            }
        }
        C3D_TexFlush(&sCapture);
    }
    return sCaptureInit ? &sCapture : NULL;
}
