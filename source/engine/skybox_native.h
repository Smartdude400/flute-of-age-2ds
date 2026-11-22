// engine/skybox_native.h (rewritten: native)
// - Skybox texture loads of Environment_UpdateSkybox (asynchronous in the original): request/poll.
// - Skybox face geometry: the original built one display list per face half in skyboxCtx->dListBuf; the
//   translated builders (z_vr_box.c) write native quad lists into the same buffer instead.
#pragma once
#include "engine/types.h"

struct EnvironmentContext;
void SkyboxNative_Request(struct EnvironmentContext* envCtx, void* dst, u32 vromStart, u32 size);
// Called at the start of each Environment_UpdateSkybox: a request issued during an earlier update
// becomes complete (A4: one update of latency, like the original DMA in practice).
void SkyboxNative_Tick(struct EnvironmentContext* envCtx);
// The original osRecvMesg(&loadQueue, NULL, OS_MESG_NOBLOCK) == 0.
bool SkyboxNative_Poll(struct EnvironmentContext* envCtx);
u32 SkyboxNative_VromOf(const void* dst);

// Original vertex layout (16 bytes): also the renderer's vertex layout (engine/pmesh.h PVertex).
typedef union SkyVtx {
    struct {
        s16 ob[3];
        u16 flag;
        s16 tc[2];
        u8 cn[4];
    } v;
} SkyVtx;

#define SKYFACE_LIST_BYTES (150 * 8) // one original display list of the face builders
#define SKYFACE_MAX_IDX ((SKYFACE_LIST_BYTES - 8) / 2)
typedef struct SkyFaceList {
    u16 face;
    u16 numIdx;
    u16 vtxBase;
    u16 pad;
    u16 idx[SKYFACE_MAX_IDX];
} SkyFaceList;

struct SkyboxContext;
SkyFaceList* SkyFace_Begin(struct SkyboxContext* ctx, s32 listIndex, s32 faceNum);
void SkyFace_Vertex(SkyFaceList* list, s32 vtxBase);
void SkyFace_Quad(SkyFaceList* list, s32 v0, s32 v1, s32 v2, s32 v3, s32 flag);
void SkyFace_End(SkyFaceList* list);
// converted face texture of the sky file bound to a texture buffer (staticSegments[i])
struct DsTexture;
const struct DsTexture* SkyboxNative_FaceTexture(const void* staticSegment, int face);
