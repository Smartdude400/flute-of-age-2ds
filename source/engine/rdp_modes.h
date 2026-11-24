// engine/rdp_modes.h (imported: include/ultra64/gbi.h lines 593-946, render-mode bits only)
// The original RDP render-mode words (G_SETOTHERMODE_L bits 3..31). Game code that builds render-mode
// commands at runtime passes them to Gfx3D_SetRenderMode, which decodes them like tools/dl2pica.py.
#ifndef ENGINE_RDP_MODES_H
#define ENGINE_RDP_MODES_H

/* G_SETOTHERMODE_L gSetRenderMode */
#define AA_EN           0x0008
#define Z_CMP           0x0010
#define Z_UPD           0x0020
#define IM_RD           0x0040
#define CLR_ON_CVG      0x0080
#define CVG_DST_CLAMP   0x0000
#define CVG_DST_WRAP    0x0100
#define CVG_DST_FULL    0x0200
#define CVG_DST_SAVE    0x0300
#define ZMODE_OPA       0x0000
#define ZMODE_INTER     0x0400
#define ZMODE_XLU       0x0800
#define ZMODE_DEC       0x0C00
#define CVG_X_ALPHA     0x1000
#define ALPHA_CVG_SEL   0x2000
#define FORCE_BL        0x4000
#define TEX_EDGE        0x0000  /* used to be 0x8000 */

#define G_BL_CLR_IN     0
#define G_BL_CLR_MEM    1
#define G_BL_CLR_BL     2
#define G_BL_CLR_FOG    3
#define G_BL_1MA        0
#define G_BL_A_MEM      1
#define G_BL_A_IN       0
#define G_BL_A_FOG      1
#define G_BL_A_SHADE    2
#define G_BL_1          2
#define G_BL_0          3

