// engine/rdp_gbi.h (imported + rewritten: native)
// Constants imported from include/ultra64/gbi.h (texture tile flags 3746-3754, image formats 330-347,
// combiner operands and modes 380-496, combiner word layout 3543-3565, geometry / other modes 274-301, 497-591), and
// native versions of the RDP commands the 2D interface code uses (text boxes, HUD). The commands
// ignore their display-list cursor argument (the native renderer records its own command lists):
//   gDPLoadTextureBlock*   -> Gfx3D_LoadTextureBlock (texels: raw original bytes in game memory, or a handle)
//   gSPTextureRectangle    -> Gfx3D_TextureRectangle (original 10.2 / 10.5 / 5.10 fixed-point arguments)
//   gDPSetCombineLERP/Mode -> Gfx3D_SetCombine (the original G_SETCOMBINE words)
#ifndef ENGINE_RDP_GBI_H
#define ENGINE_RDP_GBI_H
#include "engine/gfx3d.h"
#include "engine/rdp_modes.h"
#include "engine/vtx.h"
#include "sys_matrix.h"

#ifndef _SHIFTL
#define _SHIFTL(v, s, w) ((unsigned int)(((unsigned int)(v) & ((0x01 << (w)) - 1)) << (s)))
#endif

#define G_TX_LOADTILE   7
#define G_TX_RENDERTILE 0

#define G_TX_NOMIRROR   (0 << 0)
#define G_TX_WRAP       (0 << 1)
#define G_TX_MIRROR     (1 << 0)
#define G_TX_CLAMP      (1 << 1)
#define G_TX_NOMASK     0
#define G_TX_NOLOD      0


/*
 * G_SETIMG fmt: set image formats
 */
#define G_IM_FMT_RGBA   0
#define G_IM_FMT_YUV    1
#define G_IM_FMT_CI     2
#define G_IM_FMT_IA     3
#define G_IM_FMT_I      4

/*
 * G_SETIMG siz: set image pixel size
 */
#define G_IM_SIZ_4b     0
#define G_IM_SIZ_8b     1
#define G_IM_SIZ_16b    2
#define G_IM_SIZ_32b    3
#define G_IM_SIZ_DD     5

/*
 * G_SETCOMBINE: color combine modes
 */
/* Color combiner constants: */
#define G_CCMUX_COMBINED        0
#define G_CCMUX_TEXEL0          1
#define G_CCMUX_TEXEL1          2
#define G_CCMUX_PRIMITIVE       3
#define G_CCMUX_SHADE           4
#define G_CCMUX_ENVIRONMENT     5
#define G_CCMUX_CENTER          6
#define G_CCMUX_SCALE           6
#define G_CCMUX_COMBINED_ALPHA  7
#define G_CCMUX_TEXEL0_ALPHA    8
#define G_CCMUX_TEXEL1_ALPHA    9
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA     11
#define G_CCMUX_ENV_ALPHA       12
#define G_CCMUX_LOD_FRACTION    13
#define G_CCMUX_PRIM_LOD_FRAC   14
#define G_CCMUX_NOISE           7
#define G_CCMUX_K4              7
#define G_CCMUX_K5              15
#define G_CCMUX_1               6
#define G_CCMUX_0               31

/* Alpha combiner constants: */
#define G_ACMUX_COMBINED        0
#define G_ACMUX_TEXEL0          1
#define G_ACMUX_TEXEL1          2
#define G_ACMUX_PRIMITIVE       3
#define G_ACMUX_SHADE           4
#define G_ACMUX_ENVIRONMENT     5
#define G_ACMUX_LOD_FRACTION    0
#define G_ACMUX_PRIM_LOD_FRAC   6
#define G_ACMUX_1               6
#define G_ACMUX_0               7

