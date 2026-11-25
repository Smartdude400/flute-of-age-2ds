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
#include "engine/vtx.h"
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
    u8 op; // 1 env, 2 prim, 3 draw, 4 render mode
    u8 rgba[4];
    u32 mode[2]; // op 4: native MODE_* mask, value
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
    u32 genMtx;     // bumped whenever a matrix input of the palette changes (camera, base, limbs, segments)
    u32 genLights;  // bumped whenever the light group changes
} R;

// ---- caches of what the GPU already has (reset at frame start and when the 2D layer takes the GPU)
static struct {
    bool valid;
    const void* verts;
    bool lit;
    const u8* palKey; // mesh palette (type, id pairs) last uploaded
    u8 palN;
    u32 palGen;
    u32 lightGen, lookGen;
    bool lightsLoaded;
    bool palLoaded;
    float texA[4], texB[4];
    int boolLit, boolTexgen;
    const C3D_Tex* bound[2];
    struct { u32 mode, geom; GfxCombine combine; u8 prim[4], env[4], fog[4], blend[4]; u8 primLod; bool fogValid; } rs;
} sC;

static void cache_reset(void) {
    memset(&sC, 0, sizeof(sC));
    sC.boolLit = sC.boolTexgen = -1;
}

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
    // display lists embedded in actor overlays (tools/build_overlays.py): handles are original VRAM
    // addresses = segment 0, which the game never binds
    DsObj* ovl = DsObj_Load("/ovl/ovldata.pobj");
    if (ovl == NULL) {
        log_printf("gfx3d: overlay display lists missing\n");
    }
    Asset_SetSegment(0, ovl);
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
            u32 param = ct->param;
            C3D_TexSetFilter(ct, bilerp ? GPU_LINEAR : GPU_NEAREST, bilerp ? GPU_LINEAR : GPU_NEAREST);
            C3D_TexSetWrap(ct, wrap_mode(t->flags & TEXF_CLAMP_S, t->flags & TEXF_MIRROR_S),
                           wrap_mode(t->flags & TEXF_CLAMP_T, t->flags & TEXF_MIRROR_T));
            if (sC.bound[u] != ct || ct->param != param) {
                C3D_TexBind(u, ct);
                sC.bound[u] = ct;
            }
        } else if (sC.bound[u] != &sWhite) {
            C3D_TexBind(u, &sWhite);
            sC.bound[u] = &sWhite;
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
    if (sC.valid && sC.rs.mode == R.mode && sC.rs.geom == R.geom && sC.rs.primLod == R.primLod &&
        sC.rs.fogValid == R.fogLutValid && memcmp(&sC.rs.combine, &R.combine, sizeof(GfxCombine)) == 0 &&
        memcmp(sC.rs.prim, R.prim, 4) == 0 && memcmp(sC.rs.env, R.env, 4) == 0 &&
        memcmp(sC.rs.fog, R.fogColor, 4) == 0 && memcmp(sC.rs.blend, R.blend, 4) == 0) {
        return;
    }
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
    sC.valid = true;
    sC.rs.mode = R.mode;
    sC.rs.geom = R.geom;
    sC.rs.combine = R.combine;
    sC.rs.primLod = R.primLod;
    sC.rs.fogValid = R.fogLutValid;
    memcpy(sC.rs.prim, R.prim, 4);
    memcpy(sC.rs.env, R.env, 4);
    memcpy(sC.rs.fog, R.fogColor, 4);
    memcpy(sC.rs.blend, R.blend, 4);
}

// Uniform rows (column-vector convention) of F * (M * V * P)^T, F = screen rotation + depth range fix:
// x' = y, y' = -x (portrait framebuffer), z' = (z - w) / 2 (near -> -1, far -> 0).
static void upload_palette(const PMeshHdr* mesh) {
    if (sC.palLoaded && sC.palGen == R.genMtx && sC.palN == mesh->numPalette &&
        (mesh->numPalette == 0 || memcmp(sC.palKey, mesh->palette, 2 * mesh->numPalette) == 0)) {
        return;
    }
    sC.palLoaded = true;
    sC.palGen = R.genMtx;
    sC.palN = mesh->numPalette;
    sC.palKey = &mesh->palette[0][0];
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
        cache_reset(); // the 2D layer shares uniform registers and render state
    }
}

static const PMeshHdr sNoPalette; // numPalette 0: every vertex uses the caller's matrix

static u32 sDrawCalls, sDrawTris;
static u64 sTickState, sTickSubmit, sTick2D;
u32 Gfx3D_TakeDrawStats(u32* tris) {
    u32 d = sDrawCalls;
    *tris = sDrawTris;
    sDrawCalls = sDrawTris = 0;
    return d;
}
void Gfx3D_TakeTickStats(u64* state, u64* submit, u64* twoD) {
    *state = sTickState;
    *submit = sTickSubmit;
    *twoD = sTick2D;
    sTickState = sTickSubmit = sTick2D = 0;
}

