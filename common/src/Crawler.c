#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include "Win32Int.h"
#else
#include <stdint.h>
#include <unistd.h>
#endif


#include "Common.h"
#include "FixP.h"
#include "Enums.h"
#include "Vec.h"
#include "LoadBitmap.h"
#include "Engine.h"
#include "CActor.h"
#include "Mesh.h"
#include "Renderer.h"
#include "Dungeon.h"
#include "Core.h"
#include "Derelict.h"
#include "EDirection_Utils.h"
#include "MapWithCharKey.h"
#include "SoundSystem.h"
#include "UI.h"
#include "Parser.h"

#ifndef SHORT_VIEW_ANGLE
#define ANGLE_TURN_THRESHOLD 40
#define ANGLE_TURN_STEP 5
#else
#define ANGLE_TURN_THRESHOLD 5
#define ANGLE_TURN_STEP 1
#endif

int turning = 0;
int leanX = 0;
int leanY = 0;

int showPromptToAbandonMission = FALSE;
int needsToRedrawHUD = FALSE;

void printMessageTo3DView(const char *message);

const char *AbandonMission_Title = "Abandon game?";
const char *AbandonMission_options[6] = {"Continue", "End game"};
const enum EGameMenuState AbandonMission_navigation[2] = {kResumeCurrentState, kMainMenu};
const int AbandonMission_count = 2;
uint8_t showDialogEntry = 0;
extern int currentSelectedItem;
extern struct CActor playerCrawler;

const static char *storyPoint[] = {
    "",
    "It's been so long you don't even remember why. Every day they try to extract information, further frying your implants. Until...\n\"Come with me! I've cleared the way, but hurry! Reinforcements are in the way. There's a transport coming. We need to reach...\naaaagh!\"",
    "I wonder how many of my former comrades are being held here - perhaps our collective strength could be useful."
};

uint8_t drawActionsWindow = 0;
uint8_t selectedAction = 0xFF;

const static char *CrawlerActions_optionsPickUse[] = {
    "Use",
    "Pick",
    "Use with...",
};

const static char *CrawlerActions_optionsDrop[] = {
    "Use",
    "Drop"
};

const static char *commandTemplate_optionsPickUse[] = {
    "use",
    "pick",
    "use-with"
};

const static char *commandTemplate_optionsDrop[] = {
    "use",
    "drop"
};


struct Item* getNthPlayerObject(int nth) {
    struct ObjectNode *head;
    struct Item *itemPtr = NULL;
    struct Item *item = NULL;
    
    head = getPlayerItems();
    needsToRedrawHUD = TRUE;

    while (head != NULL && itemPtr == NULL) {
        if (nth == 0) {
            return getItem(head->item);
        }
        
        nth--;
        
        head = head->next;
    }

    return NULL;
}

struct Item* itemInFrontOfPlayer() {
    struct ObjectNode *head;
    struct Item *itemPtr = NULL;
    
    struct Item *item = NULL;
    struct Vec2i offseted = mapOffsetForDirection(playerCrawler.rotation);
    head = getRoom(getPlayerRoom())->itemsPresent->next;
    offseted.x += playerCrawler.position.x;
    offseted.y += playerCrawler.position.y;

    needsToRedrawHUD = TRUE;

    while (head != NULL && itemPtr == NULL) {
        if (offseted.x == (getItem(head->item)->position.x) &&
            offseted.y == (getItem(head->item)->position.y)) {
            return getItem(head->item);
        }
        head = head->next;
    }

    return NULL;
}


void drawInventoryWindow(void) {
    struct ObjectNode *head;
    int c;
    struct Item *itemPtr;
    const char *item = NULL;
    char bufferItem[256];
    char bufferWindow[256];
    int biggest = 9; /* length of "Inventory" */
    int candidateBiggest = 0;
    int totalItems = 0;
    const char **itemList;
    const char **listItemPtr;

    head = getPlayerItems();

    while (head != NULL) {
        itemPtr = getItem(head->item);
        item = itemPtr->name;

        candidateBiggest = strlen(item);
        if (candidateBiggest > biggest) {
            biggest = candidateBiggest;
        }

        head = head->next;
        ++totalItems;
    }

    listItemPtr = itemList = allocMem( sizeof(char*) * totalItems, GENERAL_MEMORY, 1);
    itemPtr = NULL;
    item = NULL;
    head = getPlayerItems();

    while (head != NULL) {
        itemPtr = getItem(head->item);
        *listItemPtr = itemPtr->name;
        head = head->next;
        ++listItemPtr;
    }

    drawWindowWithOptions(13,
                          /*(YRES_FRAMEBUFFER / 8) - itemsCount - 2*/ 0,
                          biggest + 2,
                          totalItems + 2,
                          "Inventory",
                          itemList,
                          totalItems,
                          cursorPosition);
}

