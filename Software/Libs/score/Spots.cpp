#include "score/Spots.hpp"

namespace score {

namespace {

const SpotInfo kSpots[kSpotCount] = {
    { "3PT · L-Corner",   "3PT · L-CORNER",   Category::Three,  12,  28 },
    { "3PT · L-Wing",     "3PT · L-WING",     Category::Three,  36,  92 },
    { "3PT · Top",        "3PT · TOP",        Category::Three, 100, 118 },
    { "3PT · R-Wing",     "3PT · R-WING",     Category::Three, 164,  92 },
    { "3PT · R-Corner",   "3PT · R-CORNER",   Category::Three, 188,  28 },
    { "Mid · L-Baseline", "MID · L-BASELINE", Category::Mid,    44,  26 },
    { "Mid · L-Elbow",    "MID · L-ELBOW",    Category::Mid,    68,  76 },
    { "Mid · Top-Key",    "MID · TOP-KEY",    Category::Mid,   100,  94 },
    { "Mid · R-Elbow",    "MID · R-ELBOW",    Category::Mid,   132,  76 },
    { "Mid · R-Baseline", "MID · R-BASELINE", Category::Mid,   156,  26 },
    { "Free Throw",       "FREE THROW",       Category::Ft,    100,  77 },
    { "Layup · L",        "LAYUP · L",        Category::Layup,  82,  34 },
    { "Layup · R",        "LAYUP · R",        Category::Layup, 118,  34 },
};

const char* const kCategoryNames[kCategoryCount] = { "3PT", "Mid-range", "Free throw", "Layup" };
const char* const kCategoryShort[kCategoryCount] = { "3PT", "Mid", "FT", "Layup" };

uint8_t clampCategory(Category c)
{
    const auto i = static_cast<uint8_t>(c);
    return i < kCategoryCount ? i : kCategoryCount - 1;
}

} // namespace

const SpotInfo& spotInfo(Spot s)
{
    const auto i = static_cast<uint8_t>(s);
    return kSpots[i < kSpotCount ? i : kSpotCount - 1];
}

const char* categoryName(Category c)  { return kCategoryNames[clampCategory(c)]; }
const char* categoryShort(Category c) { return kCategoryShort[clampCategory(c)]; }

} // namespace score
