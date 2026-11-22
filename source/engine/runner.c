// engine/runner.c (rewritten: native)
// Replaces Graph_ThreadEntry/Graph_Update (src/code/graph.c). One loop on the main thread:
//   - each game frame consumes exactly R_UPDATE_RATE retrace input samples (fixed timestep,
//     identical to the original's "one game frame per R_UPDATE_RATE retraces"),
//   - runs GameState_Update (logic + native drawing) inside one citro3d frame.
#include <3ds.h>
#include <stdlib.h>
#include "engine/runner.h"
#include "engine/gfx.h"
#include "engine/gfx3d.h"
#include "engine/screen.h"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/fault.h"
#include "engine/system_arena.h"
#include "game.h"
#include "regs.h"
#include "z_game_dlftbls.h"

static struct GraphicsContext sGfxCtx;

// Test-harness marker read over the emulator's RPC: magic "FOA3DSMK", game frames, fault flag.
volatile u32 gTestMarker[4] = { 0x464F4133, 0x44534D4B, 0, 0 };

u32 Runner_VBlankCount(void) {
    return input_consumed();
}

u64 Runner_OsTime(void) {
    return (u64)input_consumed() * 781250u; // 46875000 / 60
}

void Runner_MarkFault(void) {
    gTestMarker[3] = 1;
}

static void frame(GameState* gameState) {
    // Fixed timestep: wait until R_UPDATE_RATE retrace samples exist, then consume exactly that many.
    int rate = R_UPDATE_RATE;
    if (rate < 1) {
        rate = 1;
    }
    while ((int)input_pending() < rate) {
        svcSleepThread(500000);
    }
    input_consume(rate);

    GameState_ReqPadData(gameState);
    Gfx3D_BeginFrame();
    GameState_Update(gameState); // records the frame's draws (opaque / translucent / overlay)
    Screen_BeginFrame();
    Gfx3D_EndFrame(); // replays them on the GPU in the original order
    Screen_EndFrame();

    Gfx3D_FrameReset();
    sGfxCtx.frameCount++;
    gTestMarker[2] = sGfxCtx.frameCount;
    if (sGfxCtx.callback != NULL) {
        sGfxCtx.callback(&sGfxCtx, sGfxCtx.callbackParam);
    }
}

// Test harnesses (P2 renderer evidence): a native game state wrapping test/viewer.c or test/scene_viewer.c.
#ifdef PORT_TEST_HARNESS
#include "play_state.h"
#include "test/viewer.h"
#include "test/scene_viewer.h"
static void Harness_Main(GameState* s) {
    if (PORT_TEST_HARNESS == 1) Viewer_Main((PlayState*)s); else SceneViewer_Main((PlayState*)s);
}
static void Harness_Destroy(GameState* s) {
    (void)s;
}
static void Harness_Init(GameState* s) {
    s->main = Harness_Main;
    s->destroy = Harness_Destroy;
    if (PORT_TEST_HARNESS == 1) Viewer_Init((PlayState*)s); else SceneViewer_Init((PlayState*)s, PORT_TEST_SCENE);
}
#endif

void Runner_Run(void) {
#ifdef PORT_TEST_HARNESS
    {
        GameState* gameState = SystemArena_Malloc(sizeof(PlayState));
        GameState_Init(gameState, Harness_Init, &sGfxCtx);
        while (aptMainLoop()) {
            frame(gameState);
        }
        exit(0);
    }
#endif
    GameStateOverlay* nextOvl = &gGameStateOverlayTable[GAMESTATE_SETUP];
    while (nextOvl != NULL && nextOvl->init != NULL) {
        GameStateOverlay* ovl = nextOvl;
        GameState* gameState = SystemArena_Malloc(ovl->instanceSize);
        if (gameState == NULL) {
            Fault_Crash("CLASS SIZE= %lu bytes", (unsigned long)ovl->instanceSize);
        }
        GameState_Init(gameState, (GameStateFunc)ovl->init, &sGfxCtx);
        while (GameState_IsRunning(gameState)) {
            if (!aptMainLoop()) {
                exit(0); // closed from the HOME menu
            }
            frame(gameState);
        }
        nextOvl = GameStateTable_FindByInit((void*)GameState_GetInit(gameState));
        if (nextOvl == NULL || nextOvl->init == NULL) {
            log_printf("[runner] next gamestate not ported (init=%p)\n", (void*)GameState_GetInit(gameState));
        }
        GameState_Destroy(gameState);
        sGfxCtx.callback = NULL;
        SystemArena_Free(gameState);
    }
    Fault_Crash("runner: no next gamestate");
}
