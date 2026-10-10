/**
 ******************************************************************************
 * @file    ListMenu.cpp
 * @brief   Vertical wheel menu: teal pill plus dim neighbour rows.
 ******************************************************************************
 */

#include "gui/widgets/ListMenu.hpp"

namespace
{

constexpr int32_t kRowX = 20;
constexpr int32_t kRowW = 200;
constexpr int32_t kPillPad = 8;    // text inset inside the pill

lv_obj_t* makeLabel(lv_obj_t* parent, Theme::Font f, int32_t x, int32_t yCentre, int32_t w, uint32_t color)
{
    lv_obj_t* l = Theme::label(parent, f, "", x, Theme::topFor(f, yCentre), w, LV_TEXT_ALIGN_CENTER, color);
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_height(l, lv_font_get_line_height(Theme::font(f)));
    return l;
}

} // namespace

ListMenu::Geometry ListMenu::homeGeometry()
{
    Geometry g { 80, 168, 38, -1, 115, Theme::Font::SemiBold20, Theme::Font::Regular16 };
    g.below  = 3;
    g.rowGap = 25;   // 115, 140, 165
    return g;
}

ListMenu::Geometry ListMenu::pickerGeometry()
{
    Geometry g { 165, 176, 33, 129, 192, Theme::Font::SemiBold20, Theme::Font::Regular14 };
    g.below    = 1;
    g.rowColor = Theme::kDim;
    return g;
}

ListMenu::Geometry ListMenu::workoutsGeometry()
{
    Geometry g { 106, 182, 47, 69, 144, Theme::Font::Medium18, Theme::Font::Regular16 };
    g.below  = 2;
    g.rowGap = 24;   // 144, 168
    return g;
}

ListMenu::Geometry ListMenu::pauseGeometry()
{
    Geometry g { 137, 160, 33, -1, 168, Theme::Font::Medium18, Theme::Font::Regular16 };
    g.below  = 2;
    g.rowGap = 24;   // 168, 192
    return g;
}

ListMenu::ListMenu(lv_obj_t* parent, const Geometry& g)
    : mGeom(g)
{
    if (mGeom.below > kMaxBelow) {
        mGeom.below = kMaxBelow;
    }
    const int32_t pillX = 120 - mGeom.pillW / 2;
    const int32_t textW = mGeom.pillW - 2 * kPillPad;

    if (mGeom.prevY >= 0) {
        mPrev = makeLabel(parent, mGeom.itemFont, kRowX, mGeom.prevY, kRowW, mGeom.rowColor);
    }
    for (uint8_t i = 0; i < mGeom.below; ++i) {
        mBelow[i] = makeLabel(parent, mGeom.itemFont, kRowX, mGeom.nextY + i * mGeom.rowGap, kRowW, mGeom.rowColor);
    }

    mPill = Theme::box(parent, pillX, mGeom.pillY - mGeom.pillH / 2, mGeom.pillW, mGeom.pillH,
                       Theme::kAccent, LV_RADIUS_CIRCLE);
    // Single-line position; refresh() moves line 1 up when there is a line 2.
    mSel1 = makeLabel(parent, mGeom.selFont, pillX + kPillPad, mGeom.pillY, textW, Theme::kBg);
    mSel2 = makeLabel(parent, Theme::Font::Regular14, pillX + kPillPad, mGeom.pillY + mGeom.pillH / 4,
                      textW, Theme::kBg);
    refresh();
}

void ListMenu::setItems(const Item* items, uint16_t count, uint16_t selected)
{
    mItems    = items;
    mCount    = items ? count : 0;
    mSelected = (mCount == 0) ? 0 : (selected < mCount ? selected : static_cast<uint16_t>(mCount - 1));
    refresh();
}

void ListMenu::setCircular(bool circular)
{
    mCircular = circular;
    refresh();
}

void ListMenu::next()
{
    if (mCount == 0) {
        return;
    }
    if (mSelected + 1 < mCount) {
        ++mSelected;
    } else if (mCircular) {
        mSelected = 0;
    }
    refresh();
}

void ListMenu::prev()
{
    if (mCount == 0) {
        return;
    }
    if (mSelected > 0) {
        --mSelected;
    } else if (mCircular) {
        mSelected = static_cast<uint16_t>(mCount - 1);
    }
    refresh();
}

int32_t ListMenu::at(int32_t offset) const
{
    const int32_t n = mCount;
    if (n == 0) {
        return -1;
    }
    // Rows below claim neighbours first; the row above gets one only if it
    // is not already on screen below.
    const int32_t shownBelow = mCircular ? (mGeom.below < n - 1 ? mGeom.below : n - 1) : mGeom.below;
    if (offset > 0 && offset > shownBelow) {
        return -1;
    }
    if (offset < 0 && mCircular && shownBelow + 1 >= n) {
        return -1;
    }
    const int32_t i = static_cast<int32_t>(mSelected) + offset;
    if (mCircular) {
        return ((i % n) + n) % n;
    }
    return (i >= 0 && i < n) ? i : -1;
}

void ListMenu::showRow(lv_obj_t* row, int32_t index)
{
    if (!row) {
        return;
    }
    if (index < 0) {
        Theme::setHidden(row, true);
        return;
    }
    const Item& it = mItems[index];
    Theme::setText(row, it.line1);
    lv_obj_set_style_text_color(row, Theme::rgb(it.color ? it.color : mGeom.rowColor), LV_PART_MAIN);
    Theme::setHidden(row, false);
}

void ListMenu::refresh()
{
    const bool any = mCount > 0;
    Theme::setHidden(mPill, !any);
    Theme::setHidden(mSel1, !any);
    Theme::setHidden(mSel2, true);

    showRow(mPrev, any ? at(-1) : -1);
    for (uint8_t i = 0; i < mGeom.below; ++i) {
        showRow(mBelow[i], any ? at(i + 1) : -1);
    }
    if (!any) {
        return;
    }

    const Item& sel = mItems[mSelected];
    Theme::setText(mSel1, sel.line1);
    const bool two = sel.line2 != nullptr && sel.line2[0] != '\0';
    const int32_t y1 = two ? mGeom.pillY - mGeom.pillH / 5 - 1 : mGeom.pillY;
    lv_obj_set_y(mSel1, Theme::topFor(mGeom.selFont, y1));
    if (two) {
        Theme::setText(mSel2, sel.line2);
        Theme::setHidden(mSel2, false);
    }
}
