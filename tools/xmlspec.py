"""Reads the asset XML specs in tools/assets_xml -> symbols with offsets.

Elements without an Offset attribute are laid out sequentially after the previous element
(cursor += element size), with 8-byte alignment for display lists, textures and vertex arrays.
<Version Pattern="regex"> blocks apply only when the pattern matches the target ROM version.
Elements that DO carry an explicit Offset act as a self-check of the computed cursor.
"""
import glob
import os
import re
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
XML_ROOT = os.path.join(HERE, "assets_xml")  # asset XML specs (symbol names and offsets of the ROM files)
VERSION = "ntsc-1.0"

TEX_BPP = {"rgba16": 16, "rgba32": 32, "ci4": 4, "ci8": 8, "i4": 4, "i8": 8, "ia4": 4, "ia8": 8, "ia16": 16, "i1": 1}
LIMB_SIZE = {"Standard": 0xC, "LOD": 0x10, "Skin": 0x10, "Curve": 0x8, "Legacy": 0x20}
SCALAR = {"s8": 1, "u8": 1, "s16": 2, "u16": 2, "s32": 4, "u32": 4, "f32": 4, "f64": 8}
mismatches = []


def find_xml(name):
    hits = glob.glob(os.path.join(XML_ROOT, "**", name + ".xml"), recursive=True)
    return hits[0] if hits else None


def elem_size(el):
    tag = el.tag
    a = el.attrib
    if tag == "DList":
        return int(a.get("Length", "0"), 0) * 8, 8
    if tag == "Texture":
        bpp = TEX_BPP.get(a.get("Format", "rgba16"), 16)
        return int(a["Width"]) * int(a["Height"]) * bpp // 8, 8
    if tag == "Blob":
        return int(a.get("Size", "0"), 0), 1
    if tag == "Array":
        count = int(a.get("Count", "0"), 0)
        child = next(iter(el), None)
        if child is None:
            return 0, 4
        if child.tag == "Vtx":
            return count * 16, 8
        if child.tag == "Vector":
            return count * SCALAR.get(child.get("Type", "s16"), 2) * int(child.get("Dimensions", "3")), 2
        if child.tag == "Scalar":
            return count * SCALAR.get(child.get("Type", "s16"), 2), SCALAR.get(child.get("Type", "s16"), 2)
        return 0, 4
    if tag == "Vector":
        return SCALAR.get(a.get("Type", "s16"), 2) * int(a.get("Dimensions", "3")), 2
    if tag == "Scalar":
        return SCALAR.get(a.get("Type", "s16"), 2), SCALAR.get(a.get("Type", "s16"), 2)
    if tag == "Limb":
        return LIMB_SIZE.get(a.get("LimbType", a.get("Type", "Standard")), 0xC), 4
    if tag == "LimbTable":
        return int(a.get("Count", "0"), 0) * 4, 4
    if tag == "Skeleton":
        return (0xC if a.get("Type") == "Flex" else 0x8), 4
    if tag == "Animation":
        return 0x10, 4
    if tag == "PlayerAnimation":
        return 0x8, 4
    if tag == "Collision":
        return 0x2C, 4
    if tag == "Mtx":
        return 0x40, 8
    return 0, 1


def walk(parent, cursor, syms, fname):
    for el in parent:
        if el.tag == "Version":
            if re.fullmatch(el.get("Pattern", ".*"), VERSION):
                cursor = walk(el, cursor, syms, fname)
            continue
        off = el.get("Offset")
        size, align = elem_size(el)
        if off is not None:
            if off.startswith(".+"):
                # relative offset: skip padding after the previous element
                if cursor is None:
                    continue
                o = cursor + int(off[2:], 0)
                off = None  # treat as computed (no self-check)
                if el.get("Name"):
                    syms.append((el.tag, el.get("Name"), o, dict(el.attrib)))
                cursor = o + size
                continue
            try:
                o = int(off, 16)
            except ValueError:
                continue
            if el.get("Name") and cursor is not None and size:
                aligned = (cursor + align - 1) & ~(align - 1)
                if o != aligned and abs(o - aligned) < 0x10:
                    mismatches.append((fname, el.get("Name"), hex(o), hex(aligned)))
            cursor = o
        elif cursor is None:
            continue
        else:
            cursor = (cursor + align - 1) & ~(align - 1)
        if el.get("Name"):
            attrs = dict(el.attrib)
            if el.tag == "Array" and len(el):
                # element description (e.g. <Vector Type="s16" Dimensions="3"/>)
                attrs["ElemTag"] = el[0].tag
                attrs.update({"Elem" + k: v for k, v in el[0].attrib.items()})
            syms.append((el.tag, el.get("Name"), cursor, attrs))
        cursor = cursor + size if size else None if off is None else cursor + size
    return cursor


def load(name):
    """Returns {file_name: {"segment": int, "symbols": [(tag, name, offset, attrs)]}}"""
    path = find_xml(name)
    if path is None:
        raise FileNotFoundError(name)
    root = ET.parse(path).getroot()
    out = {}
    for f in root.iter("File"):
        syms = []
        walk(f, 0, syms, f.get("Name"))
        seen = set()
        uniq = []
        for s in syms:
            if s[1] not in seen:
                seen.add(s[1])
                uniq.append(s)
        out[f.get("Name")] = {"segment": int(f.get("Segment", "0")), "symbols": uniq}
    return out


if __name__ == "__main__":
    import sys
    d = load(sys.argv[1])
    for fn, v in d.items():
        print(fn, "segment", v["segment"], len(v["symbols"]), "symbols")
        for s in v["symbols"]:
            if len(sys.argv) > 2 and sys.argv[2] in s[1]:
                print("  ", s[0], s[1], hex(s[2]))
    print("cursor mismatches vs explicit offsets:", mismatches[:10], len(mismatches))
