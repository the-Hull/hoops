/**
 ******************************************************************************
 * @file    Theme.hpp
 * @brief   Hoops fonts, palette and text helpers, plus the SDK's LVGL drawing
 *          helpers.
 *
 * Mirrors RunLVGL's Theme: the drawing helpers live in the SDK
 * (SDK/GUI/LVGL/Draw.hpp) and are pulled into this namespace so screens write
 * Theme::arc(...). What is the app's own: the Poppins faces by name, the
 * palette (docs/mockup-layout.md "Colour mapping"), and small formatting
 * helpers. Coordinates are 240 x 240 screen px; arc angles are TouchGFX's
 * (0 = 12 o'clock, clockwise).
 ******************************************************************************
 */

#ifndef THEME_HPP
#define THEME_HPP

#include <cstddef>
#include <cstdint>

#include "lvgl.h"

#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

namespace Theme
{

/// The Poppins faces in assets/fonts, by weight and pixel size.
enum class Font : uint8_t {
    Regular14, Regular16, Regular18,
    Medium18, Medium25, Medium40,
    SemiBold20, SemiBold25, SemiBold30, SemiBold35, SemiBold40, SemiBold60,
    Light60,
    Italic18,
};

const lv_font_t* font(Font f);

// Palette (SDK 64-colour values, rendered exactly by the display).
constexpr uint32_t kText   = SDK::GUI::Color::WHITE;
constexpr uint32_t kDim    = SDK::GUI::Color::GRAY;
constexpr uint32_t kLine   = SDK::GUI::Color::GRAY_DARK;
constexpr uint32_t kAccent = SDK::GUI::Color::TEAL;
constexpr uint32_t kGood   = SDK::GUI::Color::GREEN;
constexpr uint32_t kBad    = SDK::GUI::Color::RED;
constexpr uint32_t kAmber  = SDK::GUI::Color::YELLOW_DARK;
constexpr uint32_t kBg     = SDK::GUI::Color::BLACK;

/// Colour for a shooting percentage: kGood >= 50 %, kAmber 35-49 %, kBad
/// < 35 %; kDim with no attempts. A threshold lookup only: the percentages
/// themselves come from the service.
uint32_t pctColor(uint16_t makes, uint16_t attempts);

// The SDK's drawing helpers, under the app's name for them.
using SDK::LVGL::Draw::rgb;
using SDK::LVGL::Draw::init;
using SDK::LVGL::Draw::applyScreen;
using SDK::LVGL::Draw::setHidden;
using SDK::LVGL::Draw::container;
using SDK::LVGL::Draw::hline;
using SDK::LVGL::Draw::box;
using SDK::LVGL::Draw::dot;
using SDK::LVGL::Draw::arc;
using SDK::LVGL::Draw::setArc;
using SDK::LVGL::Draw::setArcColor;

/// Set a label's text, replacing the middle dot U+00B7 (in no face; spot
/// names use it, "3PT · L-Wing") with '-'. Texts longer than 63 bytes are
/// cut. Use this, not lv_label_set_text(), for any text that may hold one.
void setText(lv_obj_t* label, const char* text);

/// Draw::label() with one of the app's faces, its text passed through
/// setText(). The box is at (x, y) (top left) with width w.
lv_obj_t* label(lv_obj_t* parent, Font f, const char* text, int32_t x, int32_t y, int32_t w,
                lv_text_align_t align = LV_TEXT_ALIGN_CENTER, uint32_t color = kText);

/// Screen header: Regular14 (the mockups' headers are 11-13 px), centred
/// across the screen, letter-spaced 2 px. Unlike label(), whose y is the top
/// of the text box, @p yCentre is the text's vertical centre, as mockup y
/// values are. Screens that want a bigger header place a label themselves.
lv_obj_t* header(lv_obj_t* parent, const char* text, int32_t yCentre, uint32_t color = kAccent);

/// Top of a label box in @p f whose text should be centred on @p yCentre.
int32_t topFor(Font f, int32_t yCentre);

/// Duration: "m:ss" under an hour ("0:24", "12:41"), "h:mm:ss" from an
/// hour up ("1:02:03").
void fmtDuration(char* out, size_t n, uint32_t seconds);

/// YYYYMMDD -> "6.10." (day.month.); "--" for 0.
void fmtDate(char* out, size_t n, uint32_t ymd);

} // namespace Theme

#endif // THEME_HPP
