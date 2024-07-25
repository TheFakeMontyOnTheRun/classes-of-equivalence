//
// Created by Daniel Monteiro on 2019-07-26.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <assert.h>
#include <stdint.h>

extern "C" {
#include "Enums.h"
#include "Core.h"
#include "Derelict.h"
#include "Parser.h"
}

extern struct ObjectNode *collectedObject;

using testing::Eq;

extern struct Item item[TOTAL_ITEMS];
extern struct Room rooms[TOTAL_ROOMS];

TEST(TestVictoryCondition,
     placingTheBombOnLab1WithoutTheMendingWillExplodeStationWhileAtNotAtShipWillCauseYouToGoDownWithIt) {

    initStation();

    parseCommand("move", "0");
    ASSERT_FALSE(isPlayerAtRoom("lss-daedalus"));

    addObjectToRoom(getRoomIdByName("reactor-core"), getItemNamed("emp-bomb"));
    addObjectToRoom(getRoomIdByName("lss-daedalus"), getItemNamed("fuel-rods"));

    ASSERT_EQ(kNormalGameplay, getGameStatus());
    getItemNamed("emp-controller")->useCallback(NULL);
    ASSERT_EQ(kBadVictory, getGameStatus());
}

TEST(TestVictoryCondition,
     placingTheBombOnLab1WithTheMendingWillFailToExplodeStationProperlyWithYouAtTheShipSurvivingAndLosingYourLicense) {

    initStation();

    ASSERT_TRUE(isPlayerAtRoom("lss-daedalus"));

    ASSERT_FALSE(hasItemInRoom("reactor-core", "emp-bomb"));
    ASSERT_TRUE(hasItemInRoom("reactor-core", "fuel-rods"));

    ASSERT_EQ(kNormalGameplay, getGameStatus());
    getItemNamed("emp-controller")->useCallback(NULL);
    ASSERT_EQ(kBadGameOver, getGameStatus());
}

TEST(TestVictoryCondition, placingTheEmpBombOnOtherPlaceAndHaveThePlayerStillOnTheShipWillCauseYouToFailAndDie) {

    initStation();
    parseCommand("move", "0");

    ASSERT_FALSE(isPlayerAtRoom("lss-daedalus"));

    addObjectToRoom(getRoomIdByName("elevator-level-1"), getItemNamed("emp-bomb"));

    ASSERT_TRUE(hasItemInRoom("reactor-core", "fuel-rods"));

    ASSERT_EQ(kNormalGameplay, getGameStatus());
    getItemNamed("emp-controller")->useCallback(NULL);
    ASSERT_EQ(kBadGameOver, getGameStatus());
}