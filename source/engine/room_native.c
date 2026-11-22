// engine/room_native.c (rewritten: native)
#include <stdio.h>
#include "engine/room_native.h"
#include "engine/asset.h"
#include "engine/dsobj.h"
#include "engine/log.h"

static DsObj* sPending;

DsObj* Vrom_Load(u32 vromStart) {
    char path[40];
    snprintf(path, sizeof(path), "/vrom/%08lX.pobj", (unsigned long)vromStart);
    DsObj* obj = DsObj_Load(path);
    if (obj == NULL) {
        log_printf("Vrom_Load %08lx: missing\n", (unsigned long)vromStart);
    }
    return obj;
}

void RoomNative_Request(RoomContext* roomCtx, u32 vromStart) {
    (void)roomCtx;
    sPending = Vrom_Load(vromStart);
}

void* RoomNative_TakeLoaded(RoomContext* roomCtx) {
    (void)roomCtx;
    DsObj* obj = sPending;
    sPending = NULL;
    return obj;
}

void RoomNative_Bind(Room* room) {
    Asset_SetSegment(3, (DsObj*)room->segment);
}

void* RoomNative_Header(Room* room) {
    Asset_SetSegment(3, (DsObj*)room->segment);
    return Asset_Resolve((void*)(uintptr_t)(0x80000000u | 0x03000000u));
}

void RoomNative_Free(Room* room) {
    if (room->segment != NULL) {
        if (Asset_GetSegment(3) == room->segment) {
            Asset_SetSegment(3, NULL);
        }
        DsObj_Free((DsObj*)room->segment);
    }
}

void RoomNative_StubImage(void) {
    static bool s;
    if (!s) {
        s = true;
        log_printf("[STUB] Room_DrawImage (prerendered rooms)\n");
    }
}

void* RoomNative_HeaderOf(Room* room) {
    return RoomNative_Header(room);
}
