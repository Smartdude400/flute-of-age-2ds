// engine/stub.h (rewritten: native) - logged bring-up stubs (listed in STUBS.md).
#pragma once
void Stub_Log(const char* kind, const char* name);
#define PRESENTATION_STUB(name) do { static bool s_; if (!s_) { s_ = true; Stub_Log("PSTUB", name); } } while (0)
#define LOGIC_STUB(name) do { static bool s_; if (!s_) { s_ = true; Stub_Log("LSTUB", name); } } while (0)
