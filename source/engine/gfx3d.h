// engine/gfx3d.h - native 3D renderer used by the (rewritten) draw paths of game code.
//
// The original built separate display lists per frame for opaque geometry, translucent geometry and the
// overlay, and the hardware executed them in that order. Here every call is RECORDED into the current
// target buffer (opaque / translucent / overlay) and the buffers are replayed in the same order at the end
// of the frame, on the PICA200, by a state machine that executes the converted mesh programs.
#pragma once
#include "engine/types.h"
#include "game_types.h"
#include "engine/dsobj.h"

typedef enum { GFX_BUF_OPA, GFX_BUF_XLU, GFX_BUF_OVL, GFX_BUF_COUNT } GfxBuf;

void Gfx3D_Init(void);
void Gfx3D_BeginFrame(void); // clears the command buffers (called by the runner)
void Gfx3D_EndFrame(void);   // replays opaque, translucent, overlay (called by the runner)
// Replay of the last recorded frame at time t between the previous logic frame (0) and it (1): matched matrices
// are blended (60 Hz presentation). Gfx3D_EndFrame is Gfx3D_Replay(1) plus marking the frame as a blend source.
void Gfx3D_Replay(float t);
// Draw context of the following matrix commands (record time): matrices are matched with the previous
// frame's by context, type and order. Set by the game's draw entry points (actor, room, effect, sky).
#define INTERP_CTX_NONE 0xFFFFFFFFu   // not interpolated (HUD, menus)
#define INTERP_CTX_CAMERA 0xFFFFFF00u // + buffer
void Gfx3D_InterpSetContext(u32 ctx);
u32 Gfx3D_InterpGetContext(void);
void Gfx3D_InterpReset(void); // nothing to blend from (scene change)
void Gfx3D_SetTarget(GfxBuf buf);
GfxBuf Gfx3D_GetTarget(void);

// Camera: game projection (P) and viewing (V) float matrices, row-vector convention (v * V * P).
void Gfx3D_SetCamera(const MtxF* projection, const MtxF* viewing); // opaque + translucent buffers
// Model matrix in game units (the game's current MtxF).
void Gfx3D_LoadMtxF(const MtxF* model);
void Gfx3D_StoreSlot(int slot); // (DS-era API, no-op)
void Gfx3D_RestoreSlot(int slot);
// Segments: which loaded object answers a segmented address (asset resolution; immediate).
void Gfx3D_SetSegmentObj(int seg, DsObj* obj);
DsObj* Gfx3D_GetSegmentObj(int seg);
void Gfx3D_SetSegmentTex(int seg, const DsTexture* tex);
// Draws the converted display list identified by its original segmented address / asset handle.
bool Gfx3D_DrawDL(u32 segAddr);
void Gfx3D_DrawMesh(const DsObj* obj, const DsMeshHdr* mesh);
void Gfx3D_SetLights(const u8 ambient[3], int count, const s8 dirs[][3], const u8 cols[][3]);
// the same, into the current buffer only (a light group set in one list, e.g. gSPSetLights1 on POLY_OPA_DISP)
void Gfx3D_SetLightsHere(const u8 ambient[3], int count, const s8 dirs[][3], const u8 cols[][3]);
void Gfx3D_SetEnvColor(u8 r, u8 g, u8 b, u8 a);
// Flexible skeletons: limb matrices (segment 0x0D table) and limb lists, in limb order.
void Gfx3D_FlexBegin(void);
void Gfx3D_FlexAdd(const MtxF* m, DListRef dl); // dl may be NULL (matrix slot only)
void Gfx3D_FlexEnd(void);
#define GFX3D_DRAW_CURRENT(dl) (Gfx3D_LoadMtxF(Matrix_GetCurrent()), Gfx3D_DrawDL((u32)(uintptr_t)(dl)))

