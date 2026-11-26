// engine/f3d.c (native) - runtime display-list compiler, see f3d.h.
// Semantics: F3DEX2 / RDP as documented in include/ultra64/gbi.h (opcodes, word layouts); the walk follows the
// command order exactly. Differences to the original hardware that remain are listed in docs/ARCHITECTURE.md.
#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <string.h>
#include "engine/f3d.h"
#include "engine/gfx3d.h"
#include "engine/log.h"

#ifdef PORT_F3D
bool gF3DEnabled = true; // make F3D=1: the original lists compiled at run time
#else
bool gF3DEnabled = false; // default: the offline-compiled meshes (same images in every comparison so far)
#endif

// ---- F3DEX2 opcodes
enum {
    G_SPNOOP = 0x00, G_VTX = 0x01, G_MODIFYVTX = 0x02, G_CULLDL = 0x03, G_BRANCH_Z = 0x04, G_TRI1 = 0x05,
    G_TRI2 = 0x06, G_QUAD = 0x07, G_TEXTURE = 0xD7, G_POPMTX = 0xD8, G_GEOMETRYMODE = 0xD9, G_MTX = 0xDA,
    G_MOVEWORD = 0xDB, G_MOVEMEM = 0xDC, G_DL = 0xDE, G_ENDDL = 0xDF, G_RDPHALF_1 = 0xE1, G_SETOTHERMODE_L = 0xE2,
    G_SETOTHERMODE_H = 0xE3, G_TEXRECT = 0xE4, G_TEXRECTFLIP = 0xE5, G_RDPSETOTHERMODE = 0xEF, G_LOADTLUT = 0xF0,
    G_SETTILESIZE = 0xF2, G_LOADBLOCK = 0xF3, G_LOADTILE = 0xF4, G_SETTILE = 0xF5, G_SETFOGCOLOR = 0xF8,
    G_SETBLENDCOLOR = 0xF9, G_SETPRIMCOLOR = 0xFA, G_SETENVCOLOR = 0xFB, G_SETCOMBINE = 0xFC, G_SETTIMG = 0xFD
};
enum { G_MWO_POINT_RGBA = 0x10, G_MWO_POINT_ST = 0x14 };
enum { FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I };
static const u8 kBits[4] = { 4, 8, 16, 32 };

// segments the game binds per draw: their lists / textures / vertices are resolved by the renderer at replay
static bool dyn_dl_seg(int s) { return s == 0x01 || (s >= 0x08 && s <= 0x0D); }
static bool dyn_tex_seg(int s) { return s >= 0x08 && s <= 0x0C; }
#define VTX_RUNTIME_SEG 0x08 // vertex buffers the game fills each frame (skinned limbs)
#define MTX_LIMB_SEG 0x0D    // flex skeleton limb matrices

// extra matrix sources of runtime meshes (pmesh.h MSRC_*): a matrix of the asset data, multiplied or loaded
#define MSRC_CONSTMUL 3
#define MSRC_CONSTLOAD 4

// ---------------------------------------------------------------- texture state (per buffer, record time)
typedef struct {
    u8 set, fmt, siz, pal, cms, cmt, masks, maskt, shifts, shiftt;
    u16 line, tmem, uls, ult, lrs, lrt;
} Tile;
typedef struct {
    u8 used, block, siz, dynSeg;
    u16 tmem, width, uls, ult;
    u32 bytes; // LOADBLOCK: bytes loaded (0 = unknown)
    const u8* addr;
} TmemLoad;
typedef struct {
    Tile tiles[8];
    TmemLoad tmem[8];
    const u8* tlut;
    u32 omh;
    u8 texTile;
    u8 dynsize; // tiles whose size a runtime list may set (bit per tile)
    // image of the last SETTIMG
    const u8* img;
    u8 imgFmt, imgSiz, imgDynSeg;
    u16 imgWidth;
} TexState;

#define NBUF (GFX_BUF_COUNT + 1) // + the state of lists called at replay
static TexState sState[NBUF];
static u32 sStateHash[NBUF]; // hash of sState (valid when sStateHashOk)
static bool sStateHashOk[NBUF];

static u32 hash_bytes(const void* p, size_t n, u32 h) {
    const u8* b = p;
    for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}

static TmemLoad* tmem_find(TexState* st, u16 tmem, bool create) {
    for (int i = 0; i < 8; i++) {
        if (st->tmem[i].used && st->tmem[i].tmem == tmem) return &st->tmem[i];
    }
    if (!create) return NULL;
    for (int i = 0; i < 8; i++) {
        if (!st->tmem[i].used) {
            memset(&st->tmem[i], 0, sizeof(TmemLoad));
            st->tmem[i].used = 1;
            st->tmem[i].tmem = tmem;
            return &st->tmem[i];
        }
    }
    memmove(&st->tmem[0], &st->tmem[1], sizeof(TmemLoad) * 7); // oldest load replaced
    memset(&st->tmem[7], 0, sizeof(TmemLoad));
    st->tmem[7].used = 1;
    st->tmem[7].tmem = tmem;
    return &st->tmem[7];
}

