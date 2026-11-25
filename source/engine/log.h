// (rewritten: native)
// engine/log.h - native logging: debug output service (emulator log), SD-card log file and the
// bottom-screen text console.
#pragma once
#include <stdarg.h>
void log_init(void);
// text console on the bottom screen (fault reports; replaces the game's bottom screen)
void log_console(void);
void log_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
