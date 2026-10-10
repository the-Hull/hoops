#include <gtest/gtest.h>
#include <initializer_list>
#include <utility>
#include "score/Stats.hpp"
using namespace score;

namespace {
constexpr uint8_t idx(Spot s) { return static_cast<uint8_t>(s); }
constexpr uint8_t idx(Category c) { return static_cast<uint8_t>(c); }

Session make(std::initializer_list<std::pair<Spot, bool>> shots) {
    Session s{};
    for (const auto& p : shots) {
        s.shots[s.count] = { idx(p.first), static_cast<uint8_t>(p.second ? 1 : 0), s.count };
        ++s.count;
    }
    return s;
}
} // namespace

TEST(Stats, PctRounding) {
    EXPECT_EQ(pct(1, 3), 33);
    EXPECT_EQ(pct(2, 3), 67);
    EXPECT_EQ(pct(0, 0), 0);
    EXPECT_EQ(pct(5, 10), 50);
}
TEST(Stats, PerSpotAndCategory) {
    const Session s = make({ { Spot::LWing3, true }, { Spot::LWing3, false }, { Spot::LWing3, true },
                             { Spot::Ft, true }, { Spot::Ft, true } });
    const SessionStats st = SessionStats::compute(s);
    EXPECT_EQ(st.spot[idx(Spot::LWing3)].makes, 2);    EXPECT_EQ(st.spot[idx(Spot::LWing3)].attempts, 3);
    EXPECT_EQ(st.spot[idx(Spot::Ft)].makes, 2);        EXPECT_EQ(st.spot[idx(Spot::Ft)].attempts, 2);
    EXPECT_EQ(st.spot[idx(Spot::Top3)].attempts, 0);
    EXPECT_EQ(st.cat[idx(Category::Three)].makes, 2);  EXPECT_EQ(st.cat[idx(Category::Three)].attempts, 3);
    EXPECT_EQ(st.cat[idx(Category::Ft)].makes, 2);     EXPECT_EQ(st.cat[idx(Category::Ft)].attempts, 2);
    EXPECT_EQ(st.cat[idx(Category::Mid)].attempts, 0);
    EXPECT_EQ(st.total.makes, 4);                      EXPECT_EQ(st.total.attempts, 5);
}
TEST(Stats, LongestStreak) {
    const Session s = make({ { Spot::LCorner3, true }, { Spot::LWing3, true }, { Spot::Top3, false },
                             { Spot::RWing3, true }, { Spot::RCorner3, true }, { Spot::Ft, true },
                             { Spot::LayupL, false } });
    const SessionStats st = SessionStats::compute(s);
    EXPECT_EQ(st.longestStreak, 3);
    EXPECT_EQ(st.streakSpot, idx(Spot::Ft));

    const SessionStats none = SessionStats::compute(make({ { Spot::Ft, false } }));
    EXPECT_EQ(none.longestStreak, 0);
    EXPECT_EQ(none.streakSpot, kNoSpot);
}
TEST(Stats, BestWorstNeedFiveAttempts) {
    const SessionStats few = SessionStats::compute(make({ { Spot::Ft, true }, { Spot::Ft, true },
                                                          { Spot::Ft, true }, { Spot::Ft, false } }));
    EXPECT_EQ(few.bestSpot, kNoSpot);
    EXPECT_EQ(few.worstSpot, kNoSpot);

    const SessionStats st = SessionStats::compute(make({
        { Spot::Ft, true }, { Spot::Ft, true }, { Spot::Ft, true }, { Spot::Ft, true }, { Spot::Ft, true },
        { Spot::Top3, true }, { Spot::Top3, false }, { Spot::Top3, false }, { Spot::Top3, false }, { Spot::Top3, false },
        { Spot::LWing3, false } }));                   // < 5 attempts: ignored despite 0%
    EXPECT_EQ(st.bestSpot, idx(Spot::Ft));
    EXPECT_EQ(st.worstSpot, idx(Spot::Top3));
}
TEST(Stats, ShotsPerMin) {
    Session s{};
    s.elapsed_s = 600;
    s.count = 37;
    EXPECT_EQ(SessionStats::compute(s).shotsPerMin10, 37);
    EXPECT_EQ(SessionStats::compute(s).elapsed_s, 600u);
    s.elapsed_s = 0;
    EXPECT_EQ(SessionStats::compute(s).shotsPerMin10, 0);
}
TEST(Stats, Hr) {
    Session s{};
    s.hrSum = 1410; s.hrN = 10; s.hrMax = 172;
    const SessionStats st = SessionStats::compute(s);
    EXPECT_EQ(st.hrAvg, 141);
    EXPECT_EQ(st.hrMax, 172);
    s.hrSum = 0; s.hrN = 0; s.hrMax = 0;
    EXPECT_EQ(SessionStats::compute(s).hrAvg, 0);
}
