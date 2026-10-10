/**
 ******************************************************************************
 * @file    Ring.hpp
 * @brief   The bezel ring, centred (120,120), radius 112, in one of three
 *          looks: progress (track + fill), segments (one per workout shot) or
 *          dashed (pause).
 *
 * Arc objects are kept and reused; switching between looks of different
 * stroke widths recreates them (the arc's box size depends on its width).
 ******************************************************************************
 */

#ifndef RING_HPP
#define RING_HPP

#include <cstdint>

#include "lvgl.h"

class Ring
{
public:
    explicit Ring(lv_obj_t* parent);

    /// Grey track (width 5) plus a rounded fill (width 8) from 12 o'clock
    /// clockwise over permille/1000 of the circle.
    void setProgress(uint16_t permille, uint32_t color);

    /// n segments (width 8, ~3.3 deg gaps, from 12 o'clock); state per
    /// segment: 0 empty (GRAY_DARK), 1 make (GREEN), 2 miss (RED). n <= 50.
    void setSegments(uint8_t n, const uint8_t* state);

    /// 24 short arcs (7.5 deg every 15 deg, width 3).
    void setDashed(uint32_t color);

    void hide();

    static constexpr uint8_t kMaxSegments = 50;
    static constexpr uint8_t kDashes      = 24;

private:
    enum class Look : uint8_t { None, Progress, Segments, Dashed };

    /// Make sure @p n pool arcs of stroke @p width exist; hides the rest.
    void ensurePool(uint8_t n, int32_t width);

    lv_obj_t* mParent;
    lv_obj_t* mTrack = nullptr;
    lv_obj_t* mFill  = nullptr;
    lv_obj_t* mPool[kMaxSegments] = {};
    uint8_t   mPoolCount = 0;
    int32_t   mPoolWidth = 0;
    Look      mLook      = Look::None;
};

#endif // RING_HPP
