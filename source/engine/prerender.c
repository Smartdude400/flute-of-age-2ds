// engine/prerender.c (rewritten: native)
// Replaces src/code/prerender.c. The original captured the framebuffer/coverage into RAM through
// display lists and ran filter passes on the CPU. Here only the bookkeeping fields are kept (game
// code reads width/height), and the capture/redraw of the pause background are native presentation
// entry points (P-series stubs until the DS display-capture path is written).
#include <string.h>
#include "prerender.h"
#include "engine/stub.h"
#include "engine/gfx3d.h"
#include "engine/screen.h"

void PreRender_SetValuesSave(PreRender* this, u32 width, u32 height, void* fbuf, void* zbuf, void* cvg) {
    this->widthSave = width;
    this->heightSave = height;
    this->fbufSave = fbuf;
    this->cvgSave = cvg;
    this->zbufSave = zbuf;
    this->ulxSave = 0;
    this->ulySave = 0;
    this->lrxSave = width - 1;
    this->lrySave = height - 1;
}

void PreRender_Init(PreRender* this) {
    memset(this, 0, sizeof(PreRender));
}

void PreRender_SetValues(PreRender* this, u32 width, u32 height, void* fbuf, void* zbuf) {
    this->width = width;
    this->height = height;
    this->fbuf = fbuf;
    this->zbuf = zbuf;
    this->ulx = 0;
    this->uly = 0;
    this->lrx = width - 1;
    this->lry = height - 1;
}

void PreRender_Destroy(PreRender* this) {
    (void)this;
}

// The original ran the VI's coverage anti-aliasing and divot filters over the saved N64 framebuffer on the
// CPU; they repair N64 coverage artifacts that the captured PICA200 image does not have.
void PreRender_ApplyFilters(PreRender* this) {
    (void)this;
}

// The original copied the framebuffer at this point of the display list (called from the overlay list, i.e.
// after the opaque and translucent world) and later drew the copy instead of the world while paused.
static void x_saveframe(const void* p) {
    (void)p;
    Screen_CaptureFrame();
}

void PreRender_SaveFrame(PreRender* this) {
    (void)this;
    Gfx3D_Record(GFX_BUF_OVL, x_saveframe, 4);
}

void Gfx2D_DrawCapture(const C3D_Tex* tex);
static void x_drawsaved(const void* p) {
    (void)p;
    const C3D_Tex* t = Screen_CaptureTexture();
    if (t != NULL) {
        Gfx2D_DrawCapture(t);
    }
}

void PreRender_DrawSaved(PreRender* this) {
    (void)this;
    Gfx3D_Record(GFX_BUF_OPA, x_drawsaved, 4);
}

// The original copied the colour image fbuf (where the last offscreen draw went, e.g. the pause menu's
// player preview) into fbufSave at this point of the list; later texture loads from fbufSave sample the copy.
void PreRender_SaveFramebuffer(PreRender* this, struct Gfx** gfxP) {
    (void)gfxP;
    Gfx3D_OffscreenSave(this->fbufSave, this->widthSave, this->heightSave);
}

// The original wrote the image's coverage values into cvgSave, read only by PreRender_ApplyFilters' coverage
// anti-aliasing (see there).
void PreRender_DrawCoverage(PreRender* this, struct Gfx** gfxP) {
    (void)this;
    (void)gfxP;
}
