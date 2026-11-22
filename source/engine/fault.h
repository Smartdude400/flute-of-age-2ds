// (rewritten: native)
#pragma once
// Native crash handler: prints the reason on both screens/debug port and halts.
void Fault_Crash(const char* fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
