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

static Workout two() { Workout w{}; w.id = 7; w.stepCount = 2;
  w.steps[0] = { (uint8_t)Spot::Ft,   10, { (uint8_t)GoalType::Attempts, 0, 3 } };
  w.steps[1] = { (uint8_t)Spot::Top3, 0,  { (uint8_t)GoalType::Makes,    0, 2 } }; return w; }
TEST(EngineWorkout, AttemptsGoalEntersRest) { SessionEngine e; e.startWorkout(two(), 1);
  EXPECT_EQ(e.snapshot().mode, 1); EXPECT_EQ(e.snapshot().stepCount, 2); EXPECT_EQ(e.snapshot().goalValue, 3);
  e.shot(true); e.shot(false); EXPECT_EQ(e.shot(true), SessionEngine::Event::StepDone);
  EXPECT_EQ(e.state(), SessionState::Rest); auto s = e.snapshot(); EXPECT_EQ(s.restRemaining_s, 10); EXPECT_EQ(s.nextSpot, (uint8_t)Spot::Top3);
  EXPECT_EQ(e.lastStep().makes, 2); EXPECT_EQ(e.lastStep().attempts, 3); EXPECT_EQ(e.lastStep().rest_s, 10);
  EXPECT_EQ(e.shot(true), SessionEngine::Event::None); }   // no shots in rest
TEST(EngineWorkout, RestCountsDownThenAdvances) { SessionEngine e; e.startWorkout(two(), 1); e.shot(true); e.shot(true); e.shot(true);
  for (int i = 0; i < 9; ++i) EXPECT_EQ(e.tick(), SessionEngine::Event::None);
  EXPECT_EQ(e.tick(), SessionEngine::Event::RestOver); EXPECT_EQ(e.state(), SessionState::Active);
  auto s = e.snapshot(); EXPECT_EQ(s.stepIndex, 1); EXPECT_EQ(s.spot, (uint8_t)Spot::Top3); EXPECT_EQ(s.stepAttempts, 0); EXPECT_EQ(s.totalAttempts, 3); EXPECT_EQ(s.elapsed_s, 10u); }
TEST(EngineWorkout, SkipAndExtendRest) { SessionEngine e; e.startWorkout(two(), 1); e.shot(true); e.shot(true); e.shot(true);
  EXPECT_TRUE(e.extendRest(15)); EXPECT_EQ(e.snapshot().restRemaining_s, 25); EXPECT_TRUE(e.skipRest()); EXPECT_EQ(e.snapshot().stepIndex, 1); EXPECT_FALSE(e.skipRest()); }
TEST(EngineWorkout, MakesGoalAndWorkoutDone) { SessionEngine e; e.startWorkout(two(), 1); e.shot(true); e.shot(true); e.shot(true); e.skipRest();
  e.shot(false); e.shot(true); EXPECT_EQ(e.shot(true), SessionEngine::Event::WorkoutDone);
  EXPECT_EQ(e.state(), SessionState::Active); EXPECT_TRUE(e.snapshot().flags & Snapshot::kWorkoutDone);
  EXPECT_EQ(e.shot(true), SessionEngine::Event::None); EXPECT_EQ(e.snapshot().totalAttempts, 7); } // extra shots allowed
TEST(EngineWorkout, UndoStopsAtStepBoundary) { SessionEngine e; e.startWorkout(two(), 1); e.shot(true); e.shot(true); e.shot(true); e.skipRest();
  bool m; EXPECT_FALSE(e.undo(m)); e.shot(false); EXPECT_TRUE(e.undo(m)); EXPECT_FALSE(e.undo(m)); }
TEST(EngineWorkout, TimeGoal) { Workout w{}; w.stepCount = 1; w.steps[0] = { (uint8_t)Spot::Ft, 0, { (uint8_t)GoalType::Time, 0, 5 } };
  SessionEngine e; e.startWorkout(w, 1); for (int i = 0; i < 4; ++i) EXPECT_EQ(e.tick(), SessionEngine::Event::None);
  EXPECT_EQ(e.tick(), SessionEngine::Event::WorkoutDone); }
TEST(EngineWorkout, ZeroRestAdvancesImmediately) { Workout w = two(); w.steps[0].rest_s = 0; SessionEngine e; e.startWorkout(w, 1);
  e.shot(true); e.shot(true); EXPECT_EQ(e.shot(true), SessionEngine::Event::StepDone); EXPECT_EQ(e.state(), SessionState::Active); EXPECT_EQ(e.snapshot().stepIndex, 1); }
TEST(EngineWorkout, PauseDuringRestResumesRest) { SessionEngine e; e.startWorkout(two(), 1); e.shot(true); e.shot(true); e.shot(true);
  e.pause(); e.tick(); EXPECT_EQ(e.snapshot().restRemaining_s, 10); e.resume(); EXPECT_EQ(e.state(), SessionState::Rest); }
