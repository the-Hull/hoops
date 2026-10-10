#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include "score/History.hpp"
#include "score/Serializer.hpp"
using namespace score;

namespace {
constexpr uint8_t idx(Spot s) { return static_cast<uint8_t>(s); }
constexpr uint8_t idx(Category c) { return static_cast<uint8_t>(c); }

SessionSummary sum(uint32_t id) {
    SessionSummary s{};
    s.id = id;
    return s;
}
Workout named(const char* name) {
    Workout w{};
    std::snprintf(w.name, sizeof w.name, "%s", name);
    w.stepCount = 1;
    for (Step& st : w.steps) st = { static_cast<uint8_t>(Spot::Ft), 0, { 0, 0, 10 } };   // all in range
    return w;
}
} // namespace

TEST(History, SummarizeSplitsByCategory) {
    Session s{};
    s.id = 77; s.workoutId = 2; s.elapsed_s = 300;
    const struct { Spot spot; uint8_t made; } shots[] = {
        { Spot::LWing3, 1 }, { Spot::Top3, 0 }, { Spot::RCorner3, 1 },   // 3PT 2/3
        { Spot::LElbowMid, 0 },                                          // Mid 0/1
        { Spot::Ft, 1 }, { Spot::Ft, 1 },                                // FT 2/2
        { Spot::LayupL, 1 }, { Spot::LayupR, 0 },                        // Layup 1/2
    };
    for (const auto& sh : shots) { s.shots[s.count] = { idx(sh.spot), sh.made, s.count }; ++s.count; }
    s.shots[s.count] = { idx(Spot::Ft), 1, 0 };   // beyond count: ignored

    const SessionSummary r = summarize(s, 20261010);
    EXPECT_EQ(r.id, 77u);
    EXPECT_EQ(r.ymd, 20261010u);
    EXPECT_EQ(r.workoutId, 2);
    EXPECT_EQ(r.elapsed_s, 300u);
    EXPECT_EQ(r.catMakes[idx(Category::Three)], 2); EXPECT_EQ(r.catAtt[idx(Category::Three)], 3);
    EXPECT_EQ(r.catMakes[idx(Category::Mid)], 0);   EXPECT_EQ(r.catAtt[idx(Category::Mid)], 1);
    EXPECT_EQ(r.catMakes[idx(Category::Ft)], 2);    EXPECT_EQ(r.catAtt[idx(Category::Ft)], 2);
    EXPECT_EQ(r.catMakes[idx(Category::Layup)], 1); EXPECT_EQ(r.catAtt[idx(Category::Layup)], 2);
    EXPECT_EQ(r.totalPct, 63);   // 5/8 = 62.5 → 63
    EXPECT_EQ(r.pad[0] | r.pad[1] | r.pad[2], 0);
    EXPECT_EQ(r.pad0[0] | r.pad0[1], 0);
    uint8_t raw[sizeof r];
    std::memcpy(raw, &r, sizeof r);
    EXPECT_EQ(raw[26], 0);   // pad0, before elapsed_s
    EXPECT_EQ(raw[27], 0);
    EXPECT_EQ(raw[28], 300 & 0xFF);   // elapsed_s (little-endian)
}
TEST(History, SummarizeEmptySession) {
    const Session s{};
    const SessionSummary r = summarize(s, 1);
    EXPECT_EQ(r.totalPct, 0);
    for (uint8_t c = 0; c < kCategoryCount; ++c) EXPECT_EQ(r.catAtt[c], 0);
}
TEST(History, PushKeepsNewestLast) {
    SessionIndex ix{};
    EXPECT_EQ(ix.push(sum(1)), 0u);
    EXPECT_EQ(ix.push(sum(2)), 0u);
    EXPECT_EQ(ix.push(sum(3)), 0u);
    EXPECT_EQ(ix.count, 3);
    EXPECT_EQ(ix.items[0].id, 1u);
    EXPECT_EQ(ix.items[2].id, 3u);
    ASSERT_NE(ix.newest(0), nullptr); EXPECT_EQ(ix.newest(0)->id, 3u);
    ASSERT_NE(ix.newest(2), nullptr); EXPECT_EQ(ix.newest(2)->id, 1u);
    EXPECT_EQ(ix.newest(3), nullptr);
}
TEST(History, PushSameIdReplacesRow) {
    SessionIndex ix{};
    ix.push(sum(1)); ix.push(sum(2)); ix.push(sum(3));
    SessionSummary again = sum(2);
    again.totalPct = 77;
    EXPECT_EQ(ix.push(again), 0u);
    EXPECT_EQ(ix.count, 3);
    EXPECT_EQ(ix.items[1].id, 2u);           // in place, not appended
    EXPECT_EQ(ix.items[1].totalPct, 77);
    EXPECT_EQ(ix.newest(0)->id, 3u);
    // Full index: a repeat still evicts nothing.
    SessionIndex full{};
    for (uint32_t id = 1; id <= kIndexCapacity; ++id) full.push(sum(id));
    EXPECT_EQ(full.push(sum(1)), 0u);
    EXPECT_EQ(full.count, kIndexCapacity);
    EXPECT_EQ(full.items[0].id, 1u);
}
TEST(History, NewestOnEmpty) {
    const SessionIndex ix{};
    EXPECT_EQ(ix.newest(0), nullptr);
}
TEST(History, PushEvictsOldestWhenFull) {
    SessionIndex ix{};
    for (uint32_t id = 1; id <= kIndexCapacity; ++id) EXPECT_EQ(ix.push(sum(id)), 0u);
    EXPECT_EQ(ix.count, kIndexCapacity);
    EXPECT_EQ(ix.push(sum(51)), 1u);
    EXPECT_EQ(ix.count, kIndexCapacity);
    EXPECT_EQ(ix.items[0].id, 2u);
    EXPECT_EQ(ix.newest(0)->id, 51u);
    EXPECT_EQ(ix.find(1), nullptr);
    EXPECT_EQ(ix.push(sum(52)), 2u);
    EXPECT_EQ(ix.newest(ix.count), nullptr);
    EXPECT_EQ(ix.newest(ix.count - 1)->id, 3u);
}
TEST(History, FindAndRemoveCompacts) {
    SessionIndex ix{};
    for (uint32_t id = 10; id <= 14; ++id) ix.push(sum(id));
    ASSERT_NE(ix.find(12), nullptr); EXPECT_EQ(ix.find(12)->id, 12u);
    EXPECT_EQ(ix.find(99), nullptr);
    EXPECT_TRUE(ix.remove(12));
    EXPECT_EQ(ix.count, 4);
    EXPECT_EQ(ix.find(12), nullptr);
    const uint32_t expect[] = { 10, 11, 13, 14 };
    for (uint8_t i = 0; i < 4; ++i) EXPECT_EQ(ix.items[i].id, expect[i]);
    EXPECT_FALSE(ix.remove(12));
    EXPECT_EQ(ix.count, 4);
    EXPECT_TRUE(ix.remove(14));
    EXPECT_EQ(ix.newest(0)->id, 13u);
}
namespace {
struct Spots13 { SpotStat a[kSpotCount]; };
Spots13 one(Spot s, uint16_t makes, uint16_t attempts) {
    Spots13 r{};
    r.a[idx(s)] = { makes, attempts };
    return r;
}
} // namespace

