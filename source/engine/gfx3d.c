// engine/gfx3d.c (rewritten: native) - 3D renderer on the PICA200 (see gfx3d.h).
//
// Recording: every call appends a command (an exec function + payload) to the current target buffer.
// Replay (Gfx3D_EndFrame): opaque, translucent, overlay - the original's display-list order.
// Mesh programs (engine/pmesh.h) are executed by a render-state machine holding the same state the
// original graphics pipeline kept: geometry mode, render mode, combiner, prim/env/fog/blend colours,
// texture tiles, matrices, lights, segment bindings. Each draw translates that state into PICA state:
// texture combiner (engine/tev.c), blending, depth, alpha test, fog LUT, culling, texture units, and the
// vertex shader uniforms (gfx3d.v.pica).
#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <string.h>
#include "engine/gfx3d.h"
#include "engine/gfx2d.h"
#include "engine/screen.h"
#include "engine/asset.h"
#include "engine/tev.h"
#include "engine/log.h"
#include "light.h"
#include "gfx3d_shbin.h"

// ============================================================================ command buffers
#define CMDBUF_SIZE (768 * 1024)
typedef struct CmdHdr {
    Gfx3DExecFn fn;
    u32 size; // payload bytes (8-aligned)
} CmdHdr;

static u8* sCmd[GFX_BUF_COUNT];
static u32 sCmdLen[GFX_BUF_COUNT];
static GfxBuf sTarget = GFX_BUF_OPA;

void* Gfx3D_Record(GfxBuf buf, Gfx3DExecFn fn, size_t payloadSize) {
    payloadSize = (payloadSize + 7) & ~(size_t)7;
    if (sCmdLen[buf] + sizeof(CmdHdr) + payloadSize > CMDBUF_SIZE) {
        static bool warned;
        if (!warned) {
            warned = true;
            log_printf("gfx3d: command buffer %d full\n", buf);
        }
        return NULL;
    }
    CmdHdr* h = (CmdHdr*)(sCmd[buf] + sCmdLen[buf]);
    h->fn = fn;
    h->size = payloadSize;
    sCmdLen[buf] += sizeof(CmdHdr) + payloadSize;
    return h + 1;
}

static void* rec(Gfx3DExecFn fn, size_t size) {
    return Gfx3D_Record(sTarget, fn, size);
}

void Gfx3D_SetTarget(GfxBuf buf) {
    sTarget = buf;
}

GfxBuf Gfx3D_GetTarget(void) {
    return sTarget;
}

// ============================================================================ replay state
typedef struct Tile {
    const DsTexture* tex;
    u8 kind;
    u8 flags;
    u16 tileW, tileH;
    s16 uls, ult;
    u8 shiftS, shiftT;
} Tile;

typedef enum { SEG_NONE, SEG_TEX, SEG_DL, SEG_REC, SEG_SCROLL, SEG_RAW } SegKind;

#define SEGREC_MAX 6
typedef struct SegOp {
    u8 op; // 1 env, 2 prim, 3 draw
    u8 rgba[4];
    const DsObj* obj;
    const PMeshHdr* mesh;
} SegOp;

typedef struct SegScroll {
    u8 tile[2];
    s16 x[2], y[2];
    u8 count;
} SegScroll;

typedef struct Segment {
    SegKind kind;
    const DsTexture* tex;
    const DsObj* obj; // SEG_DL: object holding the lists, base = src address
    u32 src;
    u8 nops;
    SegOp ops[SEGREC_MAX];
    SegScroll scroll;
} Segment;

static struct {
    u32 geom, mode;
    GfxCombine combine;
    u8 prim[4], env[4], fogColor[4], blend[4];
    u8 primLod;
    u16 texScaleS, texScaleT;
    u8 texTile;
    bool texOn;
    Tile unit[2];
    bool scrollSet[8];
    s16 scrollS[8], scrollT[8];
    Segment seg[16];
    MtxF base;
    MtxF limb[64];
    int numLimb;
    MtxF segMtx[16][4];
    MtxF view, proj, vp; // vp = view * projection
    float ambient[3];
    float lightDir[7][3], lightCol[7][3];
    int numLights;
    s16 fogNear, fogFar;
    bool fogLutValid;
    bool program3D; // the 3D shader is bound
} R;

static DVLB_s* sDvlb;
static shaderProgram_s sProg;
static int uPal, uLightDir, uLightCol, uAmbient, uTexA, uTexB, uLookX, uLookY, uLit, uTexgen;
static C3D_AttrInfo sAttrUnlit, sAttrLit;
static C3D_Tex sWhite;
static C3D_FogLut sFogLut;
static DsObj* sSetupDL;
static MtxF sIdentity;

static void mtx_identity(MtxF* m) {
    memset(m, 0, sizeof(*m));
    m->mf[0][0] = m->mf[1][1] = m->mf[2][2] = m->mf[3][3] = 1.0f;
}

// row-vector convention: out = a * b
static void mtx_mul(MtxF* out, const MtxF* a, const MtxF* b) {
    MtxF r;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            r.mf[i][j] = a->mf[i][0] * b->mf[0][j] + a->mf[i][1] * b->mf[1][j] + a->mf[i][2] * b->mf[2][j] +
                         a->mf[i][3] * b->mf[3][j];
        }
    }
    *out = r;
}

