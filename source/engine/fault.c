// engine/fault.c (rewritten: native)
// Crash handler: reports the reason on the log/console, flags the test marker and halts (the HOME
// menu can still close the application).
#include <3ds.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine/fault.h"
#include "engine/log.h"
#include "engine/runner.h"

void Fault_Crash(const char* fmt, ...) {
    static char msg[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    log_console();
    log_printf("FAULT: %s\n", msg);
    Runner_MarkFault();
    while (aptMainLoop()) {
        gspWaitForVBlank();
    }
    exit(1);
}

// Game crash entry points (game fault.h), routed to the native fault screen.
void Fault_AddHungupAndCrash(const char* file, int line) {
    Fault_Crash("HungUp %s:%d", file, line);
}

void Fault_AddHungupAndCrashImpl(const char* exp1, const char* exp2) {
    Fault_Crash("%s %s", exp1, exp2 ? exp2 : "");
}

void LogUtils_HungupThread(const char* name, int line) {
    Fault_Crash("HungUp %s:%d", name, line);
}
