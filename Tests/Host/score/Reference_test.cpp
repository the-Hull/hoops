// Format lock between the C++ store and tools/workout-builder.html: writes a known WorkoutStore to
// reference-workouts.bin in the working directory (ctest runs in build-host). Importing it into the
// builder and exporting again must give the same bytes (cmp).
#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include "score/History.hpp"
#include "score/Serializer.hpp"
using namespace score;

namespace {
Step step(Spot spot, GoalType type, uint16_t value, uint8_t rest) {
    return { static_cast<uint8_t>(spot), rest, { static_cast<uint8_t>(type), 0, value } };
}
} // namespace

TEST(Reference, WritesReferenceWorkoutStore) {
    WorkoutStore store{};
    store.count = 2;

    Workout& a = store.items[0];
    a.id = 100;
    std::snprintf(a.name, sizeof a.name, "%s", "Ref Alpha");
    a.stepCount = 3;
    a.steps[0] = step(Spot::Top3,   GoalType::Attempts, 10, 15);
    a.steps[1] = step(Spot::Ft,     GoalType::Makes,     5,  0);
    a.steps[2] = step(Spot::LayupL, GoalType::Time,     60, 30);

    Workout& b = store.items[1];
    b.id = 101;
    const char* betaName = "Ref Beta 19 chars!!";
    ASSERT_EQ(std::strlen(betaName), 19u);
    std::snprintf(b.name, sizeof b.name, "%s", betaName);
    b.stepCount = 8;
    const Spot threes[] = { Spot::LCorner3, Spot::LWing3, Spot::Top3, Spot::RWing3, Spot::RCorner3 };
    for (uint8_t i = 0; i < 5; ++i) b.steps[i] = step(threes[i], GoalType::Attempts, 20, 30);
    b.steps[5] = step(Spot::LBaselineMid, GoalType::Attempts, 25,  0);
    b.steps[6] = step(Spot::RBaselineMid, GoalType::Makes,    10, 60);
    b.steps[7] = step(Spot::Ft,           GoalType::Attempts, 50,  0);

    ASSERT_TRUE(store.valid());

    uint8_t buf[sizeof(ser::Header) + sizeof(WorkoutStore)];
    const size_t n = ser::encode(ser::kMagicWorkouts, &store, sizeof store, buf, sizeof buf);
    ASSERT_EQ(n, 716u);
    EXPECT_EQ(buf[0], 0x48);   // 'H'
    EXPECT_EQ(buf[1], 0x57);   // 'W'
    EXPECT_EQ(buf[2], 0x4B);   // 'K'
    EXPECT_EQ(buf[3], 0x31);   // '1'

    std::FILE* f = std::fopen("reference-workouts.bin", "wb");
    ASSERT_NE(f, nullptr);
    const size_t written = std::fwrite(buf, 1, n, f);
    ASSERT_EQ(std::fclose(f), 0);
    ASSERT_EQ(written, n);
}
