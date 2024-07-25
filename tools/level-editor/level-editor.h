#ifndef LEVELEDITOR_H
#define LEVELEDITOR_H

#define HORIZONTAL_LINE 1

#define VERTICAL_LINE 2

#define LEFT_NEAR_LINE 4

#define LEFT_FAR_LINE 8


struct Map {
    int sizeX;
    int sizeY;
    uint32_t *geometry;
};

void setFlags(struct Map* map, int x, int y, uint32_t flags );

uint32_t getFlags(struct Map* map, int x, int y);

struct Map initMap(int sizeX, int sizeY);

void saveMap(const char* filename, struct Map* map);

struct Map loadMap(const char* filename);

#endif