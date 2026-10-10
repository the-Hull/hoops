/**
 ******************************************************************************
 * @file    Theme.cpp
 * @brief   Hoops fonts, palette and text helpers.
 ******************************************************************************
 */

#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

#include "score/Stats.hpp"
#include "score/Workout.hpp"

#include <cstdio>
#include <cstring>

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

namespace
{
Scheme sScheme = Scheme::Dark;
} // namespace

void setScheme(Scheme s)
{
    using namespace SDK::GUI::Color;
    const bool light = s == Scheme::Light;
    kBg     = light ? WHITE       : BLACK;
    kText   = light ? BLACK       : WHITE;
    kDim    = light ? GRAY_DARK   : GRAY;
    kLine   = light ? GRAY        : GRAY_DARK;
    kAccent = TEAL;
    kGood   = light ? GREEN_MID   : GREEN;
    kBad    = RED;
    kAmber  = light ? ORANGE_DARK : YELLOW_DARK;
    sScheme = light ? Scheme::Light : Scheme::Dark;
}

Scheme scheme()
{
    return sScheme;
}

void init()
{
    SDK::LVGL::Draw::init();
    setScheme(Scheme::Dark);
}

uint32_t pctColor(uint16_t makes, uint16_t attempts)
{
    if (attempts == 0) {
        return kDim;
    }
    // The rounded percentage the text shows (fmtPct), so "50 %" is never amber.
    const uint8_t p = score::pct(makes, attempts);
    if (p >= 50u) {
        return kGood;
    }
    if (p >= 35u) {
        return kAmber;
    }
    return kBad;
}

namespace
{

/// Copy @p text into @p buf (64 bytes), replacing U+00B7 with '-'.
void sanitize(char (&buf)[64], const char* text)
{
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
}

} // namespace

void setText(lv_obj_t* label, const char* text)
{
    char buf[64];
    sanitize(buf, text);
    lv_label_set_text(label, buf);
}

int32_t textWidth(Font f, const char* text, int32_t letterSpace)
{
    char buf[64];
    sanitize(buf, text);
    lv_point_t size;
    lv_text_get_size(&size, buf, font(f), letterSpace, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
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

int32_t topForBaseline(Font f, int32_t yBaseline)
{
    const lv_font_t* fn = font(f);
    return yBaseline - (lv_font_get_line_height(fn) - fn->base_line);
}

lv_obj_t* header(lv_obj_t* parent, const char* text, int32_t yCentre, uint32_t color)
{
    lv_obj_t* l = label(parent, Font::Regular14, text, 0, topFor(Font::Regular14, yCentre), 240,
                        LV_TEXT_ALIGN_CENTER, color);
    lv_obj_set_style_text_letter_space(l, 2, LV_PART_MAIN);
    return l;
}

void fmtDuration(char* out, size_t n, uint32_t seconds)
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

void fmtPct(char* out, size_t n, uint16_t makes, uint16_t attempts)
{
    if (attempts == 0) {
        snprintf(out, n, "--");
    } else {
        snprintf(out, n, "%u %%", static_cast<unsigned>(score::pct(makes, attempts)));
    }
}

void fmtGoal(char* out, size_t n, uint8_t type, uint16_t value)
{
    switch (static_cast<score::GoalType>(type)) {
        case score::GoalType::Makes: snprintf(out, n, "%u mk", static_cast<unsigned>(value)); break;
        case score::GoalType::Time:  snprintf(out, n, "%u s", static_cast<unsigned>(value)); break;
        default:                     snprintf(out, n, "x%u", static_cast<unsigned>(value)); break;
    }
}

const char* spotShort(const char* name)
{
    const char* p = strstr(name, kMidDot);
    return p ? p + strlen(kMidDot) : name;
}

lv_obj_t* fitLabel(lv_obj_t* parent, const char* const* texts, uint8_t nTexts, const Font* fonts, uint8_t nFonts,
                   int32_t x, int32_t w, int32_t yCentre, lv_text_align_t align, uint32_t color)
{
    const char* text  = texts[nTexts - 1];
    Font        f     = fonts[nFonts - 1];
    int32_t     space = -1;
    bool        found = false;
    for (uint8_t t = 0; t < nTexts && !found; ++t) {
        for (uint8_t i = 0; i < nFonts && !found; ++i) {
            for (int32_t ls = 0; ls >= -1 && !found; --ls) {
                if (textWidth(fonts[i], texts[t], ls) <= w) {
                    text = texts[t], f = fonts[i], space = ls, found = true;
                }
            }
        }
    }
    lv_obj_t* l = label(parent, f, text, x, topFor(f, yCentre), w, align, color);
    lv_obj_set_style_text_letter_space(l, space, LV_PART_MAIN);
    return l;
}

int32_t baselineFor(Font f, int32_t yCentre)
{
    const lv_font_t* fn = font(f);
    return topFor(f, yCentre) + lv_font_get_line_height(fn) - fn->base_line;
}

void pair(lv_obj_t* parent, Font fa, const char* a, uint32_t ca, Font fb, const char* b, uint32_t cb,
          int32_t yCentre, int32_t gap, int32_t xCentre)
{
    const int32_t wa = textWidth(fa, a);
    const int32_t wb = textWidth(fb, b);
    const int32_t x  = xCentre - (wa + gap + wb) / 2;
    label(parent, fa, a, x, topFor(fa, yCentre), wa + 1, LV_TEXT_ALIGN_LEFT, ca);
    label(parent, fb, b, x + wa + gap, topForBaseline(fb, baselineFor(fa, yCentre)), wb + 1, LV_TEXT_ALIGN_LEFT, cb);
}

} // namespace Theme
