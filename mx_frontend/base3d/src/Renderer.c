#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef WIN32
#include "Win32Int.h"
#else

#include <stdint.h>
#include <unistd.h>

#endif

#include "Common.h"
#include "FixP.h"
#include "Vec.h"
#include "Common.h"
#include "Enums.h"
#include "CActor.h"
#include "MapWithCharKey.h"
#include "LoadBitmap.h"
#include "CTile3DProperties.h"
#include "Renderer.h"
#include "VisibilityStrategy.h"
#include "PackedFileReader.h"
#include "Core.h"
#include "Engine.h"
#include "Derelict.h"
#include "Globals.h"
#include "UI.h"
#include "Mesh.h"

#define STANDARD_HEIGHT (Div(intToFix(230), intToFix(100)))

extern const char *focusItemName;
extern int leanX, leanY, turning;

int pointerClickPositionX = -1;
int pointerClickPositionY = -1;

int hasSnapshot = FALSE;
FixP_t playerHeight = 0;
FixP_t walkingBias = 0;
FixP_t playerHeightChangeRate = 0;
FixP_t playerHeightTarget = 0;
int visibilityCached = FALSE;
int needsToRedrawVisibleMeshes = TRUE;
struct MapWithCharKey occluders;
struct MapWithCharKey colliders;
struct MapWithCharKey enemySightBlockers;
struct Bitmap *defaultFont;
extern int needsToRedrawHUD;
int enable3DRendering = TRUE;
struct MapWithCharKey animations;

#ifndef AGS
FramebufferPixelFormat framebuffer[XRES_FRAMEBUFFER * YRES_FRAMEBUFFER];
FramebufferPixelFormat previousFrame[XRES_FRAMEBUFFER * YRES_FRAMEBUFFER];
OutputPixelFormat palette[256];
#else
FramebufferPixelFormat *framebuffer;
#endif

enum EDirection cameraDirection;
struct Vec3 mCamera;
long gameTicks = 0;
int dirtyLineY0 = 0;
int dirtyLineY1 = YRES_FRAMEBUFFER;
const int distanceForPenumbra = 16;
struct MapWithCharKey tileProperties;
struct MapWithCharKey customMeshes;
struct Vec2i cameraPosition;
extern uint8_t usedTexture;
enum ECommand mBufferedCommand = kCommandNone;
extern struct Texture *textures;
struct Bitmap *itemSprites[TOTAL_ITEMS];
int turnTarget = 0;
int turnStep = 0;
FixP_t xCameraOffset;
FixP_t yCameraOffset;
FixP_t zCameraOffset;
uint8_t enableSmoothMovement = FALSE;

struct Projection projectionVertices[8];

enum EVisibility *visMap;
struct Vec2i *distances;

char *messageLogBuffer;

int messageLogBufferCoolDown = 0;

void printMessageTo3DView(const char *message);

void enter2D(void) {

}

void enter3D(void) {

}

void printMessageTo3DView(const char *message) {
    strcpy(messageLogBuffer, message);
    messageLogBufferCoolDown = 1000;
}

void loadTileProperties(const uint8_t levelNumber) {
    char buffer[64];
    struct StaticBuffer data;
    int16_t c;

    setLoggerDelegate(printMessageTo3DView);

    clearMap(&customMeshes);
    clearMap(&occluders);
    clearMap(&colliders);
    clearMap(&enemySightBlockers);
    sprintf (buffer, "props%d.bin", levelNumber);

    data = loadBinaryFileFromPath(buffer);
    loadPropertyList(&buffer[0], &tileProperties, &customMeshes);

    for (c = 0; c < 256; ++c) {
        struct CTile3DProperties *prop =
                (struct CTile3DProperties *) getFromMap(&tileProperties, c);

        if (prop) {

            if (prop->mBlockVisibility) {
                setInMap(&occluders, c, &occluders);
            }

            setInMap(&enemySightBlockers, c,
                     prop->mBlockEnemySight ? &enemySightBlockers : NULL);
        } else {
            setInMap(&occluders, c, NULL);
            setInMap(&enemySightBlockers, c, NULL);
        }
    }

    disposeDiskBuffer(data);
}

