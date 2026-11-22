// adapted from flute-of-age/src/code/z_game_dlftbls.c + Graph_GetNextGameState (graph.c)
// Gamestates are statically linked on DS (no overlay loading), so each table entry is {init, size}.
#include "z_game_dlftbls.h"
#include "console_logo_state.h"
#include "setup_state.h"
#include "title_setup_state.h"
#include "play_state.h"

// Not yet ported: MapSelect (debug), FileSelect (Phase 4). Entries are NULL until then.
GameStateOverlay gGameStateOverlayTable[GAMESTATE_ID_MAX] = {
    [GAMESTATE_SETUP] = { (void*)Setup_Init, sizeof(SetupState) },
    [GAMESTATE_MAP_SELECT] = { NULL, 0 },
    [GAMESTATE_CONSOLE_LOGO] = { (void*)ConsoleLogo_Init, sizeof(ConsoleLogoState) },
    [GAMESTATE_PLAY] = { (void*)Play_Init, sizeof(PlayState) },
    [GAMESTATE_TITLE_SETUP] = { (void*)TitleSetup_Init, sizeof(TitleSetupState) },
    [GAMESTATE_FILE_SELECT] = { NULL, 0 },
};

GameStateOverlay* GameStateTable_FindByInit(void* init) {
    if (init == NULL) {
        return NULL;
    }
    for (int i = 0; i < GAMESTATE_ID_MAX; i++) {
        if (gGameStateOverlayTable[i].init == init) {
            return &gGameStateOverlayTable[i];
        }
    }
    return NULL;
}
