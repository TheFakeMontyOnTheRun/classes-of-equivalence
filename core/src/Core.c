/*
Created by Daniel Monteiro on 2019-07-26.
*/
#include <stddef.h>

#ifdef __MWERKS__
#define CLI_BUILD
#endif

#ifndef SMD

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include "Win32Int.h"
#else

#include <stdint.h>

#endif
#else
#include <genesis.h>
#endif

#include "Enums.h"
#include "Common.h"
#include "Core.h"

#define WALK_STEP 1

/**
 * there's an implicit dummy first
 */
int roomCount = 1;
/**
 *
 */
#ifdef AGS
__attribute__((target("arm"), section(".ewram"), noinline))
#endif
struct Room rooms[TOTAL_ROOMS];
/**
 *
 */
uint8_t itemsCount = 0;
#ifdef AGS
__attribute__((target("arm"), section(".ewram"), noinline))
#endif
/**
 *
 */
struct Item item[TOTAL_ITEMS];

#ifdef AGS
__attribute__((target("arm"), section(".ewram"), noinline))
#endif
/*
 *
 */
struct ObjectNode objectNodes[TOTAL_ITEMS];
/**
 *
 */
struct ObjectNode collectedObjectHead;

#ifdef AGS
__attribute__((target("arm"), section(".ewram"), noinline))
#endif
/**
 *
 */
struct ObjectNode roomObjectHeads[TOTAL_ROOMS];
/**
 *
 */
struct ObjectNode *collectedObject = NULL;
/**
 *
 */
enum EGameStates gameStatus;
/**
 *
 */
ErrorHandlerCallback errorHandlerCallback = NULL;

struct Character party[TOTAL_CHARACTERS_IN_PARTY];

void writeToLog(const char *errorMsg) {
#ifdef HAS_STDIO
    if (errorHandlerCallback == NULL) {
        puts("-------");
        puts(errorMsg);
        puts("--!!---");
    } else {
        errorHandlerCallback(errorMsg);
    }
#else
    /* We assume the error handler was already set. If you don't have stdio, you probably know what you're doing */
    errorHandlerCallback(errorMsg);
#endif
}

void cantBeUsedCallback(struct Item *item) {
    (void)item;
    defaultLogger("You can't use it like this.");
}

void cantBeUsedWithOthersCallback(struct Item *item1, struct Item *item2) {
    (void)item1;
    (void)item2;
    defaultLogger("Nothing happens.");
}

struct Item *addItem(const char *description,
#ifdef INCLUDE_ITEM_DESCRIPTIONS
        const char *info,
#endif
#ifdef ITEMS_HAVE_WEIGHT
        uint8_t weight,
#endif
                     uint8_t pickable,
                     int8_t positionX,
                     int8_t positionY,
                     uint8_t hasVisuals) {

    struct Item *toReturn = &item[itemsCount];

    toReturn->index = itemsCount++;
    toReturn->name = description;
#ifdef INCLUDE_ITEM_DESCRIPTIONS
    toReturn->info = info;
#endif
    toReturn->pickable = pickable;
    toReturn->position.x = positionX;
    toReturn->position.y = positionY;

    toReturn->useCallback = cantBeUsedCallback;
    toReturn->useWithCallback = cantBeUsedWithOthersCallback;
    toReturn->hasVisuals = hasVisuals;

    return toReturn;
}

struct Room *addRoom(
        const char *name,
#ifdef INCLUDE_ROOM_DESCRIPTIONS
        const char *info,
#endif
        uint8_t sizeX, uint8_t sizeY, uint8_t chanceOfRandomBattle, const int8_t connections[6]) {

    struct Room *toReturn = &rooms[roomCount];
    toReturn->name = name;
#ifdef INCLUDE_ROOM_DESCRIPTIONS
    toReturn->info = info;
#endif
    toReturn->sizeX = sizeX;
    toReturn->sizeY = sizeY;
    toReturn->chanceOfRandomBattle = chanceOfRandomBattle;

    memCopyToFrom((void*)toReturn->connections, (void*)connections, 6);

    /* add list head to make manipulations easier */
    toReturn->itemsPresent = &roomObjectHeads[roomCount++];
    memFill(toReturn->itemsPresent, 0, sizeof(struct ObjectNode));

    return toReturn;
}