void Crawler_initStateCallback(enum EGameMenuState tag) {
    int c;

    if (tag == kPlayGame) {
        initStation();
        showDialogEntry = 1;
        timeUntilNextState = 1000;
        gameTicks = 0;
        enteredThru = 0;
        memFill(&gameSnapshot, 0, sizeof(struct GameSnapshot));
    } else {
        timeUntilNextState = 0;
    }

    if (tag == kEscapedBattle) {
        timeUntilNextState = 1000;
        printMessageTo3DView("You managed to escape!");
    }

    if (tag == kEnemiesFledBattle) {
        timeUntilNextState = 1000;
        printMessageTo3DView("The opposing party has fled!");
    }

    currentPresentationState = kWaitingForInput;

    showPromptToAbandonMission = FALSE;

    biggestOption = strlen(AbandonMission_Title);

    for (c = 0; c < AbandonMission_count; ++c) {
        size_t len = strlen(AbandonMission_options[c]);

        if (len > biggestOption) {
            biggestOption = len;
        }
    }

    playerHeight = 0;
    playerHeightChangeRate = 0;

    if (tag == kPlayGame) {
        clearMap(&tileProperties);
        initRoom(getPlayerRoom());
    }
}

void recenterView(void) {
    if (leanX > 0 && !turning) {
        leanX -= ANGLE_TURN_STEP;
    }

    if (leanX < 0 && !turning) {
        leanX += ANGLE_TURN_STEP;
    }

    if (leanY > 0) {
        leanY -= ANGLE_TURN_STEP;
    }

    if (leanY < 0) {
        leanY += ANGLE_TURN_STEP;
    }

    if (leanX > 0 && turning) {
        if (leanX < ANGLE_TURN_THRESHOLD) {
            leanX += ANGLE_TURN_STEP;
        } else if (leanX == ANGLE_TURN_THRESHOLD) {
            visibilityCached = FALSE;
            mBufferedCommand = kCommandRight;
            leanX = -ANGLE_TURN_THRESHOLD;
            turning = 0;
        }
    }

    if (leanX < 0 && turning) {
        if (leanX > -ANGLE_TURN_THRESHOLD) {
            leanX -= ANGLE_TURN_STEP;
        } else if (leanX == -ANGLE_TURN_THRESHOLD) {
            visibilityCached = FALSE;
            mBufferedCommand = kCommandLeft;
            leanX = ANGLE_TURN_THRESHOLD;
            turning = 0;
        }
    }
}

void redrawHUD(void) {
    int line = 0;
    struct ObjectNode *head;
    int c;
    struct Item *itemPtr = itemInFrontOfPlayer();
    
    if (itemPtr != NULL) {
        size_t len;
        int lines;
        len = strlen(itemPtr->name);
        lines = 2 + (len / (XRES / 8));
        fillRect(0, YRES_FRAMEBUFFER - (lines * 8), XRES, (lines * 8), getPaletteEntry(0xFF000000), 1);
        drawTextAtWithMarginWithFiltering(1, (YRES / 8) - lines, XRES, itemPtr->name, getPaletteEntry(0xFFFFFFFF), ' ');
    }
    
    if (drawActionsWindow) {
      struct Item *item = NULL;
        
      item = itemInFrontOfPlayer();

      if (item != NULL) {
          drawWindowWithOptions(0,
			    /*(YRES_FRAMEBUFFER / 8) - 10*/0,
			    11 + 2,
			    5,
			    "Action",
			    CrawlerActions_optionsPickUse,
			    3,
                (selectedAction == 0xFF) ? cursorPosition : selectedAction);
      } else {
	      drawWindowWithOptions(0,
			    /*(YRES_FRAMEBUFFER / 8) - 10*/0,
			    11 + 2,
			    4,
			    "Action",
			    CrawlerActions_optionsDrop,
			    2,
			    (selectedAction == 0xFF) ? cursorPosition : selectedAction);
      }
        if (selectedAction != 0xFF) {
            drawInventoryWindow();
        }

    }
}

