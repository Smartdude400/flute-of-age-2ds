// stubs_bringup.c (stub: temporary, see STUBS.md) - rewritten native placeholder code
// stubs_bringup.c - TEMPORARY bring-up stubs. Every function here is listed in STUBS.md and
// must be gone before the final build. Each logs its first call so it can't hide.
#include "game.h"
#include "view.h"
#include "sram.h"
#include "sys_matrix.h"
#include "play_state.h"
#include "engine/log.h"

#define STUB_ONCE(name) do { static bool s; if (!s) { s = true; log_printf("[STUB] %s\n", name); } } while (0)


// S3 closed on the 3DS port: the real Play state (z_play.c) is linked.

// ---- not yet ported modules (each listed in STUBS.md; reaching one is logged, never silent) ----
#include "actor.h"
#include "fault.h"
#include "engine/fault.h"
#include "engine/stub.h"

// S10 closed (P3): the player actor is translated (code/z_player.c).

// S11: the pause menu (ovl_kaleido_scope).
void KaleidoScope_Update(PlayState* play) { (void)play; LOGIC_STUB("KaleidoScope_Update"); }
void KaleidoScope_Draw(PlayState* play) { (void)play; LOGIC_STUB("KaleidoScope_Draw"); }

