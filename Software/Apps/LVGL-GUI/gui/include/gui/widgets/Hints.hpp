/**
 ******************************************************************************
 * @file    Hints.hpp
 * @brief   The four button hints at the screen corners.
 *
 * Corners: UP top left, DOWN bottom left, SELECT top right, BACK bottom
 * right. Dot = white r4 dot at (30,57) (30,183) (210,57) (210,183). The
 * shooting-screen badges are 18 px discs at (31,55) (31,183) (209,55)
 * (209,183) with a black glyph: Make green check, Miss red cross, Undo white
 * undo arrow, PauseIcon white pause bars. Off shows nothing.
 ******************************************************************************
 */

#ifndef HINTS_HPP
#define HINTS_HPP

#include <cstdint>

#include "lvgl.h"

class Hints
{
public:
    enum Style : uint8_t { Off, Dot, Make, Miss, Undo, PauseIcon };

    explicit Hints(lv_obj_t* parent);

    void set(Style up, Style down, Style select, Style back);

private:
    static constexpr uint8_t kCorners = 4;

    void build(uint8_t corner, Style s);

    lv_obj_t* mCorner[kCorners] = {};
    Style     mStyle[kCorners]  = { Off, Off, Off, Off };
};

#endif // HINTS_HPP
