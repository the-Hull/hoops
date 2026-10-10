/**
 * @file  History.hpp
 * @brief Session summaries ring (newest last) and user workout store; flash PODs.
 */

#pragma once

#include "score/Session.hpp"
#include "score/Stats.hpp"
#include "score/Workout.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace score {

static_assert(kCategoryCount == 4, "SessionSummary layout assumes 4 categories");

// Persisted PODs: padding-free (explicit pads, zeroed).
struct SessionSummary {
    uint32_t id, ymd;
    uint16_t workoutId;
    uint16_t catMakes[kCategoryCount], catAtt[kCategoryCount];   ///< indexed by Category
    uint8_t  pad0[2];
    uint32_t elapsed_s;
    uint8_t  totalPct, pad[3];
};
static_assert(std::is_trivially_copyable<SessionSummary>::value, "SessionSummary is memcpy'd to flash");
static_assert(sizeof(SessionSummary) == 36 && offsetof(SessionSummary, elapsed_s) == 28,
              "SessionSummary flash layout changed");
static_assert(std::has_unique_object_representations<SessionSummary>::value, "SessionSummary: no padding");

SessionSummary summarize(const Session& s, uint32_t ymd);

constexpr uint8_t kIndexCapacity = 50, kTrendCount = 5;

struct SessionIndex {
    uint8_t        count;
    uint8_t        pad[3];
    SessionSummary items[kIndexCapacity];   ///< oldest first
    SpotStat       last[kSpotCount];        ///< the newest session's makes/attempts at that spot (attempts > 0)
    SpotStat       best[kSpotCount];        ///< best session % at that spot with ≥ kMinAttemptsForRank attempts
                                            ///< there (ties keep the older); all-time, eviction keeps them

    uint32_t push(const SessionSummary& s);            ///< returns evicted id or 0; same id replaces its row
    const SessionSummary* find(uint32_t id) const;
    const SessionSummary* newest(uint8_t back) const;  ///< back 0 = newest, nullptr past the start
    bool remove(uint32_t id);
    bool valid() const;   ///< count in range (check after load)
    /// Updates last/best from one session's per-spot totals (SessionStats::compute(s).spot); call after push().
    void recordSpots(const SpotStat (&spot)[kSpotCount]);
    void clear();         ///< count 0, rows and records zeroed
};
static_assert(sizeof(SessionIndex) == 4 + 36 * kIndexCapacity + 4 * kSpotCount * 2, "SessionIndex flash layout changed");
static_assert(std::is_trivially_copyable<SessionIndex>::value, "SessionIndex is memcpy'd to flash");
static_assert(std::has_unique_object_representations<SessionIndex>::value, "SessionIndex: no padding");

struct WorkoutStore {
    uint8_t count;
    uint8_t pad[3];
    Workout items[kMaxUserWorkouts];

    bool add(Workout w, uint8_t& idOut);   ///< assigns the lowest free id ≥ kFirstUserWorkoutId
    bool replace(const Workout& w);        ///< by w.id
    bool remove(uint8_t id);
    const Workout* find(uint8_t id) const;
    uint8_t nextCustomNumber() const;      ///< count + 1, for "Custom N"
    bool valid() const;   ///< count, ids and steps (Workout::validSteps) in range (check after load)
};
static_assert(std::is_trivially_copyable<WorkoutStore>::value, "WorkoutStore is memcpy'd to flash");
static_assert(std::has_unique_object_representations<WorkoutStore>::value, "WorkoutStore: no padding");

} // namespace score
