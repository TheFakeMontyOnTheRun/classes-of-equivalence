#ifndef LEVELEDITOR_H
#define LEVELEDITOR_H

#define CELL_VOID 32

#define HORIZONTAL_LINE 1

#define VERTICAL_LINE 2

#define LEFT_NEAR_LINE 4

#define LEFT_FAR_LINE 8

#define CELL_FLOOR 16


struct Map {
    int sizeX;
    int sizeY;
    uint32_t *geometry;
};

void setFlags(struct Map *map, int x, int y, uint32_t flags);

uint32_t getFlags(struct Map *map, int x, int y);

struct Map initMap(int sizeX, int sizeY);

void saveMap(const char *filename, struct Map *map);

struct Map loadMap(const char *filename);

void onClick(int buttonIndex, int buttonX, int buttonY, int width, int height);

void initMapEditor();

void onDrawLineClicked();

void onDrawSquareClicked();

void setLineWidth(uint8_t width);

void setColour(uint32_t colour);

void drawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);

void fillRect(uint32_t x0, uint32_t y0, uint32_t width, uint32_t height);

void redrawGrid(int width, int height);

#endif