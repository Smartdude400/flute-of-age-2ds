// engine/object_native.h (rewritten: native)
#pragma once
#include "object.h"
#include "engine/dsobj.h"
DsObj* Vrom_LoadInto(u32 vromStart, void* region, u32 regionSize, u32* used);
DsObj* Vrom_Load(u32 vromStart);
void ObjectNative_Load(ObjectContext* objectCtx, s32 slot, s16 objectId); // synchronous
void ObjectNative_FreeFrom(ObjectContext* objectCtx, s32 firstSlot);
// Plain byte-record ROM data copied as-is (romfs/vrom/<vrom>.bin): true if such a file exists.
bool RomFile_ReadRaw(u32 vromStart, void* dst, u32 size);
