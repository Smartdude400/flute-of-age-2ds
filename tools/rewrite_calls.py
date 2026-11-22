"""Balanced-parenthesis call rewriting (for call sites whose arguments contain nested calls).
Used by rewrite_draw.rewrite(): texture-scroll segment binds -> native Gfx3D_SetTexScroll*."""
import re


def split_args(s):
    args, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == "," and depth == 0:
            args.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        args.append(cur.strip())
    return args


def find_calls(text, name):
    """Yields (start, end, argstring) for each `name(` ... `)` with balanced parentheses."""
    for m in re.finditer(r"\b" + re.escape(name) + r"\(", text):
        i = m.end()
        depth = 1
        while i < len(text) and depth:
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
            i += 1
        yield m.start(), i, text[m.end():i - 1]


def rewrite_tex_scroll(text):
    out, last, n = [], 0, 0
    for start, end, args in list(find_calls(text, "gSPSegment")):
        a = split_args(args)
        if len(a) != 3:
            continue
        inner = a[2]
        m = re.match(r"(Gfx_TwoTexScroll|Gfx_TexScroll)\((.*)\)$", inner, re.S)
        if not m:
            continue
        sub = [("0" if x == "G_TX_RENDERTILE" else x) for x in split_args(m.group(2))[1:]]  # drop gfx context
        fn = "Gfx3D_SetTexScroll2" if m.group(1) == "Gfx_TwoTexScroll" else "Gfx3D_SetTexScroll"
        if fn == "Gfx3D_SetTexScroll":
            sub = ["0"] + sub  # Gfx_TexScroll(gfxCtx, x, y, w, h) has no tile argument: tile 0
        out.append(text[last:start])
        call = f"{fn}({a[1]}, {', '.join(sub)})"
        buf = {"POLY_OPA_DISP++": "OPA", "POLY_XLU_DISP++": "XLU", "OVERLAY_DISP++": "OVL"}.get(a[0].strip())
        out.append(f"GFX_TO({buf}, {call})" if buf else call)
        last = end
        n += 1
    out.append(text[last:])
    return "".join(out), n
