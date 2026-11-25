// adapted from flute-of-age/src/code/z_room.c
#include "room.h"
#include "gfx_setupdl.h"

#include "versions.h"
#include "actor.h"
#include "alignment.h"
#include "array_count.h"
#include "attributes.h"
#include "bgcheck.h"
#include "camera.h"
#include "game.h"
#include "gfx.h"
#include "map.h"
#if PLATFORM_N64
#endif
#include "printf.h"
#include "regs.h"
#include "stack_pad.h"
#include "sys_matrix.h"
#include "translation.h"
#include "audio.h"
#include "play_state.h"
#include "player.h"
#include "save.h"
#include "engine/gfx3d.h"
#include "engine/asset.h"
#include "engine/room_native.h"
#include "scene.h"
#include "skin_matrix.h"
#include "z_math.h"

#include "libu64/debug.h"
#include "libu64/pad.h"
#include "game_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

Vec3f D_801270A0 = { 0.0f, 0.0f, 0.0f };

// D_801270B0: unused render-state display list, not ported

void Room_DrawNormal(PlayState* play, Room* room, u32 flags);
void Room_DrawImage(PlayState* play, Room* room, u32 flags);
void Room_DrawCullable(PlayState* play, Room* room, u32 flags);

void (*sRoomDrawHandlers[ROOM_SHAPE_TYPE_MAX])(PlayState* play, Room* room, u32 flags) = {
    Room_DrawNormal,   // ROOM_SHAPE_TYPE_NORMAL
    Room_DrawImage,    // ROOM_SHAPE_TYPE_IMAGE
    Room_DrawCullable, // ROOM_SHAPE_TYPE_CULLABLE
};

void func_80095AA0(UNUSED PlayState* play, UNUSED Room* room, UNUSED Input* input, UNUSED s32 arg3) {
}

void Room_DrawNormal(PlayState* play, Room* room, u32 flags) {
    s32 i;
    RoomShapeNormal* roomShape;
    RoomShapeDListsEntry* entry;


    if (flags & ROOM_DRAW_OPA) {
        func_800342EC(&D_801270A0, play);
        RoomNative_Bind(room); // gSPSegment(POLY_OPA_DISP++, 0x03, room->segment)
        func_80093C80(play);
        GFX_TO(OPA, Gfx3D_LoadMtxF(&gIdentityMtxF));
    }

    if (flags & ROOM_DRAW_XLU) {
        func_8003435C(&D_801270A0, play);
        RoomNative_Bind(room); // gSPSegment(POLY_XLU_DISP++, 0x03, room->segment)
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        GFX_TO(XLU, Gfx3D_LoadMtxF(&gIdentityMtxF));
    }

    roomShape = &room->roomShape->normal;
    entry = Asset_Resolve(roomShape->entries);
    for (i = 0; i < roomShape->numEntries; i++) {
        if ((flags & ROOM_DRAW_OPA) && (entry->opa != NULL)) {
            GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(entry->opa)));
        }

        if ((flags & ROOM_DRAW_XLU) && (entry->xlu != NULL)) {
            GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(entry->xlu)));
        }

        entry++;
    }

}

typedef enum RoomCullableDebugMode {
    /* 0 */ ROOM_CULL_DEBUG_MODE_OFF,
    /* 1 */ ROOM_CULL_DEBUG_MODE_UP_TO_TARGET,
    /* 2 */ ROOM_CULL_DEBUG_MODE_ONLY_TARGET
} RoomCullableDebugMode;

typedef struct RoomShapeCullableEntryLinked {
    /* 0x00 */ RoomShapeCullableEntry* entry;
    /* 0x04 */ f32 boundsNearZ;
    /* 0x08 */ struct RoomShapeCullableEntryLinked* prev;
    /* 0x0C */ struct RoomShapeCullableEntryLinked* next;
} RoomShapeCullableEntryLinked; // size = 0x10