/* typical CC cycle 1 modes */
#define G_CC_PRIMITIVE              0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE
#define G_CC_SHADE                  0, 0, 0, SHADE, 0, 0, 0, SHADE
#define G_CC_MODULATEI              TEXEL0, 0, SHADE, 0, 0, 0, 0, SHADE
#define G_CC_MODULATEIA             TEXEL0, 0, SHADE, 0, TEXEL0, 0, SHADE, 0
#define G_CC_MODULATEIDECALA        TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0
#define G_CC_MODULATERGB            G_CC_MODULATEI
#define G_CC_MODULATERGBA           G_CC_MODULATEIA
#define G_CC_MODULATERGBDECALA      G_CC_MODULATEIDECALA
#define G_CC_MODULATEI_PRIM         TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define G_CC_MODULATEIA_PRIM        TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0
#define G_CC_MODULATEIDECALA_PRIM   TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0
#define G_CC_MODULATERGB_PRIM       G_CC_MODULATEI_PRIM
#define G_CC_MODULATERGBA_PRIM      G_CC_MODULATEIA_PRIM
#define G_CC_MODULATERGBDECALA_PRIM G_CC_MODULATEIDECALA_PRIM
#define G_CC_DECALRGB               0, 0, 0, TEXEL0, 0, 0, 0, SHADE
#define G_CC_DECALRGBA              0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0
#define G_CC_BLENDI                 ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_BLENDIA                ENVIRONMENT, SHADE, TEXEL0, SHADE, TEXEL0, 0, SHADE, 0
#define G_CC_BLENDIDECALA           ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_BLENDRGBA              TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, SHADE
#define G_CC_BLENDRGBDECALA         TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, TEXEL0
#define G_CC_ADDRGB                 1, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_ADDRGBDECALA           1, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_REFLECTRGB             ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_REFLECTRGBDECALA       ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_HILITERGB              PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_HILITERGBA             PRIMITIVE, SHADE, TEXEL0, SHADE, PRIMITIVE, SHADE, TEXEL0, SHADE
#define G_CC_HILITERGBDECALA        PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_SHADEDECALA            0, 0, 0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_BLENDPE                PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0
#define G_CC_BLENDPEDECALA          PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, TEXEL0

/* oddball modes */
#define _G_CC_BLENDPE               ENVIRONMENT, PRIMITIVE, TEXEL0, PRIMITIVE, TEXEL0, 0, SHADE, 0
#define _G_CC_BLENDPEDECALA         ENVIRONMENT, PRIMITIVE, TEXEL0, PRIMITIVE, 0, 0, 0, TEXEL0
#define _G_CC_TWOCOLORTEX           PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
/* used for 1-cycle sparse mip-maps, primitive color has color of lowest LOD */
#define _G_CC_SPARSEST              PRIMITIVE, TEXEL0, LOD_FRACTION, TEXEL0, PRIMITIVE, TEXEL0, LOD_FRACTION, TEXEL0
#define G_CC_TEMPLERP               TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0

/* typical CC cycle 1 modes, usually followed by other cycle 2 modes */
#define G_CC_TRILERP                TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0
#define G_CC_INTERFERENCE           TEXEL0, 0, TEXEL1, 0, TEXEL0, 0, TEXEL1, 0

/*
 *  One-cycle color convert operation
 */
#define G_CC_1CYUV2RGB              TEXEL0, K4, K5, TEXEL0, 0, 0, 0, SHADE

/*
 *  NOTE: YUV2RGB expects TF step1 color conversion to occur in 2nd clock.
 * Therefore, CC looks for step1 results in TEXEL1
 */
#define G_CC_YUV2RGB                TEXEL1, K4, K5, TEXEL1, 0, 0, 0, 0

/* typical CC cycle 2 modes */
#define G_CC_PASS2                  0, 0, 0, COMBINED, 0, 0, 0, COMBINED
#define G_CC_MODULATEI2             COMBINED, 0, SHADE, 0, 0, 0, 0, SHADE
#define G_CC_MODULATEIA2            COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0
#define G_CC_MODULATERGB2           G_CC_MODULATEI2
#define G_CC_MODULATERGBA2          G_CC_MODULATEIA2
#define G_CC_MODULATEI_PRIM2        COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define G_CC_MODULATEIA_PRIM2       COMBINED, 0, PRIMITIVE, 0, COMBINED, 0, PRIMITIVE, 0
#define G_CC_MODULATERGB_PRIM2      G_CC_MODULATEI_PRIM2
#define G_CC_MODULATERGBA_PRIM2     G_CC_MODULATEIA_PRIM2
#define G_CC_DECALRGB2              0, 0, 0, COMBINED, 0, 0, 0, SHADE
/*
 * ?
#define G_CC_DECALRGBA2             COMBINED, SHADE, COMBINED_ALPHA, SHADE, 0, 0, 0, SHADE
*/
#define G_CC_BLENDI2                ENVIRONMENT, SHADE, COMBINED, SHADE, 0, 0, 0, SHADE
#define G_CC_BLENDIA2               ENVIRONMENT, SHADE, COMBINED, SHADE, COMBINED, 0, SHADE, 0
#define G_CC_CHROMA_KEY2            TEXEL0, CENTER, SCALE, 0, 0, 0, 0, 0
#define G_CC_HILITERGB2             ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, SHADE
#define G_CC_HILITERGBA2            ENVIRONMENT, COMBINED, TEXEL0, COMBINED, ENVIRONMENT, COMBINED, TEXEL0, COMBINED
#define G_CC_HILITERGBDECALA2       ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, TEXEL0
#define G_CC_HILITERGBPASSA2        ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, COMBINED


