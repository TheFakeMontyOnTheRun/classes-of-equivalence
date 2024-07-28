#include <stdint.h>
#include <stdio.h>

#include "level-editor.h"

void saveMap(const char* filename, struct Map* map) {
    FILE *file = fopen(filename, "w");
    if (file != NULL) {
        for (int y = 0; y < map->sizeY; ++y) {
            uint8_t byte;
            for (int x = 0; x < map->sizeX; ++x) {
                uint32_t  flags = getFlags(map, x, y);

                if( (flags & ( HORIZONTAL_LINE + VERTICAL_LINE ) ) == (HORIZONTAL_LINE + VERTICAL_LINE) ) {
                    byte = '<';
                } else if( (flags & (LEFT_FAR_LINE ) ) == (LEFT_FAR_LINE ) ) {
                    byte = '\\';
                } else if( (flags & (LEFT_NEAR_LINE ) ) == (LEFT_NEAR_LINE ) ) {
                    byte = '/';
                } else if( (flags & (HORIZONTAL_LINE + VERTICAL_LINE ) ) == (HORIZONTAL_LINE ) ) {
                    byte = '-';
                } else if( (flags & (HORIZONTAL_LINE + VERTICAL_LINE ) ) == ( VERTICAL_LINE ) ) {
                    byte = '|';
                } else if( (flags & (CELL_FLOOR ) ) == ( CELL_FLOOR ) ) {
                    byte = '.';
                } else {
                    byte = ',';
                }



                fwrite(&byte, 1, 1, file);
            }
            byte = '\n';
            fwrite(&byte, 1, 1, file);
        }
        
        fclose(file);
    }
}
