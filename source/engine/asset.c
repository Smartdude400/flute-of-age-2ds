// engine/asset.c (rewritten: native)
#include "engine/types.h"
#include <string.h>
#include "engine/asset.h"
#include "engine/gfx3d.h"
#include "engine/log.h"

static DsObj* sSeg[16];
// CPU segments the game pointed at its own memory (gSegments[n] = buffer): handles resolve into it
static u8* sRawCpu[16];


typedef struct { u32 addr, off; } TableEnt;
static const char* const kTables[] = { "SKEL", "ANIM", "PANM", "COLL", "SCEN", "ROOM", "LANM", "PATH", "CSDT", "ARRY" };
// Note: SCEN/ROOM tables are keyed by the segment base address (0x02000000 / 0x03000000).

// Every converted record is {u32 nRelocs; u32 relocs[n]; payload}. Relocate payload-relative
// offsets into absolute pointers (once per load; marked by setting nRelocs' top bit).
static void relocate_table(u8* table) {
    u32 count = *(u32*)table;
    TableEnt* ents = (TableEnt*)(table + 4);
    for (u32 i = 0; i < count; i++) {
        u32* rec = (u32*)(table + ents[i].off);
        if (rec[0] & 0x80000000u) {
            continue;
        }
        u32 n = rec[0];
        u8* payload = (u8*)(rec + 1 + n);
        for (u32 r = 0; r < n; r++) {
            u32* field = (u32*)(payload + rec[1 + r]);
            *field = (u32)(uintptr_t)(payload + *field);
        }
        rec[0] |= 0x80000000u;
    }
}

void Asset_SetSegment(int seg, DsObj* obj) {
    Gfx3D_SetSegmentObj(seg, obj);
    Asset_SetCpuSegment(seg, obj);
}

void Asset_SetCpuSegment(int seg, DsObj* obj) {
    sSeg[seg & 15] = obj;
    sRawCpu[seg & 15] = NULL;
    // once per load: the game binds objects every frame, also while the pause menu has overwritten their data
    if (obj == NULL || obj->relocated) {
        return;
    }
    obj->relocated = true;
    for (unsigned t = 0; t < sizeof(kTables) / sizeof(kTables[0]); t++) {
        u8* table = (u8*)DsObj_FindExtra(obj, kTables[t], NULL);
        if (table != NULL) {
            relocate_table(table);
        }
    }
}

DsObj* Asset_GetSegment(int seg) {
    return sSeg[seg & 15];
}

static void* find_in_table(const u8* table, u32 src) {
    u32 count = *(const u32*)table;
    const TableEnt* ents = (const TableEnt*)(table + 4);
    int lo = 0, hi = (int)count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (ents[mid].addr == src) {
            const u32* rec = (const u32*)(table + ents[mid].off);
            u32 n = rec[0] & 0x7FFFFFFFu;
            return (void*)(rec + 1 + n);
        }
        if (ents[mid].addr < src) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return NULL;
}

const DsTexture* Asset_FindTexture(const void* token);

void Asset_CopyRecords(const DsObj* obj, u8* dst, u32 size) {
    for (unsigned t = 0; t < sizeof(kTables) / sizeof(kTables[0]); t++) {
        u32 tableSize = 0;
        const u8* table = DsObj_FindExtra(obj, kTables[t], &tableSize);
        if (table == NULL) {
            continue;
        }
        u32 count = *(const u32*)table;
        const TableEnt* ents = (const TableEnt*)(table + 4);
        for (u32 i = 0; i < count; i++) {
            const u32* rec = (const u32*)(table + ents[i].off);
            u32 off = ents[i].addr & 0xFFFFFF;
            if ((rec[0] & 0x7FFFFFFFu) != 0 || off >= size) {
                continue; // records holding pointers stay with their object
            }
            const u8* payload = (const u8*)(rec + 1);
            u32 n = (i + 1 < count ? ents[i + 1].off : tableSize) - ents[i].off - 4; // to the next record
            if (n > size - off) {
                n = size - off;
            }
            memcpy(dst + off, payload, n); // in address order: alignment padding is overwritten by the next one
        }
    }
}

void Asset_SetCpuSegmentRaw(int seg, void* base) {
    sRawCpu[seg & 15] = base;
}

void* Asset_Resolve(const void* p) {
    if (!ASSET_IS_TOKEN(p)) {
        return (void*)p;
    }
    if (sRawCpu[ASSET_SEG(p)] != NULL) {
        return sRawCpu[ASSET_SEG(p)] + (ASSET_SRC(p) & 0xFFFFFF);
    }
    DsObj* obj = sSeg[ASSET_SEG(p)];
    u32 src = ASSET_SRC(p);
    if (obj != NULL) {
        for (unsigned t = 0; t < sizeof(kTables) / sizeof(kTables[0]); t++) {
            const u8* table = DsObj_FindExtra(obj, kTables[t], NULL);
            if (table != NULL) {
                void* r = find_in_table(table, src);
                if (r != NULL) {
                    return r;
                }
            }
        }
    }
    // textures and display lists are handles for the renderer (the original converted their segmented
    // address to a pointer only to hand it back to the graphics pipeline): pass the handle through
    if (obj != NULL && (Asset_FindTexture(p) != NULL || DsObj_FindMesh(obj, src) != NULL)) {
        return (void*)p;
    }
    log_printf("Asset_Resolve: unresolved %08lx (caller %p, seg %s)\n", (unsigned long)(uintptr_t)p,
               __builtin_return_address(0), obj != NULL ? obj->path : "-");
    return NULL;
}

const DsTexture* Asset_FindTexture(const void* token) {
    if (!ASSET_IS_TOKEN(token)) {
        return NULL;
    }
    DsObj* obj = sSeg[ASSET_SEG(token)];
    if (obj == NULL) {
        return NULL;
    }
    const u32* t = DsObj_FindExtra(obj, "TXMP", NULL);
    if (t == NULL) {
        return NULL;
    }
    u32 count = t[0], src = ASSET_SRC(token);
    for (u32 i = 0; i < count; i++) {
        if (t[1 + i * 2] == src) {
            return DsObj_Texture(obj, t[2 + i * 2]);
        }
    }
    return NULL;
}
