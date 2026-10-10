/**
 ******************************************************************************
 * @file    Court.hpp
 * @brief   Half court with the 13 shooting spots.
 *
 * Drawn in mockup units (200 x 135, docs/mockup-layout.md SpotPicker) scaled
 * by `scale` px per unit, with the unit origin at screen (x, y): baseline,
 * lane, free-throw half circle, three-point line, amber hoop, and one dot
 * per spot (score::spotInfo().courtX/Y), GRAY_DARK until coloured. One spot
 * can be ringed in white as the selection.
 ******************************************************************************
 */

#ifndef COURT_HPP
#define COURT_HPP

#include <cstdint>

#include "lvgl.h"

#include "score/Spots.hpp"

class Court
{
public:
    Court(lv_obj_t* parent, int32_t x, int32_t y, float scale, int32_t lineW, int32_t spotR);

    void setSpotColor(uint8_t spot, uint32_t color);
    void setSelected(uint8_t spot /* 0xFF none */);

    static constexpr uint8_t kNone = 0xFF;

private:
    int32_t px(float units) const;   ///< mockup units -> px offset from the origin

    lv_obj_t* line(lv_obj_t* parent, lv_point_precise_t* pts, const float (*units)[2], uint32_t n);

    int32_t mX, mY;
    float   mScale;
    int32_t mLineW, mSpotR;

    // lv_line keeps a pointer to its points: they live here.
    lv_point_precise_t mBaseline[2];
    lv_point_precise_t mLane[4];
    lv_point_precise_t mLeft3[2];
    lv_point_precise_t mRight3[2];

    lv_obj_t* mSpots[score::kSpotCount] = {};
    lv_obj_t* mRing     = nullptr;
    uint8_t   mSelected = kNone;
};

#endif // COURT_HPP
