// (rewritten: native)
// engine/room_native.h - native room loading/binding used by the adapted z_room.c
#pragma once
#include "room.h"
void RoomNative_Request(RoomContext* roomCtx, u32 vromStart); // loads the converted room now
void* RoomNative_TakeLoaded(RoomContext* roomCtx);            // -> DsObj* (stored in Room.segment)
void RoomNative_Bind(Room* room);                             // segment 3 := this room
void* RoomNative_Header(Room* room);                          // native SceneCmd list of the room
void RoomNative_Free(Room* room);
void* RoomNative_HeaderOf(Room* room); // header of a room without changing load state
