"""Restore the display-list buffer (opaque / translucent / overlay) and the exact render-state preset of
every draw call in the translated game code.

The DS-era rewrite (tools/rewrite_draw.py) turned `gSPDisplayList(POLY_XLU_DISP++, x)` into
`Gfx3D_DrawDL(x)` and every `Gfx_SetupDL_NNxxx(...)` into `Gfx3D_ResetMaterialState(0|1)`, losing which of the
original's three display lists a call went to and which preset was used. This tool recovers both by
aligning, function by function, the sequence of draw events in the ORIGINAL source with the sequence of
native calls in the translated file (difflib on event types), then rewrites each native call:
  preset       -> Gfx3D_SetupDLTo(GFX_BUF_X, N)
  other calls  -> GFX_TO(X, call)          (engine/gfx3d.h: selects the buffer, then makes the call)
Calls without an aligned original event take the buffer of the nearest preceding aligned event.
usage: python tools/restore_buffers.py [--apply] [files...]   (default: every game file using Gfx3D_)"""
import difflib
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rewrite_calls import find_calls  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.join(HERE, "..", "source", "game")
ORIG = os.path.join(HERE, "..", "..", "flute-of-age")

BUF = {"POLY_OPA_DISP": "OPA", "POLY_XLU_DISP": "XLU", "OVERLAY_DISP": "OVL", "POLY_KAL_DISP": "OVL",
       "WORK_DISP": "OPA"}
BUFRE = r"(POLY_OPA_DISP|POLY_XLU_DISP|OVERLAY_DISP|POLY_KAL_DISP|WORK_DISP)"

# ---- original-side event patterns: (type, regex, buffer group or fixed, preset group or fixed)
ORIG_PATTERNS = [
    ("SETUP", re.compile(r"\bGfx_SetupDL_(\d+)(Opa|Xlu2?|Overlay|NoCDXlu)\s*\("), None, None),
    ("SETUP", re.compile(BUFRE + r"\s*=\s*Gfx_SetupDL_(\d+)\w*\(\s*" + BUFRE), None, None),
    ("SETUP", re.compile(BUFRE + r"\s*=\s*Gfx_SetupDL\(\s*" + BUFRE + r"\s*,\s*(?:SETUPDL_)?(\d+)"), None, None),
    ("SETUP", re.compile(r"\bfunc_80093C80\s*\("), "OPA", 25),
    ("SETUP", re.compile(BUFRE + r"\s*=\s*func_800947AC\(\s*" + BUFRE), None, 65),
    ("DL", re.compile(r"\bgSPDisplayList\(\s*" + BUFRE), None, None),
    ("PRIM", re.compile(r"\bgDPSetPrimColor\(\s*" + BUFRE), None, None),
    ("ENV", re.compile(r"\bgDPSetEnvColor\(\s*" + BUFRE), None, None),
    ("SEG", re.compile(r"\bgSPSegment\(\s*" + BUFRE), None, None),
    ("MTX", re.compile(r"\b(?:MATRIX_FINALIZE_AND_LOAD|gSPMatrix)\(\s*" + BUFRE), None, None),
    ("SKEL", re.compile(r"\bSkelAnime_Draw\w*\(([^;]*?)\)\s*;", re.S), None, None),
    ("FILL", re.compile(r"\bgDPFillRectangle\(\s*" + BUFRE), None, None),
]

CUR_PATTERNS = [
    ("SETUP", re.compile(r"\bGfx3D_ResetMaterialState\(")),
    ("DL", re.compile(r"\bGfx3D_DrawDL\(")),
    ("DLM", re.compile(r"\bGFX3D_DRAW_CURRENT\(")),
    ("PRIM", re.compile(r"\bGfx3D_SetPrimColor\(")),
    ("ENV", re.compile(r"\bGfx3D_SetEnvColor\(")),
    ("SEG", re.compile(r"\bGfx3D_(?:SetSegment|SetTexScroll2?|SegDLBegin|SetSegmentMtx)\(")),
    ("MTX", re.compile(r"\bGfx3D_LoadMtxF\(")),
    ("SKEL", re.compile(r"\bSkelAnime_Draw\w*\(")),
    ("FILL", re.compile(r"\bGfx3D_FillScreenPrim\(")),
]

FUNC_RE = re.compile(r"^[A-Za-z_][\w \t\*]*?\b(\w+)\s*\([^;{]*\)\s*\{", re.M)


def functions(text):
    """name -> (start, end) of top-level function definitions."""
    out = {}
    for m in FUNC_RE.finditer(text):
        name = m.group(1)
        if name in ("if", "for", "while", "switch"):
            continue
        i = m.end() - 1
        depth = 0
        while i < len(text):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        out[name] = (m.start(), i + 1)
    return out


def strip_comments(s):
    return re.sub(r"//[^\n]*|/\*.*?\*/", lambda m: " " * len(m.group(0)), s, flags=re.S)


