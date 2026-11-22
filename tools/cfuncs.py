"""Small helper to remove or replace top-level C function definitions by name (for adapting)."""
import re


def find_func(text, name):
    """Returns (start, end) span of the top-level definition of `name`, or None."""
    m = re.search(r"^[A-Za-z_][^\n;{}]*\b" + re.escape(name) + r"\s*\([^;{]*\)\s*\{", text, re.M)
    if not m:
        return None
    i = m.end() - 1
    depth = 0
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                end = i + 1
                while end < len(text) and text[end] == "\n":
                    end += 1
                return m.start(), end
        i += 1
    raise ValueError("unbalanced " + name)


def remove_funcs(text, names, note=None):
    for n in names:
        span = find_func(text, n)
        while span:
            repl = f"// {n}: {note}\n\n" if note else ""
            text = text[:span[0]] + repl + text[span[1]:]
            span = find_func(text, n)
    return text


def stub_funcs(text, names, kind="PRESENTATION_STUB"):
    """Replace function bodies with a logged stub (draw-only functions). Return type preserved:
    non-void functions return 0 / NULL. Every use is listed in STUBS.md (P-series)."""
    import re as _re
    for n in names:
        span = find_func(text, n)
        if span is None:
            raise KeyError(n)
        head = text[span[0]:text.index("{", span[0])]
        ret = head.strip().split(n)[0].strip()
        body = f'{{\n    {kind}("{n}");\n'
        if ret not in ("void", "static void", "static inline void"):
            body += "    return 0;\n"
        body += "}\n\n"
        text = text[:span[0]] + head + body + text[span[1]:]
    return text