TEST(History, RecordSpotsSetsLastAndBest) {
    SessionIndex ix{};
    ix.recordSpots(one(Spot::Ft, 4, 5).a);
    EXPECT_EQ(ix.last[idx(Spot::Ft)].makes, 4);
    EXPECT_EQ(ix.last[idx(Spot::Ft)].attempts, 5);
    EXPECT_EQ(ix.best[idx(Spot::Ft)].makes, 4);
    EXPECT_EQ(ix.best[idx(Spot::Ft)].attempts, 5);
}
TEST(History, LastAlwaysReplaced) {
    SessionIndex ix{};
    ix.recordSpots(one(Spot::Top3, 8, 10).a);
    ix.recordSpots(one(Spot::Top3, 1, 10).a);   // worse
    EXPECT_EQ(ix.last[idx(Spot::Top3)].makes, 1);
    EXPECT_EQ(ix.last[idx(Spot::Top3)].attempts, 10);
    EXPECT_EQ(ix.best[idx(Spot::Top3)].makes, 8);
    EXPECT_EQ(ix.best[idx(Spot::Top3)].attempts, 10);
    ix.recordSpots(one(Spot::Top3, 9, 10).a);   // better
    EXPECT_EQ(ix.best[idx(Spot::Top3)].makes, 9);
}
TEST(History, BestNeedsRankThreshold) {
    SessionIndex ix{};
    ix.recordSpots(one(Spot::LayupL, 3, 4).a);   // 4 < kMinAttemptsForRank
    EXPECT_EQ(ix.last[idx(Spot::LayupL)].attempts, 4);
    EXPECT_EQ(ix.best[idx(Spot::LayupL)].makes, 0);
    EXPECT_EQ(ix.best[idx(Spot::LayupL)].attempts, 0);
    ix.recordSpots(one(Spot::LayupL, 3, 5).a);
    EXPECT_EQ(ix.best[idx(Spot::LayupL)].makes, 3);
    EXPECT_EQ(ix.best[idx(Spot::LayupL)].attempts, 5);
}
TEST(History, BestTieKeepsOlder) {
    SessionIndex ix{};
    ix.recordSpots(one(Spot::Ft, 5, 10).a);    // 50 %
    ix.recordSpots(one(Spot::Ft, 10, 20).a);   // 50 % again
    EXPECT_EQ(ix.best[idx(Spot::Ft)].makes, 5);
    EXPECT_EQ(ix.best[idx(Spot::Ft)].attempts, 10);
    EXPECT_EQ(ix.last[idx(Spot::Ft)].attempts, 20);
}
TEST(History, BestUpgradesFromUnranked) {
    SessionIndex ix{};
    ix.best[idx(Spot::Ft)] = { 2, 3 };         // unranked (67 %), e.g. from an older rule
    ix.recordSpots(one(Spot::Ft, 3, 5).a);     // 60 %, but ranked
    EXPECT_EQ(ix.best[idx(Spot::Ft)].makes, 3);
    EXPECT_EQ(ix.best[idx(Spot::Ft)].attempts, 5);
}
TEST(History, SpotsWithoutAttemptsUntouched) {
    SessionIndex ix{};
    ix.recordSpots(one(Spot::LWing3, 6, 10).a);
    ix.recordSpots(one(Spot::RWing3, 2, 5).a);   // LWing3 has 0 attempts here
    EXPECT_EQ(ix.last[idx(Spot::LWing3)].makes, 6);
    EXPECT_EQ(ix.last[idx(Spot::LWing3)].attempts, 10);
    EXPECT_EQ(ix.best[idx(Spot::LWing3)].makes, 6);
    for (uint8_t s = 0; s < kSpotCount; ++s) {
        if (s == idx(Spot::LWing3) || s == idx(Spot::RWing3)) continue;
        EXPECT_EQ(ix.last[s].attempts, 0) << int(s);
        EXPECT_EQ(ix.best[s].attempts, 0) << int(s);
    }
}
TEST(History, ClearZeroesEverything) {
    SessionIndex ix{};
    for (uint32_t id = 1; id <= 3; ++id) ix.push(sum(id));
    ix.recordSpots(one(Spot::Ft, 7, 10).a);
    ix.clear();
    const SessionIndex zero{};
    EXPECT_EQ(ix.count, 0);
    EXPECT_EQ(std::memcmp(&ix, &zero, sizeof ix), 0);
}
TEST(History, IndexSizeAndMagic) {
    EXPECT_EQ(sizeof(SessionIndex), 4u + 36u * 50u + 4u * 13u * 2u);
    EXPECT_EQ(ser::kMagicIndex, 0x32584948u);
}
TEST(History, IndexValid) {
    SessionIndex ix{};
    EXPECT_TRUE(ix.valid());
    ix.count = kIndexCapacity;
    EXPECT_TRUE(ix.valid());
    ix.count = kIndexCapacity + 1;
    EXPECT_FALSE(ix.valid());
    ix.count = 0xFF;
    EXPECT_FALSE(ix.valid());
}