/**
 * Handle room drawing for the "cullable" type of room shape.
 *
 * Each entry referenced by the room shape struct is attached to display lists, and a position and radius indicating the
 * bounding sphere for the geometry drawn.
 * The first step Z-sorts the entries, and excludes the entries with a bounding sphere that is entirely before or
 * beyond the rendered depth range.
 * The second step draws the entries that remain, from nearest to furthest.
 */
void Room_DrawCullable(PlayState* play, Room* room, u32 flags) {
    RoomShapeCullable* roomShape;
    RoomShapeCullableEntry* roomShapeCullableEntry;
    RoomShapeCullableEntryLinked linkedEntriesBuffer[ROOM_SHAPE_CULLABLE_MAX_ENTRIES];
    RoomShapeCullableEntryLinked* head = NULL;
    RoomShapeCullableEntryLinked* tail = NULL;
    RoomShapeCullableEntryLinked* iter;
    STACK_PAD(s32);
    RoomShapeCullableEntryLinked* insert;
    s32 j;
    s32 i;
    Vec3f pos;
    Vec3f projectedPos;
    f32 projectedW;
    STACK_PAD(s32);
    RoomShapeCullableEntry* roomShapeCullableEntries;
    RoomShapeCullableEntry* roomShapeCullableEntryIter;
    f32 entryBoundsNearZ;


    if (flags & ROOM_DRAW_OPA) {
        func_800342EC(&D_801270A0, play);
        RoomNative_Bind(room); // gSPSegment(POLY_OPA_DISP++, 0x03, room->segment)
        func_80093C80(play);
        GFX_TO(OPA, Gfx3D_LoadMtxF(&gIdentityMtxF));
    }

    if (flags & ROOM_DRAW_XLU) {
        func_8003435C(&D_801270A0, play);
        RoomNative_Bind(room); // gSPSegment(POLY_XLU_DISP++, 0x03, room->segment)
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        GFX_TO(XLU, Gfx3D_LoadMtxF(&gIdentityMtxF));
    }

    if (1) {}

    roomShape = &room->roomShape->cullable;
    roomShapeCullableEntry = Asset_Resolve(roomShape->entries);
    insert = linkedEntriesBuffer;

    ASSERT(roomShape->numEntries <= ROOM_SHAPE_CULLABLE_MAX_ENTRIES, "polygon2->num <= SHAPE_SORT_MAX", "../z_room.c",
           317);

    roomShapeCullableEntries = roomShapeCullableEntry;

    // Pick and sort entries by depth
    for (i = 0; i < roomShape->numEntries; i++, roomShapeCullableEntry++) {

        // Project the entry position, to get the depth it is at.
        pos.x = roomShapeCullableEntry->boundsSphereCenter.x;
        pos.y = roomShapeCullableEntry->boundsSphereCenter.y;
        pos.z = roomShapeCullableEntry->boundsSphereCenter.z;
        SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, &pos, &projectedPos, &projectedW);

        // If the entry bounding sphere isn't fully before the rendered depth range
        if (-(f32)roomShapeCullableEntry->boundsSphereRadius < projectedPos.z) {

            // Compute the depth of the nearest point in the entry's bounding sphere
            entryBoundsNearZ = projectedPos.z - roomShapeCullableEntry->boundsSphereRadius;

            // If the entry bounding sphere isn't fully beyond the rendered depth range
            if (entryBoundsNearZ < play->lightCtx.zFar) {

                // This entry will be rendered
                insert->entry = roomShapeCullableEntry;
                insert->boundsNearZ = entryBoundsNearZ;

                // Insert into the linked list, ordered by ascending depth of the nearest point in the bounding sphere
                iter = head;
                if (iter == NULL) {
                    head = tail = insert;
                    insert->prev = insert->next = NULL;
                } else {
                    do {
                        if (insert->boundsNearZ < iter->boundsNearZ) {
                            break;
                        }
                        iter = iter->next;
                    } while (iter != NULL);

                    if (iter == NULL) {
                        insert->prev = tail;
                        insert->next = NULL;
                        tail->next = insert;
                        tail = insert;
                    } else {
                        insert->prev = iter->prev;
                        if (insert->prev == NULL) {
                            head = insert;
                        } else {
                            insert->prev->next = insert;
                        }
                        iter->prev = insert;
                        insert->next = iter;
                    }
                }

                insert++;
            }
        }
    }

    // if this is real then I might not be
    R_ROOM_CULL_NUM_ENTRIES = roomShape->numEntries & 0xFFFF & 0xFFFF & 0xFFFF;

    // Draw entries, from nearest to furthest
    for (i = 1; head != NULL; head = head->next, i++) {
        DListRef displayList;

        roomShapeCullableEntry = head->entry;

        if (R_ROOM_CULL_DEBUG_MODE != ROOM_CULL_DEBUG_MODE_OFF) {
            // Debug mode drawing

            // This loop does nothing
            roomShapeCullableEntryIter = roomShapeCullableEntries;
            for (j = 0; j < roomShape->numEntries; j++, roomShapeCullableEntryIter++) {
                if (roomShapeCullableEntry == roomShapeCullableEntryIter) {
                    break;
                }
            }

            if (((R_ROOM_CULL_DEBUG_MODE == ROOM_CULL_DEBUG_MODE_UP_TO_TARGET) && (i <= R_ROOM_CULL_DEBUG_TARGET)) ||
                ((R_ROOM_CULL_DEBUG_MODE == ROOM_CULL_DEBUG_MODE_ONLY_TARGET) && (i == R_ROOM_CULL_DEBUG_TARGET))) {
                if (flags & ROOM_DRAW_OPA) {
                    displayList = roomShapeCullableEntry->opa;
                    if (displayList != NULL) {
                        GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(displayList)));
                    }
                }

                if (flags & ROOM_DRAW_XLU) {
                    displayList = roomShapeCullableEntry->xlu;
                    if (displayList != NULL) {
                        GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(displayList)));
                    }
                }
            }
        } else {
            if (flags & ROOM_DRAW_OPA) {
                displayList = roomShapeCullableEntry->opa;
                if (displayList != NULL) {
                    GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(displayList)));
                }
            }

            if (flags & ROOM_DRAW_XLU) {
                displayList = roomShapeCullableEntry->xlu;
                if (displayList != NULL) {
                    GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(displayList)));
                }
            }
        }
    }

    R_ROOM_CULL_USED_ENTRIES = i - 1;

}

