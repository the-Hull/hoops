#include <gtest/gtest.h>
#include <cstring>
#include "score/Workout.hpp"
using namespace score;

TEST(Workout, PresetsAreWellFormed) {
    for (uint8_t i = 0; i < kPresetCount; ++i) {
        const Workout& w = preset(i);
        EXPECT_EQ(w.id, i + 1);
        EXPECT_GT(std::strlen(w.name), 0u);
        ASSERT_GE(w.stepCount, 1); ASSERT_LE(w.stepCount, kMaxSteps);
        for (uint8_t s = 0; s < w.stepCount; ++s) {
            EXPECT_LT(w.steps[s].spot, kSpotCount);
            EXPECT_GT(w.steps[s].goal.value, 0);
        }
        EXPECT_EQ(findPreset(w.id), &w);
    }
    EXPECT_EQ(findPreset(0), nullptr);
    EXPECT_EQ(findPreset(99), nullptr);
}
TEST(Workout, PresetContents) {
    EXPECT_STREQ(preset(0).name, "Around the World"); EXPECT_EQ(preset(0).stepCount, 5);
    EXPECT_EQ(preset(0).steps[0].spot, static_cast<uint8_t>(Spot::LCorner3));
    EXPECT_STREQ(preset(1).name, "Free Throw 50");    EXPECT_EQ(preset(1).steps[0].goal.value, 50);
    EXPECT_STREQ(preset(2).name, "Mikan Layups");     EXPECT_EQ(preset(2).stepCount, 2);
    EXPECT_STREQ(preset(3).name, "Mid-range Ladder"); EXPECT_EQ(preset(3).steps[0].goal.type, static_cast<uint8_t>(GoalType::Makes));
    EXPECT_STREQ(preset(4).name, "Game Shots");       EXPECT_EQ(preset(4).stepCount, 4);
    EXPECT_EQ(preset(4).steps[2].spot, static_cast<uint8_t>(Spot::Ft));
}
TEST(Workout, TotalsAndEstimate) {
    const Workout& g = preset(4);            // 4 × 10 attempts, rest 30
    EXPECT_EQ(g.totalShots(), 40);
    EXPECT_EQ(g.estimateSeconds(), 40u * 12u + 4u * 30u);
    Workout t{}; t.stepCount = 1; t.steps[0] = { 0, { static_cast<uint8_t>(GoalType::Time), 60 }, 0 };
    EXPECT_EQ(t.totalShots(), 0);
    EXPECT_EQ(t.estimateSeconds(), 60u);
}
