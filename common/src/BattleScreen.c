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
#define TOTAL_MONSTER_COUNT 4
uint8_t monsterHP[TOTAL_MONSTER_COUNT];
uint8_t monsterType[TOTAL_MONSTER_COUNT];
uint8_t battleActions[TOTAL_MONSTER_COUNT + TOTAL_CHARACTERS_IN_PARTY];
uint8_t battleDamages[TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT];
uint8_t battleTargets[TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT];

enum EBattleStates currentBattleState;
int8_t animationTimer;
uint8_t monstersPresent = 0;
uint8_t aliveMonsters = 0;
uint8_t aliveHeroes = 0;

const char *BattleScreen_options[kDummyBattleOptionsCount] = {
        "Attack", "Defend", "Special", "Run"};

void BattleScreen_initStateCallback(enum EGameMenuState tag) {
    (void)tag;
    cursorPosition = 1;
    needsToRedrawVisibleMeshes = 0;
    currentCharacter = 0;
    foe = loadBitmap("cop.img");

    aliveMonsters = monstersPresent = 1 + (rand() % (TOTAL_MONSTER_COUNT - 1));

    for (int c = 0; c < aliveMonsters; ++c) {
        monsterHP[c] = 20 + (rand() % 3);
        monsterType[c] = rand() % 15;
    }

    aliveHeroes = 0;
    for (int d = 0; d < TOTAL_CHARACTERS_IN_PARTY; d++) {
        if (party[d].inParty && party[d].hp > 0) {
            ++aliveHeroes;
        }
    }

    currentBattleState = kPlayerSelectingMoves;
}

void BattleScreen_repaintCallback(void) {
    char buffer[2][32];
    int c;
    clearScreen();

    if (currentBattleState == kAttackPhase) {
        if (animationTimer == 10) { /* only a single blink */
            int damage = battleDamages[currentCharacter];

            if (currentCharacter >= TOTAL_CHARACTERS_IN_PARTY) {
                if (monsterHP[currentCharacter - TOTAL_CHARACTERS_IN_PARTY] == 0) {
                    goto done_flashing;
                }
            } else {
                if (!party[currentCharacter].inParty || party[currentCharacter].hp == 0) {
                    goto done_flashing;
                }
            }

            if ( damage > 0 ) {
                fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF0000FF), 0);
            } else if ( damage < 0 ) {
                fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF00FF00), 0);
            } else {
                fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFFFFFFFF), 0);
            }
        }
    }

    done_flashing:

    needsToRedrawVisibleMeshes = 0;

    for (c = 1; c < YRES_FRAMEBUFFER; c = c * 2) {
        drawLine(0, c + YRES_FRAMEBUFFER / 2, XRES_FRAMEBUFFER, c + YRES_FRAMEBUFFER / 2, getPaletteEntry(0xFFFF0000));
    }

    /*
     This is seriously weird, but there is method to my madness. I need to append strings and this seemed like the most
     economical way to do so. I alternate using buffers on sprintf (since I don't think it's wise to feed the same
     buffer as src and dst).
    */
    sprintf(&buffer[0][0], "");
    sprintf(&buffer[1][0], "");

    for (c = 0; c < monstersPresent; c++) {
        /*
         to make it even weider, we have (monstersPresent - c - 1). This is required, since otherwise, we would print
         monster HPs from bottom to top.
         */
        sprintf(&buffer[ c & 1 ? 0 : 1 ][0], "H %d\n%s", monsterHP[(monstersPresent - c - 1)], &buffer[c & 1 ? 1 : 0][0]);
    }

    /*
     It might happen that if we have an odd number of monsters, the "final" version of the buffer ends on buffer[1],
     so I alternate once more, to ensure the desired result is in buffer[0]
     */
    if (((monstersPresent - 1) & 1) == 0) {
        sprintf(&buffer[ c & 1 ? 0 : 1][0], "%s", &buffer[c & 1 ? 1 : 0][0]);
    }

    drawTextWindow((XRES_FRAMEBUFFER / 8) - 9, 1, 6, 2 + monstersPresent, "Robot", &buffer[0][0]);

    drawBitmap(13 * 8, - 16 + (YRES_FRAMEBUFFER - foe->height) / 2, foe, 1);


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

    for (c = 0; c < TOTAL_CHARACTERS_IN_PARTY; c++) {
        if (party[c].inParty) {
            sprintf(&buffer[0][0], "H %d\nE %d", party[c].hp, party[c].energy);
            drawTextWindow( c * 7, (YRES_FRAMEBUFFER / 8) - 6, 6, 4, party[c].name, &buffer[0] );
        }
    }
}

enum EGameMenuState BattleScreen_tickCallback(enum ECommand cmd, void* data) {

    (void)data;

    if (currentBattleState == kAttackPhase) {

        if (animationTimer == 10) {

            /* Monsters */
            if (currentCharacter >= TOTAL_CHARACTERS_IN_PARTY) {
                int monsterIndex = currentCharacter - TOTAL_CHARACTERS_IN_PARTY;
                if (monsterHP[monsterIndex] > 0 ) {
                    if (battleDamages[monsterIndex] >= monsterHP[monsterIndex] ) {
                        monsterHP[monsterIndex] = 0;
                        aliveMonsters--;
                    } else {
                        monsterHP[monsterIndex] -= battleDamages[monsterIndex];
                    }
                } else {
                    /* Since the monster is dead or not present, this will advance the animation faster */
                    animationTimer = 0;
                }

            }  else {
                if (currentCharacter < (TOTAL_CHARACTERS_IN_PARTY) && party[currentCharacter].inParty && party[currentCharacter].hp > 0 ) {
                    if ((battleDamages[currentCharacter]) >= party[currentCharacter].hp) {
                        party[currentCharacter].hp = 0;
                        aliveHeroes--;
                    } else {
                        party[currentCharacter].hp -= battleDamages[currentCharacter];
                    }
                } else {
                    /* Since the hero is dead or not present, this will advance the animation faster */
                    animationTimer = 0;
                }
            }
        }

        animationTimer--;
        if (animationTimer < 0) {
            animationTimer = 10;

            currentCharacter++;

            if (currentCharacter == (TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT)) {
                currentCharacter = 0;
                currentBattleState = kPlayerSelectingMoves;

                if (aliveHeroes == 0) {
                    return kBadGameOverEpilogue;
                }

                if (aliveMonsters == 0) {
                    return kBackToGame;
                }
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
                    int c;
                    currentCharacter = 0;
                    currentBattleState = kAttackPhase;

                    for (c = 0; c < (TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT); ++c) {
                        battleDamages[c] = (rand() % 10);
                    }


                    for (c = 0; c < (TOTAL_MONSTER_COUNT); ++c) {
                        battleTargets[c] = (rand() % aliveHeroes);
                    }


                    for (c = 0; c < (TOTAL_CHARACTERS_IN_PARTY); ++c) {
                        battleTargets[c] = (rand() % aliveMonsters);
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
