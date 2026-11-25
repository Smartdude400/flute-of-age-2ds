// engine/pmesh.h (rewritten: native)
// Mesh programs produced by the offline display-list compiler (tools/dl2pica.py). Every field is already
// decoded into these native enums; the renderer (gfx3d.c) executes the ops in order.
#pragma once
#include "engine/types.h"

// Vertex: 16 bytes, read by the PICA200 vertex shader (gfx3d.v.pica).
typedef struct PVertex {
    s16 x, y, z; // model coordinates (original units)
    u8 mtx;      // index into the mesh's matrix palette
    u8 pad;
    s16 s, t;    // texture coordinates, s10.5 (original units)
    u8 c[4];     // colour (unlit) or signed normal + alpha (lit)
} PVertex;

typedef struct PMeshHdr {
    u32 numVerts, vertsOff;
    u32 numIdx, idxOff;
    u32 numOps, opsOff;
    u8 numPalette;
    u8 vtxSeg; // 0: static vertices; else the segment of a runtime vertex buffer (placeholders: x = index)
    u8 pad[2];
    u8 palette[8][2]; // matrix source: type (MSRC_*), id
} PMeshHdr;

enum { MSRC_BASE, MSRC_LIMB, MSRC_SEGMUL };

// Ops: word 0 = op | a << 8 | b << 16, followed by payload words.
enum {
    OP_END,
    OP_DRAW,      // first index, index count
    OP_GEOM,      // clear mask, set mask (GEOM_*)
    OP_MODE,      // mask, value (MODE_* fields)
    OP_COMBINE,   // 16 bytes: cycle0 rgb a,b,c,d alpha a,b,c,d, cycle1 ... (CC_*)
    OP_PRIM,      // a: lod fraction, b: min level; rgba
    OP_ENV,       // rgba
    OP_TEX,       // a: unit; kind<<16|index|seg, flags|fmt<<8|siz<<12, W|H<<16, uls|ult<<16, shiftS|shiftT<<4
    OP_TEXSCALE,  // a: render tile, b: on; s<<16|t
    OP_CALLSEG,   // a: segment; offset
    OP_LIGHTS,
    OP_FOGCOLOR,  // rgba
    OP_BLENDCOLOR // rgba
};

enum { TEXKIND_NONE, TEXKIND_STATIC, TEXKIND_SEGMENT };
#define TEXF_CLAMP_S 1
#define TEXF_MIRROR_S 2
#define TEXF_CLAMP_T 4
#define TEXF_MIRROR_T 8
#define TEXF_RUNTIME_TILE 16 // tile offsets come from a runtime list (texture scroll)

// Geometry flags (same meaning as the original geometry mode bits)
#define GEOM_ZBUFFER (1 << 0)
#define GEOM_SHADE (1 << 1)
#define GEOM_CULL_FRONT (1 << 2)
#define GEOM_CULL_BACK (1 << 3)
#define GEOM_FOG (1 << 4)
#define GEOM_LIGHTING (1 << 5)
#define GEOM_TEXGEN (1 << 6)
#define GEOM_TEXGEN_LINEAR (1 << 7)
#define GEOM_SMOOTH (1 << 8)

// Render mode word fields
#define MODE_CYCLE(m) ((m) & 3)           // 0 one-cycle, 1 two-cycle, 2 copy, 3 fill
#define MODE_TEXFILT(m) (((m) >> 2) & 3)  // 0 point, 2 bilinear, 3 average
#define MODE_ALPHACMP(m) (((m) >> 4) & 3) // 0 none, 1 threshold, 3 dither
#define MODE_ZCMP (1 << 6)
#define MODE_ZUPD (1 << 7)
#define MODE_ZMODE(m) (((m) >> 8) & 3) // 0 opaque, 1 interpenetrating, 2 translucent, 3 decal
#define MODE_CVG_X_ALPHA (1 << 10)
#define MODE_ALPHA_CVG_SEL (1 << 11)
#define MODE_FORCE_BL (1 << 12)
#define MODE_IM_RD (1 << 13)
#define MODE_TLUT(m) (((m) >> 14) & 3)
#define MODE_BL1(m) (((m) >> 16) & 0xFF) // blender cycle 1: P<<6 | A<<4 | M<<2 | B
#define MODE_BL2(m) (((m) >> 24) & 0xFF) // blender cycle 2
enum { BL_P_IN, BL_P_MEM, BL_P_BLEND, BL_P_FOG };
enum { BL_A_IN, BL_A_FOG, BL_A_SHADE, BL_A_ZERO };
enum { BL_B_1MA, BL_B_MEM, BL_B_ONE, BL_B_ZERO };

// Combiner operands
enum {
    CC_COMBINED, CC_TEXEL0, CC_TEXEL1, CC_PRIM, CC_SHADE, CC_ENV, CC_ONE, CC_ZERO, CC_NOISE, CC_CENTER, CC_K4,
    CC_SCALE, CC_COMBINED_A, CC_TEXEL0_A, CC_TEXEL1_A, CC_PRIM_A, CC_SHADE_A, CC_ENV_A, CC_LOD_FRAC,
    CC_PRIM_LOD_FRAC, CC_K5
};
typedef struct GfxCombine {
    u8 c[2][8]; // per cycle: rgb a, b, c, d, alpha a, b, c, d
} GfxCombine;
