/**
 ******************************************************************************
 * @file    Hints.cpp
 * @brief   The four button hints at the screen corners.
 ******************************************************************************
 */

#include "gui/widgets/Hints.hpp"

#include "gui/theme/Theme.hpp"

namespace
{

// Each corner is a 24 x 24 box centred on its badge; glyph coordinates are in
// that box, the badge centre at (12, 12).
constexpr int32_t kBox   = 24;
constexpr int32_t kC     = kBox / 2;
constexpr int32_t kBadgeR = 9;
constexpr int32_t kDotR  = 4;

struct Corner {
    int16_t badgeX, badgeY;   // badge centre
    int16_t dotX, dotY;       // dot centre
};
constexpr Corner kCornerPos[4] = {
    { 31, 55, 30, 57 },     // UP
    { 31, 183, 30, 183 },   // DOWN
    { 209, 55, 210, 57 },   // SELECT
    { 209, 183, 210, 183 }, // BACK
};

// lv_line keeps a pointer to its points, so they are static.
const lv_point_precise_t kTick[]   = { { 8, 12 }, { 11, 15 }, { 16, 9 } };
const lv_point_precise_t kCrossA[] = { { 8, 8 }, { 16, 16 } };
const lv_point_precise_t kCrossB[] = { { 16, 8 }, { 8, 16 } };
const lv_point_precise_t kUndoHead[] = { { 6, 10 }, { 8, 13 }, { 10, 10 } };

void glyphLine(lv_obj_t* parent, const lv_point_precise_t* pts, uint32_t n)
{
    lv_obj_t* l = lv_line_create(parent);
    lv_obj_set_pos(l, 0, 0);
    lv_line_set_points(l, pts, n);
    lv_obj_set_style_line_width(l, 2, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(l, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(l, Theme::rgb(Theme::kBg), LV_PART_MAIN);
}

} // namespace

Hints::Hints(lv_obj_t* parent)
{
    for (uint8_t i = 0; i < kCorners; ++i) {
        mCorner[i] = Theme::container(parent, kCornerPos[i].badgeX - kC, kCornerPos[i].badgeY - kC, kBox, kBox);
    }
}

void Hints::set(Style up, Style down, Style select, Style back)
{
    const Style s[kCorners] = { up, down, select, back };
    for (uint8_t i = 0; i < kCorners; ++i) {
        if (s[i] != mStyle[i]) {
            build(i, s[i]);
        }
    }
}

void Hints::build(uint8_t corner, Style s)
{
    lv_obj_t* box = mCorner[corner];
    lv_obj_clean(box);
    mStyle[corner] = s;

    switch (s) {
        case Off:
            return;

        case Dot: {
            const Corner& p = kCornerPos[corner];
            Theme::dot(box, kC + (p.dotX - p.badgeX), kC + (p.dotY - p.badgeY), kDotR, Theme::kText);
        } return;

        case Make:
            Theme::dot(box, kC, kC, kBadgeR, Theme::kGood);
            glyphLine(box, kTick, 3);
            return;

        case Miss:
            Theme::dot(box, kC, kC, kBadgeR, Theme::kBad);
            glyphLine(box, kCrossA, 2);
            glyphLine(box, kCrossB, 2);
            return;

        case Undo:
            // Anticlockwise arrow: an open ring from 9 o'clock round to 5
            // o'clock, the head at 9 o'clock pointing down.
            Theme::dot(box, kC, kC, kBadgeR, Theme::kText);
            Theme::arc(box, kC, kC, 4, 2, -90, 150, Theme::kBg, false);
            glyphLine(box, kUndoHead, 3);
            return;

        case PauseIcon:
            Theme::dot(box, kC, kC, kBadgeR, Theme::kText);
            Theme::box(box, 8, 7, 3, 10, Theme::kBg);
            Theme::box(box, 13, 7, 3, 10, Theme::kBg);
            return;
    }
}
