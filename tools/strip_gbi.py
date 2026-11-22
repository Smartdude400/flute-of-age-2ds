"""Remove display-list command statements from C function bodies while keeping every other statement.

Used for draw functions that interleave GAME LOGIC (RNG draws, state stepping) with display-list
construction: the logic stays line for line, the GBI statements go, and the native renderer draws
from the same state. A removed statement that is the sole body of a brace-less `if`/`else`/loop is
replaced by `;` so control flow is unchanged.

API: strip(body, extra_names=()) -> (new_body, removed_count)"""
import re

BASE = (r"gs?SP\w+|gs?DP\w+|gSP\w+|gDP\w+|Gfx_SetupDL\w*|Gfx_Texture\w+|func_800947AC|func_80094E54|"
        r"Gfx_SetupDLAtPtr|Gfx_BranchTexScroll|Gfx_EnvColor|Gfx_PrimColor|MATRIX_FINALIZE_AND_LOAD|"
        r"Matrix_Finalize|CLOSE_DISPS|OPEN_DISPS|Gfx_SetFog\w*|Gfx_SetupFrame")
ASSIGN = re.compile(r"\b(?:OVERLAY_DISP|POLY_OPA_DISP|POLY_XLU_DISP|POLY_KAL_DISP|WORK_DISP|gfx|displayListHead|"
                    r"displayList|dList|gfxP|gfxHead)\s*(?:=|\+=)")


def _control_header_before(text, pos):
    """True if the statement at pos is the unbraced body of if/else/for/while."""
    k = pos - 1
    while k >= 0 and text[k] in " \t\r\n":
        k -= 1
    if k < 0:
        return False
    if text[k] == ")":
        # find matching '(' and check the keyword before it
        depth, j = 0, k
        while j >= 0:
            if text[j] == ")":
                depth += 1
            elif text[j] == "(":
                depth -= 1
                if depth == 0:
                    break
            j -= 1
        pre = text[:j].rstrip()
        return bool(re.search(r"\b(?:if|for|while)$", pre))
    return text[k - 3:k + 1] == "else" if k >= 3 else False


def strip(body, extra_names=()):
    names = BASE + ("|" + "|".join(re.escape(n) for n in extra_names) if extra_names else "")
    call = re.compile(r"\b(?:" + names + r")\(")
    out, i, n, removed = [], 0, len(body), 0
    while i < n:
        cands = [x for x in (call.search(body, i), ASSIGN.search(body, i)) if x]
        if not cands:
            out.append(body[i:])
            break
        m = min(cands, key=lambda x: x.start())
        line_start = body.rfind("\n", 0, m.start()) + 1
        if body[line_start:m.start()].strip():
            out.append(body[i:m.end()])
            i = m.end()
            continue
        j, depth = m.start(), 0
        while j < n:
            c = body[j]
            if c in "([{":
                depth += 1
            elif c in ")]}":
                depth -= 1
            elif c == ";" and depth == 0:
                break
            j += 1
        sole = _control_header_before(body, m.start())
        out.append(body[i:line_start])
        if sole:
            out.append(body[line_start:m.start()] + "; // native: display-list statement removed\n")
        i = j + 1
        if i < n and body[i] == "\n":
            i += 1
        removed += 1
    return "".join(out), removed