// ---------------------------------------------------------------- segments
static const u8* seg_ptr(u32 addr, u32 need) {
    int seg = (addr >> 24) & 15;
    u32 off = addr & 0xFFFFFF;
    DsObj* o = Gfx3D_GetSegmentObj(seg);
    if (o != NULL && o->raw != NULL) {
        return (off + need <= o->rawSize) ? o->raw + off : NULL;
    }
    const u8* r = Gfx3D_RawSegment(seg);
    return r != NULL ? r + off : NULL;
}

static u16 be16(const u8* p) { return (u16)(p[0] << 8 | p[1]); }
static u32 be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

// ---------------------------------------------------------------- textures (decoded from the original bytes)
typedef struct {
    const u8* addr;
    const u8* tlut;
    u16 w, h, stride, availRows, W, H;
    u8 fmt, siz, pal, tlutIA, clampS, clampT, maskS, maskT;
} TexKey;
#define TEXCACHE 2048
static struct { TexKey k; DsTexture* t; } sTex[TEXCACHE];
static int sNumTex;

static u32 pow2ceil(u32 v) {
    u32 p = 8;
    while (p < v) p <<= 1;
    return p;
}

static void texel(const TexKey* k, const u8* row, int x, u8 out[4]) {
    u32 v;
    switch (k->siz) {
        case 0: v = (row[x >> 1] >> ((x & 1) ? 0 : 4)) & 0xF; break;
        case 1: v = row[x]; break;
        case 2: v = be16(row + 2 * x); break;
        default: v = be32(row + 4 * x); break;
    }
    if (k->fmt == FMT_CI) {
        u16 e = 0;
        if (k->tlut != NULL) {
            int idx = k->siz == 0 ? (k->pal << 4) + (int)v : (int)(v & 0xFF);
            e = be16(k->tlut + 2 * idx);
        }
        if (k->tlutIA) {
            out[0] = out[1] = out[2] = e >> 8;
            out[3] = e & 0xFF;
        } else {
            v = e;
            out[0] = ((v >> 11) & 31) * 255 / 31;
            out[1] = ((v >> 6) & 31) * 255 / 31;
            out[2] = ((v >> 1) & 31) * 255 / 31;
            out[3] = (v & 1) ? 255 : 0;
        }
        return;
    }
    if (k->fmt == FMT_RGBA) {
        if (k->siz == 3) {
            out[0] = v >> 24; out[1] = v >> 16; out[2] = v >> 8; out[3] = v;
        } else {
            out[0] = ((v >> 11) & 31) * 255 / 31;
            out[1] = ((v >> 6) & 31) * 255 / 31;
            out[2] = ((v >> 1) & 31) * 255 / 31;
            out[3] = (v & 1) ? 255 : 0;
        }
        return;
    }
    if (k->fmt == FMT_IA) {
        u8 i, a;
        if (k->siz == 0) { i = ((v >> 1) & 7) * 255 / 7; a = (v & 1) ? 255 : 0; }
        else if (k->siz == 1) { i = (v >> 4) * 17; a = (v & 15) * 17; }
        else { i = v >> 8; a = v & 0xFF; }
        out[0] = out[1] = out[2] = i;
        out[3] = a;
        return;
    }
    // I (and YUV, unused): intensity in colour and alpha
    u8 i = k->siz == 0 ? (u8)(v * 17) : (u8)v;
    out[0] = out[1] = out[2] = out[3] = i;
}

static GPU_TEXCOLOR pica_format(const TexKey* k) {
    if (k->fmt == FMT_RGBA) return k->siz == 3 ? GPU_RGBA8 : GPU_RGBA5551;
    if (k->fmt == FMT_CI) return k->tlutIA ? GPU_LA8 : GPU_RGBA5551;
    if (k->fmt == FMT_IA) return k->siz == 2 ? GPU_LA8 : GPU_LA4;
    return k->siz == 0 ? GPU_LA4 : GPU_LA8;
}

static const DsTexture* get_texture(const TexKey* k) {
    for (int i = 0; i < sNumTex; i++) {
        if (memcmp(&sTex[i].k, k, sizeof(TexKey)) == 0) return sTex[i].t;
    }
    if (sNumTex >= TEXCACHE) {
        static bool warned;
        if (!warned) { warned = true; log_printf("f3d: texture cache full\n"); }
        return NULL;
    }
    GPU_TEXCOLOR pf = pica_format(k);
    DsTexture* t = calloc(1, sizeof(DsTexture));
    if (t == NULL || !C3D_TexInit(&t->tex, k->W, k->H, pf)) {
        free(t);
        return NULL;
    }
    t->w = k->W;
    t->h = k->H;
    t->format = pf;
    t->state = 1;
    int bpp = pf == GPU_RGBA8 ? 4 : pf == GPU_LA4 ? 1 : 2;
    u8* dst = t->tex.data;
    for (u32 y = 0; y < k->H; y++) {
        // wrap / clamp as the RDP addresses the tile (see tools/dl2pica.py convert_texture)
        u32 sy = (y < k->h || !k->clampT) ? y % k->h : k->h - 1u;
        if (k->maskT && (1u << k->maskT) < k->h) sy = y % (1u << k->maskT);
        const u8* row = k->addr + (sy % k->availRows) * k->stride; // rows past the loaded ones repeat them
        for (u32 x = 0; x < k->W; x++) {
            u32 sx = (x < k->w || !k->clampS) ? x % k->w : k->w - 1u;
            if (k->maskS && (1u << k->maskS) < k->w) sx = x % (1u << k->maskS);
            u8 c[4];
            texel(k, row, sx, c);
            u32 m = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
            u8* o = dst + (((y >> 3) * (k->W >> 3) + (x >> 3)) * 64 + m) * bpp;
            if (pf == GPU_RGBA8) {
                o[0] = c[3]; o[1] = c[2]; o[2] = c[1]; o[3] = c[0];
            } else if (pf == GPU_RGBA5551) {
                u16 v = ((c[0] >> 3) << 11) | ((c[1] >> 3) << 6) | ((c[2] >> 3) << 1) | (c[3] >= 128);
                o[0] = v; o[1] = v >> 8;
            } else if (pf == GPU_LA8) {
                o[0] = c[3]; o[1] = c[0];
            } else {
                o[0] = (c[0] & 0xF0) | (c[3] >> 4);
            }
        }
    }
    C3D_TexFlush(&t->tex);
    sTex[sNumTex].k = *k;
    sTex[sNumTex].t = t;
    sNumTex++;
    return t;
}