#define GCCc0w0(saRGB0, mRGB0, saA0, mA0)   \
   (_SHIFTL((saRGB0), 20, 4) |              \
    _SHIFTL((mRGB0),  15, 5) |              \
    _SHIFTL((saA0),   12, 3) |              \
    _SHIFTL((mA0),     9, 3))

#define GCCc1w0(saRGB1, mRGB1)  \
   (_SHIFTL((saRGB1), 5, 4) |   \
    _SHIFTL((mRGB1),  0, 5))

#define GCCc0w1(sbRGB0, aRGB0, sbA0, aA0)   \
   (_SHIFTL((sbRGB0), 28, 4) |              \
    _SHIFTL((aRGB0),  15, 3) |              \
    _SHIFTL((sbA0),   12, 3) |              \
    _SHIFTL((aA0),     9, 3))

#define GCCc1w1(sbRGB1, saA1, mA1, aRGB1, sbA1, aA1)    \
   (_SHIFTL((sbRGB1), 24, 4) |                          \
    _SHIFTL((saA1),   21, 3) |                          \
    _SHIFTL((mA1),    18, 3) |                          \
    _SHIFTL((aRGB1),   6, 3) |                          \
    _SHIFTL((sbA1),    3, 3) |                          \
    _SHIFTL((aA1),     0, 3))

/* geometry mode (gbi.h 274-301, F3DEX_GBI_2 branch) */
#define G_ZBUFFER               0x00000001
#define G_SHADE                 0x00000004
#define G_TEXTURE_ENABLE        0x00000000
#define G_SHADING_SMOOTH        0x00200000
#define G_CULL_FRONT            0x00000200
#define G_CULL_BACK             0x00000400
#define G_CULL_BOTH             0x00000600
#define G_FOG                   0x00010000
#define G_LIGHTING              0x00020000
#define G_TEXTURE_GEN           0x00040000
#define G_TEXTURE_GEN_LINEAR    0x00080000
#define G_LOD                   0x00100000
#define G_CLIPPING              0x00000000

/* other modes (gbi.h 497-591) */
/*
 * G_SETOTHERMODE_L sft: shift count
 */
#define G_MDSFT_ALPHACOMPARE    0
#define G_MDSFT_ZSRCSEL         2
#define G_MDSFT_RENDERMODE      3
#define G_MDSFT_BLENDER         16

/*
 * G_SETOTHERMODE_H sft: shift count
 */
#define G_MDSFT_BLENDMASK       0   /* unsupported */
#define G_MDSFT_ALPHADITHER     4
#define G_MDSFT_RGBDITHER       6

#define G_MDSFT_COMBKEY         8
#define G_MDSFT_TEXTCONV        9
#define G_MDSFT_TEXTFILT        12
#define G_MDSFT_TEXTLUT         14
#define G_MDSFT_TEXTLOD         16
#define G_MDSFT_TEXTDETAIL      17
#define G_MDSFT_TEXTPERSP       19
#define G_MDSFT_CYCLETYPE       20
#define G_MDSFT_COLORDITHER     22  /* unsupported in HW 2.0 */
#define G_MDSFT_PIPELINE        23

/* G_SETOTHERMODE_H gPipelineMode */
#define G_PM_1PRIMITIVE     (1 << G_MDSFT_PIPELINE)
#define G_PM_NPRIMITIVE     (0 << G_MDSFT_PIPELINE)

/* G_SETOTHERMODE_H gSetCycleType */
#define G_CYC_1CYCLE        (0 << G_MDSFT_CYCLETYPE)
#define G_CYC_2CYCLE        (1 << G_MDSFT_CYCLETYPE)
#define G_CYC_COPY          (2 << G_MDSFT_CYCLETYPE)
#define G_CYC_FILL          (3 << G_MDSFT_CYCLETYPE)

/* G_SETOTHERMODE_H gSetTexturePersp */
#define G_TP_NONE           (0 << G_MDSFT_TEXTPERSP)
#define G_TP_PERSP          (1 << G_MDSFT_TEXTPERSP)

/* G_SETOTHERMODE_H gSetTextureDetail */
#define G_TD_CLAMP          (0 << G_MDSFT_TEXTDETAIL)
#define G_TD_SHARPEN        (1 << G_MDSFT_TEXTDETAIL)
#define G_TD_DETAIL         (2 << G_MDSFT_TEXTDETAIL)

