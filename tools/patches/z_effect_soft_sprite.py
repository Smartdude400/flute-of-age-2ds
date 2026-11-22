"""EffectSs_Spawn on a statically linked build (see z_effect_soft_sprite_dlftbls.c)."""
import os
p = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_effect_soft_sprite.c")
s = open(p, encoding="utf-8").read()
s = s.replace(r'''            Overlay_Load(overlayEntry->file.vromStart, overlayEntry->file.vromEnd, overlayEntry->vramStart,
                         overlayEntry->vramEnd, overlayEntry->loadedRamAddr);''',
              r'''            // native: code is statically linked; the reservation above keeps the original heap usage''')
s = s.replace(r'''        profile = (void*)(uintptr_t)((overlayEntry->profile != NULL)
                                         ? (void*)((uintptr_t)overlayEntry->profile -
                                                   (intptr_t)((uintptr_t)overlayEntry->vramStart -
                                                              (uintptr_t)overlayEntry->loadedRamAddr))
                                         : NULL);''',
              r'''        profile = overlayEntry->profile; // native: statically linked, no relocation''')
s = s.replace(r'''    if (profile->init == NULL) {''', r'''    if (profile == NULL) {
        ActorNative_LogUnportedEffect(type);
        return;
    }

    if (profile->init == NULL) {''', 1)
s = s.replace('#include "effect.h"\n', '#include "effect.h"\n#include "engine/actor_native.h"\n', 1)
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("patched", p)