LogDelegate defaultLogger = writeToLog;

void setErrorHandlerCallback(ErrorHandlerCallback callback) {
    errorHandlerCallback = callback;
}

struct WorldPosition *getPlayerPosition(void) {
    return &party[0].position;
}

void setPlayerPosition(struct WorldPosition *pos) {
    party[0].position.x = pos->x;
    party[0].position.y = pos->y;
}

#ifndef CAN_PICK_OBJECT_AT_ANY_DISTANCE
uint8_t isCloseToObject(struct WorldPosition *pos, struct Item *_item) {
    return (abs(pos->x - _item->position.x) + abs(pos->y - _item->position.y)) <= 1;
}
#endif

enum EGameStates getGameStatus(void) {
    return gameStatus;
}

struct ObjectNode *getPlayerItems(void) {
    return collectedObject->next;
}

struct Item *getItemNamed(const char *name) {
    uint8_t c;

    for (c = 0; c < itemsCount; ++c) {
        if (!strcmp(item[c].name, name)) {
            return &item[c];
        }
    }

    return NULL;
}

#ifdef MORE_OBJECTS
uint8_t getRoomIdByName(const char *name) {
    uint8_t c;

    if (name != NULL) {
        for (c = 1; c < roomCount; ++c) {
            if (!strcmp(rooms[c].name, name)) {
                return c;
            }
        }
    }

    return 0;
}
#endif

struct Room *getRoomByName(const char *name) {
    uint8_t c;

    if (name != NULL) {
        for (c = 1; c < roomCount; ++c) {
            if (!strcmp(rooms[c].name, name)) {
                return &rooms[c];
            }
        }
    }

    return NULL;
}

void addObjectToList(struct Item *itemToAdd, struct ObjectNode *listHead) {
    struct ObjectNode *head = listHead;

    while (head->next != NULL) {
        if (getItem(head->item) == itemToAdd) {
            /* Object already belongs to the list! */
            return;
        }

        head = head->next;
    }

    head->next = &objectNodes[itemToAdd->index];
    memFill(head->next, 0, sizeof(struct ObjectNode));
    head->next->item = itemToAdd->index;
}

void removeObjectFromList(struct Item *itemToRemove, struct ObjectNode *listHead) {
    struct ObjectNode *head = listHead->next;
    struct ObjectNode *prev = listHead;

    while (head != NULL) {
        if (head->item == itemToRemove->index) {
            prev->next = head->next;
            return;
        }

        prev = head;
        head = head->next;
    }
    /* Object doesn't belong to the list! */
}

void removeObjectFromRoom(struct Item *itemToRemove) {
    if (itemToRemove->roomId != 0) {
        removeObjectFromList(itemToRemove, rooms[itemToRemove->roomId].itemsPresent);
        itemToRemove->roomId = 0;
    }
}


void addObjectToRoom(uint8_t roomId, struct Item *itemToAdd) {
    struct Room *roomToAddObject = &rooms[roomId];
    removeObjectFromRoom(itemToAdd);
    addObjectToList(itemToAdd, roomToAddObject->itemsPresent);
    itemToAdd->roomId = roomId;
}

struct WorldPosition worldPosOffsetForDirection(const enum EDirection aDirection) {

    struct WorldPosition toReturn;

    switch (aDirection) {
        case kEast:
            toReturn.x = 1;
            toReturn.y = 0;
            break;
        case kWest:
            toReturn.x = -1;
            toReturn.y = 0;
            break;
        case kSouth:
            toReturn.x = 0;
            toReturn.y = 1;
            break;
        case kNorth:
            toReturn.x = 0;
            toReturn.y = -1;
            break;
        default:
            assert (FALSE);
    }

