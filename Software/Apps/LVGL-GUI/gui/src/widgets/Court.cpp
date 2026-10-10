/**
 ******************************************************************************
 * @file    Court.cpp
 * @brief   Half court with the 13 shooting spots.
 ******************************************************************************
 */

#include "gui/widgets/Court.hpp"

#include <cmath>

#include "gui/theme/Theme.hpp"

namespace
{

// Court geometry in mockup units.
const float kBaselineU[2][2] = { { 2, 10 }, { 198, 10 } };
const float kLaneU[4][2]     = { { 73, 10 }, { 73, 74 }, { 127, 74 }, { 127, 10 } };
const float kLeft3U[2][2]    = { { 22, 10 }, { 22, 45 } };
const float kRight3U[2][2]   = { { 178, 10 }, { 178, 45 } };

constexpr float kFtCx = 100, kFtCy = 74, kFtR = 20;
constexpr float kHoopCx = 100, kHoopCy = 25, kHoopR = 5;
constexpr float kArc3R = 80.5f;

constexpr int32_t kRingGap = 5;   // selection ring radius = spotR + kRingGap
constexpr int32_t kRingW   = 2;

} // namespace

Court::Court(lv_obj_t* parent, int32_t x, int32_t y, float scale, int32_t lineW, int32_t spotR)
    : mX(x)
    , mY(y)
    , mScale(scale)
    , mLineW(lineW)
    , mSpotR(spotR)
{
    line(parent, mBaseline, kBaselineU, 2);
    line(parent, mLane, kLaneU, 4);
    line(parent, mLeft3, kLeft3U, 2);
    line(parent, mRight3, kRight3U, 2);

    // Free-throw half circle bulging towards half court; the three-point arc
    // joins the two corner lines through the top of the key.
    Theme::arc(parent, mX + px(kFtCx), mY + px(kFtCy), px(kFtR), mLineW, 90, 270, Theme::kLine, false);
    Theme::arc(parent, mX + px(kHoopCx), mY + px(kHoopCy), px(kArc3R), mLineW, 104, 256, Theme::kLine, false);
    Theme::arc(parent, mX + px(kHoopCx), mY + px(kHoopCy), px(kHoopR), 2, 0, 360, Theme::kAmber, false);

    for (uint8_t i = 0; i < score::kSpotCount; ++i) {
        const score::SpotInfo& s = score::spotInfo(i);
        mSpots[i] = Theme::dot(parent, mX + px(s.courtX), mY + px(s.courtY), mSpotR, Theme::kLine);
    }

    mRing = Theme::arc(parent, mX, mY, mSpotR + kRingGap, kRingW, 0, 360, Theme::kText, false);
    Theme::setHidden(mRing, true);
}

int32_t Court::px(float units) const
{
    return static_cast<int32_t>(lroundf(units * mScale));
}

lv_obj_t* Court::line(lv_obj_t* parent, lv_point_precise_t* pts, const float (*units)[2], uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) {
        pts[i].x = static_cast<lv_value_precise_t>(px(units[i][0]));
        pts[i].y = static_cast<lv_value_precise_t>(px(units[i][1]));
    }
    lv_obj_t* l = lv_line_create(parent);
    lv_obj_set_pos(l, mX, mY);
    lv_line_set_points(l, pts, n);
    lv_obj_set_style_line_width(l, mLineW, LV_PART_MAIN);
    lv_obj_set_style_line_color(l, Theme::rgb(Theme::kLine), LV_PART_MAIN);
    return l;
}

void Court::setSpotColor(uint8_t spot, uint32_t color)
{
    if (spot >= score::kSpotCount) {
        return;
    }
    lv_obj_set_style_bg_color(mSpots[spot], Theme::rgb(color), LV_PART_MAIN);
}

void Court::setSelected(uint8_t spot)
{
    mSelected = spot < score::kSpotCount ? spot : kNone;
    if (mSelected == kNone) {
        Theme::setHidden(mRing, true);
        return;
    }
    const score::SpotInfo& s = score::spotInfo(spot);
    const int32_t outer = mSpotR + kRingGap + (kRingW + 1) / 2;   // as Draw::arc sizes it
    lv_obj_set_pos(mRing, mX + px(s.courtX) - outer, mY + px(s.courtY) - outer);
    Theme::setHidden(mRing, false);
}
