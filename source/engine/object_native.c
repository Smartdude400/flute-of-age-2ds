// engine/object_native.c (rewritten: native)
// Places converted objects (resident part of .pobj) inside the game's object space, in slot order,
// mirroring how the original packed object files there (slot i follows slot i-1). Falls back to
// the heap if an object does not fit. The original object-space reservation from the game arena is
// kept unchanged, so the actor heap size is identical to the original.
#include <stdio.h>
#include "engine/object_native.h"
#include "engine/asset.h"
#include "engine/log.h"

#define MAX_SLOTS 19

static u8* sSlotEnd[MAX_SLOTS];

DsObj* Vrom_LoadInto(u32 vromStart, void* region, u32 regionSize, u32* used) {
    char path[40];
    snprintf(path, sizeof(path), "/vrom/%08lX.pobj", (unsigned long)vromStart);
    return DsObj_LoadInto(path, region, regionSize, used);
}

void ObjectNative_Load(ObjectContext* objectCtx, s32 slot, s16 objectId) {
    ObjectEntry* entry = &objectCtx->slots[slot];
    u8* base = (slot == 0) ? (u8*)objectCtx->spaceStart : sSlotEnd[slot - 1];
    u8* end = (u8*)objectCtx->spaceEnd;
    u32 used = 0;

    if (entry->dsObj != NULL) {
        DsObj_Free(entry->dsObj);
        entry->dsObj = NULL;
    }
    if (base == NULL || base > end) {
        base = end;
    }
    entry->dsObj = Vrom_LoadInto(gObjectTable[objectId].vromStart, base, (u32)(end - base), &used);
    if (slot < MAX_SLOTS) {
        sSlotEnd[slot] = base + used;
    }
    if (entry->dsObj == NULL) {
        log_printf("object %d (vrom %08lx) not converted\n", objectId, (unsigned long)gObjectTable[objectId].vromStart);
    }
}

void ObjectNative_FreeFrom(ObjectContext* objectCtx, s32 firstSlot) {
    for (s32 i = firstSlot; i < MAX_SLOTS; i++) {
        if (objectCtx->slots[i].dsObj != NULL) {
            DsObj_Free(objectCtx->slots[i].dsObj);
            objectCtx->slots[i].dsObj = NULL;
        }
    }
}

bool RomFile_ReadRaw(u32 vromStart, void* dst, u32 size) {
    char path[48];
    snprintf(path, sizeof(path), "romfs:/vrom/%08lX.bin", (unsigned long)vromStart);
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return false;
    }
    size_t n = fread(dst, 1, size, f);
    fclose(f);
    return n == size;
}
