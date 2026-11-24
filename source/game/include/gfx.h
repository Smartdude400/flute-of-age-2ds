// gfx.h (rewritten: native)
// The original header defined the display-list graphics context and GBI helpers. In the DS port,
// game code only needs the logical screen size (all original 2D coordinates are in this
// 320x240 space; the renderer maps them to 256x192) and the opaque renderer context.
#ifndef GFX_H
#define GFX_H

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

struct GraphicsContext;
typedef struct GraphicsContext GraphicsContext;

// Frame memory (was Graph_Alloc: carved from the opaque display buffer, valid until the frame is
// drawn). Native: the renderer's frame pool, reset after the frame's commands are replayed.
#include <stddef.h>
void* Gfx3D_FrameAlloc(size_t size);
#define GRAPH_ALLOC(gfxCtx, size) Gfx3D_FrameAlloc(size)

#endif
