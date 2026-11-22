// engine/screen.c (rewritten: native) - see screen.h
#include <3ds.h>
#include <citro3d.h>
#include "engine/screen.h"

#define DISPLAY_TRANSFER_FLAGS                                                                     \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |             \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static C3D_RenderTarget* sTop;

void Screen_Init(void) {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 4);
    // The framebuffer is portrait (240 wide, 400 tall): the top LCD is mounted rotated.
    sTop = C3D_RenderTargetCreate(SCREEN_FB_H, SCREEN_FB_W, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(sTop, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
}

void Screen_SetGameViewport(void) {
    // viewport in framebuffer space: x along the 240 axis, y along the 400 axis
    C3D_SetViewport(0, (SCREEN_FB_W - SCREEN_GAME_W) / 2, SCREEN_FB_H, SCREEN_GAME_W);
}

void Screen_BeginFrame(void) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(sTop, C3D_CLEAR_ALL, 0x000000FF, 0);
    C3D_FrameDrawOn(sTop);
    Screen_SetGameViewport();
}

void Screen_EndFrame(void) {
    C3D_FrameEnd(0);
}