int* loadAnimation(char* filename) {
    struct StaticBuffer data;
    char *head;
    char *end;
    char *nameStart;
    char *buffer;
    int *ptr;
    int* anim;
    int count = 0;
    data = loadBinaryFileFromPath(filename);
    buffer = (char *) allocMem(data.size, GENERAL_MEMORY, 1);
    head = buffer;
    memCopyToFrom(head, (void *) data.data, data.size);
    end = head + data.size;
    disposeDiskBuffer(data);
    count = *head - '0';
    anim = allocMem( sizeof(int) * (count + 1), GENERAL_MEMORY, 1);
    *anim = count;
    head += 2;
    nameStart = head;
    ptr = anim + 1;
    while (head != end && (usedTexture < TOTAL_TEXTURES)) {
        char val = *head;
        if (val == '\n' || val == 0) {
            *head = 0;
            uint8_t index = usedTexture;
            makeTextureFrom(nameStart);
            *ptr = (index);
            ++ptr;
            
            nameStart = head + 1;
        }
        ++head;
    }

    disposeMem(buffer);
    return anim;
}

void loadTexturesForLevel(const uint8_t levelNumber) {
    char tilesFilename[64];
    struct StaticBuffer data;
    char *head;
    char *end;
    char *nameStart;
    char *buffer;

    sprintf (tilesFilename, "tiles%d.lst", levelNumber);

    data = loadBinaryFileFromPath(tilesFilename);
    buffer = (char *) allocMem(data.size, GENERAL_MEMORY, 1);
    head = buffer;
    memCopyToFrom(head, (void *) data.data, data.size);
    end = head + data.size;
    disposeDiskBuffer(data);

    nameStart = head;

    while (head != end && (usedTexture < TOTAL_TEXTURES)) {
        char val = *head;
        if (val == '\n' || val == 0) {
            *head = 0;
            
            if (!strcmp(".anm", head - 4)) {
                uint8_t key = usedTexture;
                int *anim = loadAnimation(nameStart);
                setInMap(&animations, key, anim );
                usedTexture++; /* We waste a slot for referencing the animation */
            } else {
                makeTextureFrom(nameStart);
            }
            nameStart = head + 1;
        }
        ++head;
    }

    disposeMem(buffer);

    /* tmp */
    playerHeight = 0;
    playerHeightChangeRate = 0;
}

void renderRoomTransition(void) {
    struct Vec3 center;

    dirtyLineY0 = 0;
    dirtyLineY1 = YRES_FRAMEBUFFER;

    if (!enableSmoothMovement) {
        currentPresentationState = kWaitingForInput;
        zCameraOffset = xCameraOffset = yCameraOffset = 0;
        needsToRedrawHUD = TRUE;

    } else {
        xCameraOffset = yCameraOffset = 0;

        fillRect(0, 0, XRES + 1, YRES, getPaletteEntry(0xFF000000), 0);

        center.mY = 0;
        center.mZ = intToFix(3);
        center.mX = -intToFix(3);
        drawColumnAt(center, intToFix(3), &textures[1], MASK_LEFT, 0, 1);

        center.mX = intToFix(3);
        drawColumnAt(center, intToFix(3), &textures[1], MASK_RIGHT, 0, 1);

        center.mY = intToFix(5) - zCameraOffset;
        center.mX = -intToFix(1);
        drawColumnAt(center, intToFix(3), &textures[0], MASK_FRONT, 0, 1);
        center.mX = intToFix(1);
        drawColumnAt(center, intToFix(3), &textures[0], MASK_FRONT, 0, 1);



        center.mX = intToFix(1);
        center.mY = intToFix(2) - zCameraOffset;
        center.mZ = intToFix(3);
        drawCeilingAt(center, &textures[0], kNorth);

        center.mX = -intToFix(1);
        center.mY = intToFix(2) - zCameraOffset;
        center.mZ = intToFix(3);
        drawCeilingAt(center, &textures[0], kNorth);

        zCameraOffset -= intToFix(1) / 4;

        if (zCameraOffset == 0) {
            currentPresentationState = kCheckingForRandomBattle;
            needsToRedrawHUD = TRUE;
        }
    }
}