static void reset_state(void) {
    R.geom = 0;
    R.mode = 0;
    memset(&R.combine, 0, sizeof(R.combine));
    memset(R.prim, 255, 4);
    memset(R.env, 255, 4);
    memset(R.fogColor, 0, 4);
    memset(R.blend, 0, 4);
    R.texScaleS = R.texScaleT = 0xFFFF;
    R.texTile = 0;
    R.texOn = true;
    memset(R.unit, 0, sizeof(R.unit));
    memset(R.scrollSet, 0, sizeof(R.scrollSet));
    memset(R.seg, 0, sizeof(R.seg));
    R.base = sIdentity;
    R.numLimb = 0;
    R.numLights = 0;
    R.program3D = false;
}

void Gfx3D_Init(void) {
    for (int i = 0; i < GFX_BUF_COUNT; i++) {
        sCmd[i] = linearMemAlign(CMDBUF_SIZE, 16); // plain memory would do; linear keeps the heap free
    }
    mtx_identity(&sIdentity);
    sDvlb = DVLB_ParseFile((u32*)gfx3d_shbin, gfx3d_shbin_size);
    shaderProgramInit(&sProg);
    shaderProgramSetVsh(&sProg, &sDvlb->DVLE[0]);
    shaderInstance_s* vs = sProg.vertexShader;
    uPal = shaderInstanceGetUniformLocation(vs, "pal");
    uLightDir = shaderInstanceGetUniformLocation(vs, "lightDir");
    uLightCol = shaderInstanceGetUniformLocation(vs, "lightCol");
    uAmbient = shaderInstanceGetUniformLocation(vs, "ambient");
    uTexA = shaderInstanceGetUniformLocation(vs, "texA");
    uTexB = shaderInstanceGetUniformLocation(vs, "texB");
    uLookX = shaderInstanceGetUniformLocation(vs, "lookX");
    uLookY = shaderInstanceGetUniformLocation(vs, "lookY");
    uLit = shaderInstanceGetUniformLocation(vs, "lit");
    uTexgen = shaderInstanceGetUniformLocation(vs, "texgen");

    // vertex layout (pmesh.h PVertex): pos s16x3, mtx u8x2, st s16x2, colour u8x4 | normal s8x3 + alpha u8
    AttrInfo_Init(&sAttrUnlit);
    AttrInfo_AddLoader(&sAttrUnlit, 0, GPU_SHORT, 3);
    AttrInfo_AddLoader(&sAttrUnlit, 1, GPU_UNSIGNED_BYTE, 2);
    AttrInfo_AddLoader(&sAttrUnlit, 2, GPU_SHORT, 2);
    AttrInfo_AddLoader(&sAttrUnlit, 3, GPU_UNSIGNED_BYTE, 4);
    AttrInfo_Init(&sAttrLit);
    AttrInfo_AddLoader(&sAttrLit, 0, GPU_SHORT, 3);
    AttrInfo_AddLoader(&sAttrLit, 1, GPU_UNSIGNED_BYTE, 2);
    AttrInfo_AddLoader(&sAttrLit, 2, GPU_SHORT, 2);
    AttrInfo_AddLoader(&sAttrLit, 3, GPU_BYTE, 3);
    AttrInfo_AddLoader(&sAttrLit, 4, GPU_UNSIGNED_BYTE, 1);

    C3D_TexInit(&sWhite, 8, 8, GPU_RGBA8);
    memset(sWhite.data, 0xFF, sWhite.size);
    C3D_TexFlush(&sWhite);

    sSetupDL = DsObj_Load("/setupdl.pobj");
    if (sSetupDL == NULL) {
        log_printf("gfx3d: render-state presets missing\n");
    }
    reset_state();
}

// ============================================================================ state translation
static float shift_factor(int sh) {
    if (sh == 0) return 1.0f;
    return sh <= 10 ? 1.0f / (float)(1 << sh) : (float)(1 << (16 - sh));
}

static GPU_TEXTURE_WRAP_PARAM wrap_mode(bool clamp, bool mirror) {
    if (mirror) return GPU_MIRRORED_REPEAT;
    return clamp ? GPU_CLAMP_TO_EDGE : GPU_REPEAT;
}

static void apply_textures(float texA[4], float texB[4]) {
    bool bilerp = MODE_TEXFILT(R.mode) != 0;
    float scaleS = (R.texScaleS == 0xFFFF ? 65536.0f : (float)R.texScaleS) / 65536.0f;
    float scaleT = (R.texScaleT == 0xFFFF ? 65536.0f : (float)R.texScaleT) / 65536.0f;
    for (int u = 0; u < 2; u++) {
        Tile* t = &R.unit[u];
        const DsTexture* tex = t->tex;
        float W = 8, H = 8;
        if (tex != NULL) {
            W = tex->w;
            H = tex->h;
            C3D_Tex* ct = (C3D_Tex*)&tex->tex;
            C3D_TexSetFilter(ct, bilerp ? GPU_LINEAR : GPU_NEAREST, bilerp ? GPU_LINEAR : GPU_NEAREST);
            C3D_TexSetWrap(ct, wrap_mode(t->flags & TEXF_CLAMP_S, t->flags & TEXF_MIRROR_S),
                           wrap_mode(t->flags & TEXF_CLAMP_T, t->flags & TEXF_MIRROR_T));
            C3D_TexBind(u, ct);
        } else {
            C3D_TexBind(u, &sWhite);
        }
        int tileNo = (R.texTile + u) & 7;
        float uls = t->uls / 4.0f, ult = t->ult / 4.0f;
        if ((t->flags & TEXF_RUNTIME_TILE) && R.scrollSet[tileNo]) {
            uls = R.scrollS[tileNo] / 4.0f;
            ult = R.scrollT[tileNo] / 4.0f;
        }
        float off = bilerp ? 0.5f : 0.0f;
        float as = scaleS * shift_factor(t->shiftS) / 32.0f;
        float at = scaleT * shift_factor(t->shiftT) / 32.0f;
        texA[u * 2 + 0] = as / W;
        texB[u * 2 + 0] = (-uls + off) / W;
        texA[u * 2 + 1] = -at / H;
        texB[u * 2 + 1] = 1.0f - (-ult + off) / H;
    }
}

