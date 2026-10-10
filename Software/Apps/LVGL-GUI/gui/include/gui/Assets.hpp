/**
 ******************************************************************************
 * @file    Assets.hpp
 * @brief   Declarations of the generated fonts under assets/fonts.
 ******************************************************************************
 */

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "lvgl.h"

// The generated definitions are C, so the names must not be mangled. GCC
// leaves global variables unmangled anyway; MSVC (the PC simulator) does not.
#ifdef __cplusplus
extern "C" {
#endif

// Fonts (Poppins, 2 bpp). Light 60, Medium 40, SemiBold 40 and SemiBold 60
// carry 0x20-0x3A (space, digits, '%', '/', ':' ...) plus a few letters; the
// rest cover printable ASCII. No face carries U+00B7 (the middle dot), which
// is why Theme::setText() swaps it for '-'.
LV_FONT_DECLARE(poppins_italic_18);
LV_FONT_DECLARE(poppins_italic_20);
LV_FONT_DECLARE(poppins_light_60);
LV_FONT_DECLARE(poppins_medium_18);
LV_FONT_DECLARE(poppins_medium_25);
LV_FONT_DECLARE(poppins_medium_40);
LV_FONT_DECLARE(poppins_regular_14);
LV_FONT_DECLARE(poppins_regular_16);
LV_FONT_DECLARE(poppins_regular_18);
LV_FONT_DECLARE(poppins_semibold_20);
LV_FONT_DECLARE(poppins_semibold_25);
LV_FONT_DECLARE(poppins_semibold_30);
LV_FONT_DECLARE(poppins_semibold_35);
LV_FONT_DECLARE(poppins_semibold_40);
LV_FONT_DECLARE(poppins_semibold_60);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ASSETS_HPP
