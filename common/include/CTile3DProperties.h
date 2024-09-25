#ifndef TILEPROPS_H
#define TILEPROPS_H

typedef uint8_t TextureIndex;

struct CTile3DProperties;

enum GeometryType {
    kNoGeometry,
    kCube,
    kLeftNearWall,
    kRightNearWall,
    kFloor,
    kRampNorth,
    kRampEast,
    kRampSouth,
    kRampWest,
    kWallNorth,
    kWallWest,
    kWallCorner,
    kCustomMeshStart
};

struct TextureInstance {
    TextureIndex* frames;
    uint8_t frameTime;
    uint8_t frameNumbers;
    uint8_t currentFrameTime;
    uint8_t currentFrame;
};

struct CTile3DProperties {
    int mNeedsAlphaTest;
    int mBlockVisibility;
    int mBlockMovement;
    int mBlockEnemySight;
    int mRepeatMainTexture;
    struct TextureInstance mCeilingTexture;
    struct TextureInstance mFloorTexture;
    struct TextureInstance mMainWallTexture;
    uint8_t mGeometryType;
    struct TextureInstance mCeilingRepeatedTexture;
    struct TextureInstance mFloorRepeatedTexture;
    uint8_t mCeilingRepetitions;
    uint8_t mFloorRepetitions;
    FixP_t mCeilingHeight;
    FixP_t mFloorHeight;
};

void loadPropertyList(const char *propertyFile, struct MapWithCharKey *map, struct MapWithCharKey *meshes);

#endif
