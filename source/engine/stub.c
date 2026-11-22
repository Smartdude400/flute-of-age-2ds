// engine/stub.c (rewritten: native)
#include <stdbool.h>
#include "engine/stub.h"
#include "engine/log.h"

void Stub_Log(const char* kind, const char* name) {
    log_printf("[%s] %s\n", kind, name);
}
