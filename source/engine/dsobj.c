// engine/dsobj.c (rewritten: native)
// Loads a converted object container (.pobj; C type name kept from the DS port) from RomFS. Only the
// resident part (header, tables, meshes, extras) is read into RAM, into a caller-provided region
// when it fits (e.g. the game's object space) or the heap. Texture blobs stay in the file.
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/dsobj.h"
#include "engine/res.h"
#include "engine/crc32.h"
#include "engine/log.h"
#include "engine/ptex.h"

typedef struct { u32 off, size; } TexEnt;
typedef struct { u32 src, off; } MeshEnt;
typedef struct { char tag[4]; u32 off, size; } ExtraEnt;

static u32 sHeapFallbacks;

u32 DsObj_HeapFallbacks(void) {
    return sHeapFallbacks;
}

DsObj* DsObj_LoadInto(const char* path, void* region, u32 regionSize, u32* outUsed) {
    char full[72];
    snprintf(full, sizeof(full), "romfs:%s", path);
    FILE* f = fopen(full, "rb");
    if (f == NULL) {
        log_printf("pobj %s: missing\n", path);
        return NULL;
    }
    DsObjHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 || memcmp(hdr.magic, "POBJ", 4) != 0 || hdr.version != 1) {
        log_printf("pobj %s: bad header\n", path);
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    u32 fileSize = ftell(f);
    u32 resident = fileSize;
    if (hdr.numTex != 0) {
        TexEnt first;
        fseek(f, hdr.texTableOff, SEEK_SET);
        fread(&first, sizeof(first), 1, f);
        resident = first.off; // textures are stored last, in table order
    }
    u32 need = (resident + 31) & ~31u;
    u8* data;
    bool inRegion = (region != NULL) && (need <= regionSize);
    if (inRegion) {
        data = region;
    } else {
        data = linearMemAlign(need, 0x80); // GPU-visible: vertex/index arrays are drawn from here
        if (region != NULL) {
            sHeapFallbacks++;
            log_printf("pobj %s: %lu > region %lu, heap\n", path, (unsigned long)need, (unsigned long)regionSize);
        }
    }
    if (data == NULL) {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);
    if (fread(data, 1, resident, f) != resident) {
        log_printf("pobj %s: short read\n", path);
    }
    fclose(f);
    GSPGPU_FlushDataCache(data, resident);

    DsObj* obj = calloc(1, sizeof(DsObj));
    obj->data = data;
    obj->size = resident;
    obj->ownsData = !inRegion;
    obj->hdr = (const DsObjHeader*)data;
    obj->tex = calloc(hdr.numTex ? hdr.numTex : 1, sizeof(DsTexture));
    obj->numTex = hdr.numTex;
    strncpy(obj->path, path, sizeof(obj->path) - 1);
    if (outUsed) {
        *outUsed = inRegion ? need : 0;
    }
    return obj;
}

DsObj* DsObj_Load(const char* path) {
    return DsObj_LoadInto(path, NULL, 0, NULL);
}

void DsObj_Free(DsObj* obj) {
    if (obj == NULL) {
        return;
    }
    for (int i = 0; i < obj->numTex; i++) {
        if (obj->tex[i].state == 1) {
            C3D_TexDelete(&obj->tex[i].tex);
        }
    }
    free(obj->tex);
    if (obj->ownsData) {
        linearFree(obj->data);
    }
    free(obj);
}

const DsTexture* DsObj_Texture(const DsObj* obj, int index) {
    if (index < 0 || index >= obj->numTex) {
        return NULL;
    }
    if (obj->texOwner != NULL && index < obj->texOwner->numTex && obj->texOwner->tex[index].state == 1) {
        return &obj->texOwner->tex[index];
    }
    DsTexture* t = &obj->tex[index];
    if (t->state == 0) {
        const TexEnt* te = (const TexEnt*)(obj->data + obj->hdr->texTableOff);
        char full[72];
        snprintf(full, sizeof(full), "romfs:%s", obj->path);
        FILE* f = fopen(full, "rb");
        PTexHeader h;
        bool ok = false;
        if (f != NULL) {
            fseek(f, te[index].off, SEEK_SET);
            ok = fread(&h, sizeof(h), 1, f) == 1 && memcmp(h.magic, "PTEX", 4) == 0 &&
                 C3D_TexInit(&t->tex, h.width, h.height, (GPU_TEXCOLOR)h.format);
            if (ok) {
                ok = h.dataSize <= t->tex.size && fread(t->tex.data, 1, h.dataSize, f) == h.dataSize;
                if (ok) {
                    C3D_TexFlush(&t->tex);
                    t->w = h.width;
                    t->h = h.height;
                    t->format = h.format;
                } else {
                    C3D_TexDelete(&t->tex);
                }
            }
            fclose(f);
        }
        t->state = ok ? 1 : 2;
        if (!ok) {
            log_printf("tex %d of %s: load failed\n", index, obj->path);
        }
    }
    return t->state == 1 ? t : NULL;
}

const DsMeshHdr* DsObj_FindMesh(const DsObj* obj, u32 srcAddr) {
    const MeshEnt* me = (const MeshEnt*)(obj->data + obj->hdr->meshTableOff);
    int lo = 0, hi = obj->hdr->numMesh - 1;
    while (lo <= hi) { // table is sorted by srcAddr
        int mid = (lo + hi) / 2;
        if (me[mid].src == srcAddr) {
            return (const DsMeshHdr*)(obj->data + me[mid].off);
        }
        if (me[mid].src < srcAddr) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return NULL;
}

const void* DsObj_FindExtra(const DsObj* obj, const char tag[4], u32* outSize) {
    const ExtraEnt* ee = (const ExtraEnt*)(obj->data + obj->hdr->extraTableOff);
    for (int i = 0; i < obj->hdr->numExtra; i++) {
        if (memcmp(ee[i].tag, tag, 4) == 0) {
            if (outSize) {
                *outSize = ee[i].size;
            }
            return obj->data + ee[i].off;
        }
    }
    return NULL;
}
