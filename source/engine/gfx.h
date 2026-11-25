// (rewritten: native)
// engine/gfx.h - native renderer context passed to gamestates as gameState->gfxCtx.
#pragma once
#include "engine/types.h"

struct GraphicsContext {
    u32 frameCount; // game frames submitted to the GPU
    // Called once per frame after the frame is submitted (original Graph_Update called it after the
    // graphics task completed; z_kankyo uses it for the sun depth test and light glow check).
    void (*callback)(struct GraphicsContext*, void*);
    void* callbackParam;
    void* curFrameBuffer; // the original's framebuffer being drawn; native captures use the GPU target (NULL)
};
