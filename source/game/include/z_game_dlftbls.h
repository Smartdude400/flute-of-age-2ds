// adapted from flute-of-age/include/z_game_dlftbls.h (overlay/ROM fields removed: static linking)
#ifndef Z_GAME_DLFTBLS_H
#define Z_GAME_DLFTBLS_H

#include "game_types.h"

typedef struct GameStateOverlay {
    void* init;
    u32 instanceSize;
} GameStateOverlay;

#define DEFINE_GAMESTATE_INTERNAL(typeName, enumName) enumName,
#define DEFINE_GAMESTATE(typeName, enumName, name) DEFINE_GAMESTATE_INTERNAL(typeName, enumName)
typedef enum GameStateId {
#include "tables/gamestate_table.h"
    GAMESTATE_ID_MAX
} GameStateId;
#undef DEFINE_GAMESTATE
#undef DEFINE_GAMESTATE_INTERNAL

extern GameStateOverlay gGameStateOverlayTable[GAMESTATE_ID_MAX];
GameStateOverlay* GameStateTable_FindByInit(void* init);

#endif