static GPU_BLENDFACTOR blend_src(int a) {
    switch (a) {
        case BL_A_IN: return GPU_SRC_ALPHA;
        case BL_A_FOG: return GPU_CONSTANT_ALPHA;
        case BL_A_SHADE: return GPU_SRC_ALPHA;
        default: return GPU_ZERO;
    }
}

static GPU_BLENDFACTOR blend_dst(int b, int a) {
    switch (b) {
        case BL_B_1MA: return a == BL_A_FOG ? GPU_ONE_MINUS_CONSTANT_ALPHA : (a == BL_A_ZERO ? GPU_ONE : GPU_ONE_MINUS_SRC_ALPHA);
        case BL_B_MEM: return GPU_DST_ALPHA;
        case BL_B_ONE: return GPU_ONE;
        default: return GPU_ZERO;
    }
}

static void update_fog_lut(void) {
    // Original: fog alpha = clamp((z_ndc * 500 + 500 - near) * 256 / (far - near), 0, 255) per vertex.
    // PICA: LUT over window depth d (1 = near plane, 0 = far plane here: z_ndc = 1 - 2d), value = visibility.
    float data[256];
    float rng = (float)(R.fogFar - R.fogNear);
    bool unset = R.fogFar <= R.fogNear; // fog factors never set: the original's fog alpha stays 0
    if (rng < 1.0f) rng = 1.0f;
    for (int i = 0; i <= 128; i++) {
        float d = i / 128.0f;
        float zndc = 1.0f - 2.0f * d;
        float f = (zndc * 500.0f + 500.0f - R.fogNear) * 256.0f / rng;
        f = f < 0 ? 0 : (f > 255 ? 255 : f);
        float vis = unset ? 1.0f : 1.0f - f / 255.0f;
        if (i < 128) data[i] = vis;
        if (i > 0) data[128 + i - 1] = vis - data[i - 1];
    }
    FogLut_FromArray(&sFogLut, data);
    R.fogLutValid = true;
}

static TevProgram sTevCache[48];
static GfxCombine sTevKey[48];
static u8 sTevTwo[48];
static int sTevNext;

static const TevProgram* tev_for(const GfxCombine* c, bool two) {
    for (int i = 0; i < 48; i++) {
        if (sTevTwo[i] == (two ? 2 : 1) && memcmp(&sTevKey[i], c, sizeof(*c)) == 0) {
            return &sTevCache[i];
        }
    }
    int i = sTevNext;
    sTevNext = (sTevNext + 1) % 48;
    sTevKey[i] = *c;
    sTevTwo[i] = two ? 2 : 1;
    Tev_Compile(&sTevCache[i], c, two);
    if (sTevCache[i].overflow || sTevCache[i].constConflict) {
        log_printf("tev: combiner needs >6 stages or conflicting constants\n");
    }
    return &sTevCache[i];
}

static void apply_render_state(void) {
    u32 m = R.mode;
    int cyc = MODE_CYCLE(m);
    bool two = cyc == 1;
    // combiner
    GfxCombine c = R.combine;
    if (cyc >= 2) { // copy / fill: texture straight through
        memset(&c, 0, sizeof(c));
        u8 t[8] = { CC_ZERO, CC_ZERO, CC_ZERO, CC_TEXEL0, CC_ZERO, CC_ZERO, CC_ZERO, CC_TEXEL0 };
        memcpy(c.c[1], t, 8);
        two = false;
    }
    Tev_Apply(tev_for(&c, two), R.prim, R.env, R.primLod);

    // blender: in two-cycle mode cycle 1 is (normally) the fog blend and cycle 2 the surface blend
    u8 bl = two ? MODE_BL2(m) : MODE_BL1(m);
    u8 bl1 = MODE_BL1(m);
    bool fog = two && (R.geom & GEOM_FOG) && ((bl1 >> 6) == BL_P_FOG) && (((bl1 >> 4) & 3) == BL_A_SHADE);
    int P = bl >> 6, A = (bl >> 4) & 3, M = (bl >> 2) & 3, B = bl & 3;
    if ((m & MODE_FORCE_BL) && !(P == BL_P_IN && M == BL_P_IN)) {
        GPU_BLENDFACTOR s, d;
        if (P == BL_P_MEM && M == BL_P_IN) {
            s = blend_dst(B, A);
            d = blend_src(A);
        } else {
            s = blend_src(A);
            d = blend_dst(B, A);
        }
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, s, d, s, d);
        C3D_BlendingColor((u32)R.fogColor[3] << 24);
    } else {
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    }
    // alpha test: threshold against the blend colour alpha, or coverage x alpha (texture edges)
    if (MODE_ALPHACMP(m) == 1) {
        C3D_AlphaTest(true, GPU_GEQUAL, R.blend[3] ? R.blend[3] : 1);
    } else if (m & MODE_CVG_X_ALPHA) {
        C3D_AlphaTest(true, GPU_GEQUAL, 0x80);
    } else {
        C3D_AlphaTest(false, GPU_ALWAYS, 0);
    }
    // depth (near = larger depth value, see screen/projection fix-up)
    bool zbuf = (R.geom & GEOM_ZBUFFER) != 0;
    GPU_WRITEMASK wm = GPU_WRITE_COLOR | ((zbuf && (m & MODE_ZUPD)) ? GPU_WRITE_DEPTH : 0);
    C3D_DepthTest(true, (zbuf && (m & MODE_ZCMP)) ? GPU_GEQUAL : GPU_ALWAYS, wm);
    C3D_DepthMap(true, -1.0f, MODE_ZMODE(m) == 3 ? 0.0002f : 0.0f);
    // culling
    if (R.geom & GEOM_CULL_BACK) {
        C3D_CullFace(GPU_CULL_BACK_CCW);
    } else if (R.geom & GEOM_CULL_FRONT) {
        C3D_CullFace(GPU_CULL_FRONT_CCW);
    } else {
        C3D_CullFace(GPU_CULL_NONE);
    }
    // fog
    if (fog) {
        if (!R.fogLutValid) {
            update_fog_lut();
        }
        C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY, false);
        C3D_FogColor(R.fogColor[0] | (R.fogColor[1] << 8) | (R.fogColor[2] << 16));
        C3D_FogLutBind(&sFogLut);
    } else {
        C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
    }
}

