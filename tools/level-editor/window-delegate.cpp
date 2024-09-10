#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "level-editor.h"

#define GRID_SIZE 32

enum EDrawingMode {
    kPicker,
    kLine,
    kSquare
};

int pivotX, pivotY;
int cursorX, cursorY;

struct Map map;
enum EDrawingMode drawingMode = kPicker;

void initMapEditor() {
    map = initMap(32, 32);
}

void onDrawLineClicked() {
    drawingMode = kLine;
    pivotX = pivotY = -1;
}

void onDrawSquareClicked() {
    drawingMode = kSquare;
    pivotX = pivotY = -1;
}


void redrawGrid(int width, int height) {

    
        setColour(0x000000);
        setLineWidth(1);

        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                uint32_t flags = getFlags(&map, x, y);

                if ((flags & CELL_VOID) == CELL_VOID) {
                    setColour(0x555555);

                    fillRect(   x * (width / GRID_SIZE),
                                   y * (height / GRID_SIZE),
                                   (width / GRID_SIZE),
                                   (height / GRID_SIZE)
                    );
                } else if ((flags & CELL_FLOOR) == CELL_FLOOR) {
                    setColour(0xFFFFFFFF);

                    fillRect(x * (width / GRID_SIZE),
                                   y * (height / GRID_SIZE),
                                   (width / GRID_SIZE),
                                   (height / GRID_SIZE)
                    );

                }
            }
        }

        setColour(0x000000);

        for (int y = 1; y <= GRID_SIZE; ++y) {
            drawLine(0, y * (height / GRID_SIZE), width, y * (height / GRID_SIZE));
        }

        for (int x = 1; x <= GRID_SIZE; ++x) {
            drawLine(x * (width / GRID_SIZE), 0, x * (width / GRID_SIZE), height);
        }


        for (int y = 0; y < GRID_SIZE; ++y) {
            for (int x = 0; x < GRID_SIZE; ++x) {
                uint32_t flags = getFlags(&map, x, y);
                setLineWidth(4);

                if ((flags & HORIZONTAL_LINE) == HORIZONTAL_LINE) {
                    drawLine(x * (width / GRID_SIZE),
                              y * (height / GRID_SIZE),
                              (x + 1) * (width / GRID_SIZE),
                              y * (height / GRID_SIZE)
                    );
                }

                if ((flags & VERTICAL_LINE) == VERTICAL_LINE) {
                    drawLine(x * (width / GRID_SIZE),
                              y * (height / GRID_SIZE),
                              x * (width / GRID_SIZE),
                              (y + 1) * (height / GRID_SIZE)
                    );
                }

                if ((flags & LEFT_NEAR_LINE) == LEFT_NEAR_LINE) {
                    drawLine((x + 1) * (width / GRID_SIZE),
                              y * (height / GRID_SIZE),
                              (x) * (width / GRID_SIZE),
                              (y + 1) * (height / GRID_SIZE)
                    );
                }

                if ((flags & LEFT_FAR_LINE) == LEFT_FAR_LINE) {
                    drawLine( x * (width / GRID_SIZE),
                              y * (height / GRID_SIZE),
                              (x + 1) * (width / GRID_SIZE),
                              (y + 1) * (height / GRID_SIZE)
                    );
                }
                setLineWidth(1);
            }
        }
}

void onClick(int buttonIndex, int buttonX, int buttonY, int width, int height) {
    
    int grid_width = width / GRID_SIZE;
    int grid_height = height / GRID_SIZE;
    
    int x = buttonX / grid_width;
    int y = buttonY / grid_height;
    
    int x_offset = buttonX % grid_width;
    int y_offset = buttonY % grid_height;
    
    
    
    switch (drawingMode) {
        case kLine: {
            if (pivotX == -1) {
                pivotX = x;
                pivotY = y;
                printf("Pivot at: %d, %d\n", pivotX, pivotY);
            } else {
                
                if (x == pivotX) {
                    int y0 = (pivotY < y) ? pivotY : y;
                    int y1 = (pivotY < y) ? y : pivotY;
                    for (int _y = y0; _y <= y1; ++_y) {
                        printf("filling %d, %d\n", pivotX, _y);
                        uint32_t flag = getFlags(&map, pivotX, _y);
                        flag |= VERTICAL_LINE;
                        setFlags(&map, pivotX, _y, flag);
                    }
                } else if (y == pivotY) {
                    int x0 = (pivotX < x) ? pivotX : x;
                    int x1 = (pivotX < x) ? x : pivotX;
                    
                    for (int _x = x0; _x <= x1; ++_x) {
                        printf("filling %d, %d\n", _x, pivotY);
                        uint32_t flag = getFlags(&map, _x, pivotY);
                        flag |= HORIZONTAL_LINE;
                        setFlags(&map, _x, pivotY, flag);
                    }
                }
                
                
                pivotX = pivotY = -1;
                drawingMode = kPicker;
            }
        }
            break;
        case kSquare : {
            if (pivotX == -1) {
                pivotX = x;
                pivotY = y;
                printf("Pivot at: %d, %d\n", pivotX, pivotY);
            } else {
                int y0 = (pivotY < y) ? pivotY : y;
                int y1 = (pivotY < y) ? y : pivotY;
                int x0 = (pivotX < x) ? pivotX : x;
                int x1 = (pivotX < x) ? x : pivotX;
                
                uint32_t flag = buttonIndex == 1 ? CELL_FLOOR : CELL_VOID;
                
                for (int _y = y0; _y <= y1; ++_y) {
                    for (int _x = x0; _x <= x1; ++_x) {
                        printf("filling %d, %d\n", _x, _y);
                        setFlags(&map, _x, _y, flag);
                    }
                }
                
                pivotX = pivotY = -1;
                drawingMode = kPicker;
            }
        }
            break;
        case kPicker: {
            uint32_t flags;
            if (buttonIndex == 1) {
                flags = getFlags(&map, x, y);
                
                if (y_offset <= grid_height / 2) {
                    if (x_offset <= grid_width / 2) {
                        if ((flags & VERTICAL_LINE) == VERTICAL_LINE) {
                            flags &= ~VERTICAL_LINE;
                        } else {
                            flags |= VERTICAL_LINE;
                        }
                    } else {
                        if ((flags & HORIZONTAL_LINE) == HORIZONTAL_LINE) {
                            flags &= ~HORIZONTAL_LINE;
                        } else {
                            flags |= HORIZONTAL_LINE;
                        }
                    }
                } else {
                    if (x_offset <= grid_width / 2) {
                        if ((flags & LEFT_NEAR_LINE) == LEFT_NEAR_LINE) {
                            flags &= ~LEFT_NEAR_LINE;
                        } else {
                            flags |= LEFT_NEAR_LINE;
                        }
                    } else {
                        if ((flags & LEFT_FAR_LINE) == LEFT_FAR_LINE) {
                            flags &= ~LEFT_FAR_LINE;
                        } else {
                            flags |= LEFT_FAR_LINE;
                        }
                    }
                }
            } else {
                flags = getFlags(&map, x, y);
                if ((flags & CELL_FLOOR) == CELL_FLOOR) {
                    flags &= ~CELL_FLOOR;
                    flags |= CELL_VOID;
                } else {
                    flags = CELL_VOID;
                }
            }
            setFlags(&map, x, y, flags);
        }
            break;
    }
}
