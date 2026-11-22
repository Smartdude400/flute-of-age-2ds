"""Remove N64 Disk Drive (64DD) expansion hooks.

Pattern: `#if PLATFORM_N64 ... #endif` blocks that call through the 64DD hook table (B_80121220,
D_80121211/2, func_801C*, n64dd*). Without a disk drive the hook pointer is NULL and the code takes
the non-DD path, which is exactly the `#else` body when one exists. So:
  - block with #else  -> replaced by its #else body
  - block without      -> removed
Blocks with nested preprocessor conditionals are reported and left for manual handling.
usage: python tools/patches/remove_n64dd.py file.c [...]"""
import re
import sys

DD = re.compile(r"\bB_80121220\b|\bD_8012121[0-2]\b|\bfunc_801C\w+|\bn64dd\w*|\bB_80121AF0\b|\bD_80121214\b")


def process(text):
    lines = text.split("\n")
    out, i, changed, manual = [], 0, 0, 0
    while i < len(lines):
        if lines[i].strip() == "#if PLATFORM_N64":
            depth, j, else_at = 1, i + 1, None
            nested = False
            while j < len(lines) and depth:
                t = lines[j].strip()
                if t.startswith("#if"):
                    depth += 1
                    nested = True
                elif t == "#endif":
                    depth -= 1
                elif t == "#else" and depth == 1:
                    else_at = j
                j += 1
            end = j - 1
            block_if = lines[i + 1:(else_at if else_at is not None else end)]
            if any(DD.search(l) for l in block_if):
                if nested:
                    manual += 1
                    out.extend(lines[i:end + 1])
                else:
                    if else_at is not None:
                        out.append("    // native: 64DD hook removed (no disk drive); non-DD path kept")
                        out.extend(lines[else_at + 1:end])
                    else:
                        out.append("    // native: 64DD hook removed (no disk drive)")
                    changed += 1
                i = end + 1
                continue
        out.append(lines[i])
        i += 1
    return "\n".join(out), changed, manual


for p in sys.argv[1:]:
    s = open(p, encoding="utf-8").read()
    t, c, m = process(s)
    if t != s:
        open(p, "w", encoding="utf-8", newline="\n").write(t)
    print(f"{p}: {c} blocks removed, {m} need manual work")
