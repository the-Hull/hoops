/**
 * @file  Settings.hpp
 * @brief User settings: new-workout defaults, open-session target, input, feedback and display options.
 */

#pragma once

#include "score/Workout.hpp"

#include <cstdint>
#include <type_traits>

namespace score {

enum class HoldDown : uint8_t { ChangeSpot, Pause };
enum class Haptics  : uint8_t { Off, Light, Strong };
enum class Scheme   : uint8_t { Dark, Light };

/// Persisted POD (settings.bin), padding-free, 12 bytes.
struct Settings {
    uint8_t  goalType;          ///< GoalType; default Attempts
    uint8_t  rest_s;            ///< 0 / 15 / 30 / 60; default 30
    uint16_t goalValue;         ///< shots (5..50) or seconds (30..180) for Time; default 10
    uint16_t openTarget;        ///< 0 = off, else 25 / 50 / 100 / 200 shots; default 0
    uint8_t  holdDown;          ///< HoldDown; default ChangeSpot
    uint8_t  haptics;           ///< Haptics; default Strong
    uint8_t  backlightOnShot;   ///< 0/1; default 1
    uint8_t  heartRate;         ///< 0/1; default 1
    uint8_t  scheme;            ///< Scheme; default Dark
    uint8_t  csvExport;         ///< 0/1; default 1

    static Settings defaults();
    /// Every field in its range (goalValue 1..999, openTarget ≤ kMaxShots).
    bool valid() const;
};
static_assert(sizeof(Settings) == 12, "Settings flash layout changed");
static_assert(std::has_unique_object_representations<Settings>::value, "Settings: no padding");
static_assert(std::is_trivially_copyable<Settings>::value, "Settings must be a POD");

} // namespace score