// Render-state presets (the original Gfx_SetupDL_* lists, compiled offline: romfs/setupdl.pobj)
void Gfx3D_SetupDL(int preset);
void Gfx3D_SetupDLTo(GfxBuf buf, int preset); // Gfx_SetupDL_<N><Opa|Xlu|Overlay> -> (buffer, preset)
// Selects the original display-list buffer of a call, then makes it (restored by tools/restore_buffers.py).
#define GFX_TO(buf, call) (Gfx3D_SetTarget(GFX_BUF_##buf), (call))
void Gfx3D_ResetMaterialState(int xlu); // DS-era call sites: preset 25 into the opaque/translucent buffer
void Gfx3D_SetPrimColor(u8 r, u8 g, u8 b, u8 a);
// Render mode (was gDPSetRenderMode): the two original RDP mode words (engine/rdp_modes.h G_RM_*)
void Gfx3D_SetRenderMode(u32 c1, u32 c2);
// 2D interface commands (were G_SETCOMBINE, gDPSetAlphaCompare, gDPLoadTextureBlock, gSPTextureRectangle;
// see engine/rdp_gbi.h): original arguments, fixed-point coordinates as in the original
void Gfx3D_SetCombine(u32 w0, u32 w1);
void Gfx3D_SetAlphaCompare(u32 type);
void Gfx3D_LoadTextureBlock(const void* img, int fmt, int siz, int w, int h, int pal, int cms, int cmt, int masks,
                            int maskt);
void Gfx3D_TextureRectangle(int xl, int yl, int xh, int yh, int tile, int s, int t, int dsdx, int dtdy);
void Gfx3D_FillRectangle(int ulx, int uly, int lrx, int lry);
void Gfx3D_SetOtherMode(u32 h, u32 l);           // both other-mode words (gDPSetOtherMode)
void Gfx3D_GeometryModeN64(u32 clear, u32 set);  // original G_* geometry bits
// overlay camera (View_Apply*ToOverlay): projection, viewing (NULL = identity), viewport in screen pixels
void Gfx3D_SetOverlayCamera(const MtxF* projection, const MtxF* viewing, int leftX, int topY, int rightX,
                            int bottomY);
union Vtx;
void Gfx3D_Vertices(const union Vtx* v, int n, int v0);
void Gfx3D_Quad(int v0, int v1, int v2, int v3);
void Gfx3D_Tri(int v0, int v1, int v2);
void Gfx3D_LoadTextureBlockTile(int tile, const void* img, int fmt, int siz, int w, int h, int pal, int cms, int cmt,
                                int masks, int maskt);
void Gfx3D_RdpSetTileSize(int tile, int uls, int ult, int lrs, int lrt);
void Gfx3D_LoadTLUT(int slot, const void* dram, int count); // palette slots (16 entries each) from game memory
void Gfx3D_RunDL(const void* dl); // converted handle, original list in game memory, or static native list
void Gfx3D_SetOtherModePart(int high, int shift, int len, u32 data); // G_SETOTHERMODE_H / _L
// hand-built texture loads (gDPSetTextureImage, gDPSetTileCustom, gDPLoadTile)
void Gfx3D_RdpSetTextureImage(int fmt, int siz, int width, const void* img);
void Gfx3D_RdpSetTileCustom(int fmt, int siz, int uls, int ult, int lrs, int lrt, int pal, int cms, int cmt, int masks,
                            int maskt);
