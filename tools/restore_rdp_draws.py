"""Put the original bodies of 2D draw functions back into a translated source (development tool: needs the
decompilation checkout).

  python tools/restore_rdp_draws.py src/code/z_message.c game/code/z_message.c Message_DrawText ...

The first translation pass (import_source.py / rewrite_draw.py) dropped RDP commands it could not express
(texture block loads, texture rectangles, combiner words, vertex quads) and the ROM loads of the interface
textures, so text boxes and the HUD drew nothing. With engine/rdp_gbi.h and dma.h those have native
equivalents, so the original function bodies are taken as they are, with only:
  - the display-list cursor plumbing removed (`Gfx** p` parameters, `Gfx* gfx = *p;`, `*p = gfx;`, the
    `&gfx` arguments of calls between these functions, OPEN_DISPS / CLOSE_DISPS) - the native commands
    record their own lists;
  - list helpers that returned the advanced cursor (`OVERLAY_DISP = Gfx_TextureIA8(OVERLAY_DISP, ...)`)
    called for their commands only; their definitions lose the cursor parameter;
  - DMA completion waits on removed message queues dropped (the copy is immediate);
  - the same source rules as import_source.py (exact math, SEGMENTED_TO_VIRTUAL -> Asset_Resolve).
A function missing from the translated file is inserted before the first restored function."""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "flute-of-age")
GAME = os.path.join(HERE, "..", "source")
sys.path.insert(0, HERE)
from import_source import EXACT_MATH  # noqa: E402

FUNC = r"^((?:static )?(?:void|s32|u16|u8|s16|Gfx\*) {name}\(([^)]*)\) \{{\n)(.*?)^\}}"
LIST_HELPERS = r"Gfx_Texture\w+"


def extract(text, name):
    return re.search(FUNC.format(name=name), text, re.S | re.M)


def native_body(header, body):
    header = re.sub(r",\s*Gfx\*\* \w+\)", ")", header)
    header = re.sub(r"\(PlayState\* play, Gfx\*\* \w+, ", "(PlayState* play, ", header)
    if header.startswith("Gfx* "):  # list helper: no cursor
        header = "void " + header[5:]
        header = re.sub(r"\(Gfx\* \w+, ", "(", header)
        body = re.sub(r"return \w+;", "return;", body)
    drop = (r"^\s*Gfx\* gfx = \*\w+;\s*$", r"^\s*\*\w+ = gfx;\s*$", r"^\s*(OPEN|CLOSE)_DISPS\(.*\);\s*$",
            r"^\s*os(CreateMesgQueue|RecvMesg)\(&\w+->loadQueue.*\);\s*$")
    body = "\n".join(ln for ln in body.split("\n") if not any(re.match(d, ln) for d in drop))
    body = re.sub(r",\s*&gfx\)", ")", body)
    body = re.sub(r",\s*&gfx,", ",", body)
    body = re.sub(r"\(play, &gfx, ", "(play, ", body)
    body = re.sub(r"(\w+_DISP)\s*=\s*(" + LIST_HELPERS + r")\(\s*\1,\s*", r"\2(", body)
    for old, new in EXACT_MATH:
        body = re.sub(r"\b" + old + r"\(", new + "(", body)
    body = body.replace("SEGMENTED_TO_VIRTUAL(", "Asset_Resolve(")
    return header, body


def main():
    src_rel, dst_rel, names = sys.argv[1], sys.argv[2], sys.argv[3:]
    orig = open(os.path.join(SRC, src_rel), encoding="utf-8").read()
    path = os.path.join(GAME, dst_rel)
    text = open(path, encoding="utf-8").read()
    missing = []
    for name in names:
        o = extract(orig, name)
        if o is None:
            raise SystemExit(f"{name}: not in the original")
        header, body = native_body(o.group(1), o.group(3))
        new = header + "    // (restored: original body, native RDP commands via engine/rdp_gbi.h)\n" + body + "}"
        n = extract(text, name)
        if n is None:
            missing.append(new)
            print(f"{name}: inserted ({body.count(chr(10))} lines)")
            continue
        if header.split("(")[1] != n.group(1).split("(")[1]:
            print(f"{name}: signature differs, keeping the native one: {n.group(1).strip()}")
            new = n.group(1) + new[len(header):]
        text = text[:n.start()] + new + text[n.end():]
        print(f"{name}: restored ({body.count(chr(10))} lines)")
    if missing:
        first = min((extract(text, nm).start() for nm in names if extract(text, nm)), default=None)
        if first is None:
            raise SystemExit("no anchor for the inserted functions")
        text = text[:first] + "\n\n".join(missing) + "\n\n" + text[first:]
    for inc in ('#include "engine/rdp_gbi.h"', '#include "dma.h"'):
        if inc not in text:
            text = re.sub(r'(#include "[^"]+"\n)', r'\1' + inc + r'\n', text, count=1)
    open(path, "w", encoding="utf-8", newline="\n").write(text)


main()
