#include <stdint.h>
#include <stdlib.h>

#include "level-editor.h"

struct Map initMap(int sizeX, int sizeY) {
    struct Map toReturn;
    
    toReturn.geometry = (uint32_t*)calloc(4, sizeX * sizeY );
    toReturn.sizeX = sizeX;
    toReturn.sizeY = sizeY;

    for (int _y = 0; _y < sizeY; ++_y) {
        for (int _x = 0; _x < sizeX; ++_x) {
            setFlags(&toReturn, _x, _y, CELL_VOID);
        }
    }


    return toReturn;
}


void setFlags(struct Map* map, int x, int y, uint32_t flags ) {
    map->geometry[ (map->sizeX * y) + x ] = flags;
}

uint32_t getFlags(struct Map* map, int x, int y) {
    return map->geometry[ (map->sizeX * y) + x ];
}