/**
 ******************************************************************************
 * @file    Ring.hpp
 * @brief   The bezel ring, centred (120,120), radius 112, in one of three
 *          looks: progress (track + fill), segments (one per workout shot) or
 *          dashed (pause).
 *
 * One LVGL object whose draw callback paints the arcs, whatever the look:
 * a 50-segment workout ring costs the pool one object, not 50 lv_arc
 * widgets (~440 B each, which overflowed the pool when such a screen and
 * the next one coexisted during a switch). The object is created with the
 * Ring, so it sits under everything the screen adds later. Setters only
 * store the arcs and invalidate the object.
 ******************************************************************************
 */

#ifndef RING_HPP
#define RING_HPP

#include <cstdint>

#include "lvgl.h"

class Ring
{
public:
    enum class Look : uint8_t { None, Progress, Segments, Dashed };

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

    // Read back (self-test): the look and the arcs it draws.
    Look     look() const { return mLook; }
    uint8_t  arcCount() const { return mCount; }
    uint32_t arcColor(uint8_t i) const { return i < mCount ? mArcs[i].color : 0; }

    /// The Ring drawing on @p root or one of its descendants, or nullptr.
    static const Ring* find(lv_obj_t* root);

    static constexpr uint8_t kMaxSegments = 50;
    static constexpr uint8_t kDashes      = 24;

private:
    struct Arc {
        int16_t  start, end;   ///< TouchGFX degrees (0 = 12 o'clock, clockwise); end - start >= 360: full
        uint32_t color;
        uint8_t  width;
        bool     rounded;
    };

    static void drawCb(lv_event_t* e);
    void setArc(uint8_t i, int32_t start, int32_t end, uint32_t color, uint8_t width, bool rounded);
    void show(Look look, uint8_t count);

    lv_obj_t* mObj   = nullptr;
    Arc       mArcs[kMaxSegments] = {};
    uint8_t   mCount = 0;
    Look      mLook  = Look::None;
};

#endif // RING_HPP
