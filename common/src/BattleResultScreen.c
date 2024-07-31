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

int killsRequiredToAchieveLevel(int level) {
    if (level <= 0) {
        return 0;
    }
    return (2 * level) + killsRequiredToAchieveLevel(level - 1);
}

void BattleResultScreen_repaintCallback(void) {
    int line = 3;
    int c;

    clearScreen();


    drawTextAt(1, 2, "Victory!", getPaletteEntry(0xFF999999));
    for (c = 0; c < TOTAL_CHARACTERS_IN_PARTY; ++c ) {
        if (party[c].inParty && party[c].hp > 0) {
            char buffer[32];
            int killsRequired = killsRequiredToAchieveLevel(party[c].level + 1);
            sprintf(&buffer[0], "%s - %d %s %s",
                    party[c].name,
                    party[c].kills,
                    party[c].kills == 1 ? "kill" : "kills",
                    killsRequired <= party[c].kills ? "LEVEL UP!" : ""
                    );
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
    int c;
    (void)newState;

    for (c = 0; c < TOTAL_CHARACTERS_IN_PARTY; ++c ) {
        if (party[c].inParty && party[c].hp > 0) {
            int killsRequired = killsRequiredToAchieveLevel(party[c].level + 1);
            if (killsRequired <= party[c].kills) {
                ++party[c].level;
            }
        }
    }

}