static void draw_raw(const void* verts, const u16* idx, u32 count, const PMeshHdr* mesh) {
    sDrawCalls++;
    sDrawTris += count / 3;
    u64 t0 = svcGetSystemTick();
    bind_program();
    bool lit = (R.geom & GEOM_LIGHTING) != 0;
    if (sC.verts != verts || sC.lit != lit || sC.verts == NULL) {
        C3D_SetAttrInfo(lit ? &sAttrLit : &sAttrUnlit);
        C3D_BufInfo* bi = C3D_GetBufInfo();
        BufInfo_Init(bi);
        BufInfo_Add(bi, verts, sizeof(PVertex), lit ? 5 : 4, lit ? 0x43210 : 0x3210);
        sC.verts = verts;
        sC.lit = lit;
    }

    float texA[4], texB[4];
    apply_textures(texA, texB);
    if (!sC.valid || memcmp(texA, sC.texA, sizeof(texA)) != 0 || memcmp(texB, sC.texB, sizeof(texB)) != 0) {
        C3D_FVUnifSet(GPU_VERTEX_SHADER, uTexA, texA[0], texA[1], texA[2], texA[3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER, uTexB, texB[0], texB[1], texB[2], texB[3]);
        memcpy(sC.texA, texA, sizeof(texA));
        memcpy(sC.texB, texB, sizeof(texB));
    }
    int texgen = lit && (R.geom & GEOM_TEXGEN);
    if (sC.boolLit != (int)lit || sC.boolTexgen != texgen) {
        C3D_BoolUnifSet(GPU_VERTEX_SHADER, uLit, lit);
        C3D_BoolUnifSet(GPU_VERTEX_SHADER, uTexgen, texgen);
        sC.boolLit = lit;
        sC.boolTexgen = texgen;
    }
    if (lit && (!sC.lightsLoaded || sC.lightGen != R.genLights || sC.lookGen != R.genMtx)) {
        sC.lightsLoaded = true;
        sC.lightGen = R.genLights;
        sC.lookGen = R.genMtx;
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
    u64 t1 = svcGetSystemTick();
    C3D_DrawElements(GPU_TRIANGLES, count, C3D_UNSIGNED_SHORT, idx);
    u64 t2 = svcGetSystemTick();
    sTickState += t1 - t0;
    sTickSubmit += t2 - t1;
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

// ---- textures in the original N64 formats that live in game memory: texels the game builds at runtime
// (signpost shadow mask) or loads from ROM data files (font glyphs, text box frames, item icons), bound to
// a segment or loaded for a texture rectangle. Converted to PICA textures at replay and kept in a cache
// keyed by address, format, size and a hash of the texels, so unchanged data is not converted again.
#define DYNTEX_MAX 192
static struct { const void* src; u16 fmtsiz, w, h; u32 hash, frame; DsTexture t; bool init; } sDyn[DYNTEX_MAX];
static u32 sDynFrame = 1;

static u32 pow2_at_least8(u32 v) {
    u32 p = 8;
    while (p < v && p < 1024) p <<= 1;
    return p;
}

static void n64_texel(const u8* src, int fmt, int siz, int i, u8 out[4]) {
    u8 v;
    u16 c;
    switch ((fmt << 4) | siz) {
        case 0x40: // I4
            v = (src[i >> 1] >> ((i & 1) ? 0 : 4)) & 15;
            v |= v << 4;
            out[0] = out[1] = out[2] = out[3] = v;
            return;
        case 0x41: // I8
            out[0] = out[1] = out[2] = out[3] = src[i];
            return;
        case 0x30: // IA4: 3-bit intensity, 1-bit alpha
            v = (src[i >> 1] >> ((i & 1) ? 0 : 4)) & 15;
            out[0] = out[1] = out[2] = (u8)((v >> 1) * 255 / 7);
            out[3] = (v & 1) ? 0xFF : 0;
            return;
        case 0x31: // IA8
            v = src[i];
            out[0] = out[1] = out[2] = (v >> 4) * 0x11;
            out[3] = (v & 15) * 0x11;
            return;
        case 0x32: // IA16
            out[0] = out[1] = out[2] = src[2 * i];
            out[3] = src[2 * i + 1];
            return;
        case 0x02: // RGBA16 (big-endian 5551)
            c = (src[2 * i] << 8) | src[2 * i + 1];
            out[0] = ((c >> 11) & 31) * 255 / 31;
            out[1] = ((c >> 6) & 31) * 255 / 31;
            out[2] = ((c >> 1) & 31) * 255 / 31;
            out[3] = (c & 1) ? 0xFF : 0;
            return;
        case 0x03: // RGBA32
            memcpy(out, src + 4 * i, 4);
            return;
        default:
            out[0] = out[1] = out[2] = out[3] = 0xFF;
            return;
    }
}

static u32 texel_hash(const u8* p, u32 n) {
    u32 h = 2166136261u;
    for (u32 i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

static const DsTexture* dyn_texture(const void* src, u32 fmt, u32 siz, u32 w, u32 h) {
    if (src == NULL || w == 0 || h == 0 || w > 1024 || h > 1024) {
        return NULL;
    }
    u16 fs = (fmt << 4) | siz;
    u32 bytes = (w * h * (siz == 0 ? 4 : siz == 1 ? 8 : siz == 2 ? 16 : 32) + 7) / 8;
    u32 hash = texel_hash(src, bytes);
    int slot = -1;
    for (int i = 0; i < DYNTEX_MAX; i++) {
        if (sDyn[i].init && sDyn[i].src == src && sDyn[i].fmtsiz == fs && sDyn[i].w == w && sDyn[i].h == h &&
            sDyn[i].hash == hash) {
            sDyn[i].frame = sDynFrame;
            return &sDyn[i].t;
        }
        // victim: a slot not used by this frame, least recently used (never-used slots first)
        if (sDyn[i].frame != sDynFrame && (slot < 0 || sDyn[i].frame < sDyn[slot].frame)) {
            slot = i;
        }
    }
    if (slot < 0) {
        return NULL; // more distinct textures in one frame than the cache holds
    }
    u32 W = pow2_at_least8(w), H = pow2_at_least8(h);
    // same format choice as the offline converter (tools/dl2pica.py pica_format)
    GPU_TEXCOLOR pf = fmt == 0 ? (siz == 3 ? GPU_RGBA8 : GPU_RGBA5551)
                               : (siz == 0 || (fmt == 3 && siz == 1)) ? GPU_LA4 : GPU_LA8;
    DsTexture* t = &sDyn[slot].t;
    if (sDyn[slot].init && (t->w != W || t->h != H || t->format != pf)) {
        C3D_TexDelete(&t->tex);
        sDyn[slot].init = false;
    }
    if (!sDyn[slot].init) {
        if (!C3D_TexInit(&t->tex, W, H, pf)) {
            return NULL;
        }
        sDyn[slot].init = true;
        t->w = W;
        t->h = H;
        t->format = pf;
        t->state = 1;
    }
    u8* dst = t->tex.data;
    int bpp = pf == GPU_RGBA8 ? 4 : pf == GPU_LA4 ? 1 : 2;
    for (u32 y = 0; y < H; y++) {
        for (u32 x = 0; x < W; x++) {
            u8 c[4];
            n64_texel(src, fmt, siz, (y % h) * w + (x % w), c);
            u32 m = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
            u8* o = dst + (((y >> 3) * (W >> 3) + (x >> 3)) * 64 + m) * bpp; // 8x8 tiles, Morton order inside
            if (pf == GPU_RGBA8) {
                o[0] = c[3];
                o[1] = c[2];
                o[2] = c[1];
                o[3] = c[0];
            } else if (pf == GPU_RGBA5551) {
                u16 v = ((c[0] >> 3) << 11) | ((c[1] >> 3) << 6) | ((c[2] >> 3) << 1) | (c[3] >= 128);
                o[0] = v;
                o[1] = v >> 8;
            } else if (pf == GPU_LA8) {
                o[0] = c[3];
                o[1] = c[0];
            } else {
                o[0] = (c[0] & 0xF0) | (c[3] >> 4);
            }
        }
    }
    C3D_TexFlush(&t->tex);
    sDyn[slot].src = src;
    sDyn[slot].fmtsiz = fs;
    sDyn[slot].w = w;
    sDyn[slot].h = h;
    sDyn[slot].hash = hash;
    sDyn[slot].frame = sDynFrame;
    return t;
}

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
                else if (o->op == 4) R.mode = (R.mode & ~o->mode[0]) | (o->mode[1] & o->mode[0]);
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
                    const Segment* sg = &R.seg[id & 15];
                    if (sg->kind == SEG_TEX) {
                        t->tex = sg->tex;
                    } else if (sg->kind == SEG_RAW && sg->src != 0) {
                        t->tex = dyn_texture((const void*)(uintptr_t)sg->src, (op[2] >> 8) & 15, (op[2] >> 12) & 15,
                                             op[3] & 0xFFFF, op[3] >> 16);
                    } else {
                        t->tex = NULL;
                    }
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
    R.genMtx++;
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
    R.genMtx++;
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
static const u8* sRawSeg[16]; // see Gfx3D_RawSegment

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

// RDP render-mode word (other-mode low bits 3..31) -> native MODE_* fields (same decode as tools/dl2pica.py)
static u32 mode_from_rm(u32 l) {
    u32 v = 0;
    v |= ((l >> 4) & 1) << 6;
    v |= ((l >> 5) & 1) << 7;
    v |= ((l >> 10) & 3) << 8;
    v |= ((l >> 12) & 1) << 10;
    v |= ((l >> 13) & 1) << 11;
    v |= ((l >> 14) & 1) << 12;
    v |= ((l >> 6) & 1) << 13;
    u32 c1 = (((l >> 30) & 3) << 6) | (((l >> 26) & 3) << 4) | (((l >> 22) & 3) << 2) | ((l >> 18) & 3);
    u32 c2 = (((l >> 28) & 3) << 6) | (((l >> 24) & 3) << 4) | (((l >> 20) & 3) << 2) | ((l >> 16) & 3);
    return v | (c1 << 16) | (c2 << 24);
}

typedef struct { u32 mask, value; } CmdMode;
static void x_mode(const void* p) {
    const CmdMode* c = p;
    R.mode = (R.mode & ~c->mask) | (c->value & c->mask);
}

void Gfx3D_SetRenderMode(u32 c1, u32 c2) {
    u32 mask = mode_from_rm(0xFFFFFFF8u), value = mode_from_rm(c1 | c2);
    if (sRecSeg >= 0) {
        if (sRecCmd != NULL && sRecCmd->s.nops < SEGREC_MAX) {
            SegOp* o = &sRecCmd->s.ops[sRecCmd->s.nops++];
            memset(o, 0, sizeof(*o));
            o->op = 4;
            o->mode[0] = mask;
            o->mode[1] = value;
        }
        return;
    }
    CmdMode* c = rec(x_mode, sizeof(CmdMode));
    if (c) {
        c->mask = mask;
        c->value = value;
    }
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
    sRawSeg[seg] = NULL;
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
    // native sentinel lists (empty list, cull-mode presets): no geometry; or texels the game built at
    // runtime (converted at replay when a list samples this segment)
    CmdSeg* c = rec_seg(seg, SEG_RAW);
    if (c) c->s.src = (u32)(uintptr_t)handle;
    sRawSeg[seg] = handle;
}

typedef struct { float amb[3]; int n; float dir[7][3], col[7][3]; } CmdLights;
static void x_lights(const void* p) {
    const CmdLights* c = p;
    memcpy(R.ambient, c->amb, sizeof(R.ambient));
    R.numLights = c->n;
    memcpy(R.lightDir, c->dir, sizeof(R.lightDir));
    memcpy(R.lightCol, c->col, sizeof(R.lightCol));
    R.genLights++;
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
static struct { int n; MtxF m[FLEX_MAX]; const DsObj* obj[FLEX_MAX]; const PMeshHdr* mesh[FLEX_MAX]; } sFlex;

typedef struct { int n; MtxF m[FLEX_MAX]; const DsObj* obj[FLEX_MAX]; const PMeshHdr* mesh[FLEX_MAX]; } CmdFlex;
static void x_flex(const void* p) {
    const CmdFlex* c = p;
    R.numLimb = c->n;
    memcpy(R.limb, c->m, sizeof(MtxF) * c->n);
    R.genMtx++;
    for (int i = 0; i < c->n; i++) {
        if (c->mesh[i] != NULL) {
            R.base = c->m[i];
            R.genMtx++;
            exec_mesh(c->obj[i], c->mesh[i], 0);
        }
    }
}

void Gfx3D_FlexBegin(void) {
    sFlex.n = 0;
}

void Gfx3D_FlexAdd(const MtxF* m, DListRef dl) {
    if (sFlex.n < FLEX_MAX) {
        // resolved now, against the objects bound at this limb: limb callbacks may rebind segment 6
        // between limbs (multi-object actors: the Kokiri children's head / body / legs)
        sFlex.m[sFlex.n] = *m;
        sFlex.obj[sFlex.n] = NULL;
        sFlex.mesh[sFlex.n] = NULL;
        if (dl != NULL) {
            resolve_dl((u32)(uintptr_t)dl, &sFlex.obj[sFlex.n], &sFlex.mesh[sFlex.n]);
        }
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
        c->obj[i] = sFlex.obj[i];
        c->mesh[i] = sFlex.mesh[i];
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
    R.genMtx++;
}

typedef struct { int seg, n; const MtxF* m; } CmdSegMtxArr;
static void x_segmtxarr(const void* p) {
    const CmdSegMtxArr* c = p;
    for (int i = 0; i < c->n && i < 4; i++) {
        R.segMtx[c->seg][i] = c->m[i]; // frame memory: filled by game code after the bind, read at replay
    }
    R.genMtx++;
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
    sDynFrame++;
    reset_state();
    cache_reset();
    for (int b = 0; b < GFX_BUF_COUNT; b++) {
        u32 pos = 0;
        while (pos < sCmdLen[b]) {
            CmdHdr* h = (CmdHdr*)(sCmd[b] + pos);
            if (h->fn != NULL) {
                bool is2D = R.program3D;
                u64 t0 = svcGetSystemTick();
                h->fn(h + 1);
                if (is2D && !R.program3D) {
                    sTick2D += svcGetSystemTick() - t0; // a 2D command took the GPU (text, rectangles, fills)
                }
            }
            pos += sizeof(CmdHdr) + h->size;
        }
    }
}

// ============================================================================ 2D interface commands
// The original drew text boxes, the HUD and menus as RDP texture rectangles: a texture block load, the
// colour combiner words and gSPTextureRectangle in screen pixels. These are their native equivalents
// (engine/rdp_gbi.h maps the original macros onto them), replayed in order with the other commands.

// G_SETCOMBINE words -> native operands (same tables as tools/dl2pica.py combine_native)
static const u8 kCA[16] = { CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_NOISE,
                            CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO };
static const u8 kCB[16] = { CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_CENTER, CC_K4,
                            CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO };
static const u8 kCC[32] = { CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_SCALE, CC_COMBINED_A,
                            CC_TEXEL0_A, CC_TEXEL1_A, CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD_FRAC, CC_PRIM_LOD_FRAC,
                            CC_K5, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO,
                            CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO, CC_ZERO };
static const u8 kCD[8] = { CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO };
static const u8 kAABD[8] = { CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO };
static const u8 kAC[8] = { CC_LOD_FRAC, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_PRIM_LOD_FRAC, CC_ZERO };

static void x_combine(const void* p) {
    R.combine = *(const GfxCombine*)p;
}

void Gfx3D_SetCombine(u32 w0, u32 w1) {
    GfxCombine* c = rec(x_combine, sizeof(GfxCombine));
    if (c == NULL) {
        return;
    }
    u8* o = &c->c[0][0];
    o[0] = kCA[(w0 >> 20) & 0xF];
    o[1] = kCB[(w1 >> 28) & 0xF];
    o[2] = kCC[(w0 >> 15) & 0x1F];
    o[3] = kCD[(w1 >> 15) & 7];
    o[4] = kAABD[(w0 >> 12) & 7];
    o[5] = kAABD[(w1 >> 12) & 7];
    o[6] = kAC[(w0 >> 9) & 7];
    o[7] = kAABD[(w1 >> 9) & 7];
    o[8] = kCA[(w0 >> 5) & 0xF];
    o[9] = kCB[(w1 >> 24) & 0xF];
    o[10] = kCC[w0 & 0x1F];
    o[11] = kCD[(w1 >> 6) & 7];
    o[12] = kAABD[(w1 >> 21) & 7];
    o[13] = kAABD[(w1 >> 3) & 7];
    o[14] = kAC[(w1 >> 18) & 7];
    o[15] = kAABD[w1 & 7];
}

void Gfx3D_SetAlphaCompare(u32 type) {
    // G_AC_* (other-mode low bits 0-1) -> native MODE_ALPHACMP field (bits 4-5)
    CmdMode* c = rec(x_mode, sizeof(CmdMode));
    if (c) {
        c->mask = 3u << 4;
        c->value = (type & 3) << 4;
    }
}

typedef struct { const void* img; u16 w, h; u8 fmt, siz, cms, cmt, masks, maskt; } CmdLoadBlock;
static CmdLoadBlock sRectTex; // texture memory of the 2D path (replay state)

static void x_loadblock(const void* p) {
    sRectTex = *(const CmdLoadBlock*)p;
}

static void rdp_cancel(void);

void Gfx3D_LoadTextureBlock(const void* img, int fmt, int siz, int w, int h, int pal, int cms, int cmt, int masks,
                            int maskt) {
    (void)pal; // colour-indexed textures are not used by the 2D interface code
    rdp_cancel();
    CmdLoadBlock* c = rec(x_loadblock, sizeof(CmdLoadBlock));
    if (c) {
        c->img = img;
        c->w = w;
        c->h = h;
        c->fmt = fmt;
        c->siz = siz;
        c->cms = cms;
        c->cmt = cmt;
        c->masks = masks;
        c->maskt = maskt;
    }
}


// texture of the 2D path: a handle into a segment the game bound to its own memory (the HUD binds segment 2
// to the interface texture file, as the original did for the RDP) -> those original texels; any other
// handle -> the converted texture; a plain pointer -> original texels in game memory
static const DsTexture* rect_texture(const CmdLoadBlock* b) {
    if (b->img == NULL) {
        return NULL;
    }
    if (ASSET_IS_TOKEN(b->img)) {
        const Segment* sg = &R.seg[ASSET_SEG(b->img)];
        if (sg->kind == SEG_RAW && sg->src != 0) {
            const u8* p = (const u8*)(uintptr_t)sg->src + (ASSET_SRC(b->img) & 0xFFFFFF);
            return dyn_texture(p, b->fmt, b->siz, b->w, b->h);
        }
        return Asset_FindTexture(b->img);
    }
    return dyn_texture(b->img, b->fmt, b->siz, b->w, b->h);
}

typedef struct { s32 xl, yl, xh, yh; s16 s, t, dsdx, dtdy; } CmdTexRect;

static GPU_TEXTURE_WRAP_PARAM rect_wrap(u8 cm, u8 mask) {
    if (mask == 0 || (cm & 2)) { // no wrap period, or G_TX_CLAMP
        return GPU_CLAMP_TO_EDGE;
    }
    return (cm & 1) ? GPU_MIRRORED_REPEAT : GPU_REPEAT;
}

void Gfx2D_BindRectProgram(void);
void Gfx2D_TexQuad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1);

static void x_texrect(const void* p) {
    const CmdTexRect* r = p;
    const CmdLoadBlock* b = &sRectTex;
    const DsTexture* tex = rect_texture(b);
    if (tex == NULL) {
        return;
    }
    Gfx2D_BindRectProgram();
    sC.valid = false;
    apply_render_state(); // combiner, blender, alpha compare from the current state
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR); // rectangles: no depth, no culling, no fog
    C3D_CullFace(GPU_CULL_NONE);
    C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
    sC.valid = false;
    C3D_Tex* ct = (C3D_Tex*)&tex->tex;
    bool bilerp = MODE_TEXFILT(R.mode) != 0;
    C3D_TexSetFilter(ct, bilerp ? GPU_LINEAR : GPU_NEAREST, bilerp ? GPU_LINEAR : GPU_NEAREST);
    C3D_TexSetWrap(ct, rect_wrap(b->cms, b->masks), rect_wrap(b->cmt, b->maskt));
    C3D_TexBind(0, ct);
    sC.bound[0] = NULL;
    // rectangle in screen pixels (10.2), texture start in texels (10.5), step per pixel (5.10)
    float x0 = r->xl / 4.0f, y0 = r->yl / 4.0f, x1 = r->xh / 4.0f, y1 = r->yh / 4.0f;
    float s0 = r->s / 32.0f, t0 = r->t / 32.0f;
    float s1 = s0 + (x1 - x0) * (r->dsdx / 1024.0f), t1 = t0 + (y1 - y0) * (r->dtdy / 1024.0f);
    float W = tex->w, H = tex->h;
    // the GPU samples t = 1 at the first stored row (the image top): t_gpu = 1 - t
    Gfx2D_TexQuad(x0, y0, x1, y1, s0 / W, 1.0f - t0 / H, s1 / W, 1.0f - t1 / H);
}

static void rdp_flush(void);
static void rdp_cancel(void);

void Gfx3D_TextureRectangle(int xl, int yl, int xh, int yh, int tile, int s, int t, int dsdx, int dtdy) {
    (void)tile;
    rdp_flush(); // the render tile; TEXEL1 rectangles are not used by the 2D interface code
    CmdTexRect* c = rec(x_texrect, sizeof(CmdTexRect));
    if (c) {
        c->xl = xl;
        c->yl = yl;
        c->xh = xh;
        c->yh = yh;
        c->s = s;
        c->t = t;
        c->dsdx = dsdx;
        c->dtdy = dtdy;
    }
}

// RDP other-mode words -> native MODE_* fields (same decode as tools/dl2pica.py mode_fields)
static u32 mode_from_othermode(u32 h, u32 l) {
    u32 v = 0;
    v |= (h >> 20) & 3;
    v |= ((h >> 12) & 3) << 2;
    v |= (l & 3) << 4;
    v |= ((l >> 4) & 1) << 6;
    v |= ((l >> 5) & 1) << 7;
    v |= ((l >> 10) & 3) << 8;
    v |= ((l >> 12) & 1) << 10;
    v |= ((l >> 13) & 1) << 11;
    v |= ((l >> 14) & 1) << 12;
    v |= ((l >> 6) & 1) << 13;
    v |= ((h >> 14) & 3) << 14;
    u32 c1 = (((l >> 30) & 3) << 6) | (((l >> 26) & 3) << 4) | (((l >> 22) & 3) << 2) | ((l >> 18) & 3);
    u32 c2 = (((l >> 28) & 3) << 6) | (((l >> 24) & 3) << 4) | (((l >> 20) & 3) << 2) | ((l >> 16) & 3);
    return v | (c1 << 16) | (c2 << 24);
}

void Gfx3D_SetOtherMode(u32 h, u32 l) {
    CmdMode* c = rec(x_mode, sizeof(CmdMode));
    if (c) {
        c->mask = 0xFFFFFFFFu;
        c->value = mode_from_othermode(h, l);
    }
}

// original geometry-mode bits -> native GEOM_* (same map as tools/dl2pica.py GEOM_MAP)
static u32 geom_from_n64(u32 b) {
    u32 g = 0;
    if (b & 0x1) g |= GEOM_ZBUFFER;
    if (b & 0x4) g |= GEOM_SHADE;
    if (b & 0x200) g |= GEOM_CULL_FRONT;
    if (b & 0x400) g |= GEOM_CULL_BACK;
    if (b & 0x10000) g |= GEOM_FOG;
    if (b & 0x20000) g |= GEOM_LIGHTING;
    if (b & 0x40000) g |= GEOM_TEXGEN;
    if (b & 0x80000) g |= GEOM_TEXGEN_LINEAR;
    if (b & 0x200000) g |= GEOM_SMOOTH;
    return g;
}

void Gfx3D_GeometryModeN64(u32 clear, u32 set) {
    Gfx3D_GeometryMode(geom_from_n64(clear), geom_from_n64(set));
}

static void rect_state(void) {
    Gfx2D_BindRectProgram();
    sC.valid = false;
    apply_render_state(); // combiner, blender, alpha compare from the current state
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR); // overlay primitives: no depth, no culling, no fog
    C3D_CullFace(GPU_CULL_NONE);
    C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
    sC.valid = false;
}

typedef struct { s32 ulx, uly, lrx, lry; } CmdFill;
static void x_fillrect(const void* p) {
    const CmdFill* f = p;
    rect_state();
    C3D_TexBind(0, &sWhite);
    sC.bound[0] = NULL;
    // 1/2-cycle mode: the lower right edge is inclusive in pixels
    Gfx2D_TexQuad(f->ulx, f->uly, f->lrx + 1, f->lry + 1, 0, 0, 0, 0);
}

void Gfx3D_FillRectangle(int ulx, int uly, int lrx, int lry) {
    CmdFill* c = rec(x_fillrect, sizeof(CmdFill));
    if (c) {
        c->ulx = ulx;
        c->uly = uly;
        c->lrx = lrx;
        c->lry = lry;
    }
}

// ---- vertex quads under the overlay camera (HUD action button, beating heart). The overlay has its own
// projection / viewing / viewport (View_ApplyPerspectiveToOverlay, View_ApplyOrthoToOverlay); vertices
// are transformed on the CPU at replay (a handful per frame) and drawn as screen-space triangles.
typedef struct { MtxF proj, view; s16 vp[4]; } CmdOvlCam;
static CmdOvlCam sOvlCam;
typedef struct { float x, y, s, t; u8 c[4]; } ScreenVtx;
static ScreenVtx sOvlVtx[32];

static void x_ovlcam(const void* p) {
    sOvlCam = *(const CmdOvlCam*)p;
}

void Gfx3D_SetOverlayCamera(const MtxF* projection, const MtxF* viewing, int leftX, int topY, int rightX,
                            int bottomY) {
    CmdOvlCam* c = Gfx3D_Record(GFX_BUF_OVL, x_ovlcam, sizeof(CmdOvlCam));
    if (c == NULL) {
        return;
    }
    c->proj = *projection;
    if (viewing != NULL) {
        c->view = *viewing;
    } else {
        memset(&c->view, 0, sizeof(MtxF));
        c->view.mf[0][0] = c->view.mf[1][1] = c->view.mf[2][2] = c->view.mf[3][3] = 1.0f;
    }
    c->vp[0] = leftX;
    c->vp[1] = topY;
    c->vp[2] = rightX;
    c->vp[3] = bottomY;
}

// row vector times matrix, the game convention (out[j] = sum in[i] * mf[i][j])
static void vec4_mul(float out[4], const float in[4], const MtxF* m) {
    for (int j = 0; j < 4; j++) {
        out[j] = in[0] * m->mf[0][j] + in[1] * m->mf[1][j] + in[2] * m->mf[2][j] + in[3] * m->mf[3][j];
    }
}

typedef struct { int n, v0; bool overlay; Vtx v[32]; } CmdVtx; // the original vertex cache: 32 entries
static void x_vtx(const void* p) {
    const CmdVtx* c = p;
    float sx = (R.texScaleS == 0xFFFF ? 65536.0f : (float)R.texScaleS) / 65536.0f;
    float sy = (R.texScaleT == 0xFFFF ? 65536.0f : (float)R.texScaleT) / 65536.0f;
    // overlay buffer: the overlay camera; scene buffers: the scene camera over the full screen
    const MtxF* view = c->overlay ? &sOvlCam.view : &R.view;
    const MtxF* proj = c->overlay ? &sOvlCam.proj : &R.proj;
    static const s16 kFull[4] = { 0, 0, 320, 240 };
    const s16* vp = c->overlay ? sOvlCam.vp : kFull;
    float hw = (vp[2] - vp[0]) * 0.5f, hh = (vp[3] - vp[1]) * 0.5f;
    float cx = vp[0] + hw, cy = vp[1] + hh;
    for (int i = 0; i < c->n && c->v0 + i < 32; i++) {
        const Vtx_t* v = &c->v[i].v;
        float a[4] = { v->ob[0], v->ob[1], v->ob[2], 1.0f }, b[4], e[4], clip[4];
        vec4_mul(b, a, &R.base);
        vec4_mul(e, b, view);
        vec4_mul(clip, e, proj);
        float w = clip[3] != 0.0f ? clip[3] : 1.0f;
        ScreenVtx* o = &sOvlVtx[c->v0 + i];
        o->x = cx + clip[0] / w * hw; // viewport: y grows downwards on screen
        o->y = cy - clip[1] / w * hh;
        o->s = v->tc[0] / 32.0f * sx;
        o->t = v->tc[1] / 32.0f * sy;
        memcpy(o->c, v->cn, 4);
    }
}

void Gfx3D_Vertices(const Vtx* v, int n, int v0) {
    if (n > 32) {
        n = 32;
    }
    CmdVtx* c = rec(x_vtx, offsetof(CmdVtx, v) + sizeof(Vtx) * n);
    if (c) {
        c->n = n;
        c->v0 = v0;
        c->overlay = Gfx3D_GetTarget() == GFX_BUF_OVL;
        memcpy(c->v, v, sizeof(Vtx) * n);
    }
}

void Gfx2D_TexTriangles(const float* xy, const float* uv, const u8* rgba, int n);

typedef struct { u8 i[4]; u8 n; } CmdQuad;
static void x_quad(const void* p) {
    const CmdQuad* q = p;
    const CmdLoadBlock* b = &sRectTex;
    const DsTexture* tex = rect_texture(b);
    rect_state();
    float W = 1.0f, H = 1.0f;
    if (tex != NULL) {
        C3D_Tex* ct = (C3D_Tex*)&tex->tex;
        bool bilerp = MODE_TEXFILT(R.mode) != 0;
        C3D_TexSetFilter(ct, bilerp ? GPU_LINEAR : GPU_NEAREST, bilerp ? GPU_LINEAR : GPU_NEAREST);
        C3D_TexSetWrap(ct, rect_wrap(b->cms, b->masks), rect_wrap(b->cmt, b->maskt));
        C3D_TexBind(0, ct);
        W = tex->w;
        H = tex->h;
    } else {
        C3D_TexBind(0, &sWhite);
    }
    sC.bound[0] = NULL;
    static const int kTri[6] = { 0, 1, 2, 0, 2, 3 };
    float xy[12], uv[12];
    u8 rgba[24];
    for (int k = 0; k < 3 * q->n; k++) {
        const ScreenVtx* v = &sOvlVtx[q->i[kTri[k]] & 31];
        xy[2 * k] = v->x;
        xy[2 * k + 1] = v->y;
        uv[2 * k] = v->s / W;
        uv[2 * k + 1] = 1.0f - v->t / H; // the GPU samples t = 1 at the first stored row
        memcpy(&rgba[4 * k], v->c, 4);
    }
    Gfx2D_TexTriangles(xy, uv, rgba, q->n);
}

void Gfx3D_Quad(int v0, int v1, int v2, int v3) {
    rdp_flush();
    CmdQuad* c = rec(x_quad, sizeof(CmdQuad));
    if (c) {
        c->n = 2;
        c->i[0] = v0;
        c->i[1] = v1;
        c->i[2] = v2;
        c->i[3] = v3;
    }
}

// ============================================================================ original command streams
// Display buffer cursors of sources that keep their RDP commands (engine/rdp_disps.h).
typedef struct Gfx { u32 w0, w1; } Gfx; // as engine/rdp_disps.h

Gfx** Gfx3D_DispCursor(GfxBuf buf) {
    static Gfx sScratch[2];
    static Gfx* sCursor;
    Gfx3D_SetTarget(buf);
    sCursor = sScratch;
    return &sCursor;
}

// Segments the game bound to its own memory, as known while recording (gSPSegment with a plain pointer):
// lists and texels addressed through them are read from there, as the RDP did.

const u8* Gfx3D_RawSegment(int seg) {
    return sRawSeg[seg & 15];
}

// ---- texture state of hand-built loads (gDPSetTextureImage / gDPSetTile* / gDPLoadBlock / gDPLoadTile):
// turned into a texture block load (Gfx3D_LoadTextureBlock) when the next primitive is recorded
static struct {
    const u8* img;
    u8 imgFmt, imgSiz;
    u16 imgWidth;
    const u8* loaded; // image start of the last load
    u16 loadUls, loadUlt;
    struct { u8 fmt, siz, pal, cms, cmt, masks, maskt; u16 line, tmem, uls, ult, lrs, lrt; } tile[8];
    bool dirty;
} sRdp;

static void rdp_cancel(void) {
    sRdp.dirty = false;
}

static void rdp_flush(void) {
    if (!sRdp.dirty || sRdp.loaded == NULL) {
        return;
    }
    sRdp.dirty = false;
    const typeof(sRdp.tile[0])* t = &sRdp.tile[0];
    int w = ((t->lrs - t->uls) >> 2) + 1, h = ((t->lrt - t->ult) >> 2) + 1;
    Gfx3D_LoadTextureBlock(sRdp.loaded, t->fmt, t->siz, w, h, t->pal, t->cms, t->cmt, t->masks, t->maskt);
}

static const u8* rdp_addr(u32 addr) {
    if (ASSET_IS_TOKEN(addr)) {
        const u8* base = sRawSeg[ASSET_SEG(addr)];
        return base != NULL ? base + (ASSET_SRC(addr) & 0xFFFFFF) : (const u8*)(uintptr_t)addr;
    }
    if ((addr >> 24) < 16 && addr != 0) { // a segmented address inside a raw list
        const u8* base = sRawSeg[addr >> 24];
        return base != NULL ? base + (addr & 0xFFFFFF) : NULL;
    }
    return (const u8*)(uintptr_t)addr;
}

void Gfx3D_RdpSetTextureImage(int fmt, int siz, int width, const void* img) {
    sRdp.img = img;
    sRdp.imgFmt = fmt;
    sRdp.imgSiz = siz;
    sRdp.imgWidth = width;
}

static void rdp_set_tile(int tile, int fmt, int siz, int line, int tmem, int pal, int cmt, int maskt, int cms,
                         int masks) {
    typeof(sRdp.tile[0])* t = &sRdp.tile[tile & 7];
    t->fmt = fmt;
    t->siz = siz;
    t->line = line;
    t->tmem = tmem;
    t->pal = pal;
    t->cmt = cmt;
    t->maskt = maskt;
    t->cms = cms;
    t->masks = masks;
    sRdp.dirty = true;
}

static void rdp_set_tile_size(int tile, int uls, int ult, int lrs, int lrt) {
    typeof(sRdp.tile[0])* t = &sRdp.tile[tile & 7];
    t->uls = uls;
    t->ult = ult;
    t->lrs = lrs;
    t->lrt = lrt;
    sRdp.dirty = true;
}

void Gfx3D_RdpSetTileCustom(int fmt, int siz, int uls, int ult, int lrs, int lrt, int pal, int cms, int cmt, int masks,
                            int maskt) {
    rdp_set_tile(0, fmt, siz, 0, 0, pal, cmt, maskt, cms, masks);
    rdp_set_tile_size(0, uls << 2, ult << 2, lrs << 2, lrt << 2);
}

void Gfx3D_RdpLoadTile(int tile, int uls, int ult, int lrs, int lrt) {
    (void)tile;
    (void)lrs;
    (void)lrt;
    // a rectangle of the image: rows of the full image width from (uls, ult) (10.2 coordinates)
    int bpp = sRdp.imgSiz == 0 ? 4 : sRdp.imgSiz == 1 ? 8 : sRdp.imgSiz == 2 ? 16 : 32;
    sRdp.loaded = sRdp.img + ((ult >> 2) * sRdp.imgWidth + (uls >> 2)) * bpp / 8;
    sRdp.dirty = true;
}

static void rdp_load_block(void) {
    sRdp.loaded = sRdp.img;
    sRdp.dirty = true;
}

void Gfx3D_SetOtherModePart(int high, int shift, int len, u32 data) {
    u32 bits = (len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1)) << shift;
    CmdMode* c = rec(x_mode, sizeof(CmdMode));
    if (c) {
        c->mask = high ? mode_from_othermode(bits, 0) : mode_from_othermode(0, bits);
        c->value = high ? mode_from_othermode(data & bits, 0) : mode_from_othermode(0, data & bits);
    }
}

typedef struct { u16 s, t; } CmdTexScale;
static void x_texscale(const void* p) {
    const CmdTexScale* c = p;
    R.texScaleS = c->s;
    R.texScaleT = c->t;
}

// ---- display-list interpreter for 2D lists (title / file select windows, interface lists): the RDP
// state commands, vertex loads and triangles, onto the native recorded commands
static void run_dl(const u8* dl, bool bigEndian, int depth) {
    if (dl == NULL || depth > 8) {
        return;
    }
    for (int k = 0; k < 4096; k++, dl += 8) {
        u32 w0, w1;
        if (bigEndian) {
            w0 = ((u32)dl[0] << 24) | (dl[1] << 16) | (dl[2] << 8) | dl[3];
            w1 = ((u32)dl[4] << 24) | (dl[5] << 16) | (dl[6] << 8) | dl[7];
        } else {
            memcpy(&w0, dl, 4);
            memcpy(&w1, dl + 4, 4);
        }
        switch (w0 >> 24) {
            case 0x01: { // G_VTX: original big-endian records -> native Vtx
                int n = (w0 >> 12) & 0xFF, v0 = ((w0 >> 1) & 0x7F) - n;
                const u8* src = rdp_addr(w1);
                Vtx v[32];
                if (src == NULL || n > 32) {
                    break;
                }
                for (int i = 0; i < n; i++) {
                    const u8* s = src + 16 * i;
                    for (int j = 0; j < 3; j++) v[i].v.ob[j] = (s16)((s[2 * j] << 8) | s[2 * j + 1]);
                    v[i].v.flag = (s[6] << 8) | s[7];
                    v[i].v.tc[0] = (s16)((s[8] << 8) | s[9]);
                    v[i].v.tc[1] = (s16)((s[10] << 8) | s[11]);
                    memcpy(v[i].v.cn, s + 12, 4);
                }
                Gfx3D_Vertices(v, n, v0);
                break;
            }
            case 0x05: // G_TRI1
                Gfx3D_Tri(((w0 >> 16) & 0xFF) / 2, ((w0 >> 8) & 0xFF) / 2, (w0 & 0xFF) / 2);
                break;
            case 0x06: // G_TRI2
            case 0x07: // G_QUAD (as two triangles)
                Gfx3D_Tri(((w0 >> 16) & 0xFF) / 2, ((w0 >> 8) & 0xFF) / 2, (w0 & 0xFF) / 2);
                Gfx3D_Tri(((w1 >> 16) & 0xFF) / 2, ((w1 >> 8) & 0xFF) / 2, (w1 & 0xFF) / 2);
                break;
            case 0xD7: { // G_TEXTURE
                CmdTexScale* c = rec(x_texscale, sizeof(CmdTexScale));
                if (c) {
                    c->s = w1 >> 16;
                    c->t = w1 & 0xFFFF;
                }
                break;
            }
            case 0xD9: // G_GEOMETRYMODE
                Gfx3D_GeometryModeN64(~w0 & 0xFFFFFF, w1);
                break;
            case 0xDE: // G_DL: call, or branch (no return)
                if (bigEndian && (w1 >> 24) < 16 && sRawSeg[w1 >> 24] != NULL) {
                    run_dl(rdp_addr(w1), true, depth + 1);
                } else {
                    Gfx3D_RunDL((const void*)(uintptr_t)w1);
                }
                if ((w0 >> 16) & 0xFF) {
                    return;
                }
                break;
            case 0xDF: // G_ENDDL
                return;
            case 0xE2: // G_SETOTHERMODE_L
            case 0xE3: { // G_SETOTHERMODE_H
                int len = (w0 & 0xFF) + 1, shift = 32 - ((w0 >> 8) & 0xFF) - len;
                Gfx3D_SetOtherModePart((w0 >> 24) == 0xE3, shift, len, w1);
                break;
            }
            case 0xEF: // G_RDPSETOTHERMODE
                Gfx3D_SetOtherMode(w0 & 0xFFFFFF, w1);
                break;
            case 0xF2: // G_SETTILESIZE
                rdp_set_tile_size((w1 >> 24) & 7, (w0 >> 12) & 0xFFF, w0 & 0xFFF, (w1 >> 12) & 0xFFF, w1 & 0xFFF);
                break;
            case 0xF3: // G_LOADBLOCK
                rdp_load_block();
                break;
            case 0xF4: // G_LOADTILE
                Gfx3D_RdpLoadTile((w1 >> 24) & 7, (w0 >> 12) & 0xFFF, w0 & 0xFFF, (w1 >> 12) & 0xFFF, w1 & 0xFFF);
                break;
            case 0xF5: // G_SETTILE
                rdp_set_tile((w1 >> 24) & 7, (w0 >> 21) & 7, (w0 >> 19) & 3, (w0 >> 9) & 0x1FF, w0 & 0x1FF,
                             (w1 >> 20) & 0xF, (w1 >> 18) & 3, (w1 >> 14) & 0xF, (w1 >> 8) & 3, (w1 >> 4) & 0xF);
                break;
            case 0xFA: // G_SETPRIMCOLOR
                Gfx3D_SetPrimColor(w1 >> 24, w1 >> 16, w1 >> 8, w1);
                break;
            case 0xFB: // G_SETENVCOLOR
                Gfx3D_SetEnvColor(w1 >> 24, w1 >> 16, w1 >> 8, w1);
                break;
            case 0xFC: // G_SETCOMBINE
                Gfx3D_SetCombine(w0, w1);
                break;
            case 0xFD: // G_SETTIMG
                Gfx3D_RdpSetTextureImage((w0 >> 21) & 7, (w0 >> 19) & 3, (w0 & 0xFFF) + 1, rdp_addr(w1));
                break;
            case 0xE6: // syncs
            case 0xE7:
            case 0xE8:
            case 0xE9:
            case 0x00: // G_NOOP
                break;
            default: {
                static u8 sLogged[256];
                if (!sLogged[w0 >> 24]) {
                    sLogged[w0 >> 24] = 1;
                    log_printf("gfx3d: list command %02lX not interpreted\n", (unsigned long)(w0 >> 24));
                }
                break;
            }
        }
    }
}

static Gfx sOpenList[4];

Gfx* Gfx3D_OpenList(void) {
    return sOpenList;
}

void Gfx3D_RunDL(const void* dl) {
    if ((const Gfx*)dl >= sOpenList && (const Gfx*)dl < sOpenList + 4) {
        return; // an opened list: its commands are recorded in place (engine/rdp_disps.h)
    }
    u32 a = (u32)(uintptr_t)dl;
    if (ASSET_IS_TOKEN(a)) {
        const u8* base = sRawSeg[ASSET_SEG(a)];
        if (base != NULL) {
            run_dl(base + (ASSET_SRC(a) & 0xFFFFFF), true, 0); // original list in game memory
        } else {
            Gfx3D_DrawDL(a); // converted list
        }
        return;
    }
    run_dl(dl, false, 0); // static list of this program, native word order
}

void Gfx3D_Tri(int v0, int v1, int v2) {
    rdp_flush();
    CmdQuad* c = rec(x_quad, sizeof(CmdQuad));
    if (c) {
        c->n = 1;
        c->i[0] = v0;
        c->i[1] = v1;
        c->i[2] = v2;
        c->i[3] = v2;
    }
}
