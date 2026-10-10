/**
 ******************************************************************************
 * @file    Toast.cpp
 * @brief   A short message in a white pill near the bottom of the screen.
 ******************************************************************************
 */

#include "gui/widgets/Toast.hpp"

#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

// As ActiveScreen's toast (docs/mockup-layout.md §8).
constexpr int32_t kY    = 171;
constexpr int32_t kMinW = 129;
constexpr int32_t kH    = 27;
constexpr int32_t kPad  = 12;
constexpr int32_t kMaxW = 196;   // inside the round edge at y 158..185
constexpr Font kFonts[] = { Font::SemiBold20, Font::Medium18, Font::Regular16 };

} // namespace

Toast::Toast(lv_obj_t* parent)
{
    mBox  = Theme::box(parent, 120 - kMinW / 2, kY - kH / 2, kMinW, kH, Theme::kText, kH / 2);
    mText = Theme::label(parent, Font::SemiBold20, "", 20, Theme::topFor(Font::SemiBold20, kY), 200,
                         LV_TEXT_ALIGN_CENTER, Theme::kBg);
    Theme::setHidden(mBox, true);
    Theme::setHidden(mText, true);
}

Toast::~Toast()
{
    // The objects go with the screen; only the timer is ours to stop.
    if (mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
}

void Toast::show(const char* text, uint32_t ms)
{
    // The first face whose pill fits the round screen, else the smallest.
    Font f    = kFonts[0];
    int32_t w = 0;
    for (const Font cand : kFonts) {
        f = cand;
        w = Theme::textWidth(f, text) + 2 * kPad;
        if (w <= kMaxW) {
            break;
        }
    }
    if (w < kMinW) {
        w = kMinW;
    }
    lv_obj_set_x(mBox, 120 - w / 2);
    lv_obj_set_width(mBox, w);
    lv_obj_set_style_text_font(mText, Theme::font(f), LV_PART_MAIN);
    lv_obj_set_y(mText, Theme::topFor(f, kY));
    Theme::setText(mText, text);
    Theme::setHidden(mBox, false);
    Theme::setHidden(mText, false);

    if (mTimer) {
        lv_timer_set_period(mTimer, ms);
        lv_timer_reset(mTimer);
    } else {
        mTimer = lv_timer_create(&Toast::timerCb, ms, this);
    }
}

void Toast::hide()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
    Theme::setHidden(mBox, true);
    Theme::setHidden(mText, true);
}

void Toast::timerCb(lv_timer_t* t)
{
    static_cast<Toast*>(lv_timer_get_user_data(t))->hide();   // deletes the timer
}
