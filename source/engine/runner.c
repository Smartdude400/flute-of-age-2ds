// engine/runner.c (rewritten: native)
// Replaces Graph_ThreadEntry/Graph_Update (src/code/graph.c). One loop on the main thread:
//   - each game frame consumes exactly R_UPDATE_RATE retrace input samples (fixed timestep,
//     identical to the original's "one game frame per R_UPDATE_RATE retraces"),
//   - runs GameState_Update (logic + recording of the frame's draws),
//   - presents on every retrace until the next game frame: the recorded frame is replayed at
//     t = 1/R_UPDATE_RATE, 2/R_UPDATE_RATE, ... 1 between the previous and this game frame (60 Hz
//     presentation, engine/gfx3d.c "interpolation"; build with PRESENT20=1 for the original pacing).
#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h>
#include "engine/runner.h"
#include "engine/gfx.h"
#include "engine/gfx3d.h"
#include "engine/f3d.h"
#include "engine/screen.h"
#include "engine/input.h"
#include "engine/log.h"
#include "engine/fault.h"
#include "engine/system_arena.h"
#include "game.h"
#include "regs.h"
#include "z_game_dlftbls.h"
#ifdef PORT_INSPECT
#include "test/inspector.h"
#endif

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

// ---- frame timing (CPU ticks at 268 MHz; GPU times from citro3d), logged every 60 game frames
u32 Gfx3D_TakeDrawStats(u32* tris);
void Gfx3D_TakeTickStats(u64* state, u64* submit, u64* twoD);
void Gfx3D_TakeInterpStats(u32* total, u32* linked);
static u64 sPerfState, sPerfSubmit, sPerf2D, sPerfWait;
static struct { u64 logic, replay, logicMax, replayMax; float gpuProc, gpuDraw; u32 draws, tris; int n; } sPerf;

static void perf_sample(u64 logic, u64 replay) {
    sPerf.logic += logic;
    sPerf.replay += replay;
    if (logic > sPerf.logicMax) sPerf.logicMax = logic;
    if (replay > sPerf.replayMax) sPerf.replayMax = replay;
    sPerf.gpuProc += C3D_GetProcessingTime();
    sPerf.gpuDraw += C3D_GetDrawingTime();
    u32 tris;
    sPerf.draws += Gfx3D_TakeDrawStats(&tris);
    {
        u64 a, b, c;
        Gfx3D_TakeTickStats(&a, &b, &c);
        sPerfState += a;
        sPerfSubmit += b;
        sPerf2D += c;
    }
    sPerf.tris += tris;
    if (++sPerf.n == 60) {
        const float ms = 1000.0f / SYSCLOCK_ARM11;
        log_printf("[perf] logic %.2f ms (max %.2f) | replay %.2f ms (max %.2f) | gpu proc %.2f draw %.2f ms | %lu draws %lu tris\n",
                   sPerf.logic * ms / 60, sPerf.logicMax * ms, sPerf.replay * ms / 60, sPerf.replayMax * ms,
                   sPerf.gpuProc / 60, sPerf.gpuDraw / 60, (unsigned long)(sPerf.draws / 60),
                   (unsigned long)(sPerf.tris / 60));
        log_printf("[perf]   replay split: state %.2f ms, submit %.2f ms, 2D %.2f ms, frame-start wait %.2f ms\n", sPerfState * ms / 60,
                   sPerfSubmit * ms / 60, sPerf2D * ms / 60, sPerfWait * ms / 60);
        {
            u32 total, linked;
            Gfx3D_TakeInterpStats(&total, &linked);
            u32 fc, fh, ff;
            F3D_Stats(&fc, &fh, &ff);
            log_printf("[perf]   runtime lists: %lu compiled, %lu cached, %lu failed\n", (unsigned long)fc,
                       (unsigned long)fh, (unsigned long)ff);
            log_printf("[perf]   interpolation: %lu of %lu matrix commands matched\n", (unsigned long)linked,
                       (unsigned long)total);
        }
        sPerfState = sPerfSubmit = sPerf2D = sPerfWait = 0;
        memset(&sPerf, 0, sizeof(sPerf));
    }
}

static int sPresentSub, sPresentRate; // retraces presented since the last game frame / its update rate
static bool sNewGameState;            // the next game frame starts a game state: nothing to blend from
static u64 sLogicTicks;

static void present(float t) {
    u64 tw = svcGetSystemTick();
    Screen_BeginFrame(); // waits for the previous frame's GPU work (and vblank sync)
    u64 t1 = svcGetSystemTick();
    sPerfWait += t1 - tw;
    if (Screen_SnapshotPending()) {
        t = 1.0f; // the snapshot shows the recorded frame itself
    }
#ifdef PORT_INTERP_TEST_T
    t = PORT_INTERP_TEST_T; // test builds: every presented frame is a blend (shows matching errors)
#endif
    Gfx3D_Replay(t); // replays the recorded frame on the GPU in the original order
    Screen_SetFrameExact(t >= 1.0f);
    u64 t2 = svcGetSystemTick();
    Screen_EndFrame();
    perf_sample(sLogicTicks, t2 - t1); // per presented frame (logic: the game frame it shows)
}

static void frame(GameState* gameState) {
    int rate = R_UPDATE_RATE;
    if (rate < 1) {
        rate = 1;
    }
#ifndef PORT_PRESENT20
    // retraces between two game frames: present the recorded frame closer to its own time
    if (sPresentSub > 0 && sPresentSub < sPresentRate && (int)input_pending() < rate) {
        sPresentSub++;
        present((float)sPresentSub / sPresentRate);
        return;
    }
#endif
    // Fixed timestep: wait until R_UPDATE_RATE retrace samples exist, then consume exactly that many.
    while ((int)input_pending() < rate) {
        svcSleepThread(500000);
    }
    input_consume(rate);

    GameState_ReqPadData(gameState);
#ifdef PORT_INSPECT
    if (Inspector_Input(&gameState->input[0])) {
        memset(&gameState->input[0], 0, sizeof(gameState->input[0])); // the inspector took it
    }
#endif
    u64 t0 = svcGetSystemTick();
    Gfx3D_BeginFrame();
    if (sNewGameState) {
        sNewGameState = false;
        Gfx3D_InterpReset();
    }
    GameState_Update(gameState); // records the frame's draws (opaque / translucent / overlay)
    sLogicTicks = svcGetSystemTick() - t0;
#ifndef PORT_PRESENT20
    sPresentRate = rate;
    sPresentSub = 1;
    present(1.0f / rate);
#else
    present(1.0f);
#endif

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
        sNewGameState = true;
        sPresentSub = 0;
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

// Screen size globals of the original main.c (gScreenWidth / gScreenHeight, set to the logical screen;
// the original changed them only with the debug VI mode tool)
#include "main.h"

s32 gScreenWidth = 320;  // SCREEN_WIDTH (game gfx.h)
s32 gScreenHeight = 240; // SCREEN_HEIGHT
