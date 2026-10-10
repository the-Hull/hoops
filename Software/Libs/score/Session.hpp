/**
 * @file  Session.hpp
 * @brief Shooting session record: state, HR accumulators and shot log.
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace score {

constexpr uint16_t kMaxShots = 400;

struct Shot {
    uint8_t  spot;   ///< Spot
    uint8_t  made;   ///< 1 = make, 0 = miss
    uint16_t t_s;    ///< seconds since session start
};
static_assert(sizeof(Shot) == 4, "Shot must stay 4 bytes");
static_assert(std::has_unique_object_representations<Shot>::value, "Shot: no padding");

enum class SessionState : uint8_t { Idle, Active, Paused, Rest, Ended };

/// A session is in progress: Active, Paused or in Rest.
constexpr bool isRunning(SessionState s)
{
    return s == SessionState::Active || s == SessionState::Paused || s == SessionState::Rest;
}

struct Session {
    uint32_t id;
    uint16_t workoutId;        ///< 0 = free shooting
    uint16_t count;            ///< shots logged
    uint32_t elapsed_s;
    uint32_t hrSum;
    uint16_t hrN;
    uint8_t  hrMax;
    uint8_t  state;            ///< SessionState
    uint8_t  pausedFrom;       ///< SessionState before pause
    uint8_t  stepIndex;
    uint8_t  spot;             ///< current Spot
    uint8_t  workoutDone;
    uint16_t stepStartCount;   ///< count at current step start
    uint16_t stepElapsed_s;
    uint16_t restRemaining_s;
    uint8_t  pad0[2];          ///< zero; fills tail padding (shots → offset 32)
    Shot     shots[kMaxShots];
};
static_assert(sizeof(Session) == 32 + 4 * kMaxShots, "Session flash layout changed");
static_assert(std::has_unique_object_representations<Session>::value, "Session: no padding");

} // namespace score
