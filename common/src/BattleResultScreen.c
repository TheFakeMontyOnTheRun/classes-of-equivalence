#ifdef WIN32
#include "Win32Int.h"
#else
#include <stdint.h>
#endif

#include <string.h>
#include <stdio.h>

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
#include "Dungeon.h"


void BattleResultScreen_initStateCallback(enum EGameMenuState tag) {
    (void)tag;
    cursorPosition = 1;
}

void BattleResultScreen_repaintCallback(void) {
    int line = 3;


    clearScreen();


    drawTextAt(1, 2, "Victory!", getPaletteEntry(0xFF999999));
    for (int c = 0; c < TOTAL_CHARACTERS_IN_PARTY; ++c ) {
        if (party[c].inParty && party[c].hp > 0) {
            char buffer[32];
            sprintf(&buffer[0], "%s - %d %s", party[c].name, party[c].kills, party[c].kills == 1 ? "kill" : "kills" );
            drawTextAt(1, line, &buffer[0], getPaletteEntry(0xFF999999));
            ++line;
        }
        
    }
    
    drawTextAt( ((XRES_FRAMEBUFFER / 8) - 21) / 2 ,
                 (YRES_FRAMEBUFFER / 8) - 2,
                 "Press key to continue",
                 getPaletteEntry(0xFF999999));
}

enum EGameMenuState BattleResultScreen_tickCallback(enum ECommand cmd, void* data) {

    (void)data;

    switch (cmd) {

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
