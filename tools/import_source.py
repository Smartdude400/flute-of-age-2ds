"""Copy an original .c file into source/game/ as the starting point of an ADAPTED file.

Only mechanical edits happen here (provenance banner, ultra64.h -> game_types.h,
bzero/bcopy call sites -> memset/memmove, exact-math renames). Everything platform-specific is
then rewritten by hand. usage: python tools/import_source.py src/code/TwoHeadArena.c [dest_subdir]
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "flute-of-age")

EXACT_MATH = (("sinf", "ExactSinF"), ("cosf", "ExactCosF"), ("sins", "ExactSinS"), ("coss", "ExactCosS"),
              ("floorf", "ExactFloorF"), ("floor", "ExactFloor"), ("ceilf", "ExactCeilF"), ("ceil", "ExactCeil"),
              ("truncf", "ExactTruncF"), ("trunc", "ExactTrunc"), ("nearbyintf", "ExactNearbyIntF"),
              ("nearbyint", "ExactNearbyInt"), ("roundf", "ExactRoundF"), ("round", "ExactRound"),
              ("lfloorf", "ExactLFloorF"), ("lceilf", "ExactLCeilF"), ("ltruncf", "ExactLTruncF"),
              ("lnearbyintf", "ExactLNearbyIntF"), ("lroundf", "ExactLRoundF"))


def convert(text, rel, keep_rdp=False):
    text = re.sub(r'#include "ultra64(\.h|/ultratypes\.h)"', '#include "game_types.h"', text)
    text = re.sub(r"\bbzero\(([^,]+),", r"memset(\1, 0,", text)
    text = re.sub(r"\bbcopy\(([^,]+),\s*([^,]+),", r"memmove(\2, \1,", text)
    # exact game math (see game/include/exact_math.h)
    for old, new in EXACT_MATH:
        text = re.sub(r"\b" + old + r"\(", new + "(", text)
    if re.search(r"\bExact\w+\(", text) and '#include "exact_math.h"' not in text:
        text = re.sub(r'(#include "[^"]+"\n)', r'\1#include "exact_math.h"\n', text, count=1)
    text = re.sub(r'#pragma increment_block_number[^\n]*\n(\s+"[^\n]*\n)*', "", text)
    if keep_rdp:
        # RDP commands kept: engine/rdp_disps.h gives them native meaning (cursors select the buffer)
        text = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/rdp_disps.h"\n#include "dma.h"\n', text, count=1)
    else:
        from rewrite_draw import rewrite
        text, _ = rewrite(text)
    text = text.replace("SEGMENTED_TO_VIRTUAL(", "Asset_Resolve(")
    if "Asset_Resolve(" in text and '#include "engine/asset.h"' not in text:
        text = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/asset.h"\n', text, count=1)
    return f"// adapted from flute-of-age/{rel}\n" + text


def main():
    args = [a for a in sys.argv[1:] if a != "--rdp"]
    keep_rdp = "--rdp" in sys.argv
    rel = args[0]
    sub = args[1] if len(args) > 1 else ""
    text = open(os.path.join(SRC, rel), encoding="utf-8").read()
    dst = os.path.join(HERE, "..", "source", "game", sub, os.path.basename(rel))
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.exists(dst):
        sys.exit("exists: " + dst)
    open(dst, "w", encoding="utf-8", newline="\n").write(convert(text, rel, keep_rdp))
    print("->", os.path.relpath(dst, os.path.join(HERE, "..")))


if __name__ == "__main__":
    main()