void Gfx3D_RdpLoadTile(int tile, int uls, int ult, int lrs, int lrt);
const u8* Gfx3D_RawSegment(int seg); // game memory bound to a segment while recording, or NULL
// Geometry mode change (was gSPClearGeometryMode / gSPSetGeometryMode): native GEOM_* flags (engine/pmesh.h)
void Gfx3D_GeometryMode(u32 clear, u32 set);
// Segment binding: a texture handle, a display-list handle, or NULL to clear.
void Gfx3D_SetSegment(int seg, const void* handle);
struct LightsN;
void Gfx3D_SetLightsN(const struct LightsN* l, int numLights);
// Per-frame scratch memory (replaces the original graph-arena allocations of temporary draw data).
void* Gfx3D_FrameAlloc(size_t size);
void Gfx3D_FrameReset(void);
// Texture scrolling bound to a segment (the original runtime tile-size lists), original tile units.
void Gfx3D_SetTexScroll(int seg, int tile, int x, int y, int w, int h);
void Gfx3D_SetTexScroll2(int seg, int tile1, int x1, int y1, int w1, int h1, int tile2, int x2, int y2, int w2, int h2);
// Runtime segment lists: between Begin and End, SetEnvColor / SetPrimColor / DrawDL are recorded into
// the segment instead of taking effect; a converted list that calls the segment replays them.
void Gfx3D_SegDLBegin(int seg);
void Gfx3D_SegDLEnd(void);
// Frame start for a state that draws a 3D scene (was Gfx_SetupFrame): fills the frame with a colour.
void Gfx3D_SetupFrame(u8 r, u8 g, u8 b);
// Scene fog (was Gfx_SetFog2): colour and the original fog near/far (0..1000 depth range).
void Gfx3D_SetFog(u8 r, u8 g, u8 b, u8 a, s16 fogNear, s16 fogFar);
// Prerendered room background (was Room_DrawBackground2D's S2D copy): the converted image of `image` (a room
// handle) copied over the frame at (offsetX, offsetY); depth is neither tested nor written.
void Gfx3D_DrawBackground(const void* image, int width, int height, float offsetX, float offsetY);
// Offscreen image (was a draw into a small colour image, Player_DrawPauseImpl): between Begin and End the
// commands of the current buffer draw into a w x h image with this camera, cleared to opaque black (alpha
// stays 1, as the coverage bit of the original's RGBA16 image); the render state around the pass is kept, as
// the original drew it from a list of its own. Save (was PreRender_SaveFramebuffer) keeps the last image as
// the content of the w x h RGBA16 buffer dst: texture loads from dst sample it.
void Gfx3D_OffscreenBegin(int w, int h, const MtxF* proj, const MtxF* view);
void Gfx3D_OffscreenEnd(void);
void Gfx3D_OffscreenSave(const void* dst, int w, int h);
// 2DS layout: the commands recorded after this in every buffer (Gfx3D_Screen) or in the current one
// (Gfx3D_ScreenHere) draw on that screen (SCREEN_TOP / SCREEN_BOTTOM, engine/screen.h); each buffer's replay
// starts on the top screen.
void Gfx3D_Screen(int screen);
void Gfx3D_ScreenHere(int screen);
// While set, nothing is recorded (draw code run for its side effects only, e.g. pause pages not shown).
void Gfx3D_SetDiscard(bool discard);
// Full-screen rectangle in the current prim colour (was gDPFillRectangle over the whole screen).
void Gfx3D_FillScreenPrim(void);
// Segment holding a matrix (e.g. the billboard matrix in segment 1).
void Gfx3D_SetSegmentMtx(int seg, const MtxF* m);
// Array of matrices in a segment (frame memory, read at replay: may be filled after the bind).
void Gfx3D_SetSegmentMtxArray(int seg, const MtxF* m, int count);

// Skybox face (Skybox_Draw): original-layout vertices + triangle list, the face textures of the two blended
// sky files (TEXEL0, TEXEL1), drawn with the current state, texture scale 0x8000 and bilinear filtering.
void Gfx3D_DrawSky(const void* vtx, const u16* idx, int numIdx, const DsTexture* t0, const DsTexture* t1);

// ---- used by the 2D layer (engine/gfx2d.c): records a 2D command into a buffer
typedef void (*Gfx3DExecFn)(const void* payload);
void* Gfx3D_Record(GfxBuf buf, Gfx3DExecFn fn, size_t payloadSize);