// ---------------------------------------------------------------- compile output
#define MAX_VERTS 4096
#define MAX_IDX 12288
#define MAX_OPS 8192
static PVertex sVerts[MAX_VERTS];
static u16 sIdx[MAX_IDX];
static u32 sOps[MAX_OPS];
static MtxF sConst[8];
static int sNumVerts, sNumIdx, sNumOps, sNumConst, sBatch;
static u8 sPal[8][2];
static int sNumPal;
static bool sFail;
static u8 sVtxSeg;

static void flush_batch(void) {
    if (sBatch >= 0 && sNumIdx > sBatch) {
        if (sNumOps + 3 > MAX_OPS) { sFail = true; return; }
        sOps[sNumOps++] = OP_DRAW;
        sOps[sNumOps++] = sBatch;
        sOps[sNumOps++] = sNumIdx - sBatch;
    }
    sBatch = -1;
}

static void op(int n, const u32* w) {
    flush_batch();
    if (sNumOps + n > MAX_OPS) { sFail = true; return; }
    memcpy(&sOps[sNumOps], w, n * 4);
    sNumOps += n;
}

static int pal_index(u8 type, u8 id) {
    for (int i = 0; i < sNumPal; i++) {
        if (sPal[i][0] == type && sPal[i][1] == id) return i;
    }
    if (sNumPal >= 8) { sFail = true; return 0; }
    sPal[sNumPal][0] = type;
    sPal[sNumPal][1] = id;
    return sNumPal++;
}

// ---- other modes / combiner / geometry, decoded as tools/dl2pica.py does
static u32 mode_fields(u32 omh, u32 oml) {
    u32 v = 0;
    v |= (omh >> 20) & 3;
    v |= ((omh >> 12) & 3) << 2;
    v |= (oml & 3) << 4;
    v |= ((oml >> 4) & 1) << 6;
    v |= ((oml >> 5) & 1) << 7;
    v |= ((oml >> 10) & 3) << 8;
    v |= ((oml >> 12) & 1) << 10;
    v |= ((oml >> 13) & 1) << 11;
    v |= ((oml >> 14) & 1) << 12;
    v |= ((oml >> 6) & 1) << 13;
    v |= ((omh >> 14) & 3) << 14;
    u32 c1 = (((oml >> 30) & 3) << 6) | (((oml >> 26) & 3) << 4) | (((oml >> 22) & 3) << 2) | ((oml >> 18) & 3);
    u32 c2 = (((oml >> 28) & 3) << 6) | (((oml >> 24) & 3) << 4) | (((oml >> 20) & 3) << 2) | ((oml >> 16) & 3);
    return v | (c1 << 16) | (c2 << 24);
}

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

static void combine_native(u32 w0, u32 w1, u8 out[16]) {
    u32 a0 = (w0 >> 20) & 0xF, c0 = (w0 >> 15) & 0x1F, Aa0 = (w0 >> 12) & 7, Ac0 = (w0 >> 9) & 7;
    u32 a1 = (w0 >> 5) & 0xF, c1 = w0 & 0x1F;
    u32 b0 = (w1 >> 28) & 0xF, b1 = (w1 >> 24) & 0xF, Aa1 = (w1 >> 21) & 7, Ac1 = (w1 >> 18) & 7;
    u32 d0 = (w1 >> 15) & 7, Ab0 = (w1 >> 12) & 7, Ad0 = (w1 >> 9) & 7, d1 = (w1 >> 6) & 7, Ab1 = (w1 >> 3) & 7,
        Ad1 = w1 & 7;
    const u8 v[16] = { kCA[a0], kCB[b0], kCC[c0], kCD[d0], kAABD[Aa0], kAABD[Ab0], kAC[Ac0], kAABD[Ad0],
                       kCA[a1], kCB[b1], kCC[c1], kCD[d1], kAABD[Aa1], kAABD[Ab1], kAC[Ac1], kAABD[Ad1] };
    memcpy(out, v, 16);
}

static u32 geom_native(u32 bits) {
    u32 o = 0;
    if (bits & 0x1) o |= GEOM_ZBUFFER;
    if (bits & 0x4) o |= GEOM_SHADE;
    if (bits & 0x200) o |= GEOM_CULL_FRONT;
    if (bits & 0x400) o |= GEOM_CULL_BACK;
    if (bits & 0x10000) o |= GEOM_FOG;
    if (bits & 0x20000) o |= GEOM_LIGHTING;
    if (bits & 0x40000) o |= GEOM_TEXGEN;
    if (bits & 0x80000) o |= GEOM_TEXGEN_LINEAR;
    if (bits & 0x200000) o |= GEOM_SMOOTH;
    return o;
}