void createRenderListFor(uint8_t cameraX, uint8_t cameraZ, enum EDirection rotation) {

    int8_t z, x;
    struct Vec2i mapCamera;
    mapCamera.x = cameraX;
    mapCamera.y = cameraZ;
    cameraDirection = rotation;
    hasSnapshot = TRUE;

    if (!enable3DRendering) {
        return;
    }

    if (abs(yCameraOffset) <= 1000) {
        yCameraOffset = 0;
    }

    if (yCameraOffset > 0) {
        yCameraOffset -= intToFix(1) / 2;
        needsToRedrawVisibleMeshes = TRUE;
    } else if (zCameraOffset > 0) {
        zCameraOffset -= intToFix(1) / 2;
        needsToRedrawVisibleMeshes = TRUE;
    } else if (zCameraOffset < 0) {
        zCameraOffset += intToFix(1) / 2;
        needsToRedrawVisibleMeshes = TRUE;
    } else if (xCameraOffset > 0) {
        xCameraOffset -= intToFix(1) / 2;
        needsToRedrawVisibleMeshes = TRUE;
    } else if (xCameraOffset < 0) {
        xCameraOffset += intToFix(1) / 2;
        needsToRedrawVisibleMeshes = TRUE;
    } else if (yCameraOffset < 0) {
        yCameraOffset += intToFix(1) / 2;
        needsToRedrawVisibleMeshes = TRUE;
    }

    cameraPosition = mapCamera;

    switch (cameraDirection) {
        case kNorth:
            mCamera.mX = intToFix(((MAP_SIZE - 1) * 2) - (2 * cameraPosition.x));
            mCamera.mZ = intToFix((2 * cameraPosition.y) - ((MAP_SIZE * 2) - 1));
            break;

        case kSouth:
            mCamera.mX = intToFix((2 * cameraPosition.x));
            mCamera.mZ = -intToFix(2 * cameraPosition.y - 1);
            break;

        case kWest:
            mCamera.mX = intToFix((2 * cameraPosition.y));
            mCamera.mZ = intToFix((2 * cameraPosition.x) - 1);
            break;

        case kEast:
            mCamera.mX = -intToFix((2 * cameraPosition.y));
            mCamera.mZ = intToFix(((MAP_SIZE * 2) - 1) - (2 * cameraPosition.x));
            break;
    }

#ifndef FIX16
    if ((cameraPosition.x + cameraPosition.y) & 1) {
        walkingBias = WALKING_BIAS;
    } else {
        walkingBias = 0;
    }
#else
    walkingBias = 0;
#endif

    ++gameTicks;
    
    if (visibilityCached) {
        return;
    }

    visibilityCached = TRUE;
    needsToRedrawVisibleMeshes = TRUE;

    
    castVisibility(cameraDirection, visMap, cameraPosition,
                   distances, TRUE, &occluders);
}

enum ECommand getInput(void) {
    const enum ECommand toReturn = mBufferedCommand;
    mBufferedCommand = kCommandNone;
    return toReturn;
}

