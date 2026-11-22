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
// Handle -> native record (skeleton, animation, ...). Non-handles (real pointers) pass through.
void* Asset_Resolve(const void* p);
// Standalone texture symbol (TXMP table) -> uploaded DS texture, or NULL.
const DsTexture* Asset_FindTexture(const void* token);