    return toReturn;
}

void dropObjectToRoom(uint8_t roomId, struct Item *itemToDrop) {
    
    struct WorldPosition offseted = worldPosOffsetForDirection(party[0].direction);

#ifdef CLI_BUILD
    if (itemToDrop->roomId != 0) {
        defaultLogger("Object not present to drop");
    }
#endif

    if (!itemToDrop->pickable) {
        defaultLogger("Can't drop this");
        return;
    }

    removeObjectFromList(itemToDrop, collectedObject);
    addObjectToRoom(roomId, itemToDrop);

    offseted.x += party[0].position.x;
    offseted.y += party[0].position.y;

    itemToDrop->position.x = offseted.x;
    itemToDrop->position.y = offseted.y;

    
    if (itemToDrop->dropCallback != NULL) {
        itemToDrop->dropCallback(itemToDrop);
    }
}

void pickObject(struct Item *itemToPick) {
#ifndef CAN_PICK_OBJECT_AT_ANY_DISTANCE
    if (!isCloseToObject(getPlayerPosition(), itemToPick)) {
        return;
    }
#endif

    if (!itemToPick->pickable) {
        defaultLogger("Can't pick it up");
        return;
    }

    removeObjectFromRoom(itemToPick);
    addObjectToList(itemToPick, collectedObject);

    if (itemToPick->pickCallback != NULL) {
        itemToPick->pickCallback(itemToPick);
    }
}

uint8_t getPlayerRank(void) {
    return party[0].rank;
}

void setPlayerRank(uint8_t newRank) {
    party[0].rank = newRank;
}

void moveBy(uint8_t direction) {
    uint8_t c;
    uint8_t previousLocation = party[0].location;

    struct Room *room = &rooms[party[0].location];
    if (direction <= 5 && room->connections[direction] != 0) {
        room = &rooms[party[0].location];

        if (rooms[room->connections[direction]].rankRequired > party[0].rank) {
            defaultLogger("Access denied");
            return;
        }

        party[0].location = room->connections[direction];
        room = &rooms[party[0].location];

        if (direction < 4) {
            for (c = 0; c < 6; ++c) {
                if (room->connections[c] == previousLocation) {
                    direction = c;
                }
            }
        } else {
            return;
        }

        switch (direction) {
            case 2:
                party[0].position.x = rooms[party[0].location].sizeX / 2;
                party[0].position.y = rooms[party[0].location].sizeY - 1;
                break;

            case 3:
                party[0].position.x = 0;
                party[0].position.y = rooms[party[0].location].sizeY / 2;
                break;

            case 1:
                party[0].position.x = rooms[party[0].location].sizeX - 1;
                party[0].position.y = rooms[party[0].location].sizeY / 2;
                break;
            case 0:
            default:
                party[0].position.x = rooms[party[0].location].sizeX / 2;
                party[0].position.y = 0;
                break;
        }
#ifdef CLI_BUILD
        } else if ( errorHandlerCallback) {
            errorHandlerCallback("Please specify a valid direction");
#endif
    }
}

void pickObjectByName(const char *objName) {
    struct Room *room = &rooms[party[0].location];
    struct ObjectNode *itemToPick = room->itemsPresent->next;

    while (itemToPick != NULL) {
        if (!strcmp(getItem(itemToPick->item)->name, objName)) {
#ifdef MOVE_TO_OBJECT_POSITION_WHEN_PICKING
            party[0].position = getItem(itemToPick->item)->position;
#endif
            pickObject(getItem(itemToPick->item));
            return;
        }
        itemToPick = itemToPick->next;
    }
}

void dropObjectByName(const char *objName) {
    struct ObjectNode *itemToPick = collectedObject->next;


    while (itemToPick != NULL) {
        if (!strcmp(getItem(itemToPick->item)->name, objName)) {
            dropObjectToRoom(party[0].location, getItem(itemToPick->item));
            return;
        }
        itemToPick = itemToPick->next;
    }

#ifdef CLI_BUILD
    errorHandlerCallback("Unable to locate object");
#endif

}