#define GBL_c1(m1a, m1b, m2a, m2b)  \
    (m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18
#define GBL_c2(m1a, m1b, m2a, m2b)  \
    (m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16

#define RM_AA_ZB_OPA_SURF(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |             \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_RA_ZB_OPA_SURF(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |                     \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_ZB_XLU_SURF(clk)                                  \
    AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |         \
    FORCE_BL | ZMODE_XLU |                                      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_OPA_DECAL(clk)                                 \
    AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | ALPHA_CVG_SEL |      \
    ZMODE_DEC |                                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_RA_ZB_OPA_DECAL(clk)                                 \
    AA_EN | Z_CMP | CVG_DST_WRAP | ALPHA_CVG_SEL |              \
    ZMODE_DEC |                                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_ZB_XLU_DECAL(clk)                                 \
    AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |         \
    FORCE_BL | ZMODE_DEC |                                      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_OPA_INTER(clk)                                 \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |             \
    ALPHA_CVG_SEL | ZMODE_INTER |                               \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_RA_ZB_OPA_INTER(clk)                                 \
    AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |                     \
    ALPHA_CVG_SEL | ZMODE_INTER |                               \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_ZB_XLU_INTER(clk)                                 \
    AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |         \
    FORCE_BL | ZMODE_INTER |                                    \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_XLU_LINE(clk)                                  \
    AA_EN | Z_CMP | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |       \
    ALPHA_CVG_SEL | FORCE_BL | ZMODE_XLU |                      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_DEC_LINE(clk)                                  \
    AA_EN | Z_CMP | IM_RD | CVG_DST_SAVE | CVG_X_ALPHA |        \
    ALPHA_CVG_SEL | FORCE_BL | ZMODE_DEC |                      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_TEX_EDGE(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |             \
    CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |        \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_ZB_TEX_INTER(clk)                                 \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |             \
    CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_INTER | TEX_EDGE |      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_ZB_SUB_SURF(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |              \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_ZB_PCL_SURF(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |             \
    ZMODE_OPA | G_AC_DITHER |                                   \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_OPA_TERR(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |             \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_TEX_TERR(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |             \
    CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |        \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_ZB_SUB_TERR(clk)                                  \
    AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |              \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define RM_AA_OPA_SURF(clk)                                     \
    AA_EN | IM_RD | CVG_DST_CLAMP |                             \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_RA_OPA_SURF(clk)                                     \
    AA_EN | CVG_DST_CLAMP |                                     \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_XLU_SURF(clk)                                     \
    AA_EN | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | FORCE_BL |      \
    ZMODE_OPA |                                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_XLU_LINE(clk)                                     \
    AA_EN | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |               \
    ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |                      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_DEC_LINE(clk)                                     \
    AA_EN | IM_RD | CVG_DST_FULL | CVG_X_ALPHA |                \
    ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |                      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_TEX_EDGE(clk)                                     \
    AA_EN | IM_RD | CVG_DST_CLAMP |                             \
    CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |        \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_SUB_SURF(clk)                                     \
    AA_EN | IM_RD | CVG_DST_FULL |                              \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_AA_PCL_SURF(clk)                                     \
    AA_EN | IM_RD | CVG_DST_CLAMP |                             \
    ZMODE_OPA | G_AC_DITHER |                                   \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_OPA_TERR(clk)                                     \
    AA_EN | IM_RD | CVG_DST_CLAMP |                             \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_TEX_TERR(clk)                                     \
    AA_EN | IM_RD | CVG_DST_CLAMP |                             \
    CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |        \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_AA_SUB_TERR(clk)                                     \
    AA_EN | IM_RD | CVG_DST_FULL |                              \
    ZMODE_OPA | ALPHA_CVG_SEL |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define RM_ZB_OPA_SURF(clk)                                     \
    Z_CMP | Z_UPD | CVG_DST_FULL | ALPHA_CVG_SEL |              \
    ZMODE_OPA |                                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_ZB_XLU_SURF(clk)                                     \
    Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_XLU |       \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_ZB_OPA_DECAL(clk)                                    \
    Z_CMP | CVG_DST_FULL | ALPHA_CVG_SEL | ZMODE_DEC |          \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define RM_ZB_XLU_DECAL(clk)                                    \
    Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_DEC |       \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_ZB_CLD_SURF(clk)                                     \
    Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_XLU |       \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_ZB_OVL_SURF(clk)                                     \
    Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_DEC |       \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_ZB_PCL_SURF(clk)                                     \
    Z_CMP | Z_UPD | CVG_DST_FULL | ZMODE_OPA |                  \
    G_AC_DITHER |                                               \
    GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)


#define RM_OPA_SURF(clk)                                        \
    CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA |                      \
    GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define RM_XLU_SURF(clk)                                        \
    IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_OPA |               \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_TEX_EDGE(clk)                                        \
    CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | FORCE_BL |    \
    ZMODE_OPA | TEX_EDGE | AA_EN |                              \
    GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define RM_CLD_SURF(clk)                                        \
    IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |               \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define RM_PCL_SURF(clk)                                        \
    CVG_DST_FULL | FORCE_BL | ZMODE_OPA |                       \
    G_AC_DITHER |                                               \
    GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define RM_ADD(clk)                                             \
    IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |               \
    GBL_c##clk(G_BL_CLR_IN, G_BL_A_FOG, G_BL_CLR_MEM, G_BL_1)

#define RM_NOOP(clk)    \
    GBL_c##clk(0, 0, 0, 0)

#define RM_VISCVG(clk)                                          \
    IM_RD | FORCE_BL |                                          \
    GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_BL, G_BL_A_MEM)

/* for rendering to an 8-bit framebuffer */
#define RM_OPA_CI(clk)                                          \
    CVG_DST_CLAMP | ZMODE_OPA |                                 \
    GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)



#define G_RM_AA_ZB_OPA_SURF     RM_AA_ZB_OPA_SURF(1)
#define G_RM_AA_ZB_OPA_SURF2    RM_AA_ZB_OPA_SURF(2)
#define G_RM_AA_ZB_XLU_SURF     RM_AA_ZB_XLU_SURF(1)
#define G_RM_AA_ZB_XLU_SURF2    RM_AA_ZB_XLU_SURF(2)
#define G_RM_AA_ZB_OPA_DECAL    RM_AA_ZB_OPA_DECAL(1)
#define G_RM_AA_ZB_OPA_DECAL2   RM_AA_ZB_OPA_DECAL(2)
#define G_RM_AA_ZB_XLU_DECAL    RM_AA_ZB_XLU_DECAL(1)
#define G_RM_AA_ZB_XLU_DECAL2   RM_AA_ZB_XLU_DECAL(2)
#define G_RM_AA_ZB_OPA_INTER    RM_AA_ZB_OPA_INTER(1)
#define G_RM_AA_ZB_OPA_INTER2   RM_AA_ZB_OPA_INTER(2)
#define G_RM_AA_ZB_XLU_INTER    RM_AA_ZB_XLU_INTER(1)
#define G_RM_AA_ZB_XLU_INTER2   RM_AA_ZB_XLU_INTER(2)
#define G_RM_AA_ZB_XLU_LINE     RM_AA_ZB_XLU_LINE(1)
#define G_RM_AA_ZB_XLU_LINE2    RM_AA_ZB_XLU_LINE(2)
#define G_RM_AA_ZB_DEC_LINE     RM_AA_ZB_DEC_LINE(1)
#define G_RM_AA_ZB_DEC_LINE2    RM_AA_ZB_DEC_LINE(2)
#define G_RM_AA_ZB_TEX_EDGE     RM_AA_ZB_TEX_EDGE(1)
#define G_RM_AA_ZB_TEX_EDGE2    RM_AA_ZB_TEX_EDGE(2)
#define G_RM_AA_ZB_TEX_INTER    RM_AA_ZB_TEX_INTER(1)
#define G_RM_AA_ZB_TEX_INTER2   RM_AA_ZB_TEX_INTER(2)
#define G_RM_AA_ZB_SUB_SURF     RM_AA_ZB_SUB_SURF(1)
#define G_RM_AA_ZB_SUB_SURF2    RM_AA_ZB_SUB_SURF(2)
#define G_RM_AA_ZB_PCL_SURF     RM_AA_ZB_PCL_SURF(1)
#define G_RM_AA_ZB_PCL_SURF2    RM_AA_ZB_PCL_SURF(2)
#define G_RM_AA_ZB_OPA_TERR     RM_AA_ZB_OPA_TERR(1)
#define G_RM_AA_ZB_OPA_TERR2    RM_AA_ZB_OPA_TERR(2)
#define G_RM_AA_ZB_TEX_TERR     RM_AA_ZB_TEX_TERR(1)
#define G_RM_AA_ZB_TEX_TERR2    RM_AA_ZB_TEX_TERR(2)
#define G_RM_AA_ZB_SUB_TERR     RM_AA_ZB_SUB_TERR(1)
#define G_RM_AA_ZB_SUB_TERR2    RM_AA_ZB_SUB_TERR(2)

#define G_RM_RA_ZB_OPA_SURF     RM_RA_ZB_OPA_SURF(1)
#define G_RM_RA_ZB_OPA_SURF2    RM_RA_ZB_OPA_SURF(2)
#define G_RM_RA_ZB_OPA_DECAL    RM_RA_ZB_OPA_DECAL(1)
#define G_RM_RA_ZB_OPA_DECAL2   RM_RA_ZB_OPA_DECAL(2)
#define G_RM_RA_ZB_OPA_INTER    RM_RA_ZB_OPA_INTER(1)
#define G_RM_RA_ZB_OPA_INTER2   RM_RA_ZB_OPA_INTER(2)

#define G_RM_AA_OPA_SURF    RM_AA_OPA_SURF(1)
#define G_RM_AA_OPA_SURF2   RM_AA_OPA_SURF(2)
#define G_RM_AA_XLU_SURF    RM_AA_XLU_SURF(1)
#define G_RM_AA_XLU_SURF2   RM_AA_XLU_SURF(2)
#define G_RM_AA_XLU_LINE    RM_AA_XLU_LINE(1)
#define G_RM_AA_XLU_LINE2   RM_AA_XLU_LINE(2)
#define G_RM_AA_DEC_LINE    RM_AA_DEC_LINE(1)
#define G_RM_AA_DEC_LINE2   RM_AA_DEC_LINE(2)
#define G_RM_AA_TEX_EDGE    RM_AA_TEX_EDGE(1)
#define G_RM_AA_TEX_EDGE2   RM_AA_TEX_EDGE(2)
#define G_RM_AA_SUB_SURF    RM_AA_SUB_SURF(1)
#define G_RM_AA_SUB_SURF2   RM_AA_SUB_SURF(2)
#define G_RM_AA_PCL_SURF    RM_AA_PCL_SURF(1)
#define G_RM_AA_PCL_SURF2   RM_AA_PCL_SURF(2)
#define G_RM_AA_OPA_TERR    RM_AA_OPA_TERR(1)
#define G_RM_AA_OPA_TERR2   RM_AA_OPA_TERR(2)
#define G_RM_AA_TEX_TERR    RM_AA_TEX_TERR(1)
#define G_RM_AA_TEX_TERR2   RM_AA_TEX_TERR(2)
#define G_RM_AA_SUB_TERR    RM_AA_SUB_TERR(1)
#define G_RM_AA_SUB_TERR2   RM_AA_SUB_TERR(2)

#define G_RM_RA_OPA_SURF    RM_RA_OPA_SURF(1)
#define G_RM_RA_OPA_SURF2   RM_RA_OPA_SURF(2)

#define G_RM_ZB_OPA_SURF    RM_ZB_OPA_SURF(1)
#define G_RM_ZB_OPA_SURF2   RM_ZB_OPA_SURF(2)
#define G_RM_ZB_XLU_SURF    RM_ZB_XLU_SURF(1)
#define G_RM_ZB_XLU_SURF2   RM_ZB_XLU_SURF(2)
#define G_RM_ZB_OPA_DECAL   RM_ZB_OPA_DECAL(1)
#define G_RM_ZB_OPA_DECAL2  RM_ZB_OPA_DECAL(2)
#define G_RM_ZB_XLU_DECAL   RM_ZB_XLU_DECAL(1)
#define G_RM_ZB_XLU_DECAL2  RM_ZB_XLU_DECAL(2)
#define G_RM_ZB_CLD_SURF    RM_ZB_CLD_SURF(1)
#define G_RM_ZB_CLD_SURF2   RM_ZB_CLD_SURF(2)
#define G_RM_ZB_OVL_SURF    RM_ZB_OVL_SURF(1)
#define G_RM_ZB_OVL_SURF2   RM_ZB_OVL_SURF(2)
#define G_RM_ZB_PCL_SURF    RM_ZB_PCL_SURF(1)
#define G_RM_ZB_PCL_SURF2   RM_ZB_PCL_SURF(2)

#define G_RM_OPA_SURF       RM_OPA_SURF(1)
#define G_RM_OPA_SURF2      RM_OPA_SURF(2)
#define G_RM_XLU_SURF       RM_XLU_SURF(1)
#define G_RM_XLU_SURF2      RM_XLU_SURF(2)
#define G_RM_CLD_SURF       RM_CLD_SURF(1)
#define G_RM_CLD_SURF2      RM_CLD_SURF(2)
#define G_RM_TEX_EDGE       RM_TEX_EDGE(1)
#define G_RM_TEX_EDGE2      RM_TEX_EDGE(2)
#define G_RM_PCL_SURF       RM_PCL_SURF(1)
#define G_RM_PCL_SURF2      RM_PCL_SURF(2)
#define G_RM_ADD            RM_ADD(1)
#define G_RM_ADD2           RM_ADD(2)
#define G_RM_NOOP           RM_NOOP(1)
#define G_RM_NOOP2          RM_NOOP(2)
#define G_RM_VISCVG         RM_VISCVG(1)
#define G_RM_VISCVG2        RM_VISCVG(2)
#define G_RM_OPA_CI         RM_OPA_CI(1)
#define G_RM_OPA_CI2        RM_OPA_CI(2)


#define G_RM_FOG_SHADE_A    GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA)
#define G_RM_FOG_PRIM_A     GBL_c1(G_BL_CLR_FOG, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA)
#define G_RM_PASS           GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#endif
