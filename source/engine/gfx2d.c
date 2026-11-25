// engine/gfx2d.c (rewritten: native)
// Screen-space quads through citro3d immediate mode: gfx2d.v.pica shader, ortho projection over the
// game's 320x240 space, one texture combiner stage (texture x vertex colour, or vertex colour).
// Calls are recorded into the renderer's buffers (engine/gfx3d.h) so they draw in the original order:
// the frame fill goes to the opaque buffer, text/rectangles/fills to the overlay buffer.
#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "engine/gfx2d.h"
#include "engine/gfx3d.h"
#include "engine/ptex.h"
#include "engine/screen.h"
#include "engine/log.h"
#include "gfx2d_shbin.h"

void Gfx3D_NotifyProgramChanged(void);

static DVLB_s* sDvlb;
static shaderProgram_s sProg;
static int sUProj;
static C3D_Tex sFont;
static bool sFontOk;
static C3D_Mtx sProj;
static C3D_AttrInfo sAttr;

bool Gfx2D_Init(void) {
    sDvlb = DVLB_ParseFile((u32*)gfx2d_shbin, gfx2d_shbin_size);
    shaderProgramInit(&sProg);
    shaderProgramSetVsh(&sProg, &sDvlb->DVLE[0]);
    sUProj = shaderInstanceGetUniformLocation(sProg.vertexShader, "projection");
    // y grows downwards like the original 2D coordinates
    Mtx_OrthoTilt(&sProj, 0.0f, SCREEN_GAME_W, SCREEN_GAME_H, 0.0f, -1.0f, 1.0f, true);
    AttrInfo_Init(&sAttr);
    AttrInfo_AddLoader(&sAttr, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&sAttr, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(&sAttr, 2, GPU_FLOAT, 4);
    sFontOk = PTex_Load("/font/gfxprint.ptex", &sFont);
    if (sFontOk) {
        C3D_TexSetFilter(&sFont, GPU_NEAREST, GPU_NEAREST);
    }
    return sFontOk;
}

static void setup(bool textured) {
    Gfx3D_NotifyProgramChanged();
    C3D_BindProgram(&sProg);
    Screen_SetGameViewport();
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, sUProj, &sProj);
    C3D_SetAttrInfo(&sAttr);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA);
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    if (textured) {
        C3D_TexBind(0, &sFont);
        C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
        C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
    } else {
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    }
    for (int i = 1; i < 6; i++) {
        C3D_TexEnvInit(C3D_GetTexEnv(i));
    }
}

static void vtx(float x, float y, float u, float v, const float c[4]) {
    C3D_ImmSendAttrib(x, y, 0.0f, 0.0f);
    C3D_ImmSendAttrib(u, v, 0.0f, 0.0f);
    C3D_ImmSendAttrib(c[0], c[1], c[2], c[3]);
}

static void quad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, const float c[4]) {
    vtx(x0, y0, u0, v0, c);
    vtx(x1, y0, u1, v0, c);
    vtx(x1, y1, u1, v1, c);
    vtx(x0, y0, u0, v0, c);
    vtx(x1, y1, u1, v1, c);
    vtx(x0, y1, u0, v1, c);
}

static void rgba(float out[4], const u8 c[4]) {
    for (int i = 0; i < 4; i++) {
        out[i] = c[i] / 255.0f;
    }
}

// ---- recorded commands
typedef struct { s16 x, y, w, h; u8 c[4]; } CmdRect;
typedef struct { s16 x, y; u8 c[4]; char text[64]; } CmdText;

static void x_rect(const void* p) {
    const CmdRect* r = p;
    float c[4];
    rgba(c, r->c);
    setup(false);
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    quad(r->x, r->y, r->x + r->w, r->y + r->h, 0, 0, 0, 0, c);
    C3D_ImmDrawEnd();
}