// Uniform rows (column-vector convention) of F * (M * V * P)^T, F = screen rotation + depth range fix:
// x' = y, y' = -x (portrait framebuffer), z' = (z - w) / 2 (near -> -1, far -> 0).
static void upload_palette(const PMeshHdr* mesh) {
    C3D_FVec* pal = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, uPal, 56);
    int n = mesh->numPalette ? mesh->numPalette : 1;
    for (int i = 0; i < n; i++) {
        const MtxF* model = &R.base;
        MtxF tmp;
        u8 type = mesh->palette[i][0], id = mesh->palette[i][1];
        if (mesh->numPalette == 0) {
            type = MSRC_BASE;
        }
        if (type == MSRC_LIMB) {
            model = id < R.numLimb ? &R.limb[id] : &R.base;
        } else if (type == MSRC_SEGMUL) {
            mtx_mul(&tmp, &R.segMtx[id & 15][(id >> 4) & 3], &R.base);
            model = &tmp;
        }
        MtxF r;
        mtx_mul(&r, model, &R.vp);
        C3D_FVec* row = &pal[i * 7];
        // row 0 = column 1, row 1 = -column 0, row 2 = (column 2 - column 3) / 2, row 3 = column 3
        // C3D_FVec stores (w, z, y, x) in memory order: use FVec4_New(x, y, z, w)
        row[0] = FVec4_New(r.mf[0][1], r.mf[1][1], r.mf[2][1], r.mf[3][1]);
        row[1] = FVec4_New(-r.mf[0][0], -r.mf[1][0], -r.mf[2][0], -r.mf[3][0]);
        row[2] = FVec4_New(0.5f * (r.mf[0][2] - r.mf[0][3]), 0.5f * (r.mf[1][2] - r.mf[1][3]),
                           0.5f * (r.mf[2][2] - r.mf[2][3]), 0.5f * (r.mf[3][2] - r.mf[3][3]));
        row[3] = FVec4_New(r.mf[0][3], r.mf[1][3], r.mf[2][3], r.mf[3][3]);
        // normal matrix rows: n_world = n * M3 -> row j = column j of M3
        row[4] = FVec4_New(model->mf[0][0], model->mf[1][0], model->mf[2][0], 0);
        row[5] = FVec4_New(model->mf[0][1], model->mf[1][1], model->mf[2][1], 0);
        row[6] = FVec4_New(model->mf[0][2], model->mf[1][2], model->mf[2][2], 0);
    }
}

static void bind_program(void) {
    if (!R.program3D) {
        C3D_BindProgram(&sProg);
        Screen_SetGameViewport();
        R.program3D = true;
    }
}

static const PMeshHdr sNoPalette; // numPalette 0: every vertex uses the caller's matrix

static void draw_raw(const void* verts, const u16* idx, u32 count, const PMeshHdr* mesh) {
    bind_program();
    bool lit = (R.geom & GEOM_LIGHTING) != 0;
    C3D_SetAttrInfo(lit ? &sAttrLit : &sAttrUnlit);
    C3D_BufInfo* bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    BufInfo_Add(bi, verts, sizeof(PVertex), lit ? 5 : 4, lit ? 0x43210 : 0x3210);

    float texA[4], texB[4];
    apply_textures(texA, texB);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, uTexA, texA[0], texA[1], texA[2], texA[3]);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, uTexB, texB[0], texB[1], texB[2], texB[3]);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER, uLit, lit);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER, uTexgen, lit && (R.geom & GEOM_TEXGEN));
    if (lit) {
        C3D_FVUnifSet(GPU_VERTEX_SHADER, uAmbient, R.ambient[0], R.ambient[1], R.ambient[2], 1.0f);
        for (int i = 0; i < 7; i++) {
            bool on = i < R.numLights;
            C3D_FVUnifSet(GPU_VERTEX_SHADER, uLightDir + i, on ? R.lightDir[i][0] : 0, on ? R.lightDir[i][1] : 0,
                          on ? R.lightDir[i][2] : 0, 0);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, uLightCol + i, on ? R.lightCol[i][0] : 0, on ? R.lightCol[i][1] : 0,
                          on ? R.lightCol[i][2] : 0, 0);
        }
        // camera axes in world space (texgen): columns 0 and 1 of the viewing matrix
        C3D_FVUnifSet(GPU_VERTEX_SHADER, uLookX, R.view.mf[0][0], R.view.mf[1][0], R.view.mf[2][0], 0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER, uLookY, R.view.mf[0][1], R.view.mf[1][1], R.view.mf[2][1], 0);
    }
    upload_palette(mesh);
    apply_render_state();
    C3D_DrawElements(GPU_TRIANGLES, count, C3D_UNSIGNED_SHORT, idx);
}