/* G_SETOTHERMODE_H gSetTextureLOD */
#define G_TL_TILE           (0 << G_MDSFT_TEXTLOD)
#define G_TL_LOD            (1 << G_MDSFT_TEXTLOD)

/* G_SETOTHERMODE_H gSetTextureLUT */
#define G_TT_NONE           (0 << G_MDSFT_TEXTLUT)
#define G_TT_RGBA16         (2 << G_MDSFT_TEXTLUT)
#define G_TT_IA16           (3 << G_MDSFT_TEXTLUT)

/* G_SETOTHERMODE_H gSetTextureFilter */
#define G_TF_POINT          (0 << G_MDSFT_TEXTFILT)
#define G_TF_AVERAGE        (3 << G_MDSFT_TEXTFILT)
#define G_TF_BILERP         (2 << G_MDSFT_TEXTFILT)

/* G_SETOTHERMODE_H gSetTextureConvert */
#define G_TC_CONV           (0 << G_MDSFT_TEXTCONV)
#define G_TC_FILTCONV       (5 << G_MDSFT_TEXTCONV)
#define G_TC_FILT           (6 << G_MDSFT_TEXTCONV)

/* G_SETOTHERMODE_H gSetCombineKey */
#define G_CK_NONE           (0 << G_MDSFT_COMBKEY)
#define G_CK_KEY            (1 << G_MDSFT_COMBKEY)

/* G_SETOTHERMODE_H gSetColorDither */
#define G_CD_MAGICSQ        (0 << G_MDSFT_RGBDITHER)
#define G_CD_BAYER          (1 << G_MDSFT_RGBDITHER)
#define G_CD_NOISE          (2 << G_MDSFT_RGBDITHER)

#ifndef _HW_VERSION_1
#define G_CD_DISABLE        (3 << G_MDSFT_RGBDITHER)
#define G_CD_ENABLE         G_CD_NOISE  /* HW 1.0 compatibility mode */
#else
#define G_CD_ENABLE         (1 << G_MDSFT_COLORDITHER)
#define G_CD_DISABLE        (0 << G_MDSFT_COLORDITHER)
#endif

/* G_SETOTHERMODE_H gSetAlphaDither */
#define G_AD_PATTERN        (0 << G_MDSFT_ALPHADITHER)
#define G_AD_NOTPATTERN     (1 << G_MDSFT_ALPHADITHER)
#define G_AD_NOISE          (2 << G_MDSFT_ALPHADITHER)
#define G_AD_DISABLE        (3 << G_MDSFT_ALPHADITHER)

/* G_SETOTHERMODE_L gSetAlphaCompare */
#define G_AC_NONE           (0 << G_MDSFT_ALPHACOMPARE)
#define G_AC_THRESHOLD      (1 << G_MDSFT_ALPHACOMPARE)
#define G_AC_DITHER         (3 << G_MDSFT_ALPHACOMPARE)

/* G_SETOTHERMODE_L gSetDepthSource */
#define G_ZS_PIXEL          (0 << G_MDSFT_ZSRCSEL)
#define G_ZS_PRIM           (1 << G_MDSFT_ZSRCSEL)

#define G_SETCOMBINE 0xFC

// ---- native commands
// RDP_EVAL(pkt): sources that keep the original display-buffer cursors (engine/rdp_disps.h) evaluate the
// cursor argument, which selects the buffer (POLY_OPA_DISP, OVERLAY_DISP, ...). Restored functions whose
// cursor plumbing was removed (tools/restore_rdp_draws.py) do not, and draw into the current buffer.
#ifndef RDP_EVAL
#define RDP_EVAL(pkt)
#endif
#define gDPPipeSync(pkt) (RDP_EVAL(pkt)(void)0)
#define gDPLoadSync(pkt) (RDP_EVAL(pkt)(void)0)
// (min level / LOD fraction: not used by the 2D interface code)
#define gDPSetPrimColor(pkt, m, l, r, g, b, a) (RDP_EVAL(pkt) Gfx3D_SetPrimColor((r), (g), (b), (a)))
#define gDPSetEnvColor(pkt, r, g, b, a) (RDP_EVAL(pkt) Gfx3D_SetEnvColor((r), (g), (b), (a)))
#define gDPSetAlphaCompare(pkt, type) (RDP_EVAL(pkt) Gfx3D_SetAlphaCompare(type))
#define gSPSegment(pkt, seg, base) (RDP_EVAL(pkt) Gfx3D_SetSegment((seg), (const void*)(base)))
#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1)          \
    (RDP_EVAL(pkt) Gfx3D_SetCombine(                                                                             \
        _SHIFTL(G_SETCOMBINE, 24, 8) |                                                                           \
            _SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) |                          \
                        GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1),                                                     \
                    0, 24),                                                                                      \
        (unsigned int)(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0, G_ACMUX_##Ab0, G_ACMUX_##Ad0) |                      \
                       GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1, G_ACMUX_##Ac1, G_CCMUX_##d1, G_ACMUX_##Ab1,          \
                               G_ACMUX_##Ad1))))
#define gDPSetCombineMode(pkt, a, b) gDPSetCombineLERP(pkt, a, b)
#define gDPLoadTextureBlock(pkt, timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    (RDP_EVAL(pkt) Gfx3D_LoadTextureBlock((const void*)(timg), (fmt), (siz), (width), (height), (pal), (cms), \
                                          (cmt), (masks), (maskt)))
