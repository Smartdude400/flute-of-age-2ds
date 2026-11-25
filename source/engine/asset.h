// (rewritten: native)
// engine/asset.h - resolves asset handles (TOKEN | segmented address) to native converted data.
#pragma once
#include "engine/types.h"
#include "engine/dsobj.h"

#define ASSET_TOKEN_BIT 0x80000000u
#define ASSET_IS_TOKEN(p) ((((uintptr_t)(p)) & 0xF0000000u) == ASSET_TOKEN_BIT)
#define ASSET_SEG(p) ((((uintptr_t)(p)) >> 24) & 0xF)
#define ASSET_SRC(p) (((uintptr_t)(p)) & 0x0FFFFFFFu)

void Asset_SetSegment(int seg, DsObj* obj); // also applies relocations of the object's extras (once)
DsObj* Asset_GetSegment(int seg);
// CPU side only (was `gSegments[seg] = ...`): handle resolution by game code; the renderer's binding
// (gSPSegment) is Gfx3D_SetSegmentObj.
void Asset_SetCpuSegment(int seg, DsObj* obj);
// CPU side, to game memory (was `gSegments[seg] = OS_K0_TO_PHYSICAL(buffer)`): handles resolve into it
void Asset_SetCpuSegmentRaw(int seg, void* base);
// Copies the object's pointer-free records (arrays, player animation headers, ...) whose original offset is
// below `size` to dst + that offset: a native-layout image of the first `size` bytes of the original file,
// for code that kept such a chunk of a file (the pause menu's chunk of gameplay_keep).
void Asset_CopyRecords(const DsObj* obj, u8* dst, u32 size);
// Handle -> native record (skeleton, animation, ...). Non-handles (real pointers) pass through.
void* Asset_Resolve(const void* p);
// Standalone texture symbol (TXMP table) -> uploaded DS texture, or NULL.
const DsTexture* Asset_FindTexture(const void* token);
