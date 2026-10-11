#include <gtest/gtest.h>
#include <cstring>
#include "score/Session.hpp"
#include "score/Workout.hpp"
using namespace score;

TEST(Workout, PresetIdsAreContiguous) {
    EXPECT_EQ(kPresetCount, 15);
    for (uint8_t i = 0; i < kPresetCount; ++i) {
        const Workout& w = preset(i);
        EXPECT_EQ(w.id, i + 1);
        EXPECT_EQ(findPreset(w.id), &w);
    }
    EXPECT_EQ(findPreset(0), nullptr);
    EXPECT_EQ(findPreset(16), nullptr);
    EXPECT_EQ(findPreset(99), nullptr);
    EXPECT_EQ(&preset(200), &preset(kPresetCount - 1));   // out of range clamps to the last
}
TEST(Workout, PresetContents) {
    struct Row { const char* name; uint8_t steps; uint16_t volume; Spot firstSpot; uint8_t firstRest; };
    // The approved table (docs/superpowers/specs/2026-10-11-hoops-0.1.0-design.md, Part A), in id order.
    const Row rows[] = {
        { "Around the World S", 5,  50, Spot::LCorner3,     15 },
        { "Free Throw S",       1,  50, Spot::Ft,            0 },
        { "Mikan Layups S",     2,  50, Spot::LayupL,        0 },
        { "Mid-range Ladder S", 5,  50, Spot::LBaselineMid, 15 },
        { "Game Shots S",       5,  50, Spot::Top3,         30 },
        { "Warm-up 50",         5,  50, Spot::LayupL,        0 },
        { "FT Sets 5x10",       5,  50, Spot::Ft,           30 },
        { "Elbows & Key",       3,  50, Spot::LElbowMid,    15 },
        { "Around the World L", 5, 100, Spot::LCorner3,     30 },
        { "Free Throw L",       2, 100, Spot::Ft,           60 },
        { "Mikan Layups L",     2, 100, Spot::LayupL,       30 },
        { "Mid-range Ladder L", 5, 100, Spot::LBaselineMid, 30 },
        { "Game Shots M",       7, 100, Spot::Top3,         15 },
        { "Corner 3s",          4, 100, Spot::LCorner3,     15 },
        { "Pro Session 200",    8, 200, Spot::Top3,         30 },
    };
    ASSERT_EQ(sizeof(rows) / sizeof(rows[0]), static_cast<size_t>(kPresetCount));
    for (uint8_t i = 0; i < kPresetCount; ++i) {
        SCOPED_TRACE(rows[i].name);
        const Workout& w = preset(i);
        EXPECT_STREQ(w.name, rows[i].name);
        EXPECT_EQ(w.stepCount, rows[i].steps);
        EXPECT_EQ(w.totalShots(), rows[i].volume);
        EXPECT_EQ(w.steps[0].spot, static_cast<uint8_t>(rows[i].firstSpot));
        EXPECT_EQ(w.steps[0].rest_s, rows[i].firstRest);
    }
    // Mid-range Ladder S counts makes; Game Shots S passes the free-throw line third.
    EXPECT_EQ(preset(3).steps[0].goal.type, static_cast<uint8_t>(GoalType::Makes));
    EXPECT_EQ(preset(4).steps[2].spot, static_cast<uint8_t>(Spot::Ft));
}
TEST(Workout, TotalsAndEstimate) {
    ASSERT_NE(findPreset(5), nullptr);
    ASSERT_NE(findPreset(15), nullptr);
    const Workout& g = *findPreset(5);       // Game Shots S: 5 × 10 attempts, rest 30
    EXPECT_EQ(g.stepCount, 5);
    EXPECT_EQ(g.totalShots(), 50);
    EXPECT_EQ(g.estimateSeconds(), 750u);    // 50 × 12 + 5 × 30
    const Workout& p = *findPreset(15);      // Pro Session 200: 8 steps, rest 30
    EXPECT_EQ(p.stepCount, 8);
    EXPECT_EQ(p.totalShots(), 200);
    EXPECT_EQ(p.estimateSeconds(), 2640u);   // 200 × 12 + 8 × 30
    Workout t{}; t.stepCount = 1; t.steps[0] = { 0, 0, { static_cast<uint8_t>(GoalType::Time), 0, 60 } };
    EXPECT_EQ(t.totalShots(), 0);
    EXPECT_EQ(t.estimateSeconds(), 60u);
}
TEST(Workout, AllPresetsValid) {
    for (uint8_t i = 0; i < kPresetCount; ++i) {
        const Workout& w = preset(i);
        SCOPED_TRACE(w.name);
        EXPECT_TRUE(w.validSteps());
        EXPECT_LE(w.totalShots(), kMaxShots);
        EXPECT_NE(std::memchr(w.name, '\0', kNameLen), nullptr);   // NUL-terminated within kNameLen
        EXPECT_GT(std::strlen(w.name), 0u);
        for (uint8_t s = 0; s < w.stepCount; ++s) {
            if (w.steps[s].goal.type == static_cast<uint8_t>(GoalType::Attempts)) {
                // 50 = Ring::kMaxSegments (the segmented ring's limit; the GUI header is not includable here).
                EXPECT_LE(w.steps[s].goal.value, 50);
            }
        }
    }
}
TEST(Workout, ValidSteps) {
    Workout w{};
    EXPECT_FALSE(w.validSteps());   // no steps
    w.stepCount = 2;
    w.steps[0] = { static_cast<uint8_t>(Spot::Ft), 0, { static_cast<uint8_t>(GoalType::Attempts), 0, 10 } };
    w.steps[1] = { static_cast<uint8_t>(Spot::Top3), 255, { static_cast<uint8_t>(GoalType::Time), 0, 60 } };
    EXPECT_TRUE(w.validSteps());
    w.steps[2].spot = 0xEE;          // unused steps are not looked at
    EXPECT_TRUE(w.validSteps());
    w.steps[1].spot = kSpotCount;                                       EXPECT_FALSE(w.validSteps());
    w.steps[1].spot = kSpotCount - 1;                                   EXPECT_TRUE(w.validSteps());
    w.steps[1].goal.type = static_cast<uint8_t>(GoalType::Time) + 1;    EXPECT_FALSE(w.validSteps());
    w.steps[1].goal.type = static_cast<uint8_t>(GoalType::Makes);       EXPECT_TRUE(w.validSteps());
    w.steps[0].goal.value = 0;                                          EXPECT_FALSE(w.validSteps());
    w.steps[0].goal.value = 1;                                          EXPECT_TRUE(w.validSteps());
    w.stepCount = kMaxSteps + 1;                                        EXPECT_FALSE(w.validSteps());
}
