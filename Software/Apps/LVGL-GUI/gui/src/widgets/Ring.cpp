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
constexpr int32_t kR  = 112;

constexpr int32_t kTrackW   = 5;
constexpr int32_t kFillW    = 8;
constexpr int32_t kSegW     = 8;
constexpr int32_t kDashW    = 3;
constexpr int32_t kGapTenths = 33;   // 3.3 deg between segments

} // namespace

Ring::Ring(lv_obj_t* parent)
    : mParent(parent)
{
}

void Ring::ensurePool(uint8_t n, int32_t width)
{
    if (n > kMaxSegments) {
        n = kMaxSegments;
    }
    if (width != mPoolWidth) {
        for (uint8_t i = 0; i < mPoolCount; ++i) {
            lv_obj_delete(mPool[i]);
            mPool[i] = nullptr;
        }
        mPoolCount = 0;
        mPoolWidth = width;
    }
    while (mPoolCount < n) {
        mPool[mPoolCount++] = Theme::arc(mParent, kCx, kCy, kR, width, 0, 1, Theme::kLine, false);
    }
    for (uint8_t i = 0; i < mPoolCount; ++i) {
        Theme::setHidden(mPool[i], i >= n);
    }
}

void Ring::setProgress(uint16_t permille, uint32_t color)
{
    if (permille > 1000) {
        permille = 1000;
    }
    if (mLook != Look::Progress) {
        ensurePool(0, mPoolWidth);
        mLook = Look::Progress;
    }
    if (!mTrack) {
        mTrack = Theme::arc(mParent, kCx, kCy, kR, kTrackW, 0, 360, Theme::kLine, false);
        mFill  = Theme::arc(mParent, kCx, kCy, kR, kFillW, 0, 1, color, true);
    }
    Theme::setHidden(mTrack, false);
    const int32_t end = (static_cast<int32_t>(permille) * 360 + 500) / 1000;
    Theme::setHidden(mFill, end == 0);
    if (end > 0) {
        Theme::setArc(mFill, 0, end);
        Theme::setArcColor(mFill, color);
    }
}

void Ring::setSegments(uint8_t n, const uint8_t* state)
{
    if (n > kMaxSegments) {
        n = kMaxSegments;
    }
    if (mTrack) {
        Theme::setHidden(mTrack, true);
        Theme::setHidden(mFill, true);
    }
    mLook = Look::Segments;
    ensurePool(n, kSegW);
    if (n == 0) {
        return;
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
        Theme::setArc(mPool[i], s, e);
        Theme::setArcColor(mPool[i], c);
    }
}

void Ring::setDashed(uint32_t color)
{
    if (mTrack) {
        Theme::setHidden(mTrack, true);
        Theme::setHidden(mFill, true);
    }
    mLook = Look::Dashed;
    ensurePool(kDashes, kDashW);
    for (uint8_t i = 0; i < kDashes; ++i) {
        const int32_t s = i * 15;
        Theme::setArc(mPool[i], s, s + 8);   // 7.5 deg, rounded to whole degrees
        Theme::setArcColor(mPool[i], color);
    }
}

void Ring::hide()
{
    if (mTrack) {
        Theme::setHidden(mTrack, true);
        Theme::setHidden(mFill, true);
    }
    for (uint8_t i = 0; i < mPoolCount; ++i) {
        Theme::setHidden(mPool[i], true);
    }
    mLook = Look::None;
}
