#ifdef WIN32
#include "Win32Int.h"
#else
#include <stdint.h>
#endif
#include <string.h>

#include <string.h>

#include "Common.h"
#include "Enums.h"
#include "FixP.h"
#include "Vec.h"
#include "Engine.h"
#include "CActor.h"
#include "Mesh.h"
#include "Renderer.h"
#include "UI.h"
#include "Core.h"
#include "Derelict.h"
#include "SoundSystem.h"


void BattleResultScreen_initStateCallback(enum EGameMenuState tag) {
    (void)tag;
    cursorPosition = 1;
}

void BattleResultScreen_repaintCallback(void) {
    uint8_t pin;
    uint8_t holdingDisk;


    clearScreen();


    drawTextAt(1, 2, "Pointer:", getPaletteEntry(0xFF999999));
}

enum EGameMenuState BattleResultScreen_tickCallback(enum ECommand cmd, void* data) {

    uint8_t holdingDisk = getHoldingDisk();
    (void)data;

    switch (cmd) {
        case kCommandUp:
            if (cursorPosition > 0) {
                cursorPosition--;
                firstFrameOnCurrentState = 1;
            }
            break;
        case kCommandDown:
            if (cursorPosition < 2) {
                cursorPosition++;
                firstFrameOnCurrentState = 1;
            }
            break;
        case kCommandFire1:
	  initRoom(getPlayerRoom());
	  return kBackToGame;

        default:
            break;
    }

    return kResumeCurrentState;
}

void BattleResultScreen_unloadStateCallback(enum EGameMenuState newState) {
    (void)newState;
}