static void draw(const DsObj* obj, const PMeshHdr* mesh, u32 first, u32 count) {
    draw_raw(obj->data + mesh->vertsOff, (const u16*)(obj->data + mesh->idxOff) + first, count, mesh);
}

typedef struct { const void* vtx; const u16* idx; int n; const DsTexture* t[2]; } CmdSky;
static void x_sky(const void* p) {
    const CmdSky* c = p;
    for (int u = 0; u < 2; u++) {
        Tile* t = &R.unit[u];
        memset(t, 0, sizeof(*t));
        t->tex = c->t[u] ? c->t[u] : c->t[0];
        t->kind = TEXKIND_STATIC;
        t->flags = TEXF_CLAMP_S | TEXF_CLAMP_T;
    }
    R.texTile = 0;
    R.texScaleS = R.texScaleT = 0x8000;
    R.mode = (R.mode & ~(3u << 2)) | (2u << 2); // bilinear
    draw_raw(c->vtx, c->idx, c->n, &sNoPalette);
}

void Gfx3D_DrawSky(const void* vtx, const u16* idx, int numIdx, const DsTexture* t0, const DsTexture* t1) {
    if (numIdx <= 0) {
        return;
    }
    CmdSky* c = rec(x_sky, sizeof(CmdSky));
    if (c) {
        c->vtx = vtx;
        c->idx = idx;
        c->n = numIdx;
        c->t[0] = t0;
        c->t[1] = t1;
    }
}

// ============================================================================ mesh programs
static void exec_mesh(const DsObj* obj, const PMeshHdr* mesh, int depth);

static void call_segment(int seg, u32 off, int depth) {
    Segment* s = &R.seg[seg & 15];
    switch (s->kind) {
        case SEG_DL:
            if (s->obj != NULL) {
                const PMeshHdr* m = DsObj_FindMesh(s->obj, s->src + off);
                if (m != NULL) {
                    exec_mesh(s->obj, m, depth + 1);
                }
            }
            break;
        case SEG_REC:
            for (int i = 0; i < s->nops; i++) {
                const SegOp* o = &s->ops[i];
                if (o->op == 1) memcpy(R.env, o->rgba, 4);
                else if (o->op == 2) memcpy(R.prim, o->rgba, 4);
                else if (o->op == 3 && o->mesh != NULL) exec_mesh(o->obj, o->mesh, depth + 1);
            }
            break;
        case SEG_SCROLL:
            for (int i = 0; i < s->scroll.count; i++) {
                int t = s->scroll.tile[i] & 7;
                R.scrollSet[t] = true;
                R.scrollS[t] = s->scroll.x[i];
                R.scrollT[t] = s->scroll.y[i];
            }
            break;
        default:
            break; // empty / unbound segment list
    }
}

static void exec_mesh(const DsObj* obj, const PMeshHdr* mesh, int depth) {
    if (depth > 8 || mesh == NULL) {
        return;
    }
    const u32* op = (const u32*)(obj->data + mesh->opsOff);
    const u32* end = op + mesh->numOps;
    while (op < end) {
        u32 w = op[0];
        u8 code = w & 0xFF, a = (w >> 8) & 0xFF;
        switch (code) {
            case OP_END:
                return;
            case OP_DRAW:
                draw(obj, mesh, op[1], op[2]);
                op += 3;
                break;
            case OP_GEOM:
                R.geom = (R.geom & ~op[1]) | op[2];
                op += 3;
                break;
            case OP_MODE:
                R.mode = (R.mode & ~op[1]) | (op[2] & op[1]);
                op += 3;
                break;
            case OP_COMBINE:
                memcpy(&R.combine, &op[1], 16);
                op += 5;
                break;
            case OP_PRIM:
                R.prim[0] = op[1] >> 24; R.prim[1] = op[1] >> 16; R.prim[2] = op[1] >> 8; R.prim[3] = op[1];
                R.primLod = a;
                op += 2;
                break;
            case OP_ENV:
                R.env[0] = op[1] >> 24; R.env[1] = op[1] >> 16; R.env[2] = op[1] >> 8; R.env[3] = op[1];
                op += 2;
                break;
            case OP_FOGCOLOR:
                R.fogColor[0] = op[1] >> 24; R.fogColor[1] = op[1] >> 16; R.fogColor[2] = op[1] >> 8;
                R.fogColor[3] = op[1];
                op += 2;
                break;
            case OP_BLENDCOLOR:
                R.blend[0] = op[1] >> 24; R.blend[1] = op[1] >> 16; R.blend[2] = op[1] >> 8; R.blend[3] = op[1];
                op += 2;
                break;
            case OP_TEX: {
                Tile* t = &R.unit[a & 1];
                u32 kind = op[1] >> 16, id = op[1] & 0xFFFF;
                t->kind = kind;
                t->flags = op[2] & 0xFF;
                t->uls = (s16)(op[4] & 0xFFFF);
                t->ult = (s16)(op[4] >> 16);
                t->shiftS = op[5] & 0xF;
                t->shiftT = (op[5] >> 4) & 0xF;
                if (kind == TEXKIND_STATIC) {
                    t->tex = DsObj_Texture(obj, id);
                } else if (kind == TEXKIND_SEGMENT) {
                    t->tex = R.seg[id & 15].kind == SEG_TEX ? R.seg[id & 15].tex : NULL;
                } else {
                    t->tex = NULL;
                }
                op += 6;
                break;
            }
            case OP_TEXSCALE:
                R.texTile = a & 7;
                R.texOn = (w >> 16) & 1;
                R.texScaleS = op[1] >> 16;
                R.texScaleT = op[1] & 0xFFFF;
                op += 2;
                break;
            case OP_CALLSEG:
                call_segment(a, op[1], depth);
                op += 2;
                break;
            default:
                log_printf("gfx3d: bad op %u\n", code);
                return;
        }
    }
}

