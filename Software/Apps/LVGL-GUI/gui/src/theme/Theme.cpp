/**
 ******************************************************************************
 * @file    Theme.cpp
 * @brief   Hoops fonts, palette and text helpers.
 ******************************************************************************
 */

#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

#include <cstdio>

namespace Theme
{

const lv_font_t* font(Font f)
{
    switch (f) {
        case Font::Regular14:  return &poppins_regular_14;
        case Font::Regular16:  return &poppins_regular_16;
        case Font::Regular18:  return &poppins_regular_18;
        case Font::Medium18:   return &poppins_medium_18;
        case Font::Medium25:   return &poppins_medium_25;
        case Font::Medium40:   return &poppins_medium_40;
        case Font::SemiBold20: return &poppins_semibold_20;
        case Font::SemiBold25: return &poppins_semibold_25;
        case Font::SemiBold30: return &poppins_semibold_30;
        case Font::SemiBold35: return &poppins_semibold_35;
        case Font::SemiBold40: return &poppins_semibold_40;
        case Font::SemiBold60: return &poppins_semibold_60;
        case Font::Light60:    return &poppins_light_60;
        case Font::Italic18:   return &poppins_italic_18;
    }
    return &poppins_regular_18;
}

uint32_t pctColor(uint16_t makes, uint16_t attempts)
{
    if (attempts == 0) {
        return kLine;
    }
    const uint32_t m100 = static_cast<uint32_t>(makes) * 100u;
    if (m100 >= 50u * attempts) {
        return kGood;
    }
    if (m100 >= 35u * attempts) {
        return kAmber;
    }
    return kBad;
}

void setText(lv_obj_t* label, const char* text)
{
    char buf[64];
    size_t o = 0;
    for (const char* p = text ? text : ""; *p && o + 1 < sizeof(buf); ++p) {
        // U+00B7 is 0xC2 0xB7 in UTF-8.
        if (static_cast<uint8_t>(p[0]) == 0xC2 && static_cast<uint8_t>(p[1]) == 0xB7) {
            buf[o++] = '-';
            ++p;
        } else {
            buf[o++] = *p;
        }
    }
    buf[o] = '\0';
    lv_label_set_text(label, buf);
}

lv_obj_t* label(lv_obj_t* parent, Font f, const char* text, int32_t x, int32_t y, int32_t w,
                lv_text_align_t align, uint32_t color)
{
    lv_obj_t* l = SDK::LVGL::Draw::label(parent, font(f), "", x, y, w, align, color);
    setText(l, text);
    return l;
}

int32_t topFor(Font f, int32_t yCentre)
{
    return yCentre - lv_font_get_line_height(font(f)) / 2;
}

lv_obj_t* header(lv_obj_t* parent, const char* text, int32_t y, uint32_t color)
{
    lv_obj_t* l = label(parent, Font::SemiBold20, text, 0, topFor(Font::SemiBold20, y), 240,
                        LV_TEXT_ALIGN_CENTER, color);
    lv_obj_set_style_text_letter_space(l, 2, LV_PART_MAIN);
    return l;
}

void fmtMmSs(char* out, size_t n, uint32_t seconds)
{
    const unsigned h = static_cast<unsigned>(seconds / 3600u);
    const unsigned m = static_cast<unsigned>((seconds % 3600u) / 60u);
    const unsigned s = static_cast<unsigned>(seconds % 60u);
    if (h > 0) {
        snprintf(out, n, "%u:%02u:%02u", h, m, s);
    } else {
        snprintf(out, n, "%u:%02u", m, s);
    }
}

void fmtDate(char* out, size_t n, uint32_t ymd)
{
    if (ymd == 0) {
        snprintf(out, n, "--");
        return;
    }
    snprintf(out, n, "%u.%u.", static_cast<unsigned>(ymd % 100u), static_cast<unsigned>((ymd / 100u) % 100u));
}

} // namespace Theme
