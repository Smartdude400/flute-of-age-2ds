// engine/input.c (rewritten: native)
// Replaces the original controller thread. A sampling thread wakes on every top-screen retrace
// (citro3d's VBlank counter) and records the 2DS controls as an N64-layout PadState in a FIFO.
// Each game frame consumes EXACTLY R_UPDATE_RATE samples, applying the original per-retrace
// accumulation logic (PadMgr_UpdateInputs, adapted) to each one, so game-frame inputs depend only on
// the per-retrace input sequence, never on how long a frame took to compute (replays deterministic).
//
// Mapping (old 2DS: no ZL/ZR/C-stick): circle pad = control stick (analog), A = A, B = B, L = Z,
// R = R, START = START, D-pad = C-buttons (up/down/left/right), X = C-up, Y = C-left.
#include <3ds.h>
#include <citro3d.h>
#include "engine/input.h"
#include "engine/touch.h"
#include "padmgr.h"
#include "controller.h"
#include "libu64/pad.h"

#define SAMPLE_QUEUE 64
#define CIRCLE_MAX 155 // circle pad radius reported by the HID service
#define STICK_MAX 80   // N64 stick full deflection (game clamps relative values itself)

static PadState sQueue[SAMPLE_QUEUE];
typedef struct { s16 x, y; bool down; } TouchSample;
static TouchSample sTouchQueue[SAMPLE_QUEUE]; // touch screen, sampled with sQueue
static bool sTouchWasDown;
static s32 sTap = TOUCH_NONE; // tap target of the current game frame (Touch_TakeTap)
static volatile u32 sHead, sTail; // written by the sampling thread / consumer
static Input sPolled[MAXCONTROLLERS]; // equivalent of the pad manager's polled inputs
static u32 sDropped;
static u32 sConsumed; // retrace samples applied since boot
static Thread sThread;
static volatile bool sRun;

// ---- scripted input (tests / replays): sdmc:/foa_input.txt, lines "<retrace> <keys>", keys held from that
// retrace on: A B X Y L R START SELECT, DU DD DL DR (D-pad), UP DOWN LEFT RIGHT (circle pad full deflection),
// "-" = nothing, "TOUCH x y" = touch screen held at (x, y). When the file exists it replaces the real controls
// for the whole run.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define SCRIPT_MAX 512
static struct { u32 at, keys; s16 cx, cy, tx, ty; bool touch; } sScript[SCRIPT_MAX];
static int sScriptLen = -1;
static u32 sRetrace;

static void script_load(void) {
    FILE* f = fopen("sdmc:/foa_input.txt", "r");
    if (f == NULL) {
        return;
    }
    sScriptLen = 0;
    char line[128];
    while (fgets(line, sizeof(line), f) && sScriptLen < SCRIPT_MAX) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        unsigned long at;
        int n = 0;
        if (sscanf(line, "%lu %n", &at, &n) != 1) {
            continue;
        }
        u32 keys = 0;
        s16 cx = 0, cy = 0, tx = 0, ty = 0;
        bool touch = false;
        for (char* tok = strtok(line + n, " \r\n"); tok != NULL; tok = strtok(NULL, " \r\n")) {
            if (strcmp(tok, "TOUCH") == 0) {
                char* sx = strtok(NULL, " \r\n");
                char* sy = sx != NULL ? strtok(NULL, " \r\n") : NULL;
                if (sy != NULL) {
                    tx = (s16)atoi(sx);
                    ty = (s16)atoi(sy);
                    touch = true;
                }
                continue;
            }
            static const struct { const char* name; u32 key; } kNames[] = {
                { "A", KEY_A }, { "B", KEY_B }, { "X", KEY_X }, { "Y", KEY_Y }, { "L", KEY_L }, { "R", KEY_R },
                { "START", KEY_START }, { "SELECT", KEY_SELECT }, { "DU", KEY_DUP }, { "DD", KEY_DDOWN },
                { "DL", KEY_DLEFT }, { "DR", KEY_DRIGHT },
            };
            for (unsigned i = 0; i < sizeof(kNames) / sizeof(kNames[0]); i++) {
                if (strcmp(tok, kNames[i].name) == 0) keys |= kNames[i].key;
            }
            if (strcmp(tok, "UP") == 0) cy = 155;
            if (strcmp(tok, "DOWN") == 0) cy = -155;
            if (strcmp(tok, "LEFT") == 0) cx = -155;
            if (strcmp(tok, "RIGHT") == 0) cx = 155;
        }
        sScript[sScriptLen].at = at;
        sScript[sScriptLen].keys = keys;
        sScript[sScriptLen].cx = cx;
        sScript[sScriptLen].cy = cy;
        sScript[sScriptLen].tx = tx;
        sScript[sScriptLen].ty = ty;
        sScript[sScriptLen].touch = touch;
        sScriptLen++;
    }
    fclose(f);
}