// ---- texture binding of one unit at a draw (OP_TEX words) - tools/dl2pica.py tile_texture
#define TEXKIND_PTR 3
static void tile_texture(TexState* st, int tileNo, u32 w[6]) {
    memset(w, 0, 24);
    w[0] = OP_TEX;
    const Tile* t = &st->tiles[tileNo & 7];
    if (!t->set) return;
    const TmemLoad* ld = tmem_find(st, t->tmem, false);
    if (ld == NULL) return;
    u32 tw = t->lrs > t->uls ? ((t->lrs - t->uls) >> 2) + 1 : 0;
    u32 th = t->lrt > t->ult ? ((t->lrt - t->ult) >> 2) + 1 : 0;
    bool dyn = (st->dynsize >> tileNo) & 1;
    if (tw == 0 || dyn) tw = t->masks ? (1u << t->masks) : (tw ? tw : 1);
    if (th == 0 || dyn) th = t->maskt ? (1u << t->maskt) : (th ? th : 1);
    if (t->masks && !(t->cms & 2) && tw > (1u << t->masks)) tw = 1u << t->masks;
    if (t->maskt && !(t->cmt & 2) && th > (1u << t->maskt)) th = 1u << t->maskt;
    u32 flags = (((t->cms & 2) || !t->masks) ? TEXF_CLAMP_S : 0) | ((t->cms & 1) && t->masks ? TEXF_MIRROR_S : 0) |
                (((t->cmt & 2) || !t->maskt) ? TEXF_CLAMP_T : 0) | ((t->cmt & 1) && t->maskt ? TEXF_MIRROR_T : 0) |
                (dyn ? TEXF_RUNTIME_TILE : 0);
    u32 tile_words = (t->uls & 0xFFFF) | ((u32)t->ult << 16);
    u32 shifts = t->shifts | (t->shiftt << 4);
    if (ld->dynSeg) { // a texture the game binds per draw: resolved at replay
        w[1] = (TEXKIND_SEGMENT << 16) | ld->dynSeg;
        w[2] = flags | (t->fmt << 8) | (t->siz << 12);
        w[3] = (tw & 0xFFFF) | (th << 16);
        w[4] = tile_words;
        w[5] = shifts;
        return;
    }
    if (ld->addr == NULL) return;
    TexKey k;
    memset(&k, 0, sizeof(k));
    u32 bits = kBits[t->siz & 3];
    const u8* addr;
    u32 stride;
    if (ld->block) {
        stride = t->line ? t->line * 8u : tw * bits / 8u;
        addr = ld->addr;
    } else { // LOADTILE: a rectangle of a wider image
        stride = ld->width * kBits[ld->siz & 3] / 8u;
        addr = ld->addr + (ld->ult >> 2) * stride + ((ld->uls >> 2) * kBits[ld->siz & 3] / 8u);
    }
    if (stride == 0) stride = 1;
    u32 rowBytes = tw * bits / 8u;
    if (rowBytes == 0) rowBytes = 1;
    u32 avail = th;
    if (ld->block && ld->bytes) {
        u32 r = ld->bytes / rowBytes;
        avail = r < 1 ? 1 : (r < th ? r : th);
    }
    if (t->fmt == FMT_CI && st->tlut == NULL) return; // palette bound at run time: not supported here
    k.addr = addr;
    k.tlut = t->fmt == FMT_CI ? st->tlut : NULL;
    k.w = tw;
    k.h = th;
    k.stride = stride;
    k.availRows = avail;
    k.fmt = t->fmt;
    k.siz = t->siz;
    k.pal = t->fmt == FMT_CI && t->siz == 0 ? t->pal : 0;
    k.tlutIA = t->fmt == FMT_CI && ((st->omh >> 14) & 3) == 3;
    k.maskS = t->masks;
    k.maskT = t->maskt;
    k.clampS = (t->cms & 2) || !t->masks;
    k.clampT = (t->cmt & 2) || !t->maskt;
    u32 W = t->masks ? pow2ceil(1u << t->masks) : pow2ceil(tw);
    u32 H = t->maskt ? pow2ceil(1u << t->maskt) : pow2ceil(th);
    k.W = W > 1024 ? 1024 : W;
    k.H = H > 1024 ? 1024 : H;
    const DsTexture* tex = get_texture(&k);
    if (tex == NULL) return;
    w[1] = TEXKIND_PTR << 16;
    w[2] = flags | (t->fmt << 8) | (t->siz << 12);
    w[3] = (u32)(uintptr_t)tex;
    w[4] = tile_words;
    w[5] = shifts;
}

// ---------------------------------------------------------------- the walk
typedef struct { u8 type, id; } MSrc;

static void read_mtx(const u8* p, MtxF* m) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            s32 hi = (s16)be16(p + 2 * (i * 4 + j));
            u32 lo = be16(p + 32 + 2 * (i * 4 + j));
            m->mf[i][j] = (f32)((hi << 16) | lo) / 65536.0f;
        }
    }
}

