	#ifdef WIN32
#include "Win32Int.h"
#else

#include <stdint.h>

#endif

#ifndef SMD

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef WIN32

#include <unistd.h>

#endif
#else
#include <genesis.h>
typedef unsigned long size_t;
#endif

#include "Common.h"
#include "FixP.h"
#include "MapWithCharKey.h"
#include "CTile3DProperties.h"
#include "PackedFileReader.h"
#include "MapWithCharKey.h"
#include "Mesh.h"

void loadPropertyList(const char *propertyFile, struct MapWithCharKey *map, struct MapWithCharKey *meshes) {
    int c;
    struct CTile3DProperties *prop;
    struct StaticBuffer buffer = loadBinaryFileFromPath(propertyFile);
    const uint8_t *limit = buffer.data + buffer.size;
    const uint8_t *bufferHead = buffer.data;
    uint8_t meshCount = 0;
    char meshNameWithExtension[256];

    for (c = 0; c < 256; ++c) {
        struct CTile3DProperties *prop = (struct CTile3DProperties *) getFromMap(map, c);
        if (prop != NULL) {
            disposeMem(prop);
        }
    }

    clearMap(map);

    while (bufferHead != limit) {
        FixP_t val = 0;
        TextureIndex current;

        uint8_t key = *(bufferHead++);
        if (key == 0) {
            /* mesh list reached */
            meshCount = *(bufferHead++);
            goto end;
        }
		prop = (struct CTile3DProperties *) allocMem(
                sizeof(struct CTile3DProperties), GENERAL_MEMORY, 1);

        prop->mNeedsAlphaTest = *(bufferHead++);
        prop->mBlockVisibility = *(bufferHead++);
        prop->mBlockMovement = *(bufferHead++);
        prop->mBlockEnemySight = *(bufferHead++);
        prop->mRepeatMainTexture = *(bufferHead++);

        current = *(bufferHead++);
        if (current != 0xFF) {
            prop->mCeilingTexture.frames = allocMem(1, GENERAL_MEMORY, 0);
            prop->mCeilingTexture.frames[0] = current;
            prop->mCeilingTexture.currentFrame = 0;
            prop->mCeilingTexture.frameNumbers = 1;
            prop->mCeilingTexture.frameTime = prop->mCeilingTexture.currentFrameTime = 0xFE;
        } else {
            prop->mCeilingTexture.frameNumbers = 0;
        }

        current = *(bufferHead++);
        if (current != 0xFF) {
            prop->mFloorTexture.frames = allocMem(1, GENERAL_MEMORY, 0);
            prop->mFloorTexture.frames[0] = current;
            prop->mFloorTexture.currentFrame = 0;
            prop->mFloorTexture.frameNumbers = 1;
            prop->mFloorTexture.frameTime = prop->mFloorTexture.currentFrameTime = 0xFE;
        } else {
            prop->mFloorTexture.frameNumbers = 0;
        }

        current = *(bufferHead++);
        if (current != 0xFF) {
            prop->mMainWallTexture.frames = allocMem(1, GENERAL_MEMORY, 0);
            prop->mMainWallTexture.frames[0] = current;
            prop->mMainWallTexture.currentFrame = 0;
            prop->mMainWallTexture.frameNumbers = 1;
            prop->mMainWallTexture.frameTime = prop->mMainWallTexture.currentFrameTime = 0xFE;
        } else {
            prop->mMainWallTexture.frameNumbers = 0;
        }

        prop->mGeometryType = (*(bufferHead++));

        current = *(bufferHead++);
        if (current != 0xFF) {
            prop->mCeilingRepeatedTexture.frames = allocMem(1, GENERAL_MEMORY, 0);
            prop->mCeilingRepeatedTexture.frames[0] = current;
            prop->mCeilingRepeatedTexture.currentFrame = 0;
            prop->mCeilingRepeatedTexture.frameNumbers = 1;
            prop->mCeilingRepeatedTexture.frameTime = prop->mCeilingRepeatedTexture.currentFrameTime = 0xFE;
        } else {
            prop->mCeilingRepeatedTexture.frameNumbers = 0;
        }

        current = *(bufferHead++);
        if (current != 0xFF) {
            prop->mFloorRepeatedTexture.frames = allocMem(1, GENERAL_MEMORY, 0);
            prop->mFloorRepeatedTexture.frames[0] = current;
            prop->mFloorRepeatedTexture.currentFrame = 0;
            prop->mFloorRepeatedTexture.frameNumbers = 1;
            prop->mFloorRepeatedTexture.frameTime = prop->mFloorRepeatedTexture.currentFrameTime = 0xFE;
        } else {
            prop->mFloorRepeatedTexture.frameNumbers = 0;
        }

        prop->mCeilingRepetitions = *(bufferHead++);
        prop->mFloorRepetitions = *(bufferHead++);

        val += (*(bufferHead++) << 0);
        val += (*(bufferHead++) << 8);
        val += (*(bufferHead++) << 16);
        val += (*(bufferHead++) << 24);
        prop->mCeilingHeight = val;

        val = 0;
        val += (*(bufferHead++) << 0);
        val += (*(bufferHead++) << 8);
        val += (*(bufferHead++) << 16);
        val += (*(bufferHead++) << 24);
        prop->mFloorHeight = val;

        setInMap(map, key, prop);
    }
end:
    for (c = 0; c < meshCount; ++c ) {
        struct Mesh* mesh;
        uint8_t len = *(bufferHead++);

        char* meshName = (char*)allocMem(len + 1, GENERAL_MEMORY, 1);
        memCopyToFrom(meshName, (char*)bufferHead, len);
        mesh = (struct Mesh*)allocMem(sizeof(struct Mesh), GENERAL_MEMORY, 1);
        sprintf(&meshNameWithExtension[0], "%s.mdl", meshName);
        loadMesh(mesh, &meshNameWithExtension[0]);
        setInMap(meshes, kCustomMeshStart + c, mesh);
        disposeMem(meshName);
        bufferHead += len;
    }
    disposeDiskBuffer(buffer);
}
