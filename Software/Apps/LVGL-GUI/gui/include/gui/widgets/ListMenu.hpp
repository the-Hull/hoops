/**
 ******************************************************************************
 * @file    ListMenu.hpp
 * @brief   Vertical wheel menu as in the mockups: the selected item in a teal
 *          pill (black text), an optional dim row above it and dim rows below.
 *
 * No animation. Rows show the neighbours of the selected item (wrapping when
 * circular, the default); an item is never shown twice. The items array is
 * not copied: it must outlive the menu or the next setItems(). Texts pass
 * through Theme::setText(), so spot names with " · " render.
 *
 * Geometry y values are vertical centres in screen px (as the mockup's); a
 * negative prevY means no row above. `below` rows start at nextY, rowGap
 * apart. An item's colour applies when it is not selected; 0 (black, which
 * would be invisible) means the geometry's rowColor.
 ******************************************************************************
 */

#ifndef LIST_MENU_HPP
#define LIST_MENU_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/theme/Theme.hpp"

class ListMenu
{
public:
    struct Item {
        const char* line1;
        const char* line2;   ///< second line inside the pill, or nullptr
        uint32_t    color;   ///< colour when not selected (0 = rowColor)
    };

    struct Geometry {
        int16_t     pillY, pillW, pillH, prevY, nextY;
        Theme::Font selFont, itemFont;
        uint8_t     below    = 2;              ///< rows under the pill (max kMaxBelow)
        int16_t     rowGap   = 24;
        uint32_t    rowColor = Theme::kText;
    };

    static Geometry homeGeometry();       ///< pill 168x38 y80; three rows below, none above
    static Geometry pickerGeometry();     ///< pill 176x33 y165; prev y129, next y192
    static Geometry workoutsGeometry();   ///< pill 182x47 y106 (two lines); prev y69, next y144/y168
    static Geometry pauseGeometry();      ///< pill 160x33 y137; next y168/y192, none above

    static constexpr uint8_t kMaxBelow = 3;

    ListMenu(lv_obj_t* parent, const Geometry& g);

    void setItems(const Item* items, uint16_t count, uint16_t selected = 0);
    void next();
    void prev();
    uint16_t selected() const { return mSelected; }
    void refresh();
    void setCircular(bool circular);

private:
    /// Index @p offset rows from the selection, or -1 if there is none.
    int32_t at(int32_t offset) const;
    void showRow(lv_obj_t* row, int32_t index);

    Geometry    mGeom;
    lv_obj_t*   mPill  = nullptr;
    lv_obj_t*   mSel1  = nullptr;
    lv_obj_t*   mSel2  = nullptr;
    lv_obj_t*   mPrev  = nullptr;
    lv_obj_t*   mBelow[kMaxBelow] = {};
    const Item* mItems    = nullptr;
    uint16_t    mCount    = 0;
    uint16_t    mSelected = 0;
    bool        mCircular = true;
};

#endif // LIST_MENU_HPP