void updateTextureCycle(long ms) {
    int c;
    struct CTile3DProperties *tileProp;
    for (c = 0; c < 255; ++c) {
        tileProp = ((struct CTile3DProperties *) getFromMap(&tileProperties,
                                                            c));
        if (tileProp != NULL) {
            if (tileProp->mFloorTexture.frameNumbers > 1) {
                int time = tileProp->mFloorTexture.frameTime - ms;
                if ( time < 0 ) {
                    tileProp->mFloorTexture.frameTime = tileProp->mFloorTexture.frameTime + time;
                    tileProp->mFloorTexture.currentFrame = (tileProp->mFloorTexture.currentFrame + 1) % tileProp->mFloorTexture.frameNumbers;
                    needsToRedrawVisibleMeshes = TRUE;
                } else {
                    tileProp->mFloorTexture.frameTime = time;
                }
            }

            if (tileProp->mCeilingTexture.frameNumbers > 1) {
                int time = tileProp->mCeilingTexture.frameTime - ms;
                if ( time < 0 ) {
                    tileProp->mCeilingTexture.frameTime = tileProp->mCeilingTexture.frameTime + time;
                    tileProp->mCeilingTexture.currentFrame = (tileProp->mCeilingTexture.currentFrame + 1) % tileProp->mCeilingTexture.frameNumbers;
                    needsToRedrawVisibleMeshes = TRUE;
                } else {
                    tileProp->mCeilingTexture.frameTime = time;
                }
            }

            if (tileProp->mMainWallTexture.frameNumbers > 1) {
                int time = tileProp->mMainWallTexture.frameTime - ms;
                if ( time < 0 ) {
                    tileProp->mMainWallTexture.frameTime = tileProp->mMainWallTexture.frameTime + time;
                    tileProp->mMainWallTexture.currentFrame = (tileProp->mMainWallTexture.currentFrame + 1) % tileProp->mMainWallTexture.frameNumbers;
                    needsToRedrawVisibleMeshes = TRUE;
                } else {
                    tileProp->mMainWallTexture.frameTime = time;
                }
            }

            if (tileProp->mFloorRepeatedTexture.frameNumbers > 1) {
                int time = tileProp->mFloorRepeatedTexture.frameTime - ms;
                if ( time < 0 ) {
                    tileProp->mFloorRepeatedTexture.frameTime = tileProp->mFloorRepeatedTexture.frameTime + time;
                    tileProp->mFloorRepeatedTexture.currentFrame = (tileProp->mFloorRepeatedTexture.currentFrame + 1) % tileProp->mFloorRepeatedTexture.frameNumbers;
                    needsToRedrawVisibleMeshes = TRUE;
                } else {
                    tileProp->mFloorRepeatedTexture.frameTime = time;
                }
            }

            if (tileProp->mCeilingRepeatedTexture.frameNumbers > 1) {
                int time = tileProp->mCeilingRepeatedTexture.frameTime - ms;
                if ( time < 0 ) {
                    tileProp->mCeilingRepeatedTexture.frameTime = tileProp->mCeilingRepeatedTexture.frameTime + time;
                    tileProp->mCeilingRepeatedTexture.currentFrame = (tileProp->mCeilingRepeatedTexture.currentFrame + 1) % tileProp->mCeilingRepeatedTexture.frameNumbers;
                    needsToRedrawVisibleMeshes = TRUE;
                } else {
                    tileProp->mCeilingRepeatedTexture.frameTime = time;
                }
            }
        }
    }
}