// ============================================================================ recorded commands
typedef struct { MtxF proj, view; } CmdCamera;
static void x_camera(const void* p) {
    const CmdCamera* c = p;
    R.proj = c->proj;
    R.view = c->view;
    mtx_mul(&R.vp, &R.view, &R.proj);
}

void Gfx3D_SetCamera(const MtxF* projection, const MtxF* viewing) {
    for (int b = GFX_BUF_OPA; b <= GFX_BUF_XLU; b++) {
        CmdCamera* c = Gfx3D_Record(b, x_camera, sizeof(CmdCamera));
        if (c) {
            c->proj = *projection;
            c->view = *viewing;
        }
    }
}

static void x_mtx(const void* p) {
    R.base = *(const MtxF*)p;
}

void Gfx3D_LoadMtxF(const MtxF* model) {
    MtxF* m = rec(x_mtx, sizeof(MtxF));
    if (m) *m = *model;
}

void Gfx3D_StoreSlot(int slot) {
    (void)slot;
}

void Gfx3D_RestoreSlot(int slot) {
    (void)slot;
}

static DsObj* sSegObj[16];

void Gfx3D_SetSegmentObj(int seg, DsObj* obj) {
    sSegObj[seg & 15] = obj;
}

DsObj* Gfx3D_GetSegmentObj(int seg) {
    return sSegObj[seg & 15];
}

typedef struct { u8 seg; Segment s; } CmdSeg;
static void x_seg(const void* p) {
    const CmdSeg* c = p;
    R.seg[c->seg & 15] = c->s;
}

static CmdSeg* rec_seg(int seg, SegKind kind) {
    CmdSeg* c = rec(x_seg, sizeof(CmdSeg));
    if (c) {
        memset(c, 0, sizeof(*c));
        c->seg = seg;
        c->s.kind = kind;
    }
    return c;
}

void Gfx3D_SetSegmentTex(int seg, const DsTexture* tex) {
    CmdSeg* c = rec_seg(seg, SEG_TEX);
    if (c) c->s.tex = tex;
}

typedef struct { const DsObj* obj; const PMeshHdr* mesh; } CmdDraw;
static void x_draw(const void* p) {
    const CmdDraw* c = p;
    exec_mesh(c->obj, c->mesh, 0);
}

void Gfx3D_DrawMesh(const DsObj* obj, const DsMeshHdr* mesh) {
    CmdDraw* c = rec(x_draw, sizeof(CmdDraw));
    if (c) {
        c->obj = obj;
        c->mesh = mesh;
    }
}

// ---- segment lists being recorded (Gfx3D_SegDLBegin/End)
static int sRecSeg = -1;
static CmdSeg* sRecCmd;

static bool seg_record(u8 op, u8 r, u8 g, u8 b, u8 a, const DsObj* obj, const PMeshHdr* mesh) {
    if (sRecSeg < 0) {
        return false;
    }
    if (sRecCmd != NULL && sRecCmd->s.nops < SEGREC_MAX) {
        SegOp* o = &sRecCmd->s.ops[sRecCmd->s.nops++];
        o->op = op;
        o->rgba[0] = r; o->rgba[1] = g; o->rgba[2] = b; o->rgba[3] = a;
        o->obj = obj;
        o->mesh = mesh;
    }
    return true;
}

static bool resolve_dl(u32 segAddr, const DsObj** outObj, const PMeshHdr** outMesh) {
    DsObj* obj = sSegObj[(segAddr >> 24) & 15];
    if (obj == NULL) {
        return false;
    }
    const PMeshHdr* mesh = DsObj_FindMesh(obj, segAddr & 0x0FFFFFFF);
    if (mesh == NULL) {
        return false;
    }
    *outObj = obj;
    *outMesh = mesh;
    return true;
}

bool Gfx3D_DrawDL(u32 segAddr) {
    const DsObj* obj;
    const PMeshHdr* mesh;
    if (!resolve_dl(segAddr, &obj, &mesh)) {
        return false;
    }
    if (seg_record(3, 0, 0, 0, 0, obj, mesh)) {
        return true;
    }
    Gfx3D_DrawMesh(obj, mesh);
    return true;
}

void Gfx3D_SegDLBegin(int seg) {
    seg &= 15;
    if (sRecSeg == seg) {
        return; // the original bound one list to both the opaque and the translucent buffer
    }
    sRecSeg = seg;
    sRecCmd = rec_seg(seg, SEG_REC);
}

void Gfx3D_SegDLEnd(void) {
    sRecSeg = -1;
    sRecCmd = NULL;
}

typedef struct { u8 rgba[4]; } CmdColor;
static void x_env(const void* p) {
    memcpy(R.env, ((const CmdColor*)p)->rgba, 4);
}
static void x_prim(const void* p) {
    memcpy(R.prim, ((const CmdColor*)p)->rgba, 4);
}

void Gfx3D_SetEnvColor(u8 r, u8 g, u8 b, u8 a) {
    if (seg_record(1, r, g, b, a, NULL, NULL)) {
        return;
    }
    CmdColor* c = rec(x_env, sizeof(CmdColor));
    if (c) { c->rgba[0] = r; c->rgba[1] = g; c->rgba[2] = b; c->rgba[3] = a; }
}

void Gfx3D_SetPrimColor(u8 r, u8 g, u8 b, u8 a) {
    if (seg_record(2, r, g, b, a, NULL, NULL)) {
        return;
    }
    CmdColor* c = rec(x_prim, sizeof(CmdColor));
    if (c) { c->rgba[0] = r; c->rgba[1] = g; c->rgba[2] = b; c->rgba[3] = a; }
}