TEST(WorkoutStore, AddAssignsSequentialIds) {
    WorkoutStore st{};
    uint8_t id = 0;
    ASSERT_TRUE(st.add(named("A"), id)); EXPECT_EQ(id, 100);
    ASSERT_TRUE(st.add(named("B"), id)); EXPECT_EQ(id, 101);
    EXPECT_EQ(st.count, 2);
    EXPECT_EQ(st.items[1].id, 101);
    EXPECT_STREQ(st.items[1].name, "B");
}
TEST(WorkoutStore, AddReusesRemovedId) {
    WorkoutStore st{};
    uint8_t id = 0;
    st.add(named("A"), id); st.add(named("B"), id); st.add(named("C"), id);
    EXPECT_TRUE(st.remove(100));
    EXPECT_EQ(st.count, 2);
    EXPECT_EQ(st.items[0].id, 101);   // compacted
    EXPECT_EQ(st.items[1].id, 102);
    ASSERT_TRUE(st.add(named("D"), id)); EXPECT_EQ(id, 100);
    EXPECT_FALSE(st.remove(100 + kMaxUserWorkouts));
}
TEST(WorkoutStore, RefusesWhenFull) {
    WorkoutStore st{};
    uint8_t id = 0;
    for (uint8_t i = 0; i < kMaxUserWorkouts; ++i) {
        ASSERT_TRUE(st.add(named("W"), id));
        EXPECT_EQ(id, kFirstUserWorkoutId + i);
    }
    id = 0;
    EXPECT_FALSE(st.add(named("X"), id));
    EXPECT_EQ(id, 0);
    EXPECT_EQ(st.count, kMaxUserWorkouts);
}
TEST(WorkoutStore, ReplaceById) {
    WorkoutStore st{};
    uint8_t id = 0;
    st.add(named("A"), id); st.add(named("B"), id);
    Workout w = named("B2");
    w.id = 101; w.stepCount = 3;
    EXPECT_TRUE(st.replace(w));
    EXPECT_STREQ(st.find(101)->name, "B2");
    EXPECT_EQ(st.find(101)->stepCount, 3);
    EXPECT_STREQ(st.find(100)->name, "A");
    w.id = 105;
    EXPECT_FALSE(st.replace(w));
    EXPECT_EQ(st.count, 2);
}
TEST(WorkoutStore, FindAndNextCustomNumber) {
    WorkoutStore st{};
    uint8_t id = 0;
    EXPECT_EQ(st.nextCustomNumber(), 1);
    EXPECT_EQ(st.find(100), nullptr);
    st.add(named("A"), id); st.add(named("B"), id);
    ASSERT_NE(st.find(100), nullptr); EXPECT_STREQ(st.find(100)->name, "A");
    EXPECT_EQ(st.find(1), nullptr);
    EXPECT_EQ(st.nextCustomNumber(), 3);
    char name[kNameLen];
    std::snprintf(name, sizeof name, "Custom %u", static_cast<unsigned>(st.nextCustomNumber()));
    EXPECT_STREQ(name, "Custom 3");
}
TEST(WorkoutStore, Valid) {
    WorkoutStore st{};
    EXPECT_TRUE(st.valid());
    uint8_t id = 0;
    st.add(named("A"), id); st.add(named("B"), id);
    EXPECT_TRUE(st.valid());
    st.items[1].stepCount = kMaxSteps;
    EXPECT_TRUE(st.valid());
    st.items[1].stepCount = 0;          EXPECT_FALSE(st.valid());
    st.items[1].stepCount = kMaxSteps + 1; EXPECT_FALSE(st.valid());
    st.items[1].stepCount = 1;
    st.items[1].id = kFirstUserWorkoutId - 1;                    EXPECT_FALSE(st.valid());
    st.items[1].id = kFirstUserWorkoutId + kMaxUserWorkouts;     EXPECT_FALSE(st.valid());
    st.items[1].id = kFirstUserWorkoutId + kMaxUserWorkouts - 1; EXPECT_TRUE(st.valid());
    st.count = kMaxUserWorkouts + 1;
    EXPECT_FALSE(st.valid());
}
TEST(WorkoutStore, ValidChecksSteps) {
    WorkoutStore st{};
    uint8_t id = 0;
    st.add(named("A"), id);
    st.items[0].stepCount = 2;
    EXPECT_TRUE(st.valid());
    st.items[0].steps[1].spot = kSpotCount;                                     EXPECT_FALSE(st.valid());
    st.items[0].steps[1].spot = 0;                                              EXPECT_TRUE(st.valid());
    st.items[0].steps[1].goal.type = static_cast<uint8_t>(GoalType::Time) + 1;  EXPECT_FALSE(st.valid());
    st.items[0].steps[1].goal.type = static_cast<uint8_t>(GoalType::Time);      EXPECT_TRUE(st.valid());
    st.items[0].steps[1].goal.value = 0;                                        EXPECT_FALSE(st.valid());
    st.items[0].stepCount = 1;                                                  EXPECT_TRUE(st.valid());
}
