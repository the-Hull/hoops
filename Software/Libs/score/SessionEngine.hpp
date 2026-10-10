/**
 * @file  SessionEngine.hpp
 * @brief Session state machine: shots, undo, spot changes, pause, timing, HR.
 */

#pragma once

#include "score/Session.hpp"
#include "score/Spots.hpp"
#include "score/Workout.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace score {

/// Live view of the engine; POD, copied verbatim into IPC messages.
struct Snapshot {
    uint8_t  state, mode;              // SessionState; 0 open, 1 workout
    uint8_t  spot, stepIndex, stepCount, goalType;
    uint16_t goalValue;
    uint16_t stepMakes, stepAttempts, spotMakes, spotAttempts, totalMakes, totalAttempts;
    uint8_t  last5Count, last5Bits;    // bit0 = most recent, 1 = made
    uint8_t  pad0[2];
    uint32_t elapsed_s;
    uint16_t restRemaining_s, stepElapsed_s;
    uint8_t  hr, flags, nextSpot, pad;
    uint16_t workoutId;
    uint8_t  pad1[2];
    static constexpr uint8_t kFull = 1, kWorkoutDone = 2, kResumable = 4;   ///< flags
    static constexpr uint8_t kModeOpen = 0, kModeWorkout = 1;                ///< mode
};
static_assert(std::is_trivially_copyable<Snapshot>::value, "Snapshot must stay POD");
static_assert(sizeof(Snapshot) <= 48, "Snapshot must fit an IPC message");
static_assert(offsetof(Snapshot, elapsed_s) == 24, "Snapshot layout changed");
static_assert(std::has_unique_object_representations<Snapshot>::value, "Snapshot: no padding");

class SessionEngine {
public:
    enum class Event : uint8_t { None, StepDone, RestOver, WorkoutDone };
    struct StepResult { uint8_t stepIndex; uint16_t makes, attempts, rest_s; };

    bool  startOpen(Spot spot, uint32_t nowUnix);            ///< from Idle/Ended only
    bool  startWorkout(const Workout& w, uint32_t nowUnix);  ///< from Idle/Ended only
    bool  restore(const Session& saved, const Workout* w);   ///< → Paused, flags resumable
    Event shot(bool made);                                   ///< Active only
    bool  undo(bool& madeOut);                               ///< Active, within current step
    bool  changeSpot(Spot spot);                             ///< open session, Active only
    bool  pause();  bool resume();
    Event tick();                                            ///< call once per second
    bool  skipRest();  bool extendRest(uint16_t seconds);
    void  addHr(uint8_t bpm);                                ///< 0 ignored
    const Session& end();
    SessionState state() const;  bool isActive() const;      ///< Active/Paused/Rest
    const Session& session() const;  const Workout* workout() const;
    const StepResult& lastStep() const;
    Snapshot snapshot() const;

private:
    Event completeStep();  void advanceStep();  const Step& step() const;
    uint16_t stepMakes() const;
    Session mSession{}; Workout mWorkout{}; bool mHasWorkout = false; bool mResumable = false;
    StepResult mLastStep{}; uint8_t mLastHr = 0;
};

} // namespace score
