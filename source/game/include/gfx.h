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

#endif