#define JPEG_MARKER 0xFFD8FFE0

/**
 * If the data is JPEG, decode it and overwrite the initial data with the result.
 * Uses the depth frame buffer as temporary storage.
 */
// Room_DecodeJpeg: native: the backgrounds are decoded offline (tools/build_scene.py background_texture).

// native: the original built an S2D background-copy command (copy mode: the image replaces the colour, depth is
// neither tested nor written) for the 320x240 image at the given offset (camera quake).
void Room_DrawBackground2D(struct Gfx** gfxP, void* tex, void* tlut, u16 width, u16 height, u8 fmt, u8 siz, u16 tlutMode,
                           u16 tlutCount, f32 offsetX, f32 offsetY) {
    (void)gfxP;
    (void)tlut;
    (void)fmt;
    (void)siz;
    (void)tlutMode;
    (void)tlutCount;
    GFX_TO(OPA, Gfx3D_DrawBackground(tex, width, height, offsetX, offsetY));
}

#if OOT_VERSION < PAL_1_0
void func_8007FF50(struct Gfx** gfxP, void* tex, void* tlut, u16 width, u16 height, u8 fmt, u8 siz, u16 tlutMode,
                   u16 tlutCount) {
    Room_DrawBackground2D(gfxP, tex, tlut, width, height, fmt, siz, tlutMode, tlutCount, 0.0f, 0.0f);
}
#endif

#define ROOM_IMAGE_NODRAW_BACKGROUND (1 << 0)
#define ROOM_IMAGE_NODRAW_OPA (1 << 1)
#define ROOM_IMAGE_NODRAW_XLU (1 << 2)

