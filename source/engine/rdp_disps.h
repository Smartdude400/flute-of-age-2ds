// engine/rdp_disps.h (rewritten: native)
// For game sources imported with their RDP commands kept (tools/import_source.py --rdp): the display
// buffer cursors (POLY_OPA_DISP, POLY_XLU_DISP, OVERLAY_DISP) select the native command buffer, and the
// command macros of engine/rdp_gbi.h record native commands into it. Static display lists written with
// the gs* macros (imported from include/ultra64/gbi.h: 3285-3289, 3309-3319 F3DEX_GBI_2, 3610,
// 3733-3738, 4948-4952, 5147) keep the original command words and run through the display-list
// interpreter (Gfx3D_RunDL), as do lists in game memory and converted handles.
#ifndef ENGINE_RDP_DISPS_H
#define ENGINE_RDP_DISPS_H
#include "engine/gfx3d.h"

typedef struct Gfx {
    u32 w0, w1;
} Gfx;

// cursor of a buffer: selects it as the recording target; the pointer itself is a scratch value
Gfx** Gfx3D_DispCursor(GfxBuf buf);
#define POLY_OPA_DISP (*Gfx3D_DispCursor(GFX_BUF_OPA))
#define POLY_XLU_DISP (*Gfx3D_DispCursor(GFX_BUF_XLU))
#define OVERLAY_DISP (*Gfx3D_DispCursor(GFX_BUF_OVL))
#define OPEN_DISPS(gfxCtx, file, line) {
#define CLOSE_DISPS(gfxCtx, file, line) }
#define RDP_EVAL(pkt) (void)(pkt),

#include "engine/rdp_gbi.h"

// display lists: a converted handle, original commands in game memory (big-endian, e.g. lists inside a
// ROM file the game loaded), or a static list of this program (native word order)
void Gfx3D_RunDL(const void* dl);
#define gSPDisplayList(pkt, dl) (RDP_EVAL(pkt) Gfx3D_RunDL((const void*)(dl)))
#define gSPEndDisplayList(pkt) (RDP_EVAL(pkt)(void)0)

// Lists built in place and called from another buffer (Gfx_Open on POLY_OPA_DISP memory, then
// gSPDisplayList(OVERLAY_DISP++, list)): the commands are recorded where the call is, in order, so the
// opened list is a marker that the call skips.
Gfx* Gfx3D_OpenList(void);
#define Gfx_Open(gfxRef) Gfx3D_OpenList()
#define Gfx_Close(gfxRef, gfx) ((void)0)
// setup presets that returned the advanced cursor (Gfx_SetupDL_28(gfx)) into the current buffer
#define Gfx_SetupDL_28(gfx) (Gfx3D_SetupDL(28), (gfx))

#define G_GEOMETRYMODE 0xD9
#define G_ENDDL 0xDF
#define G_RDPSETOTHERMODE 0xEF
#define G_RDPPIPESYNC 0xE7
#define gsDPNoParam(cmd) { _SHIFTL(cmd, 24, 8), 0 }
#define gsDPPipeSync() gsDPNoParam(G_RDPPIPESYNC)
#define gsSPEndDisplayList() { _SHIFTL(G_ENDDL, 24, 8), 0 }
#define gsSPGeometryMode(c, s) { (_SHIFTL(G_GEOMETRYMODE, 24, 8) | _SHIFTL(~(u32)(c), 0, 24)), (u32)(s) }
#define gsSPSetGeometryMode(word) gsSPGeometryMode(0, (word))
#define gsSPClearGeometryMode(word) gsSPGeometryMode((word), 0)
#define gsDPSetOtherMode(mode0, mode1) { (_SHIFTL(G_RDPSETOTHERMODE, 24, 8) | _SHIFTL(mode0, 0, 24)), (unsigned int)(mode1) }
#define gsDPSetCombineLERP(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1)               \
    { (_SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) | \
                                                  GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1),                           \
                                              0, 24)),                                                           \
      (unsigned int)(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0, G_ACMUX_##Ab0, G_ACMUX_##Ad0) |                        \
                     GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1, G_ACMUX_##Ac1, G_CCMUX_##d1, G_ACMUX_##Ab1, G_ACMUX_##Ad1)) }
#define gsDPSetCombineMode(a, b) gsDPSetCombineLERP(a, b)

#endif
