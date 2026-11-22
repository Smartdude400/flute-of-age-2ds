"""Hand adaptation of source/game/code/z_scene.c (object loading + segment setup)."""
import os
import re

p = os.path.join(os.path.dirname(__file__), "..", "..", "source", "game", "code", "z_scene.c")
s = open(p, encoding="utf-8").read()

s = s.replace('#include "engine/asset.h"\n', '#include "engine/asset.h"\n#include "engine/object_native.h"\n', 1)
if '#include "engine/object_native.h"' not in s:
    s = re.sub(r'(#include "[^"]+"\n)', r'\1#include "engine/asset.h"\n#include "engine/object_native.h"\n', s, count=1)

# Object_LoadPersistent: synchronous load (same as the original's synchronous DMA)
s = s.replace(
    r'''    DMA_REQUEST_SYNC(objectCtx->slots[objectCtx->numEntries].segment, gObjectTable[objectId].vromStart, size,
                     "../z_scene.c", 145);''',
    r'''    // native: synchronous load of the converted object (original: synchronous DMA into the slot)
    ObjectNative_Load(objectCtx, objectCtx->numEntries, objectId);''')

# Object_InitContext: free natives from a previous scene, bind gameplay_keep to segment 4
s = s.replace(
    r'''    objectCtx->numEntries = objectCtx->numPersistentEntries = 0;''',
    r'''    ObjectNative_FreeFrom(objectCtx, 0);
    objectCtx->numEntries = objectCtx->numPersistentEntries = 0;''')
s = s.replace(
    r'''    gSegments[4] = OS_K0_TO_PHYSICAL(objectCtx->slots[objectCtx->mainKeepSlot].segment);''',
    r'''    Asset_SetSegment(4, objectCtx->slots[objectCtx->mainKeepSlot].dsObj);''')

# Object_UpdateEntries: async load -> native load at request time, completion on the next call
s = s.replace(
    r'''            if (entry->dmaRequest.vromAddr == 0) {
                osCreateMesgQueue(&entry->loadQueue, &entry->loadMsg, 1);
                objectFile = &gObjectTable[-entry->id];
                size = objectFile->vromEnd - objectFile->vromStart;

                PRINTF("OBJECT EXCHANGE BANK-%2d SIZE %8.3fK SEG=%08x\n", i, size / 1024.0f, entry->segment);

                DMA_REQUEST_ASYNC(&entry->dmaRequest, entry->segment, objectFile->vromStart, size, 0, &entry->loadQueue,
                                  NULL, "../z_scene.c", 266);
            } else if (osRecvMesg(&entry->loadQueue, NULL, OS_MESG_NOBLOCK) == 0) {
                entry->id = -entry->id; // Object now loaded
            }''',
    r'''            // native: the original started an async DMA on the first call and saw it complete on a later
            // call. The converted object is loaded on the first call and reported complete on the
            // next call (assumption A3, same model as rooms).
            if (!entry->loadPending) {
                ObjectNative_Load(objectCtx, i, -entry->id);
                entry->loadPending = 1;
            } else {
                entry->loadPending = 0;
                entry->id = -entry->id; // Object now loaded
            }''')
s = s.replace(
    r'''    entry->id = -objectId;
    entry->dmaRequest.vromAddr = 0;''',
    r'''    entry->id = -objectId;
    entry->loadPending = 0;''')
# Object_ReloadAll (after pause): synchronous reload
s = s.replace(
    r'''        DMA_REQUEST_SYNC(objectCtx->slots[i].segment, gObjectTable[id].vromStart, size, "../z_scene.c", 342);''',
    r'''        ObjectNative_Load(objectCtx, i, id);''')
# sub keep segment
s = s.replace(
    r'''    gSegments[5] = OS_K0_TO_PHYSICAL(play->objectCtx.slots[play->objectCtx.subKeepSlot].segment);''',
    r'''    Asset_SetSegment(5, play->objectCtx.slots[play->objectCtx.subKeepSlot].dsObj);''')
open(p, "w", encoding="utf-8", newline="\n").write(s)
print("patched", p)