static void compile(TexState* st, const u8* start, u32 startAddr) {
    int vtx[64];
    for (int i = 0; i < 64; i++) vtx[i] = -1;
    MSrc mstack[16] = { { MSRC_BASE, 0 } };
    int msp = 0;
    u32 emitted[2][6];
    bool emittedValid[2] = { false, false };
    u32 rdphalf = 0;
    const u8* stack[18];
    int sp = 0;
    const u8* pc = start;
    (void)startAddr;
    for (int steps = 0; pc != NULL && !sFail; steps++) {
        if (steps > 100000) { sFail = true; break; }
        u32 w0 = be32(pc), w1 = be32(pc + 4);
        u8 opc = w0 >> 24;
        pc += 8;
        switch (opc) {
            case G_VTX: {
                int n = (w0 >> 12) & 0xFF, v0 = ((w0 >> 1) & 0x7F) - n;
                if (v0 < 0 || v0 + n > 64) { sFail = true; break; }
                int seg = (w1 >> 24) & 15;
                int src = pal_index(mstack[msp].type, mstack[msp].id);
                if (seg == VTX_RUNTIME_SEG) { // placeholders: x = index into the game's buffer (pmesh.h vtxSeg)
                    if ((sVtxSeg == 0 && sNumVerts > 0) || sNumVerts + n > MAX_VERTS) { sFail = true; break; }
                    sVtxSeg = seg;
                    u32 base = (w1 & 0xFFFFFF) / 16;
                    for (int i = 0; i < n; i++) {
                        PVertex* v = &sVerts[sNumVerts];
                        memset(v, 0, sizeof(*v));
                        v->x = (s16)(base + i);
                        v->mtx = src;
                        vtx[v0 + i] = sNumVerts++;
                    }
                    break;
                }
                const u8* d = seg_ptr(w1, n * 16);
                if (d == NULL || sVtxSeg != 0 || sNumVerts + n > MAX_VERTS) { sFail = true; break; }
                for (int i = 0; i < n; i++, d += 16) {
                    PVertex* v = &sVerts[sNumVerts];
                    v->x = (s16)be16(d);
                    v->y = (s16)be16(d + 2);
                    v->z = (s16)be16(d + 4);
                    v->mtx = src;
                    v->pad = 0;
                    v->s = (s16)be16(d + 8);
                    v->t = (s16)be16(d + 10);
                    memcpy(v->c, d + 12, 4);
                    vtx[v0 + i] = sNumVerts++;
                }
                break;
            }
            case G_MODIFYVTX: { // a loaded vertex's colour or texture coordinates replaced (a copy: earlier
                                // triangles keep the old one)
                int where = (w0 >> 16) & 0xFF, i = (w0 & 0xFFFF) / 2;
                if (i >= 64 || vtx[i] < 0 || sNumVerts >= MAX_VERTS) break;
                PVertex v = sVerts[vtx[i]];
                if (where == G_MWO_POINT_RGBA) {
                    v.c[0] = w1 >> 24; v.c[1] = w1 >> 16; v.c[2] = w1 >> 8; v.c[3] = w1;
                } else if (where == G_MWO_POINT_ST) {
                    v.s = (s16)(w1 >> 16);
                    v.t = (s16)w1;
                } else {
                    break; // screen position / depth: not used by the game's lists
                }
                sVerts[sNumVerts] = v;
                vtx[i] = sNumVerts++;
                break;
            }
            case G_TRI1: case G_TRI2: case G_QUAD: {
                for (int u = 0; u < 2; u++) {
                    u32 w[6];
                    tile_texture(st, (st->texTile + u) & 7, w);
                    w[0] |= u << 8;
                    if (!emittedValid[u] || memcmp(w, emitted[u], 24) != 0) {
                        op(6, w);
                        memcpy(emitted[u], w, 24);
                        emittedValid[u] = true;
                    }
                }
                u32 tri[2] = { w0, w1 };
                for (int k = 0; k < (opc == G_TRI1 ? 1 : 2); k++) {
                    int a = vtx[((tri[k] >> 16) & 0xFF) / 2 & 63], b = vtx[((tri[k] >> 8) & 0xFF) / 2 & 63],
                        c = vtx[(tri[k] & 0xFF) / 2 & 63];
                    if (a < 0 || b < 0 || c < 0) continue;
                    if (sNumIdx + 3 > MAX_IDX) { sFail = true; break; }
                    if (sBatch < 0) sBatch = sNumIdx;
                    sIdx[sNumIdx++] = a;
                    sIdx[sNumIdx++] = b;
                    sIdx[sNumIdx++] = c;
                }
                break;
            }
            case G_DL: {
                int seg = (w1 >> 24) & 15;
                if (dyn_dl_seg(seg)) {
                    u32 w[2] = { OP_CALLSEG | (seg << 8), w1 & 0xFFFFFF };
                    op(2, w);
                    emittedValid[0] = emittedValid[1] = false; // the called list may change textures / tiles
                    st->dynsize = 0xFF;
                    break;
                }
                const u8* target = seg_ptr(w1, 8);
                if (target == NULL) { sFail = true; break; }
                if (((w0 >> 16) & 0xFF) == 0) {
                    if (sp >= 18) { sFail = true; break; }
                    stack[sp++] = pc;
                }
                pc = target;
                break;
            }
            case G_ENDDL:
                pc = sp > 0 ? stack[--sp] : NULL;
                break;
            case G_BRANCH_Z: { // level of detail: the nearest one (as at close range)
                const u8* target = seg_ptr(rdphalf, 8);
                if (target == NULL) { sFail = true; break; }
                pc = target;
                break;
            }
            case G_RDPHALF_1:
                rdphalf = w1;
                break;
            case G_GEOMETRYMODE: {
                u32 w[3] = { OP_GEOM, geom_native(~w0 & 0xFFFFFF), geom_native(w1) };
                op(3, w);
                break;
            }
            case G_SETOTHERMODE_L: case G_SETOTHERMODE_H: {
                u32 len = (w0 & 0xFF) + 1, sft = 32 - ((w0 >> 8) & 0xFF) - len;
                u32 mask = (len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1)) << sft;
                u32 m;
                if (opc == G_SETOTHERMODE_L) {
                    m = mode_fields(0, mask);
                    u32 w[3] = { OP_MODE, m, mode_fields(st->omh, w1 & mask) & m };
                    op(3, w);
                } else {
                    st->omh = (st->omh & ~mask) | (w1 & mask);
                    m = mode_fields(mask, 0);
                    u32 w[3] = { OP_MODE, m, mode_fields(st->omh, 0) & m };
                    op(3, w);
                }
                break;
            }
            case G_RDPSETOTHERMODE: {
                st->omh = w0 & 0xFFFFFF;
                u32 w[3] = { OP_MODE, 0xFFFFFFFFu, mode_fields(st->omh, w1) };
                op(3, w);
                break;
            }
            case G_SETCOMBINE: {
                u32 w[5];
                w[0] = OP_COMBINE;
                combine_native(w0 & 0xFFFFFF, w1, (u8*)&w[1]);
                op(5, w);
                break;
            }
            case G_TEXTURE: {
                st->texTile = (w0 >> 8) & 7;
                bool on = ((w0 >> 1) & 0x7F) != 0;
                u32 w[2] = { OP_TEXSCALE | (st->texTile << 8) | ((on ? 1u : 0u) << 16), w1 };
                op(2, w);
                break;
            }
            case G_SETTIMG: {
                int seg = (w1 >> 24) & 15;
                st->imgFmt = (w0 >> 21) & 7;
                st->imgSiz = (w0 >> 19) & 3;
                st->imgWidth = (w0 & 0xFFF) + 1;
                st->imgDynSeg = dyn_tex_seg(seg) ? seg : 0;
                st->img = st->imgDynSeg ? NULL : seg_ptr(w1, 1);
                break;
            }
            case G_SETTILE: {
                Tile* t = &st->tiles[(w1 >> 24) & 7];
                u16 uls = t->uls, ult = t->ult, lrs = t->lrs, lrt = t->lrt;
                t->set = 1;
                t->fmt = (w0 >> 21) & 7;
                t->siz = (w0 >> 19) & 3;
                t->line = (w0 >> 9) & 0x1FF;
                t->tmem = w0 & 0x1FF;
                t->pal = (w1 >> 20) & 0xF;
                t->cmt = (w1 >> 18) & 3;
                t->maskt = (w1 >> 14) & 0xF;
                t->shiftt = (w1 >> 10) & 0xF;
                t->cms = (w1 >> 8) & 3;
                t->masks = (w1 >> 4) & 0xF;
                t->shifts = w1 & 0xF;
                t->uls = uls; t->ult = ult; t->lrs = lrs; t->lrt = lrt;
                break;
            }
            case G_SETTILESIZE: {
                int ti = (w1 >> 24) & 7;
                Tile* t = &st->tiles[ti];
                if (!t->set) {
                    memset(t, 0, sizeof(*t));
                    t->set = 1;
                    t->siz = 2;
                }
                t->uls = (w0 >> 12) & 0xFFF;
                t->ult = w0 & 0xFFF;
                t->lrs = (w1 >> 12) & 0xFFF;
                t->lrt = w1 & 0xFFF;
                st->dynsize &= ~(1u << ti);
                break;
            }
            case G_LOADBLOCK: case G_LOADTILE: {
                const Tile* t = &st->tiles[(w1 >> 24) & 7];
                if (!t->set) break;
                TmemLoad* ld = tmem_find(st, t->tmem, true);
                ld->block = opc == G_LOADBLOCK;
                ld->addr = st->img;
                ld->dynSeg = st->imgDynSeg;
                ld->width = st->imgWidth;
                ld->siz = st->imgSiz;
                ld->uls = (w0 >> 12) & 0xFFF;
                ld->ult = w0 & 0xFFF;
                ld->bytes = ld->block ? (((w1 >> 12) & 0xFFF) + 1) * kBits[t->siz & 3] / 8 : 0;
                break;
            }
            case G_LOADTLUT:
                st->tlut = st->img; // (a palette in a per-draw segment is not followed: imgDynSeg)
                break;
            case G_SETPRIMCOLOR: {
                u32 w[2] = { OP_PRIM | ((w0 & 0xFFFF) << 8), w1 };
                op(2, w);
                break;
            }
            case G_SETENVCOLOR: { u32 w[2] = { OP_ENV, w1 }; op(2, w); break; }
            case G_SETFOGCOLOR: { u32 w[2] = { OP_FOGCOLOR, w1 }; op(2, w); break; }
            case G_SETBLENDCOLOR: { u32 w[2] = { OP_BLENDCOLOR, w1 }; op(2, w); break; }
            case G_MTX: {
                u32 p = (w0 & 0xFF) ^ 1;
                bool push = p & 1, load = p & 2, proj = p & 4;
                if (proj) break; // projection: set by the game's view (Gfx3D_SetCamera)
                int seg = (w1 >> 24) & 15;
                MSrc e;
                if (seg == MTX_LIMB_SEG) {
                    e.type = MSRC_LIMB;
                    e.id = (w1 & 0xFFFFFF) / 0x40;
                } else if (dyn_dl_seg(seg)) {
                    e.type = MSRC_SEGMUL;
                    e.id = seg | ((((w1 & 0xFFFFFF) / 0x40) & 0xF) << 4);
                } else { // a matrix of the asset data
                    const u8* m = seg_ptr(w1, 64);
                    if (m == NULL || sNumConst >= 8) { sFail = true; break; }
                    read_mtx(m, &sConst[sNumConst]);
                    e.type = load ? MSRC_CONSTLOAD : MSRC_CONSTMUL;
                    e.id = sNumConst++;
                }
                if (push && msp < 15) msp++;
                mstack[msp] = e;
                break;
            }
            case G_POPMTX: {
                int n = w1 / 64 ? (int)(w1 / 64) : 1;
                while (n-- > 0 && msp > 0) msp--;
                break;
            }
            case G_TEXRECT: case G_TEXRECTFLIP:
                pc += 16; // (not used by the game's 3D lists)
                break;
            default:
                break; // syncs, scissor, fill, movemem / moveword (lights and segments are set by game code)
        }
    }
    flush_batch();
}

