// main.c (rewritten: native) - replaces the original boot/main threads (src/boot, src/code/main.c).
#include <3ds.h>
#include "engine/log.h"
#include "engine/res.h"
#include "engine/screen.h"
#include "engine/gfx2d.h"
#include "engine/gfx3d.h"
#include "engine/input.h"
#include "engine/runner.h"
#include "engine/fault.h"

void Regs_Init(void);
void Locale_Init(void);

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    gfxInitDefault();
    log_init();
    log_printf("FoA-3DS boot\n");
    Screen_Init();
    input_init();
    if (!res_init()) {
        Fault_Crash("RomFS mount failed");
    }
    Gfx3D_Init();
    if (!Gfx2D_Init()) {
        Fault_Crash("font load failed");
    }
    Regs_Init();
    Locale_Init();
    Runner_Run();
}
