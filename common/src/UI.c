/*
* Created by Daniel Monteiro on 2019-08-02.
*/
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include "Win32Int.h"
#else

#include <stdint.h>
#include <unistd.h>

#endif

#include "Enums.h"
#include "Common.h"
#include "Core.h"
#include "FixP.h"
#include "Vec.h"
#include "LoadBitmap.h"
#include "CActor.h"
#include "Mesh.h"
#include "Renderer.h"
#include "UI.h"
#include "SoundSystem.h"


int pointerInsideRect (int x, int y, int dx, int dy) {
    int offsettedX = pointerClickPositionX - (x / 8);
    int offsettedY = pointerClickPositionY - (y / 8);

    if (pointerClickPositionX == -1) {
        return 0;
    }

    if (offsettedX >= 0 && offsettedY >= 0 && offsettedX < (dx / 8)  && offsettedY < (dy / 8)) {
        return 1;
    } else {
        return 0;
    }
}

void
drawWindowWithOptions(const int x,
                      const int y,
                      const unsigned int dx,
                      const unsigned int dy,
                      const char *title,
                      const char **options,
                      uint8_t optionsCount,
                      uint8_t selectedOption) {
    int c;

    drawWindow(x,
               y,
               dx,
               dy,
               title);

    for (c = 0; c < optionsCount; ++c) {

        int isCursor = (selectedOption == c);

        if (isCursor) {
            fillRect((x) * 8 + 1,
                     (y + 2 + c) * 8,
                     dx * 8 - 1,
                     8,
                     getPaletteEntry(0xFFFFFFFF),
                     FALSE);
        }

        drawTextAt(x + 1,
                   y + 2 + c,
                   &options[c][0],
                   isCursor ? getPaletteEntry(0xFF000000) : getPaletteEntry(0xFFFFFFFF));
    }
}

void
drawWindow(const int x, const int y, const unsigned int dx, const unsigned int dy, const char *title) {

    /* background */
    fillRect((x) * 8, (y) * 8, dx * 8, dy * 8, getPaletteEntry(0xFF000000), FALSE);
    /* frame */
    drawRect((x) * 8, (y) * 8, dx * 8, dy * 8, getPaletteEntry(0xFFFFFFFF));
    /* title tab */
    fillRect((x) * 8, (y) * 8, dx * 8, 8, getPaletteEntry(0xFFFFFFFF), FALSE);

    /* title text */
    if (title != NULL) {
        drawTextAt(x + 1, y, title, getPaletteEntry(0xFF000000));
    }
}

void
drawTextWindow(const int x, const int y, const unsigned int dx, const unsigned int dy, const char *title,
               const char *content) {
    drawWindow(x, y, dx, dy, title);
    drawTextAtWithMargin( x + 1, y + 2, ( 1 + x + dx) * 8, content, getPaletteEntry(0xFFFFFFFF));
}

enum EGameMenuState handleCursor(const int x,
                                 const int y,
                                 const unsigned int dx,
                                 const unsigned int dy,
                                 const char **optionsStr,
                                 const enum EGameMenuState* options,
                                 uint8_t optionsCount, const enum ECommand cmd, enum
                                 EGameMenuState backState) {
    int c;
    for (c = 0; c < optionsCount; ++c) {
//        size_t len = strlen(&optionsStr[c][0]);

        if (pointerInsideRect((x + 1) * 8, (y + 2 + c) * 8, dx * 8, 8)) {
            if (c == cursorPosition) {
                if (options == NULL) {
                    return c;
                } else {
                    return options[c];
                }
            } else {
                cursorPosition = c;
            }
        }
    }
    
    if ( pointerClickPositionX != -1 && !pointerInsideRect(x * 8, y * 8, dx * 8, dy * 8)) {
        return backState;
    }

    switch (cmd) {
        case kCommandBack:
            return backState;
        case kCommandUp:
            playSound(2);
            --cursorPosition;
            break;
        case kCommandDown:
            playSound(2);
            ++cursorPosition;
            break;
        case kCommandFire1:
        case kCommandFire2:
        case kCommandFire3:
            if (options == NULL) {
                return cursorPosition;
            } else {
                return options[cursorPosition];
            }
    }

    if (cursorPosition >= optionsCount) {
        cursorPosition = optionsCount - 1;
    }

    if (cursorPosition < 0) {
        cursorPosition = 0;
    }

    return kResumeCurrentState;
}

void drawGraphic(uint16_t x, uint8_t  y, uint16_t dx, uint8_t dy, const uint8_t *graphic) {
    const uint8_t *ptr = graphic;
    int buffer[6];

    while (*ptr) {
        uint8_t c;
        const uint8_t npoints = *ptr++;
        const uint8_t r = *ptr++;
        const uint8_t g = *ptr++;
        const uint8_t b = *ptr++;
        const FramebufferPixelFormat colour = getPaletteEntry( 0xFF000000 + (b << 16) + (g << 8) + r);
        const uint8_t *shape = ptr;
        int centerX = 0;
        int centerY = 0;

        if (npoints > 3) {
            ptr += 2 * npoints;
        } else if (npoints == 3) {
  	  buffer[0] = x + ((dx * shape[0]) / 128);
	  buffer[1] = y + ((dy * shape[1]) / 128);
	  buffer[2] = x + ((dx * shape[2]) / 128);
	  buffer[3] = y + ((dy * shape[3]) / 128);
	  buffer[4] = x + ((dx * shape[4]) / 128);
	  buffer[5] = y + ((dy * shape[5]) / 128);

            fillTriangle(&buffer[0], colour, getPaletteEntry(0xFFFFFFFF));

            ptr += 2 * npoints;
        } else if (npoints == 2) {
	  drawLine(x + ((dx * shape[0]) / 128),
		   y + ((dy * shape[1]) / 128),
		   x + ((dx * shape[2]) / 128),
		   y + ((dy * shape[3]) / 128),
		     colour);
            ptr += 2 * npoints;
        } else if (npoints == 1) {
	  fillRect(x + ((dx * shape[0]) / 128),
		   y + ((dy * shape[1]) / 128),
		   1,
		   1,
		   colour,
		   FALSE);
            ptr += 2 * npoints;
        }
    }
}

void clearScreen(void) {
    fillRect( 0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF000000), FALSE);
}
