#include <stdint.h>
#include <stdlib.h>

#include "level-editor.h"

struct Map initMap(int sizeX, int sizeY) {
    struct Map toReturn;
    
    toReturn.geometry = (uint32_t*)calloc(4, sizeX * sizeY );
    toReturn.sizeX = sizeX;
    toReturn.sizeY = sizeY;
    
    return toReturn;
}


void setFlags(struct Map* map, int x, int y, uint32_t flags ) {
    map->geometry[ (map->sizeX * y) + x ] = flags;
}

uint32_t getFlags(struct Map* map, int x, int y) {
    return map->geometry[ (map->sizeX * y) + x ];
}