static bool script_state(u32 retrace, u32* keys, circlePosition* c, TouchSample* t) {
    if (sScriptLen < 0) {
        return false;
    }
    *keys = 0;
    c->dx = c->dy = 0;
    t->down = false;
    for (int i = 0; i < sScriptLen && sScript[i].at <= retrace; i++) {
        *keys = sScript[i].keys;
        c->dx = sScript[i].cx;
        c->dy = sScript[i].cy;
        t->x = sScript[i].tx;
        t->y = sScript[i].ty;
        t->down = sScript[i].touch;
    }
    return true;
}

static s8 stick_axis(int v) {
    int s = v * STICK_MAX / CIRCLE_MAX;
    return (s8)(s > STICK_MAX ? STICK_MAX : (s < -STICK_MAX ? -STICK_MAX : s));
}

static PadState sample_keys(TouchSample* t) {
    PadState p = { 0 };
    hidScanInput();
    u32 k = hidKeysHeld();
    circlePosition c;
    hidCircleRead(&c);
    touchPosition tp = { 0, 0 };
    if (k & KEY_TOUCH) {
        hidTouchRead(&tp);
    }
    t->x = tp.px;
    t->y = tp.py;
    t->down = (k & KEY_TOUCH) != 0;
    script_state(sRetrace++, &k, &c, t);
    if (k & KEY_A) p.button |= BTN_A;
    if (k & KEY_B) p.button |= BTN_B;
    if (k & KEY_L) p.button |= BTN_Z;
    if (k & KEY_R) p.button |= BTN_R;
    if (k & KEY_START) p.button |= BTN_START;
    if (k & (KEY_DUP | KEY_X)) p.button |= BTN_CUP;
    if (k & KEY_DDOWN) p.button |= BTN_CDOWN;
    if (k & (KEY_DLEFT | KEY_Y)) p.button |= BTN_CLEFT;
    if (k & KEY_DRIGHT) p.button |= BTN_CRIGHT;
    p.stick_x = stick_axis(c.dx);
    p.stick_y = stick_axis(c.dy);
    return p;
}

static void push_sample(void) {
    u32 next = (sHead + 1) % SAMPLE_QUEUE;
    if (next == sTail) {
        sDropped++; // consumer is far behind; drop newest (only possible in real-time play)
        return;
    }
    sQueue[sHead] = sample_keys(&sTouchQueue[sHead]);
    sHead = next;
}

static void sampler(void* arg) {
    (void)arg;
    u32 seen = C3D_FrameCounter(0);
    while (sRun) {
        u32 now = C3D_FrameCounter(0);
        while (seen != now) { // one sample per retrace, even if this thread was delayed
            seen++;
            push_sample();
        }
        svcSleepThread(1000000); // 1 ms
    }
}

void input_init(void) {
    script_load();
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    sRun = true;
    sThread = threadCreate(sampler, NULL, 8 * 1024, prio - 1, -2, false);
}