typedef struct { u32 clear, set; } CmdGeom;
static void x_geom(const void* p) {
    const CmdGeom* c = p;
    R.geom = (R.geom & ~c->clear) | c->set;
}

void Gfx3D_GeometryMode(u32 clear, u32 set) {
    CmdGeom* c = rec(x_geom, sizeof(CmdGeom));
    if (c) {
        c->clear = clear;
        c->set = set;
    }
}

static void x_preset(const void* p) {
    int preset = *(const int*)p;
    if (sSetupDL != NULL) {
        exec_mesh(sSetupDL, DsObj_FindMesh(sSetupDL, (u32)preset), 0);
    }
}

void Gfx3D_SetupDL(int preset) {
    int* c = rec(x_preset, sizeof(int));
    if (c) *c = preset;
}

void Gfx3D_SetupDLTo(GfxBuf buf, int preset) {
    Gfx3D_SetTarget(buf);
    Gfx3D_SetupDL(preset);
}

void Gfx3D_ResetMaterialState(int xlu) {
    // Remaining DS-era call sites whose original preset could not be recovered: preset 25 (the
    // original's most common one) into the opaque or translucent buffer.
    Gfx3D_SetupDLTo(xlu ? GFX_BUF_XLU : GFX_BUF_OPA, 25);
}

const DsTexture* Asset_FindTexture(const void* token);

void Gfx3D_SetSegment(int seg, const void* handle) {
    seg &= 15;
    if (handle == NULL) {
        rec_seg(seg, SEG_NONE);
        return;
    }
    const DsTexture* t = Asset_FindTexture(handle);
    if (t != NULL) {
        Gfx3D_SetSegmentTex(seg, t);
        return;
    }
    if (ASSET_IS_TOKEN(handle)) {
        CmdSeg* c = rec_seg(seg, SEG_DL);
        if (c) {
            c->s.obj = sSegObj[ASSET_SEG(handle)];
            c->s.src = ASSET_SRC(handle);
        }
        return;
    }
    rec_seg(seg, SEG_RAW); // native sentinel lists (empty list, cull-mode presets): no geometry
}

typedef struct { float amb[3]; int n; float dir[7][3], col[7][3]; } CmdLights;
static void x_lights(const void* p) {
    const CmdLights* c = p;
    memcpy(R.ambient, c->amb, sizeof(R.ambient));
    R.numLights = c->n;
    memcpy(R.lightDir, c->dir, sizeof(R.lightDir));
    memcpy(R.lightCol, c->col, sizeof(R.lightCol));
}

static void rec_lights(const u8 ambient[3], int count, const s8 (*dirs)[3], const u8 (*cols)[3], int stride) {
    // the original wrote light groups into both the opaque and the translucent list (Lights_Draw)
    CmdLights* c = Gfx3D_Record(GFX_BUF_OPA, x_lights, sizeof(CmdLights));
    CmdLights* c2 = Gfx3D_Record(GFX_BUF_XLU, x_lights, sizeof(CmdLights));
    if (c == NULL || c2 == NULL) return;
    for (int k = 0; k < 3; k++) c->amb[k] = ambient[k] / 255.0f;
    c->n = count > 7 ? 7 : count;
    for (int i = 0; i < c->n; i++) {
        const s8* d = (const s8*)((const u8*)dirs + i * stride);
        const u8* col = (const u8*)cols + i * stride;
        float x = d[0], y = d[1], z = d[2];
        float l = sqrtf(x * x + y * y + z * z);
        if (l < 1e-3f) l = 1.0f;
        c->dir[i][0] = x / l; c->dir[i][1] = y / l; c->dir[i][2] = z / l;
        for (int k = 0; k < 3; k++) c->col[i][k] = col[k] / 255.0f;
    }
    *c2 = *c;
}

void Gfx3D_SetLights(const u8 ambient[3], int count, const s8 dirs[][3], const u8 cols[][3]) {
    rec_lights(ambient, count, dirs, cols, 3);
}

void Gfx3D_SetLightsN(const struct LightsN* l, int numLights) {
    // LightRecN: col[3], pad, colc[3], pad, dir[3], pad (16 bytes)
    rec_lights(l->a.l.col, numLights, (const s8 (*)[3])l->l[0].l.dir, (const u8 (*)[3])l->l[0].l.col, sizeof(LightN));
}

// ---- flexible skeletons
#define FLEX_MAX 64
static struct { int n; MtxF m[FLEX_MAX]; DListRef dl[FLEX_MAX]; } sFlex;

typedef struct { int n; MtxF m[FLEX_MAX]; const DsObj* obj[FLEX_MAX]; const PMeshHdr* mesh[FLEX_MAX]; } CmdFlex;
static void x_flex(const void* p) {
    const CmdFlex* c = p;
    R.numLimb = c->n;
    memcpy(R.limb, c->m, sizeof(MtxF) * c->n);
    for (int i = 0; i < c->n; i++) {
        if (c->mesh[i] != NULL) {
            R.base = c->m[i];
            exec_mesh(c->obj[i], c->mesh[i], 0);
        }
    }
}

void Gfx3D_FlexBegin(void) {
    sFlex.n = 0;
}

void Gfx3D_FlexAdd(const MtxF* m, DListRef dl) {
    if (sFlex.n < FLEX_MAX) {
        sFlex.m[sFlex.n] = *m;
        sFlex.dl[sFlex.n] = dl;
        sFlex.n++;
    }
}

