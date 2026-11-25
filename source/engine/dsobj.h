// (rewritten: native)
// engine/dsobj.h - converted object container (.pobj, written by tools/dl2pica.py build_pobj). The C
// type keeps the name DsObj from the DS port because game structures (ObjectEntry, Player) store it.
#pragma once
#include <citro3d.h>
#include "engine/types.h"
#include "engine/pmesh.h"

typedef struct DsObjHeader {
    char magic[4];
    u16 version, numTex, numMesh, numExtra;
    u32 texTableOff, meshTableOff, extraTableOff;
} DsObjHeader;

// A converted texture: PICA200 texture loaded into linear memory on first use.
typedef struct DsTexture {
    C3D_Tex tex;
    u16 w, h; // power-of-two size
    u8 state; // 0 not loaded, 1 ready, 2 failed
    u8 format;
} DsTexture;

typedef PMeshHdr DsMeshHdr;

typedef struct DsObj {
    u8* data;
    u32 size;      // resident bytes (textures excluded)
    bool ownsData; // allocated here (vs placed in a caller region)
    const DsObjHeader* hdr;
    DsTexture* tex;
    u16 numTex;    // kept here: the resident data may be overwritten while the object is unused (pause menu)
    bool relocated; // payload pointers fixed up (engine/asset.c), once per load
    const struct DsObj* texOwner; // another load of the same file whose loaded textures this one uses
    char path[40]; // RomFS path, for loading textures
} DsObj;

DsObj* DsObj_Load(const char* path);
// Loads the resident part into `region` if it fits (outUsed = bytes used, 0 if not placed there).
DsObj* DsObj_LoadInto(const char* path, void* region, u32 regionSize, u32* outUsed);
u32 DsObj_HeapFallbacks(void);
void DsObj_Free(DsObj* obj);
const DsMeshHdr* DsObj_FindMesh(const DsObj* obj, u32 srcAddr);
const DsTexture* DsObj_Texture(const DsObj* obj, int index); // loads on first use
const void* DsObj_FindExtra(const DsObj* obj, const char tag[4], u32* outSize);
