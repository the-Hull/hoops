/**
 ******************************************************************************
 * @file    StepValues.hpp
 * @brief   The goal / rest values a workout step steps through (builder and
 *          Settings), and the first step of a new workout.
 ******************************************************************************
 */

#pragma once

#include <cstdint>

#include "score/Settings.hpp"
#include "score/Spots.hpp"
#include "score/Workout.hpp"

namespace StepValues
{

// The values UP / DOWN step through.
inline constexpr uint16_t kShotValues[] = { 5, 10, 15, 20, 25, 30, 40, 50 };
inline constexpr uint16_t kTimeValues[] = { 30, 60, 90, 120, 180 };
inline constexpr uint16_t kRestValues[] = { 0, 15, 30, 60 };

struct Values {
    const uint16_t* v;
    uint8_t         n;
};

inline constexpr Values kShots = { kShotValues, sizeof(kShotValues) / sizeof(kShotValues[0]) };
inline constexpr Values kTimes = { kTimeValues, sizeof(kTimeValues) / sizeof(kTimeValues[0]) };
inline constexpr Values kRests = { kRestValues, sizeof(kRestValues) / sizeof(kRestValues[0]) };

inline bool isTime(uint8_t type) { return type == static_cast<uint8_t>(score::GoalType::Time); }

inline const Values& valuesFor(uint8_t type) { return isTime(type) ? kTimes : kShots; }

/// The next entry above (dir > 0) or below (dir < 0) @p cur, or @p cur at
/// the end of the list. A value not in the list steps onto its neighbour.
inline uint16_t stepValue(const Values& l, uint16_t cur, int8_t dir)
{
    if (dir > 0) {
        for (uint8_t i = 0; i < l.n; ++i) {
            if (l.v[i] > cur) {
                return l.v[i];
            }
        }
    } else {
        for (uint8_t i = l.n; i > 0; --i) {
            if (l.v[i - 1] < cur) {
                return l.v[i - 1];
            }
        }
    }
    return cur;
}

/// The entry closest to @p cur (the lower one on a tie).
inline uint16_t nearest(const Values& l, uint16_t cur)
{
    uint16_t best = l.v[0];
    for (uint8_t i = 1; i < l.n; ++i) {
        const int32_t d    = static_cast<int32_t>(l.v[i]) - cur;
        const int32_t dBest = static_cast<int32_t>(best) - cur;
        if ((d < 0 ? -d : d) < (dBest < 0 ? -dBest : dBest)) {
            best = l.v[i];
        }
    }
    return best;
}

inline const char* goalName(uint8_t type)
{
    switch (static_cast<score::GoalType>(type)) {
        case score::GoalType::Makes: return "Makes";
        case score::GoalType::Time:  return "Time";
        case score::GoalType::Attempts:
        default:                     return "Attempts";
    }
}

/// The first step of a new workout: Free Throw with the Settings' goal and rest.
inline score::Step defaultStep(const score::Settings& s)
{
    return { static_cast<uint8_t>(score::Spot::Ft), s.rest_s, { s.goalType, 0, s.goalValue } };
}

} // namespace StepValues
