#ifdef WIN32
#include "Win32Int.h"
#else
#include <stdint.h>
#endif

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
#include "Dungeon.h"

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

#define kDummyBattleOptionsCount  4
#define TOTAL_MONSTER_COUNT 3
#define kBattleAnimationInterval 900
#define TOTAL_MONSTER_TYPES 27
#define TOTAL_FRAMES_PER_MONSTER 2
#define TOTAL_SPLAT_FRAMES 3

struct Bitmap *foe[TOTAL_MONSTER_COUNT][TOTAL_FRAMES_PER_MONSTER];
struct Bitmap *splat[TOTAL_SPLAT_FRAMES];

uint8_t currentCharacter;

struct MonsterArchetype {
    const char* name;
    uint8_t defense: 4;
    uint8_t attack: 4;
    uint8_t agility: 4;
    uint8_t wisdom: 4;
};

static const struct MonsterArchetype monsterArchetypes[TOTAL_MONSTER_TYPES] = {
  {"sgt1", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt2", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt1", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt2", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt1", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt2", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt1", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt2", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"cmdr", 1, 3, 2, 1},
  {"sgt1", 2, 3, 4, 5},
  {"ward", 3, 3, 2, 6},
  {"loose seal", 3, 3, 2, 6},
};

/* Life points of the monsters */
uint8_t monsterHP[TOTAL_MONSTER_COUNT];
/* Types of the monsters. For now, this has no meaning */
uint8_t monsterType[TOTAL_MONSTER_COUNT];
/* Action taken by a given participant*/
uint8_t battleActions[TOTAL_MONSTER_COUNT + TOTAL_CHARACTERS_IN_PARTY];
/* Damage -caused by- a given participant */
uint8_t battleDamages[TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT];
/* The target of a given participant */
uint8_t battleTargets[TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT];

uint8_t battleOrder[TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT];
uint8_t batteCharacterOrder = 0;
uint8_t monsterTypeOffset = 0;

enum EBattleStates currentBattleState;
int32_t animationTimer;
uint8_t monstersPresent = 0;
uint8_t aliveMonsters = 0;
uint8_t aliveHeroes = 0;

const char *BattleScreen_options[kDummyBattleOptionsCount] = {
        "Attack", "Defend", "Special", "Run"};

const char *BattleScreen_optionsNoSpecial[kDummyBattleOptionsCount] = {
        "Attack", "Defend", "-", "Run"};


int8_t splatMonster = -1;
int8_t monsterAttacking = -1;

void BattleScreen_initStateCallback(enum EGameMenuState tag) {
    int c, d;
    (void) tag;
    cursorPosition = 1;
    needsToRedrawVisibleMeshes = 0;
    currentCharacter = 0;

    monsterTypeOffset = getPlayerRoom() - 1;
    
    aliveHeroes = 0;
    for (d = 0; d < TOTAL_CHARACTERS_IN_PARTY; d++) {
        if (party[d].inParty && party[d].hp > 0) {
            ++aliveHeroes;
        }
    }
    
    /* You have Nico in your party. YOU SUFFER! */
    if ((party[4].inParty && party[4].hp > 0) || getPlayerRoom() == 22 ) {
        aliveMonsters = monstersPresent = TOTAL_MONSTER_COUNT;
    } else {
        aliveMonsters = monstersPresent = 1 + (nextRandomInteger() % min( aliveHeroes, TOTAL_MONSTER_COUNT - 1));
    }

    splat[0] = loadBitmap("splat0.img");
    splat[1] = loadBitmap("splat1.img");
    splat[2] = loadBitmap("splat2.img");

    foe[0][0] = loadBitmap("bull0.img");
    foe[1][0] = loadBitmap("cuco0.img");
    foe[2][0] = loadBitmap("lady0.img");

    foe[0][1] = loadBitmap("bull1.img");
    foe[1][1] = loadBitmap("cuco1.img");
    foe[2][1] = loadBitmap("lady1.img");

    if (getPlayerRoom() == 22) {
        monsterTypeOffset = 23;
        monsterHP[0] = 40 + (nextRandomInteger() % 3);
        monsterType[0] = 23;

        monsterHP[1] = 40 + (nextRandomInteger() % 3);
        monsterType[1] = 24;

        monsterHP[2] = 40 + (nextRandomInteger() % 3);
        monsterType[2] = 25;
    } else {
        for (c = 0; c < aliveMonsters; ++c) {
            monsterHP[c] = 20 + (nextRandomInteger() % 3);
            monsterType[c] = monsterTypeOffset + (nextRandomInteger() % 3); /* We can only have 3 types of monsters at the same time */
        }
    }

    splatMonster = -1;
    monsterAttacking = -1;
    currentBattleState = kPlayerSelectingMoves;
    cursorPosition = 0;
}

void BattleScreen_repaintCallback(void) {
    char buffer[32];
    int c;
    int xWindow = 0;
    clearScreen();

    if (currentBattleState == kAttackPhase) {

        /* only a single blink */
        if (animationTimer == kBattleAnimationInterval) {

            int damage = battleDamages[currentCharacter];
            
            if (battleActions[currentCharacter] == kSpecial) {
                fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFFFF0000), 0);
            } else if (battleActions[currentCharacter] == kAttack) {
                if (currentCharacter >= TOTAL_CHARACTERS_IN_PARTY) {
                    if (monsterHP[currentCharacter - TOTAL_CHARACTERS_IN_PARTY] == 0 || /* Attacking monster is dead */
                        party[battleTargets[currentCharacter]].hp == 0 ) { /* or its target is dead */
                        animationTimer = 0;
                        goto done_flashing;
                    }
                    monsterAttacking = currentCharacter - TOTAL_CHARACTERS_IN_PARTY;
                } else {
                    if (!party[currentCharacter].inParty || /* attacking hero is dead */
                        monsterHP[battleTargets[currentCharacter] - TOTAL_CHARACTERS_IN_PARTY] == 0) { /* or its target is dead */
                        animationTimer = 0;
                        goto done_flashing;
                    }
                    damage = -damage;
                    splatMonster = battleTargets[currentCharacter] - TOTAL_CHARACTERS_IN_PARTY;
                }
                
                if (damage > 0) {
                    fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF0000FF), 0);
                } else if (damage < 0) {
                    fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF00FF00), 0);
                } else {
                    fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFFFFFFFF), 0);
                }
            }
        }
    }

    done_flashing:

    needsToRedrawVisibleMeshes = 0;

    for (c = 1; c < (YRES_FRAMEBUFFER / 2); c = c * 2) {
        fillRect(0, c + YRES_FRAMEBUFFER / 2,
                 XRES_FRAMEBUFFER, 1,
                 getPaletteEntry(0xFFFF0000), 0);
    }

    for (c = 0; c < monstersPresent; c++) {
        int monsterTypeIndex = -monsterTypeOffset + monsterType[monstersPresent - c - 1];
        int spriteFrame = (monsterAttacking == (monstersPresent - c - 1));
        /*
         to make it even weider, we have (monstersPresent - c - 1). This is required, since otherwise, we would print
         monster HPs from bottom to top.
         */
        sprintf(&buffer[0], "H %d", monsterHP[(monstersPresent - c - 1)]);

        if (monsterHP[(monstersPresent - c - 1)] > 0 ) {

            drawTextWindow(
		     11 + ( c * (32 + 16) ) / 8,
		     1,
		     6,
		     3,
		     monsterArchetypes[monsterType[monstersPresent - c - 1]].name,
		     &buffer[0]);
	  
            drawBitmap(4 + 11 * 8 + ( c * (32 + 16)), -16 + (YRES_FRAMEBUFFER - foe[monsterTypeIndex][spriteFrame]->height) / 2, foe[monsterTypeIndex][spriteFrame], 1);

            if (splatMonster == (monstersPresent - c - 1) &&
                battleTargets[currentCharacter] == ((monstersPresent - c - 1) + TOTAL_CHARACTERS_IN_PARTY) &&
                battleDamages[currentCharacter] > 0 &&
                currentBattleState == kAttackPhase &&
                party[currentCharacter].hp > 0) {

                char buffer3[16];
                int frame = (animationTimer / 300) - 1;

                /* Terrible kludge, but I'm in a hurry */
                if (frame >= 0 ) {
                    drawBitmap(12 * 8 + ( c * (splat[frame]->width + 8)), -16 + (YRES_FRAMEBUFFER - splat[frame]->height) / 2, splat[frame], 1);
                }

                sprintf(&buffer3[0], "%d HP", battleDamages[currentCharacter] );
                drawTextAt(
                           12 + ( c * (32 + 16) ) / 8,
                           2,
                           &buffer3[0],
                           getPaletteEntry(0xFF0000FF));
            }
        }
    }

    if (currentBattleState == kPlayerSelectingMoves) {
        if (party[currentCharacter].specialStype == kNone) {
            drawWindowWithOptions(
                    0,
                    (YRES_FRAMEBUFFER / 8) - 9 - kDummyBattleOptionsCount,
                    9 + 2,
                    kDummyBattleOptionsCount + 2,
                    party[currentCharacter].name,
                    BattleScreen_optionsNoSpecial,
                    kDummyBattleOptionsCount,
                    cursorPosition);
        } else {
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
    }

    for (c = 0; c < TOTAL_CHARACTERS_IN_PARTY; c++) {
        if (party[c].inParty) {
            sprintf(&buffer[0], "H %d\nE %d", party[c].hp, party[c].energy);
            drawTextWindow(xWindow, (YRES_FRAMEBUFFER / 8) - 6, 7, 4, party[c].name, &buffer[0]);

            if (currentBattleState == kAttackPhase &&
                battleDamages[currentCharacter] > 0 &&
                c == battleTargets[currentCharacter] &&
                party[c].hp > 0 &&
                monsterHP[currentCharacter - TOTAL_CHARACTERS_IN_PARTY] > 0) {

                sprintf(&buffer[0], "%d HP", battleDamages[currentCharacter] );
                drawTextAt( 1 + xWindow, (YRES_FRAMEBUFFER / 8) - 5, &buffer[0],
                       getPaletteEntry(0xFF0000FF));
            }
            xWindow += 7;
        }
    }
}

