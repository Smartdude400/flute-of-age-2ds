"""Group compile errors of one file by enclosing function. usage: errs.py build_output.txt file.c"""
import re, sys
out, path = sys.argv[1], sys.argv[2]
lines = open(path, encoding="utf-8").read().split("\n")
errs = {}
for l in open(out, encoding="utf-8", errors="replace"):
    m = re.match(re.escape(path) + r":(\d+):\d+: error: (.*)", l.strip())
    if m:
        errs.setdefault(int(m.group(1)), m.group(2))
func_at = {}
cur = "<file scope>"
for i, l in enumerate(lines, 1):
    if re.match(r"^[A-Za-z_][\w\s\*]*\b(\w+)\s*\([^;]*$", l) and not l.startswith(("typedef", "static const", "#")):
        cur = re.match(r"^[A-Za-z_][\w\s\*]*\b(\w+)\s*\(", l).group(1)
    func_at[i] = cur
by = {}
for ln, msg in sorted(errs.items()):
    by.setdefault(func_at.get(ln, "?"), []).append((ln, msg, lines[ln - 1].strip()[:110]))
for f, es in by.items():
    print(f"== {f} ({len(es)})")
    for ln, msg, src in es[:6]:
        print(f"   {ln}: {msg[:70]} | {src}")