// ---------------------------------------------------------------- cache of compiled lists
typedef struct {
    const u8* dl;
    u32 hash;
    u32 lastUsed; // frame (F3D_BeginFrame count) it was last drawn in
    s32 next;     // next entry of the same hash bucket
    u32 exitHash; // hash of exit
    DsObj obj;    // data = the compiled block
    TexState exit;
} Compiled;
#define MESHCACHE 4096
static Compiled* sMesh[MESHCACHE];
static int sNumMesh;
#define BUCKETS 2048
static s32 sBucket[BUCKETS]; // first entry + 1 (0 = empty)

static u32 bucket_of(const u8* dl, u32 hash) {
    u32 h = (u32)(uintptr_t)dl * 2654435761u ^ hash;
    return (h ^ (h >> 13)) & (BUCKETS - 1);
}
static u32 sFrame = 1;
// Flushed entries: the frame being recorded or the GPU may still use them; freed once neither does.
#define RETIRED_MAX 8192
static Compiled* sRetiredMesh[RETIRED_MAX];
static int sNumRetiredMesh;
static DsTexture* sRetiredTex[RETIRED_MAX];
static int sNumRetiredTex;
static u32 sStatCompiled, sStatCached, sStatFailed;

void F3D_Stats(u32* compiled, u32* cached, u32* failed) {
    *compiled = sStatCompiled;
    *cached = sStatCached;
    *failed = sStatFailed;
    sStatCompiled = sStatCached = sStatFailed = 0;
}