void Gfx3D_FlexEnd(void) {
    size_t size = offsetof(CmdFlex, m) + sizeof(MtxF) * FLEX_MAX + sizeof(void*) * 2 * FLEX_MAX;
    CmdFlex* c = rec(x_flex, size);
    if (c == NULL) return;
    c->n = sFlex.n;
    for (int i = 0; i < sFlex.n; i++) {
        c->m[i] = sFlex.m[i];
        c->obj[i] = NULL;
        c->mesh[i] = NULL;
        if (sFlex.dl[i] != NULL) {
            resolve_dl((u32)(uintptr_t)sFlex.dl[i], &c->obj[i], &c->mesh[i]);
        }
    }
}

// ---- scratch memory
#define FRAME_POOL_SIZE (256 * 1024)
static u8 sFramePool[FRAME_POOL_SIZE] __attribute__((aligned(16)));
static size_t sFramePos;

void* Gfx3D_FrameAlloc(size_t size) {
    size = (size + 15) & ~(size_t)15;
    if (sFramePos + size > FRAME_POOL_SIZE) {
        log_printf("gfx3d: frame pool exhausted (%u)\n", (unsigned)size);
        return NULL;
    }
    void* p = &sFramePool[sFramePos];
    sFramePos += size;
    return p;
}

void Gfx3D_FrameReset(void) {
    sFramePos = 0;
}

// ---- texture scroll lists
void Gfx3D_SetTexScroll(int seg, int tile, int x, int y, int w, int h) {
    (void)w; (void)h;
    CmdSeg* c = rec_seg(seg, SEG_SCROLL);
    if (c) {
        c->s.scroll.count = 1;
        c->s.scroll.tile[0] = tile;
        c->s.scroll.x[0] = x;
        c->s.scroll.y[0] = y;
    }
}

void Gfx3D_SetTexScroll2(int seg, int tile1, int x1, int y1, int w1, int h1, int tile2, int x2, int y2, int w2,
                         int h2) {
    (void)w1; (void)h1; (void)w2; (void)h2;
    CmdSeg* c = rec_seg(seg, SEG_SCROLL);
    if (c) {
        c->s.scroll.count = 2;
        c->s.scroll.tile[0] = tile1; c->s.scroll.x[0] = x1; c->s.scroll.y[0] = y1;
        c->s.scroll.tile[1] = tile2; c->s.scroll.x[1] = x2; c->s.scroll.y[1] = y2;
    }
}

typedef struct { int seg; MtxF m; } CmdSegMtx;
static void x_segmtx(const void* p) {
    const CmdSegMtx* s = p;
    R.segMtx[s->seg][0] = s->m;
}

typedef struct { int seg, n; const MtxF* m; } CmdSegMtxArr;
static void x_segmtxarr(const void* p) {
    const CmdSegMtxArr* c = p;
    for (int i = 0; i < c->n && i < 4; i++) {
        R.segMtx[c->seg][i] = c->m[i]; // frame memory: filled by game code after the bind, read at replay
    }
}

void Gfx3D_SetSegmentMtxArray(int seg, const MtxF* m, int count) {
    CmdSegMtxArr* c = rec(x_segmtxarr, sizeof(CmdSegMtxArr));
    if (c) {
        c->seg = seg & 15;
        c->n = count;
        c->m = m;
    }
}

void Gfx3D_SetSegmentMtx(int seg, const MtxF* m) {
    CmdSegMtx* s = rec(x_segmtx, sizeof(CmdSegMtx));
    if (s) {
        s->seg = seg & 15;
        s->m = *m;
    }
}

// ---- fog, fills, frame
typedef struct { u8 rgba[4]; s16 n, f; } CmdFog;
static void x_fog(const void* p) {
    const CmdFog* c = p;
    memcpy(R.fogColor, c->rgba, 4);
    if (c->n != R.fogNear || c->f != R.fogFar) {
        R.fogNear = c->n;
        R.fogFar = c->f;
        R.fogLutValid = false;
    }
}

void Gfx3D_SetFog(u8 r, u8 g, u8 b, u8 a, s16 fogNear, s16 fogFar) {
    CmdFog* c = rec(x_fog, sizeof(CmdFog));
    if (c) { c->rgba[0] = r; c->rgba[1] = g; c->rgba[2] = b; c->rgba[3] = a; c->n = fogNear; c->f = fogFar; }
}

static void x_fill(const void* p) {
    (void)p;
    R.program3D = false;
    Gfx2D_ExecFill(R.prim[0], R.prim[1], R.prim[2], R.prim[3]);
}

void Gfx3D_FillScreenPrim(void) {
    rec(x_fill, 0);
}

void Gfx3D_SetupFrame(u8 r, u8 g, u8 b) {
    Gfx3D_SetTarget(GFX_BUF_OPA);
    Gfx2D_Begin(r, g, b);
    Gfx3D_ResetMaterialState(0);
}

void Gfx3D_BeginFrame(void) {
    for (int i = 0; i < GFX_BUF_COUNT; i++) {
        sCmdLen[i] = 0;
    }
    sTarget = GFX_BUF_OPA;
    sRecSeg = -1;
    sRecCmd = NULL;
}

void Gfx3D_NotifyProgramChanged(void) {
    R.program3D = false;
}

void Gfx3D_EndFrame(void) {
    reset_state();
    for (int b = 0; b < GFX_BUF_COUNT; b++) {
        u32 pos = 0;
        while (pos < sCmdLen[b]) {
            CmdHdr* h = (CmdHdr*)(sCmd[b] + pos);
            if (h->fn != NULL) {
                h->fn(h + 1);
            }
            pos += sizeof(CmdHdr) + h->size;
        }
    }
}