uint8_t hasItemInRoom(const char *roomName, const char *itemName) {
    struct ObjectNode *itemToPick;

#ifdef CLI_BUILD
    struct Room* room;

    if (roomName == NULL || itemName == NULL || strlen(roomName) == 0 || strlen(itemName) == 0) {
        defaultLogger("Either the object name or the room name are null. Check your stuff");
        return 0;
    }
#endif

#ifdef CLI_BUILD
    room = getRoomByName(roomName);

    if (room != NULL) {
        itemToPick = room->itemsPresent->next;
    } else {
        errorHandlerCallback("Invalid room name");
        return 0;
    }
#else
    itemToPick = getRoomByName(roomName)->itemsPresent->next;
#endif

    while (itemToPick != NULL) {
        struct Item *pick = getItem(itemToPick->item);
        if (!strcmp(pick->name, itemName)) {
            return 1;
        }
        itemToPick = itemToPick->next;
    }

    return 0;
}

uint8_t playerHasObject(const char *itemName) {
    struct ObjectNode *itemToPick = collectedObject->next;

    while (itemToPick != NULL) {
        if (!strcmp(getItem(itemToPick->item)->name, itemName)) {
            return 1;
        }
        itemToPick = itemToPick->next;
    }
    return 0;
}


uint8_t isPlayerAtRoom(const char *roomName) {
    struct Room *room = &rooms[party[0].location];
    const char *name = room->name;
    uint8_t returnValue = !strcmp(name, roomName);
    return returnValue;
}

const char *getRoomDescription(void) {
    struct Room *room = &rooms[party[0].location];
    return room->name;
}

struct Room *getRoom(uint8_t index) {
    return &rooms[index];
}

struct Item *getItem(uint8_t index) {
    return &item[index];
}

uint8_t getPlayerRoom(void) {
    return party[0].location;
}

void useObjectNamed(const char *operand) {
    struct ObjectNode *itemToPick = getPlayerItems();

    while (itemToPick != NULL) {
        struct Item *_item = getItem(itemToPick->item);
        if (!strcmp(_item->name, operand)) {
            if (_item->useCallback != NULL) {
                _item->useCallback(_item);
            }
            return;
        }
        itemToPick = itemToPick->next;
    }

    itemToPick = getRoom(party[0].location)->itemsPresent->next;

    while (itemToPick != NULL) {
        struct Item *_item = getItem(itemToPick->item);
        if (!strcmp(_item->name, operand)) {
            if (_item->useCallback != NULL) {
                _item->useCallback(_item);
            }
            return;
        }
        itemToPick = itemToPick->next;
    }
}

#ifdef CLI_BUILD
void walkTo(const char *operands) {
    struct WorldPosition pos;
    char *xStr;
    char *yStr;
    int8_t x;
    int8_t y;

    xStr = (char*)operands;
    yStr = strtok(NULL, "\n ");
    x = atoi(xStr);
    y = atoi(yStr);
    pos.x = x;
    pos.y = y;
    setPlayerPosition(&pos);
}

#ifdef INCLUDE_ITEM_DESCRIPTIONS
void infoAboutItemNamed(const char *itemName) {

    struct ObjectNode *object1 = collectedObject->next;
    struct Room *room = &rooms[party[0].location];
    struct ObjectNode *object2 = room->itemsPresent->next;

    if (itemName == NULL || strlen(itemName) == 0) {
#ifdef INCLUDE_ROOM_DESCRIPTIONS
        defaultLogger(room->info);
#endif
        return;
    }

    while (object1 != NULL) {
        struct Item* item = getItem(object1->item);
        assert(item->name != NULL);

        if (!strcmp(item->name, itemName)) {
            defaultLogger(item->info);
            return;
        }
        object1 = object1->next;
    }

    while (object2 != NULL) {
        struct Item* item = getItem(object2->item);
        assert(item->name != NULL);

        if (!strcmp(item->name, itemName)) {
            defaultLogger(item->info);
            return;
        }
        object2 = object2->next;
    }
#ifdef CLI_BUILD
    defaultLogger("No such item could be found");
#endif
}
#endif

