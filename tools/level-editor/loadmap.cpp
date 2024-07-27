#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "level-editor.h"

struct Map loadMap(const char* filename) {
    int sizeX = 32;
    int sizeY = 32;
    struct Map map;
    map.geometry = (uint32_t*)calloc(4, sizeX * sizeY );
    map.sizeX = sizeX;
    map.sizeY = sizeY;
    
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        for (int y = 0; y < map.sizeY; ++y) {
            uint8_t byte;
            for (int x = 0; x < map.sizeX; ++x) {
                fread(&byte, 1, 1, file);
                uint32_t flags = 0;
                switch(byte) {
                    case '<':
                        flags = ( HORIZONTAL_LINE + VERTICAL_LINE );
                        break;

                    case '.':
                        flags = 0;
                        break;

                    case '|':
                        flags = (VERTICAL_LINE );
                        break;

                    case '-':
                        flags = (HORIZONTAL_LINE );
                        break;

                    case '\\':
                        flags = (LEFT_FAR_LINE );
                        break;

                    case '/':
                        flags = (LEFT_NEAR_LINE );
                        break;
                    default:
                        puts("deu merda!");
                        putchar(byte);
                }

                setFlags( &map, x, y, flags);
            }
            // \n
            fread(&byte, 1, 1, file);
        }
        
        fclose(file);
    }
    
    return map;
}
