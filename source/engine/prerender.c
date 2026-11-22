// engine/prerender.c (rewritten: native)
// Replaces src/code/prerender.c. The original captured the framebuffer/coverage into RAM through
// display lists and ran filter passes on the CPU. Here only the bookkeeping fields are kept (game
// code reads width/height), and the capture/redraw of the pause background are native presentation
// entry points (P-series stubs until the DS display-capture path is written).
#include <string.h>
#include "prerender.h"
#include "engine/stub.h"

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

void PreRender_ApplyFilters(PreRender* this) {
    (void)this;
    PRESENTATION_STUB("PreRender_ApplyFilters");
}

void PreRender_SaveFrame(PreRender* this) {
    (void)this;
    PRESENTATION_STUB("PreRender_SaveFrame");
}

void PreRender_DrawSaved(PreRender* this) {
    (void)this;
    PRESENTATION_STUB("PreRender_DrawSaved");
}
