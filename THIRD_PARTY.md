# Third-party notices

Built `.3dsx` files link the following libraries from the devkitPro 3DS toolchain. Nothing from them
is vendored in this repository.

| Component | License | Notes |
|---|---|---|
| libctru | zlib | 3DS system library (services, GPU command queue, RomFS, HID) |
| citro3d | zlib | PICA200 GPU library (render targets, texture combiner, uniforms) |
| newlib / libgcc (devkitARM) | BSD-style / GPL with runtime exception | C runtime; no obligation for produced binaries |
| picasso | MIT | shader assembler, build time only |

Game code: translated from a community decompilation of the original game (see README, *Legal*).
Asset XML specs in `tools/assets_xml/` come from the same decompilation project and describe where
data lives inside the ROM files; they contain no game data.
