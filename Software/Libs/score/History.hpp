/**
 * @file  History.hpp
 * @brief Session summaries ring (newest last) and user workout store; flash PODs.
 */

#pragma once

#include "score/Session.hpp"
#include "score/Workout.hpp"

#include <cstdint>
#include <type_traits>

namespace score {

struct SessionSummary {
    uint32_t id, ymd;
    uint16_t workoutId;
    uint16_t catMakes[4], catAtt[4];   ///< indexed by Category
    uint32_t elapsed_s;
    uint8_t  totalPct, pad[3];
};
static_assert(std::is_trivially_copyable<SessionSummary>::value, "SessionSummary is memcpy'd to flash");
static_assert(sizeof(SessionSummary) == 36, "SessionSummary flash layout changed");

SessionSummary summarize(const Session& s, uint32_t ymd);

constexpr uint8_t kIndexCapacity = 50, kTrendCount = 5;

struct SessionIndex {
    uint8_t        count;
    uint8_t        pad[3];
    SessionSummary items[kIndexCapacity];   ///< oldest first

    uint32_t push(const SessionSummary& s);            ///< returns evicted id or 0
    const SessionSummary* find(uint32_t id) const;
    const SessionSummary* newest(uint8_t back) const;  ///< back 0 = newest, nullptr past the start
    bool remove(uint32_t id);
};
static_assert(std::is_trivially_copyable<SessionIndex>::value, "SessionIndex is memcpy'd to flash");

struct WorkoutStore {
    uint8_t count;
    uint8_t pad[3];
    Workout items[kMaxUserWorkouts];

    bool add(Workout w, uint8_t& idOut);   ///< assigns the lowest free id ≥ kFirstUserWorkoutId
    bool replace(const Workout& w);        ///< by w.id
    bool remove(uint8_t id);
    const Workout* find(uint8_t id) const;
    uint8_t nextCustomNumber() const;      ///< count + 1, for "Custom N"
};
static_assert(std::is_trivially_copyable<WorkoutStore>::value, "WorkoutStore is memcpy'd to flash");

} // namespace score
