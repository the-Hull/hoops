// Format lock between the C++ store and tools/workout-builder.html: writes a known WorkoutStore to
// reference-workouts.bin in the working directory (ctest runs in build-host). Importing it into the
// builder and exporting again must give the same bytes (cmp). The expected bytes and checksum below
// lock the C++ side: a layout change fails this test and points at the builder's LAYOUT.
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

    // Pin the layout, not just the size: a field reorder (e.g. rest_s before spot) keeps every
    // static_assert happy but changes these bytes, and then the builder's LAYOUT must follow.
    // Header {magic 'HWK1', size 704, FNV-1a 32 of the payload}, store count, then workout 0:
    // id 100, "Ref Alpha" NUL-padded to 20, stepCount 3, step 0 {spot Top3, rest 15, Attempts, 0, 10}.
    const uint8_t expected[] = {
        0x48, 0x57, 0x4B, 0x31,  0xC0, 0x02, 0x00, 0x00,  0xE8, 0xD7, 0x01, 0x3E,   // header
        0x02, 0x00, 0x00, 0x00,                                                     // count, pad
        0x64, 'R', 'e', 'f', ' ', 'A', 'l', 'p', 'h', 'a', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0x03,                                                                       // stepCount
        0x02, 0x0F, 0x00, 0x00, 0x0A, 0x00,                                         // step 0
    };
    static_assert(sizeof expected == 12 + 4 + 28, "header + count + workout 0 through step 0");
    EXPECT_EQ(std::memcmp(buf, expected, sizeof expected), 0);
    // The checksum covers all 704 payload bytes (both workouts, the zeroed rest): computed once
    // from the encoder at 0.1.0 and hard-coded so any change to the bytes fails here.
    EXPECT_EQ(ser::checksum(buf + sizeof(ser::Header), n - sizeof(ser::Header)), 0x3E01D7E8u);

    std::FILE* f = std::fopen("reference-workouts.bin", "wb");
    ASSERT_NE(f, nullptr);
    const size_t written = std::fwrite(buf, 1, n, f);
    ASSERT_EQ(std::fclose(f), 0);
    ASSERT_EQ(written, n);
}