static void free_mesh(Compiled* c) {
    linearFree(c->obj.data);
    free(c);
}

void F3D_FlushCache(void) {
    for (int i = 0; i < sNumMesh; i++) {
        if (sNumRetiredMesh < RETIRED_MAX) sRetiredMesh[sNumRetiredMesh++] = sMesh[i];
        else free_mesh(sMesh[i]); // (never reached in practice)
    }
    sNumMesh = 0;
    memset(sBucket, 0, sizeof(sBucket));
    for (int i = 0; i < sNumTex; i++) {
        if (sNumRetiredTex < RETIRED_MAX) sRetiredTex[sNumRetiredTex++] = sTex[i].t;
    }
    sNumTex = 0;
}

void F3D_ReleaseRetired(void) {
    // called at a replay (GPU idle): what the frame being replayed does not use can go
    int keep = 0;
    for (int i = 0; i < sNumRetiredMesh; i++) {
        if (sRetiredMesh[i]->lastUsed >= sFrame) sRetiredMesh[keep++] = sRetiredMesh[i];
        else free_mesh(sRetiredMesh[i]);
    }
    sNumRetiredMesh = keep;
    if (keep == 0) { // textures are used through meshes: free them with the last retired mesh
        for (int i = 0; i < sNumRetiredTex; i++) {
            C3D_TexDelete(&sRetiredTex[i]->tex);
            free(sRetiredTex[i]);
        }
        sNumRetiredTex = 0;
    }
}

void F3D_BeginFrame(void) {
    memset(sState, 0, sizeof(sState));
    memset(sStateHashOk, 0, sizeof(sStateHashOk));
    sFrame++;
}