enum EGameMenuState BattleScreen_tickCallback(enum ECommand cmd, void *data) {

    (void) data;

    if (currentBattleState == kAttackPhase) {

        if (animationTimer == (kBattleAnimationInterval / 2)) {

            int target = battleTargets[currentCharacter];
            int attackerIsAlive;

            if (currentCharacter >= TOTAL_CHARACTERS_IN_PARTY) {
                attackerIsAlive = monsterHP[currentCharacter - TOTAL_CHARACTERS_IN_PARTY] > 0;
            } else {
                attackerIsAlive = party[currentCharacter].hp > 0;
            }

            /* The target must still be alive to perform his attack */
            if (attackerIsAlive) {

                if (currentCharacter < TOTAL_CHARACTERS_IN_PARTY) {
                    if (battleActions[currentCharacter] == kSpecial) {

                        if (party[currentCharacter].specialStype == kHeal) {
                            int c = 0;
                            party[currentCharacter].energy -= 4;

                            for (c = 0; c < TOTAL_CHARACTERS_IN_PARTY; ++c) {
                                if (party[c].inParty && party[c].hp > 0) {
                                    ++party[c].hp;
                                }
                            }
                        } else if (party[currentCharacter].specialStype == kOffense) {
                            int c;
                            party[currentCharacter].energy -= 2;

                            for (c = 0; c < TOTAL_MONSTER_COUNT; ++c) {
                                if (monsterHP[c] > 0) {
                                    --monsterHP[c];
                                    
                                    if (monsterHP[c] == 0) {
                                        aliveMonsters--;
                                        ++party[currentCharacter].kills;
                                    }
                                }
                            }
                        }
                    } else if (battleActions[currentCharacter] == kRun) {
                        /* Nico will decrease your speed so much that you won't be able to run */
                        if (!party[4].inParty || party[4].hp == 0 ) {
                            int roll = nextRandomInteger() % 10;
                            int agility = party[currentCharacter].agility;
                            if (roll < agility) {
                                initRoom(getPlayerRoom());
                                return kEscapedBattle;
                            }
                        }
                    }
                }

                /* Monsters */
                if (target >= TOTAL_CHARACTERS_IN_PARTY) {
                    int monsterIndex = target - TOTAL_CHARACTERS_IN_PARTY;

                    if (monsterHP[monsterIndex] > 0) {

                        if (battleActions[currentCharacter] == kAttack) {
                            if (battleDamages[currentCharacter] >= monsterHP[monsterIndex]) {
                                monsterHP[monsterIndex] = 0;
                            } else {
                                monsterHP[monsterIndex] -= battleDamages[currentCharacter];
                            }
                            
                            if (monsterHP[monsterIndex] == 0) {
                                aliveMonsters--;
                                ++party[currentCharacter].kills;
                            }
                        }

                        /* Other actions will go here, eventually */
                    } else {
                        /* Since the monster is dead or not present, this will advance the animation faster */
                        animationTimer = 0;
                    }

                } else {

                    if (party[target].inParty && party[target].hp > 0) {

                        if (battleActions[currentCharacter] == kAttack) {
                            if ((battleDamages[currentCharacter]) >= party[target].hp) {
                                party[target].hp = 0;
                                aliveHeroes--;
                            } else {
                                party[target].hp -= battleDamages[currentCharacter];
                            }
                        } else if (battleActions[currentCharacter] == kSpecial) {
                            party[target].hp += 2;
                        }

                        /* Other actions will go here, eventually */
                    } else {
                        /* Since the hero is dead or not present, this will advance the animation faster */
                        animationTimer = 0;
                    }
                }
            }
        }
#ifdef __EMSCRIPTEN__
        animationTimer-=25;
#else
        animationTimer-=50;
#endif

        if (animationTimer < 0) {
            animationTimer = kBattleAnimationInterval;
            batteCharacterOrder++;

            if (party[4].inParty && party[4].hp > 0 ) {
            	int m;
                /* Force giving priority to monsters, so they can kill Nico */
                for (m = 0; m < TOTAL_MONSTER_COUNT; ++m) {
                    if (battleOrder[TOTAL_CHARACTERS_IN_PARTY + m] == 0 && monsterHP[m] > 0) {
                        currentCharacter = TOTAL_CHARACTERS_IN_PARTY + m;
                        goto done_selecting_monster;
                    }
                }

                do {
                    currentCharacter = nextRandomInteger() % (TOTAL_MONSTER_COUNT + TOTAL_CHARACTERS_IN_PARTY);
                } while( battleOrder[currentCharacter] != 0 );

            done_selecting_monster:
                splatMonster = -1;
                monsterAttacking = -1;

            } else {
                do {
                    currentCharacter = nextRandomInteger() % (TOTAL_MONSTER_COUNT + TOTAL_CHARACTERS_IN_PARTY);
                } while( battleOrder[currentCharacter] != 0 );
            }

            battleOrder[currentCharacter] = 1;

            if (batteCharacterOrder == (TOTAL_CHARACTERS_IN_PARTY + TOTAL_MONSTER_COUNT)) {
                currentCharacter = batteCharacterOrder = 0;
                currentBattleState = kPlayerSelectingMoves;

                if (aliveHeroes == 0) {
                    return kBadGameOverEpilogue;
                }

                if (aliveMonsters == 0) {
                    if (getPlayerRoom() == 22) {
                        return kGoodVictoryEpilogue;
                    } else {
                        return kBattleResultScreen;
                    }
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
		break;
            case kCommandDown:
                if (cursorPosition < kDummyBattleOptionsCount) {
                    cursorPosition++;
                    firstFrameOnCurrentState = 1;
                }
                break;
            case kCommandFire1:
                if (cursorPosition == kSpecial &&
                    ( party[currentCharacter].specialStype == kNone ||
                      party[currentCharacter].energy < 4)) {
                    return kResumeCurrentState;
                }

                firstFrameOnCurrentState = 1;

                battleActions[currentCharacter] = cursorPosition;

                currentCharacter++;
                cursorPosition = 0;

                /* skipping dead or absent characters */
                while (currentCharacter < (TOTAL_CHARACTERS_IN_PARTY) &&
                    (!party[currentCharacter].inParty || party[currentCharacter].hp == 0)) {
                    currentCharacter++;
                }

                if (currentCharacter == (TOTAL_CHARACTERS_IN_PARTY)) {
                    int c;
                    int totalDamage = 0;
                    currentCharacter = batteCharacterOrder = 0;
                    currentBattleState = kAttackPhase;
                    
                    for (c = 0; c < (TOTAL_MONSTER_COUNT + TOTAL_CHARACTERS_IN_PARTY); ++c) {
                        battleOrder[c] = 0;
                    }

                    for (c = 0; c < (TOTAL_MONSTER_COUNT); ++c) {
                        if (monsterHP[c] > 0) {
                            int hero;
                            if (party[4].inParty && party[4].hp > 0) {
                                hero = 4;
                            } else {
                                do {
                                    hero = nextRandomInteger() % TOTAL_CHARACTERS_IN_PARTY;
                                } while (party[hero].hp == 0 || !party[hero].inParty);
                            }

                            battleTargets[c + TOTAL_CHARACTERS_IN_PARTY] = hero;
                            battleActions[c + TOTAL_CHARACTERS_IN_PARTY] = kAttack;
                        }
                    }

                    for (c = 0; c < (TOTAL_CHARACTERS_IN_PARTY); ++c) {
                        if (battleActions[c] == kAttack) {
                            int monster;

                            do {
                                monster =  TOTAL_CHARACTERS_IN_PARTY + (nextRandomInteger() % (TOTAL_MONSTER_COUNT));
                            } while ( monsterHP[monster - TOTAL_CHARACTERS_IN_PARTY] == 0);

                            battleTargets[c] = monster;
                        }
                    }

                    for (c = 0; c < TOTAL_CHARACTERS_IN_PARTY; ++c) {
                        if (party[4].inParty && party[4].hp > 0) {
                            battleDamages[c] = 0;
                        } else {
                            if (battleActions[c] == kAttack) {
                                int monsterTargetted = battleTargets[c];
                                int attack = (nextRandomInteger() % party[c].attack);
                                int agil1 = party[c].agility;
                                int agil2 = monsterArchetypes[monsterType[monsterTargetted]].agility;
                                int diffAgility = 1 + abs( agil1 - agil2);
                                int roll = nextRandomInteger();
                                int hit = roll % diffAgility;
                                int defense = ((battleActions[c + TOTAL_CHARACTERS_IN_PARTY] == kDefend) ? 2 : 1) *  monsterArchetypes[monsterType[monsterTargetted]].defense;
                                int calc = attack - ((hit == 0) ? 0 : defense);
                                battleDamages[c] = max(0, calc * party[c].level);
                                totalDamage += battleDamages[c];
                            } else if (battleActions[c] == kSpecial) {
                                int wisdom = party[c].wisdom;
                                int roll = nextRandomInteger();
                                int hit = roll % wisdom;
                                battleDamages[c] = hit * party[c].level;
                                totalDamage += battleDamages[c];
                                battleTargets[c] = 0;
                            } else {
                                battleDamages[c] = 0;
                            }
                        }
                    }

                    for (c = 0; c < TOTAL_MONSTER_COUNT; ++c) {
                        if (party[4].inParty && party[4].hp > 0) {
                            battleDamages[c + TOTAL_CHARACTERS_IN_PARTY] = party[4].hp;
                            battleTargets[c + TOTAL_CHARACTERS_IN_PARTY] = 4;
                        } else {
                            if (battleActions[c + TOTAL_CHARACTERS_IN_PARTY] == kAttack) {
                                int heroTargetted = battleTargets[c + TOTAL_CHARACTERS_IN_PARTY];
                                int attack = (nextRandomInteger() % monsterArchetypes[monsterType[c]].attack);
                                int agil1 = party[heroTargetted].agility;
                                int agil2 = monsterArchetypes[monsterType[c]].agility;
                                int diffAgility = 1 + abs( agil1 - agil2 );
                                int roll = nextRandomInteger();
                                int hit = roll % diffAgility;
                                int defense = ((battleActions[c] == kDefend) ? 2 : 1) *  party[heroTargetted].defense;
                                int calc = attack - ((hit == 0) ? 0 : defense);
                                battleDamages[c + TOTAL_CHARACTERS_IN_PARTY] = max(0, calc);
                                totalDamage += battleDamages[c + TOTAL_CHARACTERS_IN_PARTY];
                            } else if (battleActions[c + TOTAL_CHARACTERS_IN_PARTY] == kSpecial) {
                                int wisdom = monsterArchetypes[monsterType[c]].wisdom;
                                int roll = nextRandomInteger();
                                int hit = roll % wisdom;
                                battleDamages[c + TOTAL_CHARACTERS_IN_PARTY] = hit;
                                totalDamage += battleDamages[c + TOTAL_CHARACTERS_IN_PARTY];
                                battleTargets[c + TOTAL_CHARACTERS_IN_PARTY] = 0;
                            } else {
                                battleDamages[c + TOTAL_CHARACTERS_IN_PARTY] = 0;
                            }
                        }
                    }

                    if (totalDamage == 0 ) {
                        initRoom(getPlayerRoom());
                        return kEnemiesFledBattle;
                    }

                    animationTimer = kBattleAnimationInterval;
                }
                break;

            default:
                break;
        }
    }

    return kResumeCurrentState;
}

void BattleScreen_unloadStateCallback(enum EGameMenuState newState) {
    (void) newState;

    releaseBitmap(splat[0]);
    releaseBitmap(splat[1]);
    releaseBitmap(splat[2]);

    releaseBitmap(foe[0][0]);
    releaseBitmap(foe[1][0]);
    releaseBitmap(foe[2][0]);

    releaseBitmap(foe[0][1]);
    releaseBitmap(foe[1][1]);
    releaseBitmap(foe[2][1]);
}