u32 input_pending(void) {
    return (sHead - sTail + SAMPLE_QUEUE) % SAMPLE_QUEUE;
}

// adapted from PadMgr_UpdateInputs (src/code/padmgr.c): one retrace worth of input.
static void update_inputs(const PadState* pad0) {
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        Input* input = &sPolled[i];
        PadState none = { 0 };
        const PadState* contPad = (i == 0) ? pad0 : &none; // controllers 2-4 absent/cleared
        input->prev = input->cur;
        input->cur = *contPad;
        s32 buttonDiff = input->prev.button ^ input->cur.button;
        input->press.button |= (u16)(buttonDiff & input->cur.button);
        input->rel.button |= (u16)(buttonDiff & input->prev.button);
        PadUtils_UpdateRelXY(input);
        input->press.stick_x += (s8)(input->cur.stick_x - input->prev.stick_x);
        input->press.stick_y += (s8)(input->cur.stick_y - input->prev.stick_y);
    }
}

// Touch screen (engine/touch.h): an on-screen button acts as that button while touched; a touch starting on
// another target is this game frame's tap.
static void apply_touch(PadState* p, const TouchSample* t) {
    static const u16 kButtons[] = { 0, BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT, BTN_START, BTN_Z, BTN_R };
    int tag = t->down ? Gfx3D_TouchFind(t->x, t->y) : TOUCH_NONE;
    if (tag > TOUCH_NONE && tag < (int)(sizeof(kButtons) / sizeof(kButtons[0]))) {
        p->button |= kButtons[tag];
    } else if (tag != TOUCH_NONE && !sTouchWasDown) {
        sTap = tag;
    }
    sTouchWasDown = t->down;
}

s32 Touch_TakeTap(int first, int count) {
    if (sTap >= first && sTap < first + count) {
        s32 i = sTap - first;
        sTap = TOUCH_NONE;
        return i;
    }
    return -1;
}

void input_consume(int count) {
    sTap = TOUCH_NONE; // a tap not taken by the frame it started in is dropped
    for (int n = 0; n < count; n++) {
        PadState p = sQueue[sTail];
        apply_touch(&p, &sTouchQueue[sTail]);
        sTail = (sTail + 1) % SAMPLE_QUEUE;
        update_inputs(&p);
        sConsumed++;
        // The original PadMgr thread ran the retrace callback (rumble) on every retrace.
        if (gPadMgr.retraceCallback != NULL) {
            gPadMgr.retraceCallback(&gPadMgr, gPadMgr.retraceCallbackArg);
        }
    }
}

PadMgr gPadMgr;

void PadMgr_RumbleSet(PadMgr* padMgr, u8* enable) {
    (void)padMgr;
    (void)enable;
}

// adapted from PadMgr_RequestPadData (gameRequest == true path)
void PadMgr_RequestPadData(Input* inputs, s32 gameRequest) {
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        Input* inputIn = &sPolled[i];
        Input* inputOut = &inputs[i];
        if (gameRequest) {
            *inputOut = *inputIn;
            inputIn->press.button = 0;
            inputIn->press.stick_x = 0;
            inputIn->press.stick_y = 0;
            inputIn->rel.button = 0;
        } else {
            inputOut->prev = inputOut->cur;
            inputOut->cur = inputIn->cur;
            s32 buttonDiff = inputOut->prev.button ^ inputOut->cur.button;
            inputOut->press.button = inputOut->cur.button & buttonDiff;
            inputOut->rel.button = inputOut->prev.button & buttonDiff;
            PadUtils_UpdateRelXY(inputOut);
            inputOut->press.stick_x += (s8)(inputOut->cur.stick_x - inputOut->prev.stick_x);
            inputOut->press.stick_y += (s8)(inputOut->cur.stick_y - inputOut->prev.stick_y);
        }
    }
}

u32 input_dropped(void) {
    return sDropped;
}

u32 input_consumed(void) {
    return sConsumed;
}