def orig_events(body):
    ev = []
    body = strip_comments(body)
    for typ, rx, fbuf, fpre in ORIG_PATTERNS:
        for m in rx.finditer(body):
            g = m.groups()
            buf, pre = fbuf, fpre
            if typ == "SETUP":
                if rx.pattern.startswith(r"\bGfx_SetupDL_"):
                    pre = int(g[0])
                    suf = g[1]
                    buf = "OPA" if suf == "Opa" else ("OVL" if suf == "Overlay" else "XLU")
                elif "Gfx_SetupDL_(" in rx.pattern:
                    buf, pre = BUF[g[0]], int(g[1])
                elif "Gfx_SetupDL\\(" in rx.pattern:
                    buf, pre = BUF[g[0]], int(g[2])
                elif fpre == 65:
                    buf = BUF[g[0]]
            elif typ == "SKEL":
                name = body[m.start():m.start() + 40]
                args = g[0]
                bm = re.search(BUFRE, args)
                if bm:
                    buf = BUF[bm.group(1)]
                elif re.match(r"SkelAnime_Draw\w*Opa", name):
                    buf = "OPA"
                else:
                    buf = None
            else:
                buf = BUF[g[0]]
            ev.append((m.start(), typ, buf, pre))
    ev.sort()
    # a preset helper that also gets matched as a DL (gSPDisplayList inside helpers) cannot happen here:
    # helpers live in z_rcp.c, not in the files being aligned
    return [(t, b, p) for _, t, b, p in ev]


def cur_events(body, base):
    ev = []
    code = strip_comments(body)
    for typ, rx in CUR_PATTERNS:
        for m in rx.finditer(code):
            # skip definitions / already restored calls
            pre = code[max(0, m.start() - 12):m.start()]
            if "GFX_TO(" in pre or "Gfx3D_SetupDLTo" in code[m.start():m.start() + 20]:
                continue
            line_start = code.rfind(chr(10), 0, m.start()) + 1
            if re.match(r"\s*(static\s+)?[A-Za-z_]\w*[\s\*]+$", code[line_start:m.start()]):
                continue  # a function definition, not a call
            ev.append((base + m.start(), typ))
    ev.sort()
    return ev


def norm(t):
    return "DL" if t == "DLM" else t


def align(oev, cev):
    """-> list of (cur_pos, cur_type, buf, preset, matched)"""
    a = [t for t, _, _ in oev]
    b = [norm(t) for _, t in cev]
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    match = {}
    for blk in sm.get_matching_blocks():
        for k in range(blk.size):
            match[blk.b + k] = blk.a + k
    out = []
    last_buf = None
    for j, (pos, typ) in enumerate(cev):
        if j in match:
            t, buf, pre = oev[match[j]]
            if buf is not None:
                last_buf = buf
            out.append((pos, typ, buf or last_buf, pre, True))
        else:
            out.append((pos, typ, last_buf, None, False))
    # leading unmatched calls take the first known buffer
    first = next((b for _, _, b, _, _ in out if b), None)
    return [(p, t, b or first, pr, m) for p, t, b, pr, m in out]


def provenance(text):
    m = re.search(r"adapted from flute-of-age/(\S+\.c)", text)
    return os.path.join(ORIG, m.group(1)) if m else None


def process(path, apply):
    text = open(path, encoding="utf-8").read()
    src = provenance(text)
    if not src or not os.path.exists(src):
        return None
    orig = open(src, encoding="utf-8").read()
    ofuncs = functions(orig)
    cfuncs = functions(text)
    edits = []  # (pos, typ, buf, preset)
    stats = {"calls": 0, "matched": 0, "inferred": 0, "unknown": 0, "presets_exact": 0, "presets_default": 0}
    for name, (a, b) in cfuncs.items():
        cev = cur_events(text[a:b], a)
        if not cev:
            continue
        oev = orig_events(orig[ofuncs[name][0]:ofuncs[name][1]]) if name in ofuncs else []
        for pos, typ, buf, pre, matched in align(oev, cev):
            stats["calls"] += 1
            if buf is None:
                stats["unknown"] += 1
                continue
            stats["matched" if matched else "inferred"] += 1
            if typ == "SETUP":
                stats["presets_exact" if pre is not None else "presets_default"] += 1
            edits.append((pos, typ, buf, pre))
    if apply and edits:
        for pos, typ, buf, pre in sorted(edits, reverse=True):
            if typ == "SETUP":
                # replace the whole Gfx3D_ResetMaterialState(x) call
                end = text.index(")", pos) + 1
                preset = pre if pre is not None else 25
                text = text[:pos] + f"Gfx3D_SetupDLTo(GFX_BUF_{buf}, {preset})" + text[end:]
            else:
                # wrap the call: GFX_TO(BUF, call)
                for s, e, _ in find_calls(text[pos:], re.match(r"\w+", text[pos:]).group(0)):
                    end = pos + e
                    break
                text = text[:pos] + f"GFX_TO({buf}, " + text[pos:end] + ")" + text[end:]
        open(path, "w", encoding="utf-8", newline="\n").write(text)
    return stats


def main():
    apply = "--apply" in sys.argv
    files = [f for f in sys.argv[1:] if not f.startswith("--")]
    if not files:
        files = [f for f in glob.glob(os.path.join(GAME, "**", "*.c"), recursive=True)
                 if "Gfx3D_" in open(f, encoding="utf-8").read() or "SkelAnime_Draw" in open(f, encoding="utf-8").read()]
    total = {}
    for f in sorted(files):
        st = process(f, apply)
        if st is None:
            print(f"{os.path.relpath(f, GAME)}: no original (native file), skipped")
            continue
        print(f"{os.path.relpath(f, GAME)}: {st}")
        for k, v in st.items():
            total[k] = total.get(k, 0) + v
    print("TOTAL", total)


if __name__ == "__main__":
    main()
