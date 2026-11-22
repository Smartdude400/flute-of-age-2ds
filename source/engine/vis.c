// engine/vis.c (rewritten: native)
// Replaces z_viscvg.c / z_vismono.c / z_viszbuffer.c: full-screen framebuffer filters built from
// display lists (coverage, monochrome TLUT trick, z-buffer view). Init keeps the original field
// values; the filters themselves are presentation (P-series) on the DS.
#include <string.h>
#include "vis.h"
#include "engine/stub.h"

static void params_init(VisParams* p, u32 type) {
    p->type = type;
    p->setScissor = false;
    p->color1.r = 255;
    p->color1.g = 255;
    p->color1.b = 255;
    p->color1.a = 255;
}

void VisCvg_Init(VisCvg* this) {
    params_init(&this->params, VISCVG_TYPE_NOP);
}

void VisCvg_Destroy(VisCvg* this) {
    (void)this;
}

void VisCvg_Draw(VisCvg* this, DListRef* gfxP) {
    (void)this; (void)gfxP;
    PRESENTATION_STUB("VisCvg_Draw");
}

void VisMono_Init(VisMono* this) {
    memset(this, 0, sizeof(VisMono));
    params_init(&this->params, 0);
    this->params.color2.r = 0;
    this->params.color2.g = 0;
    this->params.color2.b = 0;
    this->params.color2.a = 0;
}

void VisMono_Destroy(VisMono* this) {
    (void)this; // the original freed its lazily built display list (never built natively)
}

void VisMono_Draw(VisMono* this, DListRef* gfxP) {
    (void)this; (void)gfxP;
    PRESENTATION_STUB("VisMono_Draw");
}

void VisZBuffer_Init(VisZBuffer* this) {
    params_init(&this->params, 0);
    this->params.color2.r = 0;
    this->params.color2.g = 0;
    this->params.color2.b = 0;
    this->params.color2.a = 255;
}

void VisZBuffer_Destroy(VisZBuffer* this) {
    (void)this;
}

void VisZBuffer_Draw(VisZBuffer* this, DListRef* gfxP) {
    (void)this; (void)gfxP;
    PRESENTATION_STUB("VisZBuffer_Draw");
}
