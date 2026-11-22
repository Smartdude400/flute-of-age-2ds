// engine/tev.c (rewritten: native)
// Translates the colour-combiner state (GfxCombine: per cycle (A - B) * C + D for colour and alpha, in
// native CC_* operands) into PICA200 texture-combiner stages.
//
// Sources: TEXEL0/TEXEL1 = texture units 0/1, SHADE = vertex colour, PRIM = the stage constant,
// ENV = the combiner buffer colour (no stage updates the buffer, so every stage reads ENV there),
// COMBINED = the previous stage. 1 / 0 / PRIM_LOD_FRAC / NOISE use the stage constant too.
// Exact forms: lerp (D == B -> INTERPOLATE), multiply-add (B == 0), replace, modulate, (1 - B) * C + D.
// The general form uses SUBTRACT then MULTIPLY_ADD; the PICA clamps the intermediate at 0 where the
// original kept it signed (logged once per combiner as an approximation).
// Two-cycle mode chains cycle 0 then cycle 1 (in cycle 1 TEXEL0/TEXEL1 refer to the other tile, as in
// the original pipeline); one-cycle mode uses the cycle-1 equation.
#include <string.h>
#include "engine/tev.h"
#include "engine/log.h"

enum { K_NONE, K_PRIM, K_ONE, K_ZERO, K_PRIMLOD, K_HALF };

typedef struct {
    u8 src;   // GPU_TEVSRC
    u8 alpha; // read the alpha channel (rgb context) / one-minus flag in bit 1
    u8 k;     // constant kind when src == GPU_CONSTANT
} TSrc;

typedef struct {
    u8 func;
    TSrc s[3];
} TOp;

#define ONE_MINUS 2

static TSrc operand(u8 cc, bool alphaCtx, bool cycle1) {
    TSrc t = { GPU_CONSTANT, 0, K_ZERO };
    if (cycle1) { // second cycle: the texel registers hold the other tile
        if (cc == CC_TEXEL0) cc = CC_TEXEL1; else if (cc == CC_TEXEL1) cc = CC_TEXEL0;
        else if (cc == CC_TEXEL0_A) cc = CC_TEXEL1_A; else if (cc == CC_TEXEL1_A) cc = CC_TEXEL0_A;
    }
    bool a = alphaCtx;
    switch (cc) {
        case CC_COMBINED: t = (TSrc){ GPU_PREVIOUS, a, 0 }; break;
        case CC_TEXEL0: t = (TSrc){ GPU_TEXTURE0, a, 0 }; break;
        case CC_TEXEL1: t = (TSrc){ GPU_TEXTURE1, a, 0 }; break;
        case CC_PRIM: t = (TSrc){ GPU_CONSTANT, a, K_PRIM }; break;
        case CC_SHADE: t = (TSrc){ GPU_PRIMARY_COLOR, a, 0 }; break;
        case CC_ENV: t = (TSrc){ GPU_PREVIOUS_BUFFER, a, 0 }; break;
        case CC_ONE: t = (TSrc){ GPU_CONSTANT, a, K_ONE }; break;
        case CC_NOISE: t = (TSrc){ GPU_CONSTANT, a, K_HALF }; break;
        case CC_COMBINED_A: t = (TSrc){ GPU_PREVIOUS, 1, 0 }; break;
        case CC_TEXEL0_A: t = (TSrc){ GPU_TEXTURE0, 1, 0 }; break;
        case CC_TEXEL1_A: t = (TSrc){ GPU_TEXTURE1, 1, 0 }; break;
        case CC_PRIM_A: t = (TSrc){ GPU_CONSTANT, 1, K_PRIM }; break;
        case CC_SHADE_A: t = (TSrc){ GPU_PRIMARY_COLOR, 1, 0 }; break;
        case CC_ENV_A: t = (TSrc){ GPU_PREVIOUS_BUFFER, 1, 0 }; break;
        case CC_PRIM_LOD_FRAC: t = (TSrc){ GPU_CONSTANT, a, K_PRIMLOD }; break;
        default: t = (TSrc){ GPU_CONSTANT, a, K_ZERO }; break; // LOD_FRAC (no mip chain), K4, K5, CENTER, SCALE
    }
    return t;
}

