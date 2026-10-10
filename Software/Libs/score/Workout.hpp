/**
 * @file  Workout.hpp
 * @brief Workout definitions (step list with goals) and built-in presets.
 */

#pragma once

#include "score/Spots.hpp"

#include <cstdint>

namespace score {

enum class GoalType : uint8_t { Attempts, Makes, Time };

struct Goal {
    uint8_t  type;    ///< GoalType
    uint16_t value;   ///< shots, or seconds for Time
};

struct Step {
    uint8_t spot;     ///< Spot
    Goal    goal;
    uint8_t rest_s;   ///< rest after this step
};

constexpr uint8_t kMaxSteps           = 8;
constexpr uint8_t kMaxUserWorkouts    = 10;
constexpr uint8_t kNameLen            = 20;
constexpr uint8_t kPresetCount        = 5;
constexpr uint8_t kFirstUserWorkoutId = 100;

struct Workout {
    uint8_t id;
    char    name[kNameLen];
    uint8_t stepCount;
    Step    steps[kMaxSteps];

    uint16_t totalShots() const;       ///< sum of Attempts/Makes values (Time steps count 0)
    uint32_t estimateSeconds() const;  ///< shots*12 + sum(rest) + sum(Time values)
};

/// index 0..kPresetCount-1; out-of-range clamps to the last preset.
const Workout& preset(uint8_t index);
/// ids 1..kPresetCount, nullptr otherwise.
const Workout* findPreset(uint8_t id);

} // namespace score