void useObjectsTogether(const char *operands) {

    struct ObjectNode *object1 = collectedObject->next;
    struct Room *room = &rooms[party[0].location];
    struct ObjectNode *object2 = room->itemsPresent->next;

    char *operand1 = (char*)operands;
    char *operand2 = strtok(NULL, "\n ");

    if (!playerHasObject(operand1)) {
#ifdef CLI_BUILD
        defaultLogger("You do not have this object");
#endif
        return;
    }

    if (!hasItemInRoom(getRoom(party[0].location)->name, operand2)) {
#ifdef CLI_BUILD
        defaultLogger("That object is not present in the room");
#endif
        return;
    }

    while (object1 != NULL) {
        struct Item* _item = getItem(object1->item);
        assert(_item->name != NULL);

        if (!strcmp(_item->name, operand1)) {
            goto got_first_object;
        }
        object1 = object1->next;
    }

    got_first_object:
    while (object2 != NULL) {
        struct Item* _item = getItem(object2->item);
        assert(_item->name != NULL);

        if (!strcmp(_item->name, operand2)) {
            goto got_second_object;
        }
        object2 = object2->next;
    }

    got_second_object:
    if (object1 != NULL){
        struct Item* _item = getItem(object1->item);

        if (_item != NULL && _item->useWithCallback != NULL && object2 != NULL) {
            _item->useWithCallback(_item, getItem(object2->item));
        }
    }
}
#endif

void turnLeft(void) {
	uint8_t pDir = (uint8_t) party[0].direction;
	pDir--;
    
    pDir = pDir & 3;
    party[0].direction = (enum EDirection)pDir;
}

void turnRight(void) {
	uint8_t pDir = (uint8_t) party[0].direction;
    pDir++;

    pDir = pDir & 3;
    party[0].direction = (enum EDirection)pDir;
}

void setPlayerLocation(uint8_t location) {
    party[0].location = location;
}

void walkBy(uint8_t direction) {

    switch (direction) {
        case 1:
            switch (party[0].direction) {
                case 1:
                    party[0].position.y += WALK_STEP;
                    break;
                case 2:
                    party[0].position.x -= WALK_STEP;
                    break;
                case 3:
                    party[0].position.y -= WALK_STEP;
                    break;
                case 0:
                default:
                    party[0].position.x += WALK_STEP;
                    break;
            }
            break;
        case 2:
            switch (party[0].direction) {
                case 1:
                    party[0].position.x -= WALK_STEP;
                    break;
                case 2:
                    party[0].position.y -= WALK_STEP;
                    break;
                case 3:
                    party[0].position.x += WALK_STEP;
                    break;
                case 0:
                default:
                    party[0].position.y += WALK_STEP;
                    break;
            }
            break;
        case 3:
            switch (party[0].direction) {
                case 1:
                    party[0].position.y -= WALK_STEP;
                    break;
                case 2:
                    party[0].position.x += WALK_STEP;
                    break;
                case 3:
                    party[0].position.y += WALK_STEP;
                    break;
                case 0:
                default:
                    party[0].position.x -= WALK_STEP;
                    break;
            }
            break;
        case 0:
        default:
            switch (party[0].direction) {
                case 1:
                    party[0].position.x += WALK_STEP;
                    break;
                case 2:
                    party[0].position.y += WALK_STEP;
                    break;
                case 3:
                    party[0].position.x -= WALK_STEP;
                    break;
                case 0:
                default:
                    party[0].position.y -= WALK_STEP;
                    break;
            }
            break;
    }

#ifdef CLI_BUILD
    if (party[0].position.x < 0) {
        party[0].position.x = 0;
    }

    if (party[0].position.y < 0) {
        party[0].position.y = 0;
    }


    if (party[0].position.x >= rooms[party[0].location].sizeX) {
        party[0].position.x = rooms[party[0].location].sizeX - 1;
    }

    if (party[0].position.y >= rooms[party[0].location].sizeY) {
        party[0].position.y = rooms[party[0].location].sizeY - 1;
    }
#endif
}