void Crawler_repaintCallback(void) {

    needsToRedrawVisibleMeshes = TRUE;

    if (showPromptToAbandonMission) {
        int optionsHeight = 8 * (AbandonMission_count);
        turnStep = turnTarget;

        /* The dithered filter on top of the 3D rendering*/
        fillRect(0, 0, XRES_FRAMEBUFFER, YRES_FRAMEBUFFER, getPaletteEntry(0xFF000000), TRUE);

        drawWindowWithOptions((XRES_FRAMEBUFFER / 8) - biggestOption - 4,
                              ((YRES_FRAMEBUFFER / 8) + 1) - (optionsHeight / 8) - 4,
                              biggestOption + 2,
                              (optionsHeight / 8) + 2,
                              AbandonMission_Title,
                              AbandonMission_options,
                              AbandonMission_count,
                              cursorPosition);
    } else {

        if (currentPresentationState == kRoomTransitioning) {
            renderRoomTransition();
        } else if (currentPresentationState == kWaitingForInput) {
            int c;
            int yWindow = 0;
            char buffer[64];
            renderTick(30);

            recenterView();

            fillRect(XRES, 0, 64, YRES_FRAMEBUFFER, getPaletteEntry(0xFF000000), 0);

            for (c = 0; c < TOTAL_CHARACTERS_IN_PARTY; c++) {
                if (party[c].inParty) {
                    sprintf(&buffer[0], "H %d\nE %d", party[c].hp, party[c].energy);
                    drawTextWindow((XRES / 8), yWindow, 7, 4, party[c].name, &buffer[0]);
                    yWindow += 5;
                }
            }
            
            redrawHUD();
        }
    }

    if (showDialogEntry) {
        drawTextWindow(0, 0, (XRES_FRAMEBUFFER / 8) - 1, (YRES_FRAMEBUFFER / 8) - 1, "Press any key to continue", storyPoint[showDialogEntry]);
    }
}

enum EGameMenuState Crawler_tickCallback(enum ECommand cmd, void *data) {

    /* Temporary end of the game, for the demo release */
    if (getPlayerRoom() == 13 || getPlayerRoom() == 19) {
        return kGoodVictoryEpilogue;
    }

    if (showPromptToAbandonMission) {

        switch (cmd) {
            case kCommandBack:
                return kMainMenu;
            case kCommandUp:
                timeUntilNextState = 1000;
                playSound(MENU_SELECTION_CHANGE_SOUND);
                --cursorPosition;
                break;
            case kCommandDown:
                timeUntilNextState = 1000;
                playSound(MENU_SELECTION_CHANGE_SOUND);
                ++cursorPosition;
                break;
            case kCommandFire1:
            case kCommandFire2:
            case kCommandFire3:

                if (cursorPosition == 0) {
                    showPromptToAbandonMission = FALSE;
                    needsToRedrawVisibleMeshes = TRUE;
                    return kResumeCurrentState;
                }
                timeUntilNextState = 1000;
                return AbandonMission_navigation[cursorPosition];
        }

        if (cursorPosition >= AbandonMission_count) {
            cursorPosition = AbandonMission_count - 1;
        }

        if (cursorPosition < 0) {
            cursorPosition = 0;
        }

        return kResumeCurrentState;
    }