static bool is_zero(TSrc s) {
    return s.src == GPU_CONSTANT && s.k == K_ZERO;
}
static bool is_one(TSrc s) {
    return s.src == GPU_CONSTANT && s.k == K_ONE;
}
static bool same(TSrc a, TSrc b) {
    return a.src == b.src && a.alpha == b.alpha && a.k == b.k;
}

static const TSrc kPrev = { GPU_PREVIOUS, 0, 0 };

// (A - B) * C + D -> 1 or 2 ops. Returns the count; *approx set when the result is not exact.
static int equation(TSrc A, TSrc B, TSrc C, TSrc D, TOp out[2], bool* approx, bool alphaCtx) {
    TSrc prev = kPrev;
    prev.alpha = alphaCtx;
    if (is_zero(C) || same(A, B)) {
        out[0] = (TOp){ GPU_REPLACE, { D } };
        return 1;
    }
    if (is_zero(B)) {
        if (is_zero(D)) {
            if (is_one(C)) { out[0] = (TOp){ GPU_REPLACE, { A } }; return 1; }
            if (is_one(A)) { out[0] = (TOp){ GPU_REPLACE, { C } }; return 1; }
            out[0] = (TOp){ GPU_MODULATE, { A, C } };
            return 1;
        }
        if (is_one(C)) { out[0] = (TOp){ GPU_ADD, { A, D } }; return 1; }
        if (is_one(A)) { out[0] = (TOp){ GPU_ADD, { C, D } }; return 1; }
        out[0] = (TOp){ GPU_MULTIPLY_ADD, { A, C, D } };
        return 1;
    }
    if (same(D, B)) {
        out[0] = (TOp){ GPU_INTERPOLATE, { A, B, C } };
        return 1;
    }
    if (is_one(A)) { // (1 - B) * C + D
        TSrc nb = B;
        nb.alpha |= ONE_MINUS;
        if (is_zero(D)) { out[0] = (TOp){ GPU_MODULATE, { nb, C } }; return 1; }
        out[0] = (TOp){ GPU_MULTIPLY_ADD, { nb, C, D } };
        return 1;
    }
    if (is_one(C)) { // A - B + D
        if (is_zero(D)) { out[0] = (TOp){ GPU_SUBTRACT, { A, B } }; return 1; }
        *approx = true;
        out[0] = (TOp){ GPU_SUBTRACT, { A, B } };
        out[1] = (TOp){ GPU_ADD, { prev, D } };
        return 2;
    }
    if (is_zero(A)) { // D - B * C
        *approx = D.src == GPU_PREVIOUS;
        out[0] = (TOp){ GPU_MODULATE, { B, C } };
        out[1] = (TOp){ GPU_SUBTRACT, { D, prev } };
        return 2;
    }
    *approx = true; // signed intermediate clamps at 0 on the PICA
    out[0] = (TOp){ GPU_SUBTRACT, { A, B } };
    out[1] = (TOp){ GPU_MULTIPLY_ADD, { prev, C, D } };
    if (C.src == GPU_PREVIOUS || D.src == GPU_PREVIOUS) {
        // COMBINED is no longer the cycle input in the second op: use the exact lerp-free fallback
        out[0] = (TOp){ GPU_MULTIPLY_ADD, { A, C, D } };
        return 1;
    }
    return 2;
}

static u8 rgb_operand(TSrc s) {
    bool om = s.alpha & ONE_MINUS;
    if (s.alpha & 1) {
        return om ? GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA : GPU_TEVOP_RGB_SRC_ALPHA;
    }
    return om ? GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR : GPU_TEVOP_RGB_SRC_COLOR;
}

static u8 alpha_operand(TSrc s) {
    return (s.alpha & ONE_MINUS) ? GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA : GPU_TEVOP_A_SRC_ALPHA;
}

