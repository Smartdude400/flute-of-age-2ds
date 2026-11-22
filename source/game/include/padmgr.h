// padmgr.h (rewritten: native)
// The original PadMgr was a thread polling the controllers over the serial interface. On the DS the
// VBlank interrupt samples the keys and engine/input.c applies the samples (fixed timestep). What game
// code still uses is kept: the controller-pak type per port and the per-retrace callback (rumble).
#ifndef PADMGR_H
#define PADMGR_H

#include "game_types.h"
#include "libu64/pad.h"

typedef enum ControllerPakType {
    CONT_PAK_NONE,
    CONT_PAK_RUMBLE,
    CONT_PAK_OTHER
} ControllerPakType;

struct PadMgr;
typedef void (*PadMgrRetraceCallback)(struct PadMgr* padMgr, void* arg);

typedef struct PadMgr {
    u8 pakType[MAXCONTROLLERS]; // always CONT_PAK_NONE: the DS has no rumble pak in the port
    PadMgrRetraceCallback retraceCallback;
    void* retraceCallbackArg;
} PadMgr;

extern PadMgr gPadMgr;

void PadMgr_RumbleSet(PadMgr* padMgr, u8* enable); // no rumble hardware: no-op

// Called once per retrace sample when the sample is applied (engine/input.c, game thread).
#define PADMGR_SET_RETRACE_CALLBACK(padmgr, callback, arg) \
    do {                                                   \
        (padmgr)->retraceCallback = (callback);            \
        (padmgr)->retraceCallbackArg = (arg);              \
    } while (0)

#define PADMGR_UNSET_RETRACE_CALLBACK(padmgr, callback, arg)                                \
    if ((padmgr)->retraceCallback == (callback) && (padmgr)->retraceCallbackArg == (arg)) { \
        (padmgr)->retraceCallback = NULL;                                                   \
        (padmgr)->retraceCallbackArg = NULL;                                                \
    }                                                                                       \
    (void)0

#endif
