// adapted from flute-of-age/src/code/game.c
// Kept: GameState lifecycle, arena handling, update-rate default, frame counter.
// Removed (platform-only): VI mode editing, framebuffer filters (VisCvg/VisZBuffer/VisMono),
// speed meter, RSP/RDP timing, fault clients, message-queue handshakes with the graphics thread.
// Debug overlays/drawing: none in this non-debug build (DEBUG_FEATURES=0).
#include "game.h"
#include "regs.h"
#include "engine/fault.h"
#include "engine/log.h"

void GameState_ReqPadData(GameState* gameState) {
    PadMgr_RequestPadData(gameState->input, true);
}

void GameState_Update(GameState* gameState) {
    gameState->main(gameState);
    gameState->frames++;
}

void GameState_InitArena(GameState* gameState, size_t size) {
    void* arena = GameAlloc_Malloc(&gameState->alloc, size);

    if (arena != NULL) {
        THA_Init(&gameState->tha, arena, size);
    } else {
        THA_Init(&gameState->tha, NULL, 0);
        Fault_Crash("game.c: arena alloc failed (%u bytes)", (unsigned)size);
    }
}

void GameState_Realloc(GameState* gameState, size_t size) {
    GameAlloc* alloc = &gameState->alloc;
    void* thaStart = gameState->tha.start;
    void* gameArena;

    THA_Destroy(&gameState->tha);
    GameAlloc_Free(alloc, thaStart);

    gameArena = GameAlloc_Malloc(alloc, size);
    if (gameArena != NULL) {
        THA_Init(&gameState->tha, gameArena, size);
    } else {
        THA_Init(&gameState->tha, NULL, 0);
        Fault_Crash("game.c: arena realloc failed (%u bytes)", (unsigned)size);
    }
}

void GameState_Init(GameState* gameState, GameStateFunc init, struct GraphicsContext* gfxCtx) {
    gameState->gfxCtx = gfxCtx;
    gameState->frames = 0;
    gameState->main = NULL;
    gameState->destroy = NULL;
    gameState->running = 1;
    gameState->init = NULL;
    gameState->size = 0;
    GameAlloc_Init(&gameState->alloc);
    GameState_InitArena(gameState, 0x100000);

    R_UPDATE_RATE = 3;
    init(gameState);
}

void GameState_Destroy(GameState* gameState) {
    if (gameState->destroy != NULL) {
        gameState->destroy(gameState);
    }
    THA_Destroy(&gameState->tha);
    GameAlloc_Cleanup(&gameState->alloc);
}

GameStateFunc GameState_GetInit(GameState* gameState) {
    return gameState->init;
}

size_t GameState_GetSize(GameState* gameState) {
    return gameState->size;
}

u32 GameState_IsRunning(GameState* gameState) {
    return gameState->running;
}

void* GameState_Alloc(GameState* gameState, size_t size) {
    return THA_AllocTailAlign16(&gameState->tha, size);
}

void* GameState_AllocEndAlign16(GameState* gameState, size_t size) {
    return THA_AllocTailAlign16(&gameState->tha, size);
}

s32 GameState_GetArenaSize(GameState* gameState) {
    return THA_GetRemaining(&gameState->tha);
}