void Room_DrawImageSingle(PlayState* play, Room* room, u32 flags) {
    Camera* activeCam;
    RoomShapeImageSingle* roomShape;
    RoomShapeDListsEntry* entry;
    u32 isFixedCamera;
    u32 drawBackground;
    u32 drawOpa;
    u32 drawXlu;

    activeCam = GET_ACTIVE_CAM(play);
    isFixedCamera = (activeCam->setting == CAM_SET_PREREND_FIXED);
    roomShape = &room->roomShape->image.single;
    entry = roomShape->base.entry;
    drawBackground = (flags & ROOM_DRAW_OPA) && isFixedCamera && (roomShape->source != NULL) &&
                     !(R_ROOM_IMAGE_NODRAW_FLAGS & ROOM_IMAGE_NODRAW_BACKGROUND);
    drawOpa = (flags & ROOM_DRAW_OPA) && (entry->opa != NULL) && !(R_ROOM_IMAGE_NODRAW_FLAGS & ROOM_IMAGE_NODRAW_OPA);
    drawXlu = (flags & ROOM_DRAW_XLU) && (entry->xlu != NULL) && !(R_ROOM_IMAGE_NODRAW_FLAGS & ROOM_IMAGE_NODRAW_XLU);

    if (drawOpa || drawBackground) {
        RoomNative_Bind(room); // gSPSegment(POLY_OPA_DISP++, 0x03, room->segment)

        if (drawOpa) {
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
            GFX_TO(OPA, Gfx3D_LoadMtxF(&gIdentityMtxF));
            GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(entry->opa)));
        }

        if (drawBackground) {
            // (native: no microcode switch; the background is a native command)
            Vec3f quakeOffset;

            quakeOffset = Camera_GetQuakeOffset(activeCam);
            Room_DrawBackground2D(NULL, roomShape->source, roomShape->tlut, roomShape->width, roomShape->height,
                                  roomShape->fmt, roomShape->siz, roomShape->tlutMode, roomShape->tlutCount,
                                  (quakeOffset.x + quakeOffset.z) * 1.2f + quakeOffset.y * 0.6f,
                                  quakeOffset.y * 2.4f + (quakeOffset.x + quakeOffset.z) * 0.3f);
        }
    }

    if (drawXlu) {
        RoomNative_Bind(room); // gSPSegment(POLY_XLU_DISP++, 0x03, room->segment)
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        GFX_TO(XLU, Gfx3D_LoadMtxF(&gIdentityMtxF));
        GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(entry->xlu)));
    }
}

RoomShapeImageMultiBgEntry* Room_GetImageMultiBgEntry(RoomShapeImageMulti* roomShapeImageMulti, PlayState* play) {
    Camera* activeCam = GET_ACTIVE_CAM(play);
    s32 bgCamIndex = activeCam->bgCamIndex;
    s16 overrideBgCamIndex;
    Player* player;
    RoomShapeImageMultiBgEntry* bgEntry;
    s32 i;

    overrideBgCamIndex = ((BgCamFuncData*)BgCheck_GetBgCamFuncDataImpl(&play->colCtx, bgCamIndex, BGCHECK_SCENE))
                             ->roomImageOverrideBgCamIndex;
    if (overrideBgCamIndex >= 0) {
        bgCamIndex = overrideBgCamIndex;
    }

    player = GET_PLAYER(play);
    player->actor.params = PARAMS_GET_NOSHIFT(player->actor.params, 8, 8) | bgCamIndex;

    bgEntry = roomShapeImageMulti->backgrounds;
    for (i = 0; i < roomShapeImageMulti->numBackgrounds; i++) {
        if (bgEntry->bgCamIndex == bgCamIndex) {
            return bgEntry;
        }
        bgEntry++;
    }

    PRINTF(VT_COL(RED, WHITE) T("z_room.c:カメラＩＤに一致するデータが存在しません camid=%d\n",
                                "z_room.c: Data consistent with camera id does not exist camid=%d\n") VT_RST,
           bgCamIndex);
    LogUtils_HungupThread("../z_room.c", 726);

    return NULL;
}

