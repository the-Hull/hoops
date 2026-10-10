/**
 * @file  Spots.hpp
 * @brief Court spots and shot categories (plain C++, host-testable).
 */

#pragma once

#include <cstdint>

namespace score {

enum class Category : uint8_t { Three, Mid, Ft, Layup, Count };

enum class Spot : uint8_t {
    LCorner3, LWing3, Top3, RWing3, RCorner3,
    LBaselineMid, LElbowMid, TopKeyMid, RElbowMid, RBaselineMid,
    Ft, LayupL, LayupR, Count
};

constexpr uint8_t kSpotCount     = static_cast<uint8_t>(Spot::Count);
constexpr uint8_t kCategoryCount = static_cast<uint8_t>(Category::Count);

struct SpotInfo {
    const char* name;     ///< "3PT · L-Wing"
    const char* header;   ///< "3PT · L-WING"
    Category    category;
    uint8_t     courtX;   ///< mockup court units (200 × 135)
    uint8_t     courtY;
};

/// Out-of-range spots clamp to the last entry.
const SpotInfo& spotInfo(Spot s);
inline const SpotInfo& spotInfo(uint8_t s) { return spotInfo(static_cast<Spot>(s)); }

const char* categoryName(Category c);   ///< "Mid-range"
const char* categoryShort(Category c);  ///< "Mid"

} // namespace score