void renderTick(long ms) {
    dirtyLineY0 = 0;
    dirtyLineY1 = YRES_FRAMEBUFFER;

    if (messageLogBufferCoolDown > 0) {
        messageLogBufferCoolDown -= ms;
    }

    if (!enable3DRendering) {
        return;
    }

    if (!hasSnapshot) {
        return;
    }

    updateTextureCycle(ms);

    if (playerHeight < playerHeightTarget) {
        playerHeight += playerHeightChangeRate;
    }

    if (needsToRedrawVisibleMeshes) {
        uint8_t itemsSnapshotElement = 0xFF;
        struct Vec3 tmp, tmp2;
        struct CTile3DProperties *tileProp;
        FixP_t heightDiff;
        uint8_t lastElement = 0xFF;
        uint8_t element = 0;
        struct Vec3 position;
        FixP_t tileHeight = 0;
        FixP_t twiceFloorHeight;
        FixP_t twiceCeilingHeight;
        FixP_t floorRepetitions;
        FixP_t ceilingRepetitions;
        int32_t x, z;
        int distance;
        FixP_t cameraHeight;
        uint8_t facesMask;

        needsToRedrawVisibleMeshes = FALSE;
#ifdef SDLSW
        clearRenderer();
#endif
        element = LEVEL_MAP(cameraPosition.x, cameraPosition.y);

        tileProp = ((struct CTile3DProperties *) getFromMap(&tileProperties,
                                                            element));

        if (tileProp) {
            tileHeight = tileProp->mFloorHeight;
        }

        cameraHeight = -2 * tileHeight;

        mCamera.mY = cameraHeight - STANDARD_HEIGHT;

        for (distance = (MAP_SIZE + MAP_SIZE - 1); distance >= 0; --distance) {
            uint8_t bucketPos;

            for (bucketPos = 0; bucketPos < MAP_SIZE; ++bucketPos) {

                struct Vec2i visPos = distances[(distance * MAP_SIZE) + bucketPos];

                if (visPos.x < 0 || visPos.y < 0 || visPos.x >= MAP_SIZE
                    || visPos.y >= MAP_SIZE) {
                    bucketPos = MAP_SIZE;
                    continue;
                }

                facesMask = MASK_LEFT | MASK_FRONT | MASK_RIGHT;

                switch (cameraDirection) {

                    case kNorth:
                        x = visPos.x;
                        z = visPos.y;
                        element = LEVEL_MAP(x, z);

                        itemsSnapshotElement = ITEMS_IN_MAP(x, z);

                        position.mX =
                                mCamera.mX + -intToFix(2 * ((MAP_SIZE - 1) - x));
                        position.mY = mCamera.mY;
                        position.mZ =
                                mCamera.mZ + intToFix(2 * (MAP_SIZE) - (2 * z));

                        if (x > 0) {
                            facesMask |= (LEVEL_MAP(x - 1, z) != element) ?
                                         MASK_RIGHT :
                                         0;
                        }

                        /* remember, bounds - 1! */
                        if ((x < (MAP_SIZE - 1)) && (LEVEL_MAP(x + 1, z) == element)) {
                            facesMask &= ~MASK_LEFT;
                        }

                        if ((z < (MAP_SIZE - 1)) && (LEVEL_MAP(x, z + 1) == element)) {
                            facesMask &= ~MASK_FRONT;
                        }

                        if (z == cameraPosition.y - 1) {
                            if (getFromMap(&occluders, element)) {
                                facesMask &= ~MASK_FRONT;
                                facesMask |= MASK_BEHIND;
                            } else {
                                facesMask |= MASK_FRONT;
                            }
                        }

                        break;

                    case kSouth:
                        x = visPos.x;
                        z = visPos.y;

                        element = LEVEL_MAP(x, z);
                        itemsSnapshotElement = ITEMS_IN_MAP(x, z);

                        position.mX = mCamera.mX - intToFix(2 * x);
                        position.mY = mCamera.mY;
                        position.mZ = mCamera.mZ + intToFix(2 + 2 * z);

                        /*						remember, bounds - 1!*/

                        if ((x > 0) && (LEVEL_MAP(x - 1, z) == element)) {
                            facesMask &= ~MASK_LEFT;
                        }

                        if ((x < (MAP_SIZE - 1)) && (LEVEL_MAP(x + 1, z) == element)) {
                            facesMask &= ~MASK_RIGHT;
                        }

                        if (z == (cameraPosition.y) + 1) {

                            if (getFromMap(&occluders, element)) {
                                facesMask &= ~MASK_FRONT;
                                facesMask |= MASK_BEHIND;
                            } else {
                                facesMask |= MASK_FRONT;
                            }
                        }

                        break;
                    case kWest:
                        x = visPos.y;
                        z = visPos.x;

                        element = LEVEL_MAP(z, x);
                        itemsSnapshotElement = ITEMS_IN_MAP(z, x);

                        position.mX = mCamera.mX - intToFix(2 * x);
                        position.mY = mCamera.mY;
                        position.mZ = mCamera.mZ + intToFix(2 - 2 * z);

                        /* remember, bounds - 1! */

                        if ((x > 0) && (LEVEL_MAP(z, x - 1) == element)) {
                            facesMask &= ~MASK_LEFT;
                        }

                        if ((x < (MAP_SIZE - 1)) && (LEVEL_MAP(z, x + 1) == element)) {
                            facesMask &= ~MASK_RIGHT;
                        }

                        if ((z < (MAP_SIZE - 1)) && (LEVEL_MAP(z + 1, x) == element)) {
                            facesMask &= ~MASK_FRONT;
                        }

                        if (z == (cameraPosition.x) - 1) {

                            if (getFromMap(&occluders, element)) {
                                facesMask &= ~MASK_FRONT;
                                facesMask |= MASK_BEHIND;
                            } else {
                                facesMask |= MASK_FRONT;
                            }
                        }
                        break;

                    case kEast:
                        x = visPos.y;
                        z = visPos.x;

                        /* yes, it's reversed */
                        element = LEVEL_MAP(z, x);
                        itemsSnapshotElement = ITEMS_IN_MAP(z, x);

                        position.mX = mCamera.mX + intToFix(2 * x);
                        position.mY = mCamera.mY;
                        position.mZ = mCamera.mZ + intToFix(2 * (z - MAP_SIZE + 1));


                        /* remember, bounds - 1! */
                        if ((x > 0) && (LEVEL_MAP(z, x - 1) == element)) {
                            facesMask &= ~MASK_RIGHT;
                        }

                        if ((x < (MAP_SIZE - 1)) && (LEVEL_MAP(z, x + 1) == element)) {
                            facesMask &= ~MASK_LEFT;
                        }

                        if ((z < (MAP_SIZE - 1)) && (LEVEL_MAP(z - 1, x) == element)) {
                            facesMask &= ~MASK_FRONT;
                        }

                        if (z == (cameraPosition.x) + 1) {

                            if (getFromMap(&occluders, element)) {
                                facesMask &= ~MASK_FRONT;
                                facesMask |= MASK_BEHIND;
                            } else {
                                facesMask |= MASK_FRONT;
                            }
                        }
                        break;
                    default:
                        assert (FALSE);
                }

                if (lastElement != element) {
                    tileProp = (struct CTile3DProperties *) getFromMap(&tileProperties,
                                                                       element);
                    heightDiff = tileProp->mCeilingHeight - tileProp->mFloorHeight;
                    
                    twiceFloorHeight = tileProp->mFloorHeight * 2;
                    twiceCeilingHeight = tileProp->mCeilingHeight * 2;
                    floorRepetitions = intToFix(tileProp->mFloorRepetitions);
                    ceilingRepetitions = intToFix(tileProp->mCeilingRepetitions);
                }

                if (tileProp == NULL) {
                    continue;
                }

                lastElement = element;

                tmp.mX = tmp2.mX = position.mX;
                tmp.mZ = tmp2.mZ = position.mZ;

                if (tileProp->mFloorRepeatedTexture.frameNumbers
                    && tileProp->mFloorRepetitions > 0) {

                    tmp.mY = position.mY + (twiceFloorHeight - floorRepetitions);

                    switch (tileProp->mGeometryType) {
                        case kRightNearWall:
                            drawRightNear(
                                    tmp, intToFix(tileProp->mFloorRepetitions),
                                    &textures[tileProp->mFloorRepeatedTexture.frames[tileProp->mFloorRepeatedTexture.currentFrame]],
                                    facesMask, TRUE);

                            break;

                        case kLeftNearWall:
                            drawLeftNear(
                                    tmp, intToFix(tileProp->mFloorRepetitions),
                                    &textures[tileProp->mFloorRepeatedTexture.frames[tileProp->mFloorRepeatedTexture.currentFrame]], facesMask, TRUE);
                            break;

                        case kCube:
                        case kRampNorth:
                        case kRampEast:
                        default:
                            drawColumnAt(
                                    tmp, intToFix(tileProp->mFloorRepetitions),
                                    &textures[tileProp->mFloorRepeatedTexture.frames[tileProp->mFloorRepeatedTexture.currentFrame]],
                                    facesMask, FALSE, TRUE);
                            break;
                    }
                }

                if (tileProp->mCeilingRepeatedTexture.frameNumbers
                    && tileProp->mCeilingRepetitions > 0) {

                    tmp.mY = position.mY + (twiceCeilingHeight + ceilingRepetitions);

                    switch (tileProp->mGeometryType) {
                        case kRightNearWall:
                            drawRightNear(
                                    tmp, intToFix(tileProp->mCeilingRepetitions),
                                    &textures[tileProp->mCeilingRepeatedTexture.frames[tileProp->mCeilingRepeatedTexture.currentFrame]],
                                    facesMask, TRUE);
                            break;

                        case kLeftNearWall:
                            drawLeftNear(
                                    tmp, intToFix(tileProp->mCeilingRepetitions),
                                    &textures[tileProp->mCeilingRepeatedTexture.frames[tileProp->mCeilingRepeatedTexture.currentFrame]],
                                    facesMask, TRUE);
                            break;

                        case kCube:
                        case kRampNorth:
                        case kRampEast:
                        default:
                            drawColumnAt(
                                    tmp, intToFix(tileProp->mCeilingRepetitions),
                                    &textures[tileProp->mCeilingRepeatedTexture.frames[tileProp->mCeilingRepeatedTexture.currentFrame]],
                                    facesMask, FALSE, TRUE);
                            break;
                    }
                }

                if (tileProp->mFloorTexture.frameNumbers) {
                    tmp.mY = position.mY + twiceFloorHeight;
                    drawFloorAt(tmp, &textures[tileProp->mFloorTexture.frames[tileProp->mFloorTexture.currentFrame]], cameraDirection);
                }

                if (tileProp->mCeilingTexture.frameNumbers) {
                    enum EDirection newDirection = cameraDirection;

#ifndef FLOOR_TEXTURES_DONT_ROTATE

                    if (cameraDirection == kNorth) {
                        newDirection = kSouth;
                    }
                    if (cameraDirection == kSouth) {
                        newDirection = kNorth;
                    }
#endif
                    tmp.mY = position.mY + twiceCeilingHeight;

                    drawCeilingAt(
                            tmp, &textures[tileProp->mCeilingTexture.frames[tileProp->mCeilingTexture.currentFrame]], newDirection);
                }

                if (tileProp->mGeometryType != kNoGeometry
                    && tileProp->mMainWallTexture.frameNumbers) {

                    int integerPart = fixToInt(tileProp->mCeilingHeight)
                                      - fixToInt(tileProp->mFloorHeight);

                    FixP_t adjust = 0;

                    if (((heightDiff * 2) - intToFix(integerPart)) >= intToFix(1) / 2) {
                        adjust = ((intToFix(1) / 2) / (8));
                    }
                    
                    tmp.mY = position.mY + twiceFloorHeight + heightDiff;

                    switch (tileProp->mGeometryType) {
                        case kWallNorth:
                            switch (cameraDirection) {
                                case kNorth:
                                    facesMask = MASK_BEHIND;
                                    break;
                                case kWest:
                                    facesMask = MASK_FORCE_LEFT;
                                    break;
                                case kSouth:
                                    facesMask = MASK_FRONT;
                                    break;
                                case kEast:
                                    facesMask = MASK_FORCE_RIGHT;
                                    break;
                                default:
                                    facesMask = 0;
                                    break;
                            }

                            drawColumnAt(tmp, (heightDiff + adjust),
                                         &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                         facesMask, tileProp->mNeedsAlphaTest,
                                         tileProp->mRepeatMainTexture);
                            break;
                        case kWallWest:
                            switch (cameraDirection) {
                                case kNorth:
                                    facesMask = MASK_FORCE_RIGHT;
                                    break;
                                case kWest:
                                    facesMask = MASK_BEHIND;
                                    break;
                                case kSouth:
                                    facesMask = MASK_FORCE_LEFT;
                                    break;
                                case kEast:
                                    facesMask = MASK_FRONT;
                                    break;
                                default:
                                    facesMask = 0;
                                    break;
                            }

                            drawColumnAt(tmp, (heightDiff + adjust),
                                         &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                         facesMask, tileProp->mNeedsAlphaTest,
                                         tileProp->mRepeatMainTexture);
                            break;

                        case kWallCorner:
                            switch (cameraDirection) {
                                case kNorth:
                                    facesMask = MASK_BEHIND | MASK_FORCE_RIGHT;
                                    break;
                                case kWest:
                                    facesMask = MASK_FORCE_LEFT | MASK_BEHIND;
                                    break;
                                case kSouth:
                                    facesMask = MASK_FRONT | MASK_FORCE_LEFT;
                                    break;
                                case kEast:
                                    facesMask = MASK_FORCE_RIGHT | MASK_FRONT;
                                    break;
                                default:
                                    facesMask = 0;
                                    break;
                            }

                            drawColumnAt(tmp, (heightDiff + adjust),
                                         &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                         facesMask, tileProp->mNeedsAlphaTest,
                                         tileProp->mRepeatMainTexture);
                            break;

                        case kRightNearWall:
                            drawRightNear(
                                    tmp, (heightDiff + adjust),
                                    &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                    facesMask, tileProp->mRepeatMainTexture);
                            break;

                        case kLeftNearWall:
                            drawLeftNear(
                                    tmp, (heightDiff + adjust),
                                    &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                    facesMask, tileProp->mRepeatMainTexture);
                            break;

                        case kRampNorth: {
                            uint8_t flipTextureVertical = 0;
                            tmp.mY = position.mY + twiceFloorHeight;
                            tmp2.mY = position.mY + twiceCeilingHeight;

                            flipTextureVertical = (cameraDirection == kSouth || cameraDirection == kEast);

                            drawRampAt(tmp, tmp2, &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]], cameraDirection,
                                       flipTextureVertical);
                        }
                            break;

                        case kRampSouth: {
                            uint8_t flipTextureVertical = 0;
                            tmp.mY = position.mY + twiceCeilingHeight;
                            tmp2.mY = position.mY + twiceFloorHeight;

                            flipTextureVertical = (cameraDirection == kSouth || cameraDirection == kWest);

                            drawRampAt(tmp, tmp2, &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]], cameraDirection,
                                       flipTextureVertical);
                        }
                            break;

                        case kRampEast: {
                            uint8_t flipTextureVertical = 0;
                            tmp.mY = position.mY + twiceCeilingHeight;
                            tmp2.mY = position.mY + twiceFloorHeight;

                            flipTextureVertical = (cameraDirection == kSouth || cameraDirection == kEast);

                            drawRampAt(tmp, tmp2, &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                       (cameraDirection + 1) & 3, flipTextureVertical);
                        }
                            break;
                        case kRampWest: {
                            uint8_t flipTextureVertical = 0;
                            tmp.mY = position.mY + twiceCeilingHeight;
                            tmp2.mY = position.mY + twiceFloorHeight;

                            flipTextureVertical = (cameraDirection == kNorth || cameraDirection == kWest);

                            drawRampAt(tmp, tmp2, &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                       (cameraDirection + 3) & 3, flipTextureVertical);
                        }
                            break;
                        case kCube:
                            drawColumnAt(tmp, (heightDiff + adjust),
                                         &textures[tileProp->mMainWallTexture.frames[tileProp->mMainWallTexture.currentFrame]],
                                         facesMask, tileProp->mNeedsAlphaTest,
                                         tileProp->mRepeatMainTexture);
                        default:
                            break;
                    }
                }

                if (tileProp->mGeometryType >= kCustomMeshStart) {

                    tmp.mY = position.mY + twiceFloorHeight + heightDiff;

                    drawMesh((struct Mesh*)getFromMap(&customMeshes, tileProp->mGeometryType), tmp, cameraDirection);
                }

                if (itemsSnapshotElement != 0xFF) {
                    tmp.mY = position.mY + twiceFloorHeight + intToFix(1);

                    /* lazy loading the item sprites */
                    if (itemSprites[itemsSnapshotElement] == NULL) {
                        char buffer[64];
                        sprintf(&buffer[0], "%s.img", getItem(itemsSnapshotElement)->name);
                        itemSprites[itemsSnapshotElement] = loadBitmap(&buffer[0]);
                    }

                    drawBillboardAt(tmp, itemSprites[itemsSnapshotElement], intToFix(1), 32);
                }
            }
        }


        if (turnTarget == turnStep) {

            if (currentPresentationState == kRoomTransitioning) {
                messageLogBufferCoolDown = 0;
            }

            if (messageLogBufferCoolDown > 0) {
                int len = strlen(messageLogBuffer);
                int lines = 1;
                int chars = 0;
                int c;

                for (c = 0; c < len; ++c) {

                    ++chars;

                    if (chars == 27 || messageLogBuffer[c] == '\n') {
                        chars = 0;
                        ++lines;
                    }
                }

                fillRect(0, 0, XRES, (lines * 8), getPaletteEntry(0xFF000000), 1);

                drawTextAtWithMargin(0, 0, XRES, messageLogBuffer, getPaletteEntry(0xFFFFFFFF));
            }
        }

        clippingY1 = YRES_FRAMEBUFFER;

        dirtyLineY0 = 0;
        dirtyLineY1 = YRES_FRAMEBUFFER;

    }
}