void Room_DrawImageMulti(PlayState* play, Room* room, u32 flags) {
    Camera* activeCam;
    RoomShapeImageMulti* roomShape;
    RoomShapeImageMultiBgEntry* bgEntry;
    RoomShapeDListsEntry* dListsEntry;
    u32 isFixedCamera;
    u32 drawBackground;
    u32 drawOpa;
    u32 drawXlu;

    activeCam = GET_ACTIVE_CAM(play);
    isFixedCamera = (activeCam->setting == CAM_SET_PREREND_FIXED);
    roomShape = &room->roomShape->image.multi;
    dListsEntry = roomShape->base.entry;
    bgEntry = Room_GetImageMultiBgEntry(roomShape, play);
    drawBackground = (flags & ROOM_DRAW_OPA) && isFixedCamera && (bgEntry->source != NULL) &&
                     !(R_ROOM_IMAGE_NODRAW_FLAGS & ROOM_IMAGE_NODRAW_BACKGROUND);
    drawOpa =
        (flags & ROOM_DRAW_OPA) && (dListsEntry->opa != NULL) && !(R_ROOM_IMAGE_NODRAW_FLAGS & ROOM_IMAGE_NODRAW_OPA);
    drawXlu =
        (flags & ROOM_DRAW_XLU) && (dListsEntry->xlu != NULL) && !(R_ROOM_IMAGE_NODRAW_FLAGS & ROOM_IMAGE_NODRAW_XLU);

    if (drawOpa || drawBackground) {
        RoomNative_Bind(room); // gSPSegment(POLY_OPA_DISP++, 0x03, room->segment)

        if (drawOpa) {
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
            GFX_TO(OPA, Gfx3D_LoadMtxF(&gIdentityMtxF));
            GFX_TO(OPA, Gfx3D_DrawDL((u32)(uintptr_t)(dListsEntry->opa)));
        }

        if (drawBackground) {
            // (native: no microcode switch; the background is a native command)
            Vec3f quakeOffset;

            quakeOffset = Camera_GetQuakeOffset(activeCam);
            Room_DrawBackground2D(NULL, bgEntry->source, bgEntry->tlut, bgEntry->width, bgEntry->height,
                                  bgEntry->fmt, bgEntry->siz, bgEntry->tlutMode, bgEntry->tlutCount,
                                  (quakeOffset.x + quakeOffset.z) * 1.2f + quakeOffset.y * 0.6f,
                                  quakeOffset.y * 2.4f + (quakeOffset.x + quakeOffset.z) * 0.3f);
        }
    }

    if (drawXlu) {
        RoomNative_Bind(room); // gSPSegment(POLY_XLU_DISP++, 0x03, room->segment)
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        GFX_TO(XLU, Gfx3D_LoadMtxF(&gIdentityMtxF));
        GFX_TO(XLU, Gfx3D_DrawDL((u32)(uintptr_t)(dListsEntry->xlu)));
    }
}

void Room_DrawImage(PlayState* play, Room* room, u32 flags) {
    RoomShapeImageBase* roomShape = &room->roomShape->image.base;

    if (roomShape->amountType == ROOM_SHAPE_IMAGE_AMOUNT_SINGLE) {
        Room_DrawImageSingle(play, room, flags);
    } else if (roomShape->amountType == ROOM_SHAPE_IMAGE_AMOUNT_MULTI) {
        Room_DrawImageMulti(play, room, flags);
    } else {
        LogUtils_HungupThread("../z_room.c", 841);
    }
}

void Room_Init(UNUSED PlayState* play, Room* room) {
    room->num = -1;
    room->segment = NULL;
}

/**
 * Allocates memory for rooms and fetches the first room that the player will spawn into.
 *
 * @return u32 size of the buffer reserved for room data
 */