    if (drawActionsWindow) {
        switch (cmd) {
            case kCommandBack:
                return kMainMenu;
            case kCommandUp:
                timeUntilNextState = 1000;
                playSound(MENU_SELECTION_CHANGE_SOUND);
                --cursorPosition;
                break;
            case kCommandDown:
                timeUntilNextState = 1000;
                playSound(MENU_SELECTION_CHANGE_SOUND);
                ++cursorPosition;
                break;
	    case kCommandFire2:
        case kCommandLeft:
                if (selectedAction != 0xFF) {
                    selectedAction = 0xFF;
                } else {
                    drawActionsWindow = 0;
                }
  	        break;
        case kCommandFire1:
        case kCommandRight:
            {
                char cmdBuffer[256];
                struct Item *item;
                needsToRedrawVisibleMeshes = TRUE;
                timeUntilNextState = 1000;

                item = itemInFrontOfPlayer();

                if (item) {
                    if (selectedAction == 0xFF) {
                        if (cursorPosition == 2) {
                            selectedAction = cursorPosition;
                            cursorPosition = 0;
                        } else {
                            /* Please note the index! This the case of a action that doesn't open the cascading menu */
                            parseCommand(commandTemplate_optionsPickUse[cursorPosition], item->name);
                            drawActionsWindow = 0;
                            currentSelectedItem = cursorPosition;
                            selectedAction = 0xFF;
                        }
                    } else {
                        if (selectedAction == 2) {
                            char *operand1;
                            char *operator1;
                            /* Terrible terrible hack*/
                            sprintf(&cmdBuffer[0], "use-with %s %s", getNthPlayerObject(cursorPosition)->name, item->name);
                            operator1 = strtok(&cmdBuffer[0], "\n ");
                            operand1 = strtok(NULL, "\n ");
                            parseCommand(operator1, operand1);

                            
                            drawActionsWindow = 0;
                            currentSelectedItem = cursorPosition;
                            selectedAction = 0xFF;
                        } else {
                            puts("THIS SHOULDN'T BE HAPENNING!");
                        }
                    }
                } else {
                    if (selectedAction == 0xFF) {
                        selectedAction = cursorPosition;
                        cursorPosition = 0;
                    } else {
                        parseCommand(commandTemplate_optionsDrop[selectedAction], getNthPlayerObject(cursorPosition)->name);
                        drawActionsWindow = 0;
                        currentSelectedItem = cursorPosition;
                        selectedAction = 0xFF;
                    }
                }
                
                
                loopTick(kCommandFire4);

            }
            return kResumeCurrentState;
        }
        
        if (cursorPosition >=  (itemInFrontOfPlayer() == NULL ? 2 : 3)) {
            cursorPosition = 1;
        }

        if (cursorPosition < 0) {
            cursorPosition = 0;
        }

        return kResumeCurrentState;
    }
    
    if (showDialogEntry) {
        if (cmd == kCommandFire1 || cmd == kCommandFire2) {
            showDialogEntry = 0;
        }
        return kResumeCurrentState;
    }

    if (cmd == kCommandBack) {
        showPromptToAbandonMission = TRUE;
        timeUntilNextState = 0;
        return kResumeCurrentState;
    }

    if (timeUntilNextState != kNonExpiringPresentationState) {
        timeUntilNextState -= *((long *) data);
    }

    if (currentPresentationState == kWaitingForInput) {
        
        if (cmd ==  kCommandFire1 ) {
            drawActionsWindow = 1;
            selectedAction = 0xFF;
        }

        if (timeUntilNextState < 0) {
            if (cmd == kCommandNone) {
                timeUntilNextState = 1000;
            }
        }

        if (cmd == kCommandFire1 || cmd == kCommandFire2) {
            cmd = kCommandNone;
        }

        loopTick(cmd);

        return kResumeCurrentState;
    }


    if (currentPresentationState == kCheckingForRandomBattle) {
        struct Room *playerNewRoom = getRoom(getPlayerRoom());
        int random = nextRandomInteger() % 10;
        currentPresentationState = kWaitingForInput;
        texturesUsed = 0;
        clearTextures();
        nativeTextures[0] = makeTextureFrom("arch.img");
        nativeTextures[1] = makeTextureFrom("floor.img");
        texturesUsed = 2;

        showDialogEntry = getRoom(getPlayerRoom())->storyPoint;
        getRoom(getPlayerRoom())->storyPoint = 0;

        if (getPlayerRoom() == 22 ||
            (party[4].inParty && party[4].hp > 0) ||
            random < playerNewRoom->chanceOfRandomBattle) {

            return kBattleScreen;
        } else {
            initRoom(getPlayerRoom());
        }
    }

    return kResumeCurrentState;
}

void Crawler_unloadStateCallback(enum EGameMenuState newState) {

    if (newState != kBackToGame &&
        newState != kEscapedBattle &&
        newState != kEnemiesFledBattle &&
        newState != kHackingGame &&
        newState != kBattleScreen &&
        newState != kBattleResultScreen) {

        clearTextures();
        clearTileProperties();
    }
}