static void add_stage(TevProgram* p, const TOp* rgb, const TOp* a) {
    if (p->numStages >= 6) {
        p->overflow = true;
        return;
    }
    TevStage* st = &p->stage[p->numStages++];
    static const TOp pass = { GPU_REPLACE, { { GPU_PREVIOUS, 0, 0 } } };
    static const TOp passA = { GPU_REPLACE, { { GPU_PREVIOUS, 1, 0 } } };
    if (rgb == NULL) rgb = &pass;
    if (a == NULL) a = &passA;
    st->funcRgb = rgb->func;
    st->funcA = a->func;
    for (int i = 0; i < 3; i++) {
        st->srcRgb[i] = rgb->s[i].src; // unused slots stay 0 (ignored by the function)
        st->opRgb[i] = rgb_operand(rgb->s[i]);
        st->srcA[i] = a->s[i].src;
        st->opA[i] = alpha_operand(a->s[i]);
    }
    // the stage constant: colour part from the rgb op, alpha part from the alpha op (or the rgb op
    // when it reads the constant's alpha)
    u8 kRgb = K_NONE, kA = K_NONE;
    for (int i = 0; i < 3; i++) {
        if (rgb->s[i].src == GPU_CONSTANT) {
            if (rgb->s[i].alpha & 1) {
                if (kA != K_NONE && kA != rgb->s[i].k) p->constConflict = true;
                kA = rgb->s[i].k;
            } else {
                if (kRgb != K_NONE && kRgb != rgb->s[i].k) p->constConflict = true;
                kRgb = rgb->s[i].k;
            }
        }
        if (a->s[i].src == GPU_CONSTANT) {
            if (kA != K_NONE && kA != a->s[i].k) p->constConflict = true;
            kA = a->s[i].k;
        }
    }
    st->constRgb = kRgb;
    st->constA = kA;
}

static void cycle(TevProgram* p, const u8* cc, bool cycle1, bool* approx) {
    TOp rgb[2], al[2];
    int nr = equation(operand(cc[0], false, cycle1), operand(cc[1], false, cycle1), operand(cc[2], false, cycle1),
                      operand(cc[3], false, cycle1), rgb, approx, false);
    int na = equation(operand(cc[4], true, cycle1), operand(cc[5], true, cycle1), operand(cc[6], true, cycle1),
                      operand(cc[7], true, cycle1), al, approx, true);
    // alpha-context sources always read alpha
    for (int i = 0; i < na; i++) {
        for (int j = 0; j < 3; j++) {
            al[i].s[j].alpha |= 1;
        }
    }
    int n = nr > na ? nr : na;
    for (int i = 0; i < n; i++) {
        add_stage(p, i < nr ? &rgb[i] : NULL, i < na ? &al[i] : NULL);
    }
}

void Tev_Compile(TevProgram* p, const GfxCombine* c, bool twoCycle) {
    memset(p, 0, sizeof(*p));
    bool approx = false;
    if (twoCycle) {
        cycle(p, c->c[0], false, &approx);
        cycle(p, c->c[1], true, &approx);
    } else {
        cycle(p, c->c[1], false, &approx);
    }
    p->approx = approx;
}

static u32 kcolor(u8 k, const u8 prim[4], u8 primLod, int ch) {
    switch (k) {
        case K_PRIM: return prim[ch];
        case K_ONE: return 255;
        case K_PRIMLOD: return primLod;
        case K_HALF: return 128;
        default: return 0;
    }
}

void Tev_Apply(const TevProgram* p, const u8 prim[4], const u8 env[4], u8 primLod) {
    for (int i = 0; i < 6; i++) {
        C3D_TexEnv* env6 = C3D_GetTexEnv(i);
        C3D_TexEnvInit(env6);
        if (i >= p->numStages) {
            continue;
        }
        const TevStage* st = &p->stage[i];
        C3D_TexEnvSrc(env6, C3D_RGB, st->srcRgb[0], st->srcRgb[1], st->srcRgb[2]);
        C3D_TexEnvSrc(env6, C3D_Alpha, st->srcA[0], st->srcA[1], st->srcA[2]);
        C3D_TexEnvOpRgb(env6, st->opRgb[0], st->opRgb[1], st->opRgb[2]);
        C3D_TexEnvOpAlpha(env6, st->opA[0], st->opA[1], st->opA[2]);
        C3D_TexEnvFunc(env6, C3D_RGB, st->funcRgb);
        C3D_TexEnvFunc(env6, C3D_Alpha, st->funcA);
        u32 r = kcolor(st->constRgb, prim, primLod, 0), g = kcolor(st->constRgb, prim, primLod, 1),
            b = kcolor(st->constRgb, prim, primLod, 2), a = kcolor(st->constA, prim, primLod, 3);
        C3D_TexEnvColor(env6, r | (g << 8) | (b << 16) | (a << 24));
    }
    C3D_TexEnvBufUpdate(C3D_Both, 0);
    C3D_TexEnvBufColor(env[0] | (env[1] << 8) | (env[2] << 16) | ((u32)env[3] << 24));
}