u32 Room_SetupFirstRoom(PlayState* play, RoomContext* roomCtx) {
    u32 roomBufferSize = 0;
    u32 roomSize;
    s32 i;
    s32 j;
    s32 frontRoom;
    s32 backRoom;
    u32 frontRoomSize;
    u32 backRoomSize;
    u32 cumulRoomSize;
    STACK_PAD(s32);

    // Set roomBufferSize to the largest room
    {
        RomFile* roomList = play->roomList.romFiles;

        for (i = 0; i < play->roomList.count; i++) {
            roomSize = roomList[i].vromEnd - roomList[i].vromStart;
            PRINTF("ROOM%d size=%d\n", i, roomSize);
            if (roomBufferSize < roomSize) {
                roomBufferSize = roomSize;
            }
        }
    }

    // If there are any connected rooms, find their combined size and update roomBufferSize if larger
    if ((u32)play->transitionActors.count != 0) {
        RomFile* roomList = play->roomList.romFiles;
        TransitionActorEntry* transitionActor = &play->transitionActors.list[0];

        LOG_NUM("game_play->room_rom_address.num", play->roomList.count, "../z_room.c", 912);

        for (j = 0; j < play->transitionActors.count; j++) {
            frontRoom = transitionActor->sides[0].room;
            backRoom = transitionActor->sides[1].room;
            frontRoomSize = (frontRoom < 0) ? 0 : roomList[frontRoom].vromEnd - roomList[frontRoom].vromStart;
            backRoomSize = (backRoom < 0) ? 0 : roomList[backRoom].vromEnd - roomList[backRoom].vromStart;
            cumulRoomSize = (frontRoom != backRoom) ? frontRoomSize + backRoomSize : frontRoomSize;

            PRINTF("DOOR%d=<%d> ROOM1=<%d, %d> ROOM2=<%d, %d>\n", j, cumulRoomSize, frontRoom, frontRoomSize, backRoom,
                   backRoomSize);
            if (roomBufferSize < cumulRoomSize) {
                roomBufferSize = cumulRoomSize;
            }
            transitionActor++;
        }
    }

    PRINTF_COLOR_YELLOW();
    PRINTF(T("部屋バッファサイズ=%08x(%5.1fK)\n", "Room buffer size=%08x(%5.1fK)\n"), roomBufferSize,
           roomBufferSize / 1024.0f);
    roomCtx->bufPtrs[0] = GAME_STATE_ALLOC(&play->state, roomBufferSize, "../z_room.c", 946);
    PRINTF(T("部屋バッファ開始ポインタ=%08x\n", "Room buffer initial pointer=%08x\n"), roomCtx->bufPtrs[0]);
    roomCtx->bufPtrs[1] = (void*)((uintptr_t)roomCtx->bufPtrs[0] + roomBufferSize);
    PRINTF(T("部屋バッファ終了ポインタ=%08x\n", "Room buffer end pointer=%08x\n"), roomCtx->bufPtrs[1]);
    PRINTF_RST();
    roomCtx->activeBufPage = 0;
    roomCtx->status = 0;

    frontRoom = gSaveContext.respawnFlag > 0 ? ((void)0, gSaveContext.respawn[gSaveContext.respawnFlag - 1].roomIndex)
                                             : play->spawnList[play->spawn].room;

    // Load into a room for the first time.
    // Since curRoom was initialized to `room = -1` and `segment = NULL` in Play_InitScene, the previous room
    // will also be initialized to the nulled state when this function completes.
    Room_RequestNewRoom(play, roomCtx, frontRoom);

    return roomBufferSize;
}

/**
 * Tries to create an asynchronous request to transfer room data into memory.
 * If successful, the requested room will be loaded into memory and becomes the new current room; the room that was
 * current before becomes the previous room.
 *
 * Room_RequestNewRoom will be blocked from loading new rooms until Room_ProcessRoomRequest completes room
 * initialization.
 *
 * Calling Room_RequestNewRoom outside of Room_SetupFirstRoom will allow for two rooms being initialized simultaneously.
 * This allows an actor like ACTOR_EN_HOLL to seamlessly swap the two rooms as the player moves between them. Calling
 * Room_FinishRoomChange afterward will finalize the room swap.
 *
 * @param roomNum is the id of the room to load. roomNum must NOT be the same id as curRoom.num, since this will create
 * duplicate actor instances that cannot be cleaned up by calling Room_FinishRoomChange
 * @returns bool false if the request could not be created.
 */