TEST(EngineWorkout, ChangeSpotRefusedInWorkout) { SessionEngine e; e.startWorkout(two(), 1); EXPECT_FALSE(e.changeSpot(Spot::LayupL)); }
TEST(EngineRestore, ComesBackPaused) { SessionEngine a; Workout w = two(); a.startWorkout(w, 1); a.shot(true); a.tick();
  SessionEngine b; ASSERT_TRUE(b.restore(a.session(), &w)); EXPECT_EQ(b.state(), SessionState::Paused);
  EXPECT_TRUE(b.snapshot().flags & Snapshot::kResumable); b.resume(); EXPECT_EQ(b.state(), SessionState::Active);
  EXPECT_FALSE(b.snapshot().flags & Snapshot::kResumable); EXPECT_EQ(b.snapshot().totalAttempts, 1); EXPECT_EQ(b.snapshot().stepCount, 2); }
TEST(EngineRestore, WithoutWorkoutBecomesOpen) { SessionEngine a; a.startWorkout(two(), 1); a.shot(true);
  SessionEngine b; ASSERT_TRUE(b.restore(a.session(), nullptr)); EXPECT_EQ(b.snapshot().mode, 0); b.resume(); EXPECT_TRUE(b.changeSpot(Spot::LayupL)); }
TEST(EngineRestore, EndedRefused) { SessionEngine a; a.startOpen(Spot::Ft, 1); a.end(); SessionEngine b; EXPECT_FALSE(b.restore(a.session(), nullptr)); }
TEST(EngineRestore, RestWithoutWorkoutResumesOpenActive) { SessionEngine a; a.startWorkout(two(), 1); a.shot(true); a.shot(true); a.shot(true);
  ASSERT_EQ(a.state(), SessionState::Rest);
  SessionEngine b; ASSERT_TRUE(b.restore(a.session(), nullptr)); b.resume();
  EXPECT_EQ(b.state(), SessionState::Active); EXPECT_EQ(b.snapshot().mode, 0); EXPECT_EQ(b.snapshot().spot, (uint8_t)Spot::Ft);
  EXPECT_EQ(b.tick(), SessionEngine::Event::None); EXPECT_EQ(b.snapshot().spot, (uint8_t)Spot::Ft); EXPECT_EQ(b.snapshot().stepIndex, 0);
  EXPECT_EQ(b.snapshot().restRemaining_s, 0); bool m; EXPECT_FALSE(b.undo(m)); }
TEST(EngineRestore, PausedFromRestWithoutWorkoutResumesActive) { SessionEngine a; a.startWorkout(two(), 1); a.shot(true); a.shot(true); a.shot(true); a.pause();
  SessionEngine b; ASSERT_TRUE(b.restore(a.session(), nullptr)); b.resume();
  EXPECT_EQ(b.state(), SessionState::Active); EXPECT_EQ(b.tick(), SessionEngine::Event::None); EXPECT_EQ(b.snapshot().spot, (uint8_t)Spot::Ft); }
TEST(EngineRestore, InvalidPausedFromFallsBackToActive) { SessionEngine a; a.startOpen(Spot::Ft, 1); a.shot(true);
  for (SessionState bad : { SessionState::Paused, SessionState::Ended, SessionState::Idle }) {
    Session s = a.session(); s.state = (uint8_t)SessionState::Paused; s.pausedFrom = (uint8_t)bad;
    SessionEngine b; ASSERT_TRUE(b.restore(s, nullptr)); EXPECT_TRUE(b.resume()); EXPECT_EQ(b.state(), SessionState::Active); } }
TEST(EngineRestore, RestOnLastStepRefused) { SessionEngine a; Workout w = two(); a.startWorkout(w, 1); a.shot(true); a.shot(true); a.shot(true);
  Session s = a.session(); s.stepIndex = 1; SessionEngine b; EXPECT_FALSE(b.restore(s, &w)); }
TEST(EngineWorkout, UndoCannotRemoveWorkoutCompletingShot) { SessionEngine e; e.startWorkout(two(), 1); e.shot(true); e.shot(true); e.shot(true); e.skipRest();
  e.shot(true); ASSERT_EQ(e.shot(true), SessionEngine::Event::WorkoutDone);
  bool m; EXPECT_FALSE(e.undo(m)); e.shot(true); EXPECT_TRUE(e.undo(m)); EXPECT_FALSE(e.undo(m)); }
TEST(EngineWorkout, NextSpotShownWhilePausedInRest) { SessionEngine e; e.startWorkout(two(), 1); e.shot(true); e.shot(true); e.shot(true);
  e.pause(); EXPECT_EQ(e.snapshot().nextSpot, (uint8_t)Spot::Top3); }
