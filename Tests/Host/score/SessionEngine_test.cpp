#include <gtest/gtest.h>
#include "score/SessionEngine.hpp"
using namespace score;

TEST(EngineOpen, StartAndShoot) { SessionEngine e; ASSERT_TRUE(e.startOpen(Spot::LWing3, 1000));
  EXPECT_EQ(e.state(), SessionState::Active); e.shot(true); e.shot(false); e.shot(true);
  auto s = e.snapshot(); EXPECT_EQ(s.mode, 0); EXPECT_EQ(s.spotMakes, 2); EXPECT_EQ(s.spotAttempts, 3);
  EXPECT_EQ(s.totalAttempts, 3); EXPECT_EQ(s.last5Count, 3); EXPECT_EQ(s.last5Bits, 0b101); EXPECT_EQ(e.session().id, 1000u); }
TEST(EngineOpen, StartTwiceRefused) { SessionEngine e; e.startOpen(Spot::Ft, 1); EXPECT_FALSE(e.startOpen(Spot::Ft, 2)); }
TEST(EngineOpen, UndoReturnsRemovedShot) { SessionEngine e; e.startOpen(Spot::Ft, 1); bool made = false;
  EXPECT_FALSE(e.undo(made)); e.shot(true); e.shot(false); EXPECT_TRUE(e.undo(made)); EXPECT_FALSE(made);
  EXPECT_EQ(e.snapshot().totalAttempts, 1); EXPECT_TRUE(e.undo(made)); EXPECT_TRUE(made); EXPECT_FALSE(e.undo(made)); }
TEST(EngineOpen, ChangeSpotKeepsTotals) { SessionEngine e; e.startOpen(Spot::Ft, 1); e.shot(true);
  EXPECT_TRUE(e.changeSpot(Spot::Top3)); e.shot(false); auto s = e.snapshot();
  EXPECT_EQ(s.spot, (uint8_t)Spot::Top3); EXPECT_EQ(s.spotAttempts, 1); EXPECT_EQ(s.totalAttempts, 2); }
TEST(EngineOpen, PauseBlocksShotsAndTime) { SessionEngine e; e.startOpen(Spot::Ft, 1); e.tick(); e.tick();
  EXPECT_TRUE(e.pause()); EXPECT_EQ(e.shot(true), SessionEngine::Event::None); e.tick();
  EXPECT_EQ(e.snapshot().elapsed_s, 2u); EXPECT_EQ(e.snapshot().totalAttempts, 0); EXPECT_FALSE(e.pause());
  EXPECT_TRUE(e.resume()); e.tick(); EXPECT_EQ(e.snapshot().elapsed_s, 3u); }
TEST(EngineOpen, ShotTimestampIsElapsed) { SessionEngine e; e.startOpen(Spot::Ft, 1); for (int i=0;i<7;++i) e.tick(); e.shot(true); EXPECT_EQ(e.session().shots[0].t_s, 7); }
TEST(EngineOpen, FullSession) { SessionEngine e; e.startOpen(Spot::Ft, 1); for (int i=0;i<kMaxShots;++i) e.shot(true);
  EXPECT_EQ(e.snapshot().totalAttempts, kMaxShots); e.shot(true); EXPECT_EQ(e.snapshot().totalAttempts, kMaxShots); EXPECT_TRUE(e.snapshot().flags & Snapshot::kFull); }
TEST(EngineOpen, EndFreezes) { SessionEngine e; e.startOpen(Spot::Ft, 1); e.shot(true); const Session& s = e.end();
  EXPECT_EQ(s.state, (uint8_t)SessionState::Ended); EXPECT_FALSE(e.isActive()); e.shot(true); EXPECT_EQ(e.session().count, 1);
  EXPECT_TRUE(e.startOpen(Spot::Ft, 5)); EXPECT_EQ(e.session().count, 0); }
TEST(EngineOpen, HrAverage) { SessionEngine e; e.startOpen(Spot::Ft, 1); e.addHr(140); e.addHr(160);
  EXPECT_EQ(e.snapshot().hr, 160); EXPECT_EQ(e.session().hrMax, 160); EXPECT_EQ(e.session().hrSum, 300u); EXPECT_EQ(e.session().hrN, 2); }