static void x_text(const void* p) {
    const CmdText* t = p;
    if (!sFontOk) {
        return;
    }
    float c[4];
    rgba(c, t->c);
    setup(true);
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    int x = t->x;
    for (const char* s = t->text; *s; s++, x += GFX_CHAR_X_SPACING) {
        u8 ch = (u8)*s;
        if (ch == ' ') {
            continue;
        }
        // glyph ch at cell (ch % 16, ch / 16) of the 128x128 sheet. The PICA samples t = 1 at the first
        // stored row (the image top, see tools/pica_tex.py), so t_gpu = 1 - t_image.
        float u = (ch % 16) * 8 / 128.0f, v = (ch / 16) * 8 / 128.0f;
        quad(x, t->y, x + 8, t->y + 8, u, 1.0f - v, u + 8 / 128.0f, 1.0f - (v + 8 / 128.0f), c);
    }
    C3D_ImmDrawEnd();
}

static void rec_rect(GfxBuf buf, int x, int y, int w, int h, u8 r, u8 g, u8 b, u8 a) {
    if (a == 0) {
        return;
    }
    CmdRect* c = Gfx3D_Record(buf, x_rect, sizeof(CmdRect));
    if (c) {
        c->x = x; c->y = y; c->w = w; c->h = h;
        c->c[0] = r; c->c[1] = g; c->c[2] = b; c->c[3] = a;
    }
}

void Gfx2D_Begin(u8 r, u8 g, u8 b) {
    rec_rect(GFX_BUF_OPA, 0, 0, SCREEN_GAME_W, SCREEN_GAME_H, r, g, b, 255); // original frame fill
}

void Gfx2D_Text(int x, int y, u8 r, u8 g, u8 b, u8 a, const char* text) {
    if (a == 0) {
        return;
    }
    CmdText* c = Gfx3D_Record(GFX_BUF_OVL, x_text, sizeof(CmdText));
    if (c) {
        c->x = x; c->y = y;
        c->c[0] = r; c->c[1] = g; c->c[2] = b; c->c[3] = a;
        strncpy(c->text, text, sizeof(c->text) - 1);
        c->text[sizeof(c->text) - 1] = 0;
    }
}

void Gfx2D_Rect(int x, int y, int w, int h, u8 r, u8 g, u8 b, u8 a) {
    rec_rect(GFX_BUF_OVL, x, y, w, h, r, g, b, a);
}

void Gfx2D_FillScreen(u8 r, u8 g, u8 b, s16 a) {
    if (a > 0) {
        Gfx2D_Rect(0, 0, SCREEN_GAME_W, SCREEN_GAME_H, r, g, b, (u8)(a > 255 ? 255 : a));
    }
}

// executed during replay (Gfx3D_FillScreenPrim)
void Gfx2D_ExecFill(u8 r, u8 g, u8 b, u8 a) {
    if (a == 0) {
        return;
    }
    CmdRect c = { 0, 0, SCREEN_GAME_W, SCREEN_GAME_H, { r, g, b, a } };
    x_rect(&c);
}

void Gfx2D_End(void) {
}

void Gfx2D_Begin3DOrtho(void) {
}

// ---- texture rectangles of the 2D interface code (engine/gfx3d.c x_texrect): the 2D shader with the
// renderer's combiner / blend state; the caller binds the texture and sets that state.
void Gfx2D_BindRectProgram(void) {
    Gfx3D_NotifyProgramChanged();
    C3D_BindProgram(&sProg);
    Screen_SetGameViewport();
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, sUProj, &sProj);
    C3D_SetAttrInfo(&sAttr);
}

void Gfx2D_TexQuad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1) {
    static const float kWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // shade (unused by rectangle combiners)
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    quad(x0, y0, x1, y1, u0, v0, u1, v1, kWhite);
    C3D_ImmDrawEnd();
}

void Gfx2D_TexTriangles(const float* xy, const float* uv, const u8* col, int n) {
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    for (int i = 0; i < 3 * n; i++) {
        float c[4];
        rgba(c, &col[4 * i]);
        vtx(xy[2 * i], xy[2 * i + 1], uv[2 * i], uv[2 * i + 1], c);
    }
    C3D_ImmDrawEnd();
}
