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


extern struct Character party[4];
struct Bitmap *foe;
int currentCharacter;
#define kDummyBattleOptionsCount  4

const char *BattleScreen_options[kDummyBattleOptionsCount] = {
        "Attack", "Defend", "Special", "Run"};

void BattleScreen_initStateCallback(enum EGameMenuState tag) {
    (void)tag;
    cursorPosition = 1;
    needsToRedrawVisibleMeshes = 0;
    currentCharacter = 0;
    foe = loadBitmap("helmet.img");
}

void BattleScreen_repaintCallback(void) {

    if (firstFrameOnCurrentState) {
        clearScreen();
        needsToRedrawVisibleMeshes = 0;

        for (int c = 1; c < YRES_FRAMEBUFFER; c = c * 2) {
            drawLine(0, c + YRES_FRAMEBUFFER / 2, XRES_FRAMEBUFFER, c + YRES_FRAMEBUFFER / 2, getPaletteEntry(0xFFFF0000));
        }
    }

    drawTextWindow(12, 1, 8, 3, "Robot", "HP: 36");

    drawBitmap(0, 0, foe, 1);

    drawWindowWithOptions(
            0,
            (YRES_FRAMEBUFFER / 8) - 9 - kDummyBattleOptionsCount,
            9 + 2,
            kDummyBattleOptionsCount + 2,
            "Action",
            BattleScreen_options,
            kDummyBattleOptionsCount,
            cursorPosition);

    if (party[currentCharacter].inParty) {
        drawTextWindow(0, (YRES_FRAMEBUFFER / 8) - 6, 8, 5, party[currentCharacter].name, "HP: 50\nEN: 20" );
    }
}

enum EGameMenuState BattleScreen_tickCallback(enum ECommand cmd, void* data) {

    (void)data;

    switch (cmd) {
        case kCommandUp:
            if (cursorPosition > 0) {
                cursorPosition--;
                firstFrameOnCurrentState = 1;
            }
#ifdef PAGE_FLIP_ANIMATION
            turnTarget = turnStep;
#endif
            break;
        case kCommandDown:
            if (cursorPosition < kDummyBattleOptionsCount) {
                cursorPosition++;
                firstFrameOnCurrentState = 1;
            }
#ifdef PAGE_FLIP_ANIMATION
            turnTarget = turnStep;
#endif
            break;
        case kCommandBack:
            return kBackToGame;
        case kCommandFire1:
            firstFrameOnCurrentState = 1;
            currentCharacter++;

            if (currentCharacter == 5) {
                return kBackToGame;
            }
            break;

        default:
            break;
    }

    return kResumeCurrentState;
}

void BattleScreen_unloadStateCallback(enum EGameMenuState newState) {
    (void)newState;
}
