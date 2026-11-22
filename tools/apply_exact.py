"""Re-apply the exact-math call-site renames to already-adapted files (idempotent)."""
import re, sys, glob, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from import_source import EXACT_MATH
for p in sys.argv[1:]:
    if os.path.basename(p) == "exact_math.c":
        continue
    s = open(p, encoding="utf-8").read()
    t = s
    for old, new in EXACT_MATH:
        t = re.sub(r"\b" + old + r"\(", new + "(", t)
    if t != s:
        if '#include "exact_math.h"' not in t:
            t = t.replace("#include", '#include "exact_math.h"\n#include', 1)
        open(p, "w", encoding="utf-8", newline="\n").write(t)
        print("updated", p)
