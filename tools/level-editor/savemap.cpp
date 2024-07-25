#include <stdint.h>
#include <stdio.h>

#include "level-editor.h"

void saveMap(const char* filename, struct Map* map) {
    FILE *file = fopen(filename, "w");
    if (file != NULL) {
        for (int y = 0; y < map->sizeY; ++y) {
            uint8_t byte;
            for (int x = 0; x < map->sizeX; ++x) {
                byte = 'a' + getFlags(map, x, y);
                fwrite(&byte, 1, 1, file);
            }
            byte = '\n';
            fwrite(&byte, 1, 1, file);
        }
        
        fclose(file);
    }
}
