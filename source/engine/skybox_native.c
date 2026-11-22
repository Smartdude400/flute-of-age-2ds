// engine/skybox_native.c (rewritten: native) - see skybox_native.h
#include <stddef.h>
#include "engine/skybox_native.h"
#include "engine/rom_bind.h"
#include "environment.h"

void SkyboxNative_Request(struct EnvironmentContext* envCtx, void* dst, u32 vromStart, u32 size) {
    RomBind_Set(dst, vromStart, size);
    envCtx->nativeSkyboxLoad = 1;
}

void SkyboxNative_Tick(struct EnvironmentContext* envCtx) {
    if (envCtx->nativeSkyboxLoad == 1) {
        envCtx->nativeSkyboxLoad = 2;
    }
}

bool SkyboxNative_Poll(struct EnvironmentContext* envCtx) {
    if (envCtx->nativeSkyboxLoad == 2) {
        envCtx->nativeSkyboxLoad = 0;
        return true;
    }
    return false;
}

u32 SkyboxNative_VromOf(const void* dst) {
    return (u32)RomBind_Get(dst, NULL);
}

// ---- face quad lists (written by the translated face builders in z_vr_box.c)
#include <stdio.h>
#include "skybox.h"
#include "engine/dsobj.h"
#include "engine/log.h"

SkyFaceList* SkyFace_Begin(struct SkyboxContext* ctx, s32 listIndex, s32 faceNum) {
    SkyFaceList* l = (SkyFaceList*)((u8*)ctx->dListBuf + listIndex * SKYFACE_LIST_BYTES);
    l->face = faceNum;
    l->numIdx = 0;
    l->vtxBase = 0;
    return l;
}

void SkyFace_Vertex(SkyFaceList* list, s32 vtxBase) {
    list->vtxBase = vtxBase;
}

static void tri(SkyFaceList* l, s32 a, s32 b, s32 c) {
    if (l->numIdx + 3 <= SKYFACE_MAX_IDX) {
        l->idx[l->numIdx++] = l->vtxBase + a;
        l->idx[l->numIdx++] = l->vtxBase + b;
        l->idx[l->numIdx++] = l->vtxBase + c;
    }
}

void SkyFace_Quad(SkyFaceList* l, s32 v0, s32 v1, s32 v2, s32 v3, s32 flag) {
    // the original quadrangle command: the flag selects the diagonal (first vertex of both triangles)
    s32 v[4] = { v0, v1, v2, v3 };
    s32 r = flag & 3;
    tri(l, v[r], v[(r + 1) & 3], v[(r + 2) & 3]);
    tri(l, v[r], v[(r + 2) & 3], v[(r + 3) & 3]);
}

void SkyFace_End(SkyFaceList* list) {
    (void)list;
}

#define SKY_CACHE 4
static struct { u32 vrom; DsObj* obj; } sSkyCache[SKY_CACHE];
static int sSkyNext;

const struct DsTexture* SkyboxNative_FaceTexture(const void* staticSegment, int face) {
    u32 vrom = SkyboxNative_VromOf(staticSegment);
    if (vrom == 0) {
        return NULL;
    }
    DsObj* obj = NULL;
    for (int i = 0; i < SKY_CACHE; i++) {
        if (sSkyCache[i].vrom == vrom) {
            obj = sSkyCache[i].obj;
        }
    }
    if (obj == NULL) {
        char path[40];
        snprintf(path, sizeof(path), "/vrom/%08lX.pobj", (unsigned long)vrom);
        obj = DsObj_Load(path);
        if (obj == NULL) {
            return NULL;
        }
        if (sSkyCache[sSkyNext].obj != NULL) {
            DsObj_Free(sSkyCache[sSkyNext].obj);
        }
        sSkyCache[sSkyNext].vrom = vrom;
        sSkyCache[sSkyNext].obj = obj;
        sSkyNext = (sSkyNext + 1) % SKY_CACHE;
    }
    return DsObj_Texture(obj, face);
}
