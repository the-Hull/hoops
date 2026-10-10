/**
 ******************************************************************************
 * @file    Ring.cpp
 * @brief   The bezel ring: progress, segments or dashed.
 ******************************************************************************
 */

#include "gui/widgets/Ring.hpp"

#include "gui/theme/Theme.hpp"

namespace
{

constexpr int32_t kCx = 120;
constexpr int32_t kCy = 120;
constexpr int32_t kR  = 112;   // centre line of every stroke

constexpr uint8_t kTrackW    = 5;
constexpr uint8_t kFillW     = 8;
constexpr uint8_t kSegW      = 8;
constexpr uint8_t kDashW     = 3;
constexpr int32_t kGapTenths = 33;   // 3.3 deg between segments
constexpr int32_t kOuter     = kR + (kFillW + 1) / 2;   // widest stroke's outer edge: the object's box

} // namespace

Ring::Ring(lv_obj_t* parent)
{
    mObj = lv_obj_create(parent);
    lv_obj_remove_style_all(mObj);
    lv_obj_remove_flag(mObj, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_set_size(mObj, 2 * kOuter, 2 * kOuter);
    lv_obj_set_pos(mObj, kCx - kOuter, kCy - kOuter);
    lv_obj_set_user_data(mObj, this);
    lv_obj_add_event_cb(mObj, &Ring::drawCb, LV_EVENT_DRAW_MAIN, this);
}

void Ring::setArc(uint8_t i, int32_t start, int32_t end, uint32_t color, uint8_t width, bool rounded)
{
    mArcs[i] = { static_cast<int16_t>(start), static_cast<int16_t>(end), color, width, rounded };
}

void Ring::show(Look look, uint8_t count)
{
    mLook  = look;
    mCount = count;
    lv_obj_invalidate(mObj);
}

void Ring::setProgress(uint16_t permille, uint32_t color)
{
    if (permille > 1000) {
        permille = 1000;
    }
    const int32_t end = (static_cast<int32_t>(permille) * 360 + 500) / 1000;
    setArc(0, 0, 360, Theme::kLine, kTrackW, false);
    setArc(1, 0, end, color, kFillW, true);
    show(Look::Progress, end > 0 ? 2 : 1);
}

void Ring::setSegments(uint8_t n, const uint8_t* state)
{
    if (n > kMaxSegments) {
        n = kMaxSegments;
    }
    for (uint8_t i = 0; i < n; ++i) {
        // Tenths of a degree; each segment gives half the gap to each side.
        const int32_t a0 = (static_cast<int32_t>(i) * 3600) / n + kGapTenths / 2;
        const int32_t a1 = (static_cast<int32_t>(i + 1) * 3600) / n - kGapTenths / 2;
        int32_t s = (a0 + 5) / 10;
        int32_t e = (a1 + 5) / 10;
        if (e <= s) {
            e = s + 1;
        }
        const uint8_t st = state ? state[i] : 0;
        const uint32_t c = st == 1 ? Theme::kGood : (st == 2 ? Theme::kBad : Theme::kLine);
        setArc(i, s, e, c, kSegW, false);
    }
    show(Look::Segments, n);
}

void Ring::setDashed(uint32_t color)
{
    for (uint8_t i = 0; i < kDashes; ++i) {
        const int32_t s = i * 15;
        setArc(i, s, s + 8, color, kDashW, false);   // 7.5 deg, rounded to whole degrees
    }
    show(Look::Dashed, kDashes);
}

void Ring::hide()
{
    show(Look::None, 0);
}

const Ring* Ring::find(lv_obj_t* root)
{
    const uint32_t events = lv_obj_get_event_count(root);
    for (uint32_t i = 0; i < events; ++i) {
        if (lv_event_dsc_get_cb(lv_obj_get_event_dsc(root, i)) == &Ring::drawCb) {
            return static_cast<const Ring*>(lv_obj_get_user_data(root));
        }
    }
    const uint32_t n = lv_obj_get_child_count(root);
    for (uint32_t i = 0; i < n; ++i) {
        if (const Ring* r = find(lv_obj_get_child(root, i))) {
            return r;
        }
    }
    return nullptr;
}

void Ring::drawCb(lv_event_t* e)
{
    const auto* self  = static_cast<const Ring*>(lv_event_get_user_data(e));
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t box;
    lv_obj_get_coords(self->mObj, &box);

    lv_draw_arc_dsc_t dsc;
    lv_draw_arc_dsc_init(&dsc);
    dsc.center.x = box.x1 + kOuter;
    dsc.center.y = box.y1 + kOuter;
    for (uint8_t i = 0; i < self->mCount; ++i) {
        const Arc& a = self->mArcs[i];
        if (a.end <= a.start) {
            continue;
        }
        // lv_draw_arc's radius is the outer edge; its angles are LVGL's
        // (0 = 3 o'clock). A full turn is 0..360, which folding would lose.
        dsc.radius  = static_cast<uint16_t>(kR + (a.width + 1) / 2);
        dsc.width   = a.width;
        dsc.rounded = a.rounded ? 1 : 0;
        dsc.color   = Theme::rgb(a.color);
        if (a.end - a.start >= 360) {
            dsc.start_angle = 0;
            dsc.end_angle   = 360;
        } else {
            dsc.start_angle = SDK::LVGL::Draw::arcAngle(a.start);
            dsc.end_angle   = SDK::LVGL::Draw::arcAngle(a.end);
        }
        lv_draw_arc(layer, &dsc);
    }
}