static Compiled* build(TexState* st, const u8* dl, u32 segAddr, u32 hash) {
    sNumVerts = sNumIdx = sNumOps = sNumConst = sNumPal = 0;
    sBatch = -1;
    sFail = false;
    sVtxSeg = 0;
    compile(st, dl, segAddr);
    if (sNumOps + 1 > MAX_OPS) sFail = true;
    if (sFail) return NULL;
    sOps[sNumOps++] = OP_END;
    // block: header, constant matrices (MSRC_CONST*, read through the header address), vertices, indices, ops
    u32 offConst = (sizeof(PMeshHdr) + 15) & ~15u;
    u32 offVerts = (offConst + sizeof(MtxF) * 8 + 15) & ~15u;
    u32 offIdx = (offVerts + sizeof(PVertex) * sNumVerts + 15) & ~15u;
    u32 offOps = (offIdx + 2 * sNumIdx + 15) & ~15u;
    u32 size = offOps + 4 * sNumOps;
    u8* block = linearMemAlign(size, 0x80);
    Compiled* c = calloc(1, sizeof(Compiled));
    if (block == NULL || c == NULL) {
        if (block) linearFree(block);
        free(c);
        return NULL;
    }
    memset(block, 0, offVerts);
    PMeshHdr* h = (PMeshHdr*)block;
    h->numVerts = sNumVerts;
    h->vertsOff = offVerts;
    h->numIdx = sNumIdx;
    h->idxOff = offIdx;
    h->numOps = sNumOps;
    h->opsOff = offOps;
    h->numPalette = sNumPal;
    h->vtxSeg = sVtxSeg;
    memcpy(h->palette, sPal, sizeof(sPal));
    memcpy(block + offConst, sConst, sizeof(MtxF) * sNumConst);
    memcpy(block + offVerts, sVerts, sizeof(PVertex) * sNumVerts);
    memcpy(block + offIdx, sIdx, 2 * sNumIdx);
    memcpy(block + offOps, sOps, 4 * sNumOps);
    GSPGPU_FlushDataCache(block, size);
    c->dl = dl;
    c->hash = hash;
    c->obj.data = block;
    c->obj.size = size;
    c->exit = *st;
    return c;
}

bool F3D_Resolve(u32 segAddr, int buf, const DsObj** outObj, const PMeshHdr** outMesh) {
    if (!gF3DEnabled) return false;
    int seg = (segAddr >> 24) & 15;
    DsObj* o = Gfx3D_GetSegmentObj(seg);
    if (o == NULL || o->raw == NULL) return false;
    const u8* dl = seg_ptr(segAddr, 8);
    if (dl == NULL) return false;
    TexState* st = &sState[(buf >= 0 && buf < GFX_BUF_COUNT) ? buf : GFX_BUF_COUNT];
    if (buf < 0 || buf >= GFX_BUF_COUNT) memset(st, 0, sizeof(*st)); // lists called at replay: fresh state
    int bi = (int)(st - sState);
    if (buf < 0 || buf >= GFX_BUF_COUNT || !sStateHashOk[bi]) {
        sStateHash[bi] = hash_bytes(st, sizeof(*st), 2166136261u); // only after texture state changed outside lists
        sStateHashOk[bi] = true;
    }
    u32 hash = sStateHash[bi];
    u32 b = bucket_of(dl, hash);
    for (s32 i = sBucket[b] - 1; i >= 0; i = sMesh[i]->next) {
        Compiled* c = sMesh[i];
        if (c->dl == dl && c->hash == hash) {
            c->lastUsed = sFrame;
            *st = c->exit;
            sStateHash[bi] = c->exitHash;
            *outObj = &c->obj;
            *outMesh = (const PMeshHdr*)c->obj.data;
            sStatCached++;
            return true;
        }
    }
    if (sNumMesh >= MESHCACHE) return false;
    TexState entry = *st;
    Compiled* c = build(st, dl, segAddr, hash);
    if (c == NULL) {
        *st = entry; // (hash unchanged)
        sStatFailed++;
        return false;
    }
    c->lastUsed = sFrame;
    c->exitHash = hash_bytes(&c->exit, sizeof(c->exit), 2166136261u);
    sStateHash[bi] = c->exitHash;
    c->next = sBucket[b] - 1;
    sMesh[sNumMesh] = c;
    sBucket[b] = ++sNumMesh;
    sStatCompiled++;
    *outObj = &c->obj;
    *outMesh = (const PMeshHdr*)c->obj.data;
    return true;
}

void F3D_NoteTextureBlock(int buf, int tile, const void* img, int fmt, int siz, int w, int h, int pal, int cms,
                          int cmt, int masks, int maskt) {
    if (buf < 0 || buf >= GFX_BUF_COUNT) return;
    TexState* st = &sState[buf];
    sStateHashOk[buf] = false;
    // gDPLoadTextureBlock: the image loaded at texture memory 0, the render tile pointing at it
    int lineBytes = (w * kBits[siz & 3] + 7) / 8;
    Tile* t = &st->tiles[tile & 7];
    memset(t, 0, sizeof(*t));
    t->set = 1;
    t->fmt = fmt;
    t->siz = siz;
    t->line = (lineBytes + 7) / 8;
    t->tmem = 0;
    t->pal = pal;
    t->cms = cms;
    t->cmt = cmt;
    t->masks = masks;
    t->maskt = maskt;
    t->lrs = (w - 1) << 2;
    t->lrt = (h - 1) << 2;
    TmemLoad* ld = tmem_find(st, 0, true);
    ld->block = 1;
    ld->addr = (const u8*)img;
    ld->dynSeg = 0;
    ld->siz = siz;
    ld->width = w;
    ld->bytes = (u32)lineBytes * h;
    st->dynsize &= ~(1u << (tile & 7));
}
