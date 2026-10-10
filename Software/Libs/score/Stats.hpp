/**
 * @file  Stats.hpp
 * @brief Session statistics: per-spot/category totals, streak, best/worst, pace, HR.
 */

#pragma once

#include "score/Session.hpp"
#include "score/Spots.hpp"

#include <cstdint>

namespace score {

/// Percentage rounded half-up; 0 when attempts == 0.
uint8_t pct(uint16_t makes, uint16_t attempts);

struct SpotStat {
    uint16_t makes;
    uint16_t attempts;
};

constexpr uint8_t  kNoSpot             = 0xFF;
constexpr uint16_t kMinAttemptsForRank = 5;

struct SessionStats {
    SpotStat spot[kSpotCount];
    SpotStat cat[kCategoryCount];
    SpotStat total;
    uint16_t longestStreak;
    uint8_t  streakSpot;      ///< spot where the streak ended, kNoSpot if 0
    uint8_t  bestSpot;        ///< ≥ kMinAttemptsForRank attempts, else kNoSpot
    uint8_t  worstSpot;
    uint32_t elapsed_s;
    uint16_t shotsPerMin10;   ///< shots/min × 10
    uint8_t  hrAvg;
    uint8_t  hrMax;

    static SessionStats compute(const Session& s);
};

} // namespace score
