#include <gtest/gtest.h>
#include <cstring>
#include "score/Spots.hpp"
using namespace score;

TEST(Spots, TableCoversEverySpot) {
    for (uint8_t i = 0; i < kSpotCount; ++i) {
        const SpotInfo& s = spotInfo(static_cast<Spot>(i));
        ASSERT_NE(s.name, nullptr) << i;
        EXPECT_GT(std::strlen(s.name), 2u);
        EXPECT_LT(static_cast<uint8_t>(s.category), kCategoryCount);
        EXPECT_LE(s.courtX, 200); EXPECT_LE(s.courtY, 135);
    }
}
TEST(Spots, NamesAndCategories) {
    EXPECT_STREQ(spotInfo(Spot::LWing3).name, "3PT · L-Wing");
    EXPECT_STREQ(spotInfo(Spot::LWing3).header, "3PT · L-WING");
    EXPECT_EQ(spotInfo(Spot::Ft).category, Category::Ft);
    EXPECT_EQ(spotInfo(Spot::LayupR).category, Category::Layup);
    EXPECT_EQ(spotInfo(Spot::RElbowMid).category, Category::Mid);
    EXPECT_STREQ(categoryName(Category::Mid), "Mid-range");
    EXPECT_STREQ(categoryShort(Category::Ft), "FT");
}
TEST(Spots, CourtCoordinatesMatchMockup) {
    EXPECT_EQ(spotInfo(Spot::LCorner3).courtX, 12); EXPECT_EQ(spotInfo(Spot::LCorner3).courtY, 28);
    EXPECT_EQ(spotInfo(Spot::Top3).courtX, 100);   EXPECT_EQ(spotInfo(Spot::Top3).courtY, 118);
    EXPECT_EQ(spotInfo(Spot::Ft).courtX, 100);     EXPECT_EQ(spotInfo(Spot::Ft).courtY, 77);
}
