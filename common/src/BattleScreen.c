#ifdef WIN32
#include "Win32Int.h"
#else
#include <stdint.h>
#endif
#include <string.h>

#include <string.h>
#include <stdlib.h>
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

enum EBattleAction {
    kAttack,
    kDefend,
    kSpecial,
    kRun
};

enum EBattleStates {
    kPlayerSelectingMoves,
    kAttackPhase
};

struct Bitmap *foe;
uint8_t currentCharacter;
#define kDummyBattleOptionsCount  4
#define TOTAL_MONSTER_COUNT 1
uint8_t monsterHP[TOTAL_MONSTER_COUNT];
uint8_t monsterType[TOTAL_MONSTER_COUNT];
uint8_t battleActions[TOTAL_MONSTER_COUNT + TOTAL_CHARACTERS_IN_PARTY];
int8_t battleDamages[TOTAL_CHARACTERS_IN_PARTY];

enum EBattleStates currentBattleState;
int8_t animationTimer;

const char *BattleScreen_options[kDummyBattleOptionsCount] = {
        "Attack", "Defend", "Special", "Run"};

void BattleScreen_initStateCallback(enum EGameMenuState tag) {
    (void)tag;
    cursorPosition = 1;
    needsToRedrawVisibleMeshes = 0;
    currentCharacter = 0;
    foe = loadBitmap("cop.img");

    for (int c = 0; c < TOTAL_MONSTER_COUNT; ++c) {
        monsterHP[c] = 20 + (rand() % 3);
        monsterType[c] = rand() % 15;
    }

    currentBattleState = kPlayerSelectingMoves;
}

void BattleScreen_repaintCallback(void) {
    char buffer[32];

    clearScreen();

    if (currentBattleState == kAttackPhase) {
        int damage = battleDamages[currentCharacter];

        if (animationTimer == 10) { /* only a single blink */
            if ( damage < 0 ) {
                fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF0000FF), 0);
            } else if ( damage > 0 ) {
                fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF00FF00), 0);
            } else {
                fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFFFFFFFF), 0);
            }
        }
    }



    needsToRedrawVisibleMeshes = 0;

    for (int c = 1; c < YRES_FRAMEBUFFER; c = c * 2) {
        drawLine(0, c + YRES_FRAMEBUFFER / 2, XRES_FRAMEBUFFER, c + YRES_FRAMEBUFFER / 2, getPaletteEntry(0xFFFF0000));
    }

    sprintf(&buffer[0], "H %d", monsterHP[0]);
    drawTextWindow(12, 1, 8, 3, "Robot", &buffer[0]);

    drawBitmap(14 * 7, (YRES_FRAMEBUFFER - foe->height) / 2, foe, 1);


    if (currentBattleState == kPlayerSelectingMoves) {
        drawWindowWithOptions(
                0,
                (YRES_FRAMEBUFFER / 8) - 9 - kDummyBattleOptionsCount,
                9 + 2,
                kDummyBattleOptionsCount + 2,
                party[currentCharacter].name,
                BattleScreen_options,
                kDummyBattleOptionsCount,
                cursorPosition);
    }

    for (int d = 0; d < TOTAL_CHARACTERS_IN_PARTY; d++) {
        if (party[d].inParty) {
            sprintf(&buffer[0], "H %d\nE %d", party[d].hp, party[d].energy);
            drawTextWindow( d * 7, (YRES_FRAMEBUFFER / 8) - 6, 6, 4, party[d].name, &buffer[0] );
        }
    }
}

enum EGameMenuState BattleScreen_tickCallback(enum ECommand cmd, void* data) {

    (void)data;

    if (currentBattleState == kAttackPhase) {

        if (animationTimer == 10) {
            if (currentCharacter < (TOTAL_CHARACTERS_IN_PARTY) && party[currentCharacter].inParty && party[currentCharacter].hp > 0 ) {
                if (battleDamages[currentCharacter] < 0) {
                    if ((-battleDamages[currentCharacter]) >= party[currentCharacter].hp) {
                        party[currentCharacter].hp = 0;
                    } else {
                        party[currentCharacter].hp += battleDamages[currentCharacter];
                    }

                } else {


                    if (battleDamages[currentCharacter] >= monsterHP[0] ) {
                        return kBackToGame;
                    } else {
                        monsterHP[0] -= battleDamages[currentCharacter];
                    }
                }
            }
        }

        animationTimer--;
        if (animationTimer < 0) {
            animationTimer = 10;

            currentCharacter++;

            if (currentCharacter == (TOTAL_CHARACTERS_IN_PARTY)) {
                currentCharacter = 0;
                currentBattleState = kPlayerSelectingMoves;
            }
        }
    }


    if (currentBattleState == kPlayerSelectingMoves) {

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

                battleActions[currentCharacter] = cursorPosition;

                currentCharacter++;

                /* skipping dead or absent characters */
                if (currentCharacter < (TOTAL_CHARACTERS_IN_PARTY ) && (!party[currentCharacter].inParty || party[currentCharacter].hp == 0)) {
                    currentCharacter++;
                }

                if (currentCharacter == (TOTAL_CHARACTERS_IN_PARTY)) {
                    currentCharacter = 0;
                    currentBattleState = kAttackPhase;

                    for (int c = 0; c < (TOTAL_CHARACTERS_IN_PARTY); ++c) {
                        battleDamages[c] = (rand() % 10) - 5;
                    }

                    animationTimer = 10;

                }


                break;

            default:
                break;
        }
    }

    return kResumeCurrentState;
}

void BattleScreen_unloadStateCallback(enum EGameMenuState newState) {
    (void)newState;
}