enum EDirection getPlayerDirection(void) {
    return party[0].direction;
}

void addToRoom(const char *roomName, struct Item *itemName) {
    uint8_t r;

#ifdef CLI_BUILD
    if (roomName == NULL || itemName == NULL || strlen(roomName) == 0) {
        defaultLogger("Either the object name or the room name are null. Check your stuff");
        return;
    }
#endif

    for (r = 1; r < TOTAL_ROOMS; ++r) {
        const char *desc = rooms[r].name;

        if (desc != NULL && !strcmp(desc, roomName)) {
            addObjectToRoom(r, itemName);
            return;
        }
    }

#ifdef CLI_BUILD
    defaultLogger("It was not possible to determine the room to add object");
#endif
}

void setLoggerDelegate(LogDelegate newDelegate) {
    defaultLogger = newDelegate;
}


void setPlayerDirection(enum EDirection direction) {
    party[0].direction = direction;
}

void setGameStatus(enum EGameStates newStatus) {
    gameStatus = newStatus;
}

void initCore(void) {
    defaultLogger = writeToLog;
    /* prepare for a single player in the game */
    memFill(&party[0].position, 0, sizeof(struct WorldPosition));
    setErrorHandlerCallback(NULL);

    collectedObject = &collectedObjectHead;
    memFill(collectedObject, 0, sizeof(struct ObjectNode));
    itemsCount = 0;
    roomCount = 1; /* there's an implicit dummy first */
    gameStatus = kNormalGameplay;

    party[0].location = 1;
    party[0].rank = 0;
    party[0].direction = kNorth;
    party[0].position.x = 15;
    party[0].position.y = 15;

    party[0].name = "Lako";
    party[0].inParty = 1;
    party[0].hp = 50;
    party[0].defense = 4;
    party[0].attack = 4;
    party[0].agility = 4;
    party[0].wisdom = 4;
    party[0].energy = 10;
    party[0].level = 1;
    party[0].specialStype = kNone;
    party[0].kills = 0;

    party[1].name = "Omar";
    party[1].inParty = 0;
    party[1].hp = 50;
    party[1].energy = 10;
    party[1].defense = 6;
    party[1].attack = 3;
    party[1].agility = 2;
    party[1].wisdom = 4;
    party[1].level = 1;
    party[1].specialStype = kHeal;
    party[1].kills = 0;

    party[2].name = "Lena";
    party[2].inParty = 0;
    party[2].hp = 50;
    party[2].energy = 10;
    party[2].defense = 3;
    party[2].attack = 8;
    party[2].agility = 1;
    party[2].wisdom = 3;
    party[2].level = 1;
    party[2].specialStype = kOffense;
    party[2].kills = 0;

    party[3].name = "Juka";
    party[3].inParty = 0;
    party[3].hp = 50;
    party[3].energy = 10;
    party[3].defense = 3;
    party[3].attack = 2;
    party[3].agility = 4;
    party[3].wisdom = 6;
    party[3].level = 1;
    party[3].specialStype = kOffense;
    party[3].kills = 0;

    party[4].name = "Nico";
    party[4].inParty = 0;
    party[4].hp = 5;
    party[4].energy = 0;
    party[4].defense = 1;
    party[4].attack = 1;
    party[4].agility = 1;
    party[4].wisdom = 1;
    party[4].level = 1;
    party[4].specialStype = kNone;
    party[4].kills = 0;


    memFill(&rooms, 0, TOTAL_ROOMS * sizeof(struct Room));
    memFill(&item, 0, TOTAL_ITEMS * sizeof(struct Item));
}