s32 Room_RequestNewRoom(PlayState* play, RoomContext* roomCtx, s32 roomNum) {
    if (roomCtx->status == 0) {
        u32 size;

        roomCtx->prevRoom = roomCtx->curRoom;
        roomCtx->curRoom.num = roomNum;
        roomCtx->curRoom.segment = NULL;
        roomCtx->status = 1;

        ASSERT(roomNum < play->roomList.count, "read_room_ID < game_play->room_rom_address.num", "../z_room.c", 1009);

        size = play->roomList.romFiles[roomNum].vromEnd - play->roomList.romFiles[roomNum].vromStart;
        roomCtx->roomRequestAddr = (void*)ALIGN16((uintptr_t)roomCtx->bufPtrs[roomCtx->activeBufPage] -
                                                  ((size + 8) * roomCtx->activeBufPage + 7));

        // native: the original issued an async DMA into roomRequestAddr. The converted room is loaded
        // from NitroFS now and reported complete on the next Room_ProcessRoomRequest (assumption A2).
        // (The N64DD hook B_80121220 is a disk-drive expansion path and is not ported.)
        RoomNative_Request(roomCtx, play->roomList.romFiles[roomNum].vromStart);
        roomCtx->loadPending = 1;

        roomCtx->activeBufPage ^= 1;
        return true;
    }

    return false;
}

/**
 * Completes room initialization for the room requested by a call to Room_RequestNewRoom.
 * This function does not block the thread if the room data is still being transferred.
 *
 * @returns bool false if a dma transfer is in progress.
 */
s32 Room_ProcessRoomRequest(PlayState* play, RoomContext* roomCtx) {
    if (roomCtx->status == 1) {
        if (roomCtx->loadPending) {
            roomCtx->loadPending = 0;
            roomCtx->status = 0;
            roomCtx->curRoom.segment = RoomNative_TakeLoaded(roomCtx); // native DsObj of the room
            RoomNative_Bind(&roomCtx->curRoom);

            Scene_ExecuteCommands(play, RoomNative_Header(&roomCtx->curRoom));
            Player_SetBootData(play, GET_PLAYER(play));
            Actor_SpawnTransitionActors(play, &play->actorCtx);
        } else {
            return false;
        }
    }

    return true;
}

static void Room_DrawImpl(PlayState* play, Room* room, u32 flags) {
    if (room->segment != NULL) {
        RoomNative_Bind(room);
        ASSERT(room->roomShape->base.type < ARRAY_COUNTU(sRoomDrawHandlers),
               "this->ground_shape->polygon.type < number(Room_Draw_Proc)", "../z_room.c", 1125);
        sRoomDrawHandlers[room->roomShape->base.type](play, room, flags);
    }
}

// native (60 Hz presentation): matrices matched per room (engine/gfx3d.h)
void Room_Draw(PlayState* play, Room* room, u32 flags) {
    u32 interpCtx = Gfx3D_InterpGetContext();

    Gfx3D_InterpSetContext(0xA0000000u | ((u32)room->num << 4) | flags);
    Room_DrawImpl(play, room, flags);
    Gfx3D_InterpSetContext(interpCtx);
}

/**
 * Finalizes a swap between two rooms.
 *
 * When a new room is created with Room_RequestNewRoom, the previous room and its actors remain in memory. This allows
 * an actor like ACTOR_EN_HOLL to seamlessly swap the two rooms as the player moves between them.
 */
void Room_FinishRoomChange(PlayState* play, RoomContext* roomCtx) {
    // Delete the previous room
    RoomNative_Free(&roomCtx->prevRoom);
    roomCtx->prevRoom.num = -1;
    roomCtx->prevRoom.segment = NULL;

    func_80031B14(play, &play->actorCtx);
    Actor_SpawnTransitionActors(play, &play->actorCtx);
    Map_InitRoomData(play, roomCtx->curRoom.num);
    if (!((play->sceneId >= SCENE_ELDORA_FIELD) && (play->sceneId <= SCENE_SUNMEAD_RANCH))) {
        Map_SavePlayerInitialInfo(play);
    }
    Audio_SetEnvReverb(play->roomCtx.curRoom.echo);
}