#define gDPLoadTextureBlock_4b(pkt, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt)    \
    (RDP_EVAL(pkt) Gfx3D_LoadTextureBlock((const void*)(timg), (fmt), G_IM_SIZ_4b, (width), (height), (pal), \
                                          (cms), (cmt), (masks), (maskt)))
#define gDPLoadMultiBlock_4b(pkt, timg, tmem, rtile, fmt, width, height, pal, cms, cmt, masks, maskt, shifts,  \
                             shiftt)                                                                           \
    gDPLoadTextureBlock_4b(pkt, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt)
// tile / image commands of hand-built loads (the RDP state machine in engine/gfx3d.c turns them into texture
// block loads when a primitive is drawn)
#define gDPSetTextureImage(pkt, fmt, siz, width, img) \
    (RDP_EVAL(pkt) Gfx3D_RdpSetTextureImage((fmt), (siz), (width), (const void*)(img)))
#define gDPSetTileCustom(pkt, fmt, siz, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    (RDP_EVAL(pkt) Gfx3D_RdpSetTileCustom((fmt), (siz), (uls), (ult), (lrs), (lrt), (pal), (cms), (cmt),  \
                                          (masks), (maskt)))
#define gDPLoadTile(pkt, t, uls, ult, lrs, lrt) (RDP_EVAL(pkt) Gfx3D_RdpLoadTile((t), (uls), (ult), (lrs), (lrt)))
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    (RDP_EVAL(pkt) Gfx3D_TextureRectangle((xl), (yl), (xh), (yh), (tile), (s), (t), (dsdx), (dtdy)))
#define Gfx_SetupDL_39Ptr(gfxp) Gfx3D_SetupDL(39)
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry) (RDP_EVAL(pkt) Gfx3D_FillRectangle((ulx), (uly), (lrx), (lry)))
#define gDPSetOtherMode(pkt, mode0, mode1) (RDP_EVAL(pkt) Gfx3D_SetOtherMode((mode0), (mode1)))
#define gDPSetRenderMode(pkt, c0, c1) (RDP_EVAL(pkt) Gfx3D_SetRenderMode((c0), (c1)))
#define gDPSetCycleType(pkt, type) (RDP_EVAL(pkt) Gfx3D_SetOtherModePart(1, G_MDSFT_CYCLETYPE, 2, (type)))
#define gDPSetTextureLUT(pkt, type) (RDP_EVAL(pkt) Gfx3D_SetOtherModePart(1, G_MDSFT_TEXTLUT, 2, (type)))
#define gSPClearGeometryMode(pkt, word) (RDP_EVAL(pkt) Gfx3D_GeometryModeN64((word), 0))
#define gSPSetGeometryMode(pkt, word) (RDP_EVAL(pkt) Gfx3D_GeometryModeN64(0, (word)))
// vertices: the game's own Vtx records (engine/vtx.h); drawn under the current model matrix and the camera
// of the buffer (scene camera, or the overlay camera of View_Apply*ToOverlay)
#define gSPVertex(pkt, v, n, v0) (RDP_EVAL(pkt) Gfx3D_Vertices((const Vtx*)(v), (n), (v0)))
#define gSP1Quadrangle(pkt, v0, v1, v2, v3, flag) (RDP_EVAL(pkt) Gfx3D_Quad((v0), (v1), (v2), (v3)))
// model matrix loads (G_MTX_MODELVIEW | G_MTX_LOAD; the interface code uses no other form)
#define gSPMatrix(pkt, m, p) (RDP_EVAL(pkt) Gfx3D_LoadMtxF((const MtxF*)(m)))
#define gIdentityMtx gIdentityMtxF // the identity matrix as the native float matrix
#define MATRIX_FINALIZE_AND_LOAD(pkt, gfxCtx, file, line) (RDP_EVAL(pkt) Gfx3D_LoadMtxF(Matrix_GetCurrent()))

#endif
