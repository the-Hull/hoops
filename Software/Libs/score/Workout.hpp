/**
 * @file  Workout.hpp
 * @brief Workout definitions (step list with goals) and built-in presets.
 */

#pragma once

#include "score/Spots.hpp"

#include <cstdint>
#include <type_traits>

namespace score {

enum class GoalType : uint8_t { Attempts, Makes, Time };

// Persisted PODs: padding-free (explicit pads, zeroed).
struct Goal {
    uint8_t  type;    ///< GoalType
    uint8_t  pad;
    uint16_t value;   ///< shots, or seconds for Time
};

struct Step {
    uint8_t spot;     ///< Spot
    uint8_t rest_s;   ///< rest after this step
    Goal    goal;
};
static_assert(sizeof(Goal) == 4 && std::has_unique_object_representations<Goal>::value, "Goal: no padding");
static_assert(sizeof(Step) == 6 && std::has_unique_object_representations<Step>::value, "Step: no padding");

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
    /// stepCount 1..kMaxSteps and every used step in range: spot < kSpotCount,
    /// goal type a GoalType, goal value > 0 (rest_s is free). Not the id or name.
    bool validSteps() const;
};
static_assert(sizeof(Workout) == 1 + kNameLen + 1 + sizeof(Step) * kMaxSteps, "Workout flash layout changed");
static_assert(std::has_unique_object_representations<Workout>::value, "Workout: no padding");

/// index 0..kPresetCount-1; out-of-range clamps to the last preset.
const Workout& preset(uint8_t index);
/// ids 1..kPresetCount, nullptr otherwise.
const Workout* findPreset(uint8_t id);

} // namespace score
