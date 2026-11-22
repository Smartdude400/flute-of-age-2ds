"""Header adaptation for recurring platform types (mechanical; see NATIVENESS.md):
  Gfx** -> DListRef*      Gfx* -> DListRef      (display-list references = asset handles)
  Vtx* / Mtx* / Vtx[] fields -> void* (render-only buffers, rewritten with their users)
  OSTime -> u64
usage: python tools/adapt_header_types.py header.h [...]"""
import re, sys
for p in sys.argv[1:]:
    s = open(p, encoding="utf-8").read()
    t = re.sub(r"\bGfx\s*\*\s*\*", "DListRef*", s)
    t = re.sub(r"\bGfx\s*\*", "DListRef ", t)
    t = re.sub(r"\b(?:const\s+)?(Vtx|Mtx)\s*\*\s*(\w+)", r"void* \2 /* native: was \1* (render-only) */", t)
    t = re.sub(r"\bOSTime\b", "u64", t)
    if t != s:
        open(p, "w", encoding="utf-8", newline="\n").write(t)
        print("adapted", p)
