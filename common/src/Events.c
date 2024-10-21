#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include "Win32Int.h"
#else

#include <stdint.h>
#include <unistd.h>

#endif

#include "Common.h"
#include "Enums.h"
#include "FixP.h"
#include "Vec.h"
#include "CActor.h"
#include "MapWithCharKey.h"
#include "Mesh.h"
#include "CTile3DProperties.h"
#include "Renderer.h"
#include "PackedFileReader.h"
#include "Dungeon.h"
#include "Core.h"
#include "LoadBitmap.h"

extern struct GameSnapshot gameSnapshot;


void clearMapCache(void) {
    memFill(&(ITEMS_IN_MAP(0, 0)), 0xFF, MAP_SIZE * MAP_SIZE);
}


void clearTileProperties(void) {
    int c;
    for (c = 0; c < 256; ++c) {
        struct CTile3DProperties *tileProp = (void *) getFromMap(&tileProperties, c);
        if (tileProp) {
            if (tileProp->mFloorTexture.frameNumbers > 0) {
                disposeMem(tileProp->mFloorTexture.frames);
            }

            if (tileProp->mCeilingTexture.frameNumbers > 0) {
                disposeMem(tileProp->mCeilingTexture.frames);
            }

            if (tileProp->mMainWallTexture.frameNumbers > 0) {
                disposeMem(tileProp->mMainWallTexture.frames);
            }

            if (tileProp->mFloorRepeatedTexture.frameNumbers > 0) {
                disposeMem(tileProp->mFloorRepeatedTexture.frames);
            }

            if (tileProp->mCeilingRepeatedTexture.frameNumbers > 0) {
                disposeMem(tileProp->mCeilingRepeatedTexture.frames);
            }
            disposeMem(tileProp);
        }
    }
    clearMap(&tileProperties);
}

void onLevelLoaded(int index) {
    clearMapCache();
    clearTileProperties();
    loadTexturesForLevel(index);
    loadTileProperties(index);
    enable3DRendering = TRUE;	
}

void tickMission(enum ECommand cmd) {

    dungeonTick(cmd);

    if (gameSnapshot.should_continue != kCrawlerGameInProgress) {
        gameTicks = 0;
    }
}

void setItem(const int x, const int y, uint8_t item) {
    ITEMS_IN_MAP(x, y) = item;
}

void loadMap(int map, struct MapWithCharKey *collisionMap) {

    /* all the char keys plus null terminator */
    char collisions[256 + 1];
    int c;
    char nameBuffer[256];
    struct StaticBuffer buffer;

    collisions[256] = 0;
    for (c = 0; c < 256; ++c) {
        collisions[c] = (getFromMap(collisionMap, c) != NULL) ? '1' : '0';
    }

    /* 16 bytes should be enough here... */

    sprintf(nameBuffer, "map%d.txt", map);
    buffer = loadBinaryFileFromPath(nameBuffer);
    dungeon_loadMap(buffer.data, collisions, map);

    disposeDiskBuffer(buffer);
}

int loopTick(enum ECommand command) {

    int needRedraw = 0;

    if (command == kCommandBack) {
        gameSnapshot.should_continue = kCrawlerQuit;
    } else if (command != kCommandNone || gameTicks == 0) {

        if (command == kCommandFire1 || command == kCommandFire2
            || command == kCommandFire3 || command == kCommandFire4) {

            visibilityCached = FALSE;
        }

        tickMission(command);

        if (gameSnapshot.should_continue == kCrawlerGameFinished) {
            return kCrawlerGameFinished;
        }

        if (gameTicks != 0) {
            yCameraOffset = ((struct CTile3DProperties *) getFromMap(&tileProperties,
                                                                     LEVEL_MAP(gameSnapshot.camera_x, gameSnapshot.camera_z)))->mFloorHeight -
                            ((struct CTile3DProperties *) getFromMap(&tileProperties,
                                                                     LEVEL_MAP(gameSnapshot.camera_x,
                                                                               gameSnapshot.camera_z)))->mFloorHeight;
        } else {
            yCameraOffset = 0;
        }


        needRedraw = 1;
    }

    if (zCameraOffset != 0 || xCameraOffset != 0 || yCameraOffset != 0) {
        needRedraw = 1;
    }


    if (needRedraw) {
        createRenderListFor(gameSnapshot.camera_x, gameSnapshot.camera_z, gameSnapshot.camera_rotation);
        if (!enable3DRendering) {
            enable3DRendering = TRUE;
            visibilityCached = FALSE;
        }
    }
    return gameSnapshot.should_continue;
}

void initRoom(int room) {
    int16_t c;
    gameSnapshot.should_continue = kCrawlerGameInProgress;
    mBufferedCommand = kCommandNone;
    gameTicks = 0;
    visibilityCached = FALSE;
    needsToRedrawVisibleMeshes = TRUE;
    needsToRedrawHUD = TRUE;
    clearTextures();
    onLevelLoaded(room);

    for (c = 0; c < 256; ++c) {

        struct CTile3DProperties *tile3DProperties =
                (struct CTile3DProperties *) getFromMap(&tileProperties, c);

        if (tile3DProperties) {
            setInMap(&colliders, c,
                     tile3DProperties->mBlockMovement ? &colliders : NULL);
        } else {
            setInMap(&colliders, c, NULL);
        }
    }

    loadMap(room, &colliders);
}
