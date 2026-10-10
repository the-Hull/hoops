/**
 ******************************************************************************
 * @file    ActiveScreen.cpp
 * @brief   The shooting screen: log makes and misses, undo, pause.
 ******************************************************************************
 */

#include "gui/screens/ActiveScreen.hpp"

#include <cstdio>

#include "score/Spots.hpp"
#include "score/Stats.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

// Layout, docs/mockup-layout.md §6 / §8 (vertical centres).
constexpr int32_t kHeaderY   = 54;
constexpr int32_t kHeaderMaxW = 150;   // clear of the corner badges
constexpr int32_t kCountY    = 97;
constexpr int32_t kPctY      = 142;
constexpr int32_t kLast5Y    = 167;
constexpr int32_t kLast5X[5] = { 93, 106, 120, 134, 147 };
constexpr int32_t kDotR      = 4;     // 8.8 px dots
constexpr int32_t kNewestR   = 6;     // ring round the newest dot
constexpr int32_t kFooterY   = 186;
constexpr int32_t kHeartR    = 3;
constexpr int32_t kBodyLift  = 7;     // header / count / pct move up under the toast

constexpr int32_t kToastY    = 171;
constexpr int32_t kToastMinW = 129;
constexpr int32_t kToastH    = 27;
constexpr int32_t kToastPad  = 12;

constexpr Font kHeaderFonts[] = { Font::SemiBold20, Font::Medium18, Font::Regular16 };

} // namespace

ActiveScreen::ActiveScreen(Model& model)
    : Screen(model)
    , mKeys(&KeyInput::noHold, nullptr)
{
}

ActiveScreen::~ActiveScreen()
{
    if (mToastTimer) {
        lv_timer_delete(mToastTimer);
        mToastTimer = nullptr;
    }
}

void ActiveScreen::build()
{
    mRing = std::make_unique<Ring>(mRoot);
    mRing->setProgress(0, Theme::kDim);

    mBody   = Theme::container(mRoot, 0, 0, 240, 240);
    mHeader = Theme::label(mBody, Font::SemiBold20, "", 20, Theme::topFor(Font::SemiBold20, kHeaderY), 200,
                           LV_TEXT_ALIGN_CENTER, Theme::kAccent);

    // Makes and "/attempts" share a baseline; showCount() centres the pair.
    const int32_t makesTop = Theme::topFor(Font::SemiBold60, kCountY);
    const lv_font_t* big   = Theme::font(Font::SemiBold60);
    const int32_t baseline = makesTop + lv_font_get_line_height(big) - big->base_line;
    mMakes   = Theme::label(mBody, Font::SemiBold60, "", 0, makesTop, 1, LV_TEXT_ALIGN_LEFT, Theme::kText);
    mOfTotal = Theme::label(mBody, Font::SemiBold40, "", 0, Theme::topForBaseline(Font::SemiBold40, baseline), 1,
                            LV_TEXT_ALIGN_LEFT, Theme::kDim);

    mPct = Theme::label(mBody, Font::SemiBold30, "", 20, Theme::topFor(Font::SemiBold30, kPctY), 200);

    for (uint8_t i = 0; i < kLast5; ++i) {
        mDots[i] = Theme::dot(mRoot, kLast5X[i], kLast5Y, kDotR, Theme::kLine);
    }
    mNewest = Theme::arc(mRoot, kLast5X[kLast5 - 1], kLast5Y, kNewestR, 1, 0, 360, Theme::kText, false);
    Theme::setHidden(mNewest, true);

    const int32_t footTop = Theme::topFor(Font::Regular16, kFooterY);
    mTime  = Theme::label(mRoot, Font::Regular16, "", 0, footTop, 1, LV_TEXT_ALIGN_LEFT, Theme::kDim);
    mHeart = Theme::dot(mRoot, 0, kFooterY, kHeartR, Theme::kBad);
    mHr    = Theme::label(mRoot, Font::Regular16, "", 0, footTop, 1, LV_TEXT_ALIGN_LEFT, Theme::kDim);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Undo, Hints::PauseIcon, Hints::Make, Hints::Miss);

    mToast = Theme::box(mRoot, 120 - kToastMinW / 2, kToastY - kToastH / 2, kToastMinW, kToastH, Theme::kText,
                        kToastH / 2);
    mToastText = Theme::label(mRoot, Font::SemiBold20, "", 20, Theme::topFor(Font::SemiBold20, kToastY), 200,
                              LV_TEXT_ALIGN_CENTER, Theme::kBg);
    Theme::setHidden(mToast, true);
    Theme::setHidden(mToastText, true);

    showHeader("");
    showCount(0, 0);
    showPct(0, 0, false);
    showLast5(0, 0);
    showFooter(0, 0);
}

void ActiveScreen::onShow()
{
    const auto st = mModel.hasSnapshot() ? static_cast<score::SessionState>(mModel.snapshot().state)
                                         : score::SessionState::Idle;
    mLiveSeen   = false;
    mHasLeaving = st != score::SessionState::Active && st != score::SessionState::Rest;
    mLeaving    = st;
    if (mModel.hasSnapshot() && score::isRunning(st)) {
        rebuild(mModel.snapshot());
    }
    mModel.requestSnapshot();
}

// --- Drawing -----------------------------------------------------------------

void ActiveScreen::rebuild(const score::Snapshot& s)
{
    if (s.mode == score::Snapshot::kModeWorkout) {
        rebuildWorkout(s);
    } else {
        rebuildOpen(s);
    }
}

void ActiveScreen::rebuildOpen(const score::Snapshot& s)
{
    showHeader(s.spot < score::kSpotCount ? score::spotInfo(s.spot).header : "");
    showCount(s.spotMakes, s.spotAttempts);
    const bool full = (s.flags & score::Snapshot::kFull) != 0;
    showPct(s.spotMakes, s.spotAttempts, full);
    const uint8_t pct = score::pct(s.spotMakes, s.spotAttempts);
    mRing->setProgress(static_cast<uint16_t>(pct) * 10u, Theme::pctColor(s.spotMakes, s.spotAttempts));
    showLast5(s.last5Count, s.last5Bits);
    showFooter(s.elapsed_s, s.hr);
}

void ActiveScreen::rebuildWorkout(const score::Snapshot& s)
{
    // The workout layout (step line, segmented ring, shots left) arrives in
    // a later task; meanwhile the current step's counts in the open layout.
    showHeader(s.spot < score::kSpotCount ? score::spotInfo(s.spot).header : "");
    showCount(s.stepMakes, s.stepAttempts);
    showPct(s.stepMakes, s.stepAttempts, (s.flags & score::Snapshot::kFull) != 0);
    const uint8_t pct = score::pct(s.stepMakes, s.stepAttempts);
    mRing->setProgress(static_cast<uint16_t>(pct) * 10u, Theme::pctColor(s.stepMakes, s.stepAttempts));
    showLast5(s.last5Count, s.last5Bits);
    showFooter(s.elapsed_s, s.hr);
}

void ActiveScreen::showHeader(const char* text)
{
    // Long spot names ("MID · L-BASELINE") step down a face to stay clear
    // of the corner badges.
    Font f = kHeaderFonts[0];
    for (Font c : kHeaderFonts) {
        f = c;
        if (Theme::textWidth(c, text) <= kHeaderMaxW) {
            break;
        }
    }
    lv_obj_set_style_text_font(mHeader, Theme::font(f), LV_PART_MAIN);
    lv_obj_set_y(mHeader, Theme::topFor(f, kHeaderY));
    Theme::setText(mHeader, text);
}

void ActiveScreen::showCount(uint16_t makes, uint16_t attempts)
{
    char m[8];
    char a[8];
    snprintf(m, sizeof(m), "%u", static_cast<unsigned>(makes));
    snprintf(a, sizeof(a), "/%u", static_cast<unsigned>(attempts));
    const int32_t wm = Theme::textWidth(Font::SemiBold60, m);
    const int32_t wa = Theme::textWidth(Font::SemiBold40, a);
    const int32_t x  = 120 - (wm + wa) / 2;

    lv_obj_set_x(mMakes, x);
    lv_obj_set_width(mMakes, wm + 1);
    Theme::setText(mMakes, m);
    lv_obj_set_x(mOfTotal, x + wm);
    lv_obj_set_width(mOfTotal, wa + 1);
    Theme::setText(mOfTotal, a);
}

void ActiveScreen::showPct(uint16_t makes, uint16_t attempts, bool full)
{
    if (full) {
        Theme::setText(mPct, "FULL");
        lv_obj_set_style_text_color(mPct, Theme::rgb(Theme::kAmber), LV_PART_MAIN);
        return;
    }
    char buf[8];
    if (attempts == 0) {
        snprintf(buf, sizeof(buf), "--");
    } else {
        snprintf(buf, sizeof(buf), "%u %%", static_cast<unsigned>(score::pct(makes, attempts)));
    }
    Theme::setText(mPct, buf);
    lv_obj_set_style_text_color(mPct, Theme::rgb(Theme::pctColor(makes, attempts)), LV_PART_MAIN);
}

void ActiveScreen::showLast5(uint8_t count, uint8_t bits)
{
    // Oldest on the left, newest (bit 0) on the right; unused slots dim.
    for (uint8_t i = 0; i < kLast5; ++i) {
        const uint8_t age = static_cast<uint8_t>(kLast5 - 1 - i);
        uint32_t c = Theme::kLine;
        if (age < count) {
            c = (bits >> age) & 1u ? Theme::kGood : Theme::kBad;
        }
        lv_obj_set_style_bg_color(mDots[i], Theme::rgb(c), LV_PART_MAIN);
    }
    Theme::setHidden(mNewest, count == 0);
}

void ActiveScreen::showFooter(uint32_t elapsed_s, uint8_t hr)
{
    // "12:41 · ♥148": the faces have neither the middle dot nor a heart, so
    // the time and the heart rate are split by a red dot.
    char t[16];
    char h[8];
    Theme::fmtDuration(t, sizeof(t), elapsed_s);
    if (hr == 0) {
        snprintf(h, sizeof(h), "--");
    } else {
        snprintf(h, sizeof(h), "%u", static_cast<unsigned>(hr));
    }
    constexpr int32_t kGapL = 8;   // time .. heart
    constexpr int32_t kGapR = 3;   // heart .. bpm
    const int32_t wt    = Theme::textWidth(Font::Regular16, t);
    const int32_t wh    = Theme::textWidth(Font::Regular16, h);
    const int32_t total = wt + kGapL + 2 * kHeartR + kGapR + wh;
    const int32_t x     = 120 - total / 2;

    lv_obj_set_x(mTime, x);
    lv_obj_set_width(mTime, wt + 1);
    Theme::setText(mTime, t);
    lv_obj_set_x(mHeart, x + wt + kGapL);
    lv_obj_set_x(mHr, x + wt + kGapL + 2 * kHeartR + kGapR);
    lv_obj_set_width(mHr, wh + 1);
    Theme::setText(mHr, h);
}

// --- Toast -------------------------------------------------------------------

void ActiveScreen::showToast(const char* text)
{
    int32_t w = Theme::textWidth(Font::SemiBold20, text) + 2 * kToastPad;
    if (w < kToastMinW) {
        w = kToastMinW;
    }
    lv_obj_set_x(mToast, 120 - w / 2);
    lv_obj_set_width(mToast, w);
    Theme::setText(mToastText, text);
    Theme::setHidden(mToast, false);
    Theme::setHidden(mToastText, false);
    lv_obj_set_y(mBody, -kBodyLift);

    if (mToastTimer) {
        lv_timer_reset(mToastTimer);
    } else {
        mToastTimer = lv_timer_create(&ActiveScreen::toastTimerCb, kToastMs, this);
    }
}

void ActiveScreen::hideToast()
{
    if (mToastTimer) {
        lv_timer_delete(mToastTimer);
        mToastTimer = nullptr;
    }
    Theme::setHidden(mToast, true);
    Theme::setHidden(mToastText, true);
    lv_obj_set_y(mBody, 0);
}

void ActiveScreen::toastTimerCb(lv_timer_t* t)
{
    static_cast<ActiveScreen*>(lv_timer_get_user_data(t))->hideToast();   // deletes the timer
}

// --- Input -------------------------------------------------------------------

void ActiveScreen::changeSpot()
{
    // Open sessions only.
    if (!mModel.hasSnapshot() || mModel.snapshot().mode != score::Snapshot::kModeOpen) {
        return;
    }
    mModel.pickerMode = Model::PickerMode::ChangeSpot;
    mModel.pickerSpot = mModel.snapshot().spot;
    ScreenManager::instance().goTo(ScreenId::SpotPicker);
}

void ActiveScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind == KeyInput::Kind::HoldRelease && a.btn == KeyInput::Btn::Down) {
        changeSpot();
        return;
    }
    if (a.kind != KeyInput::Kind::Click) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Select: mModel.shot(true); break;
        case KeyInput::Btn::Back:   mModel.shot(false); break;
        case KeyInput::Btn::Up:     mModel.undo(); break;
        case KeyInput::Btn::Down:
            mModel.pause();
            ScreenManager::instance().goTo(ScreenId::Pause);
            break;
        case KeyInput::Btn::None:   break;
    }
}

// --- Model events ------------------------------------------------------------

void ActiveScreen::onSnapshot(const score::Snapshot& s)
{
    const auto st = static_cast<score::SessionState>(s.state);
    if (st == score::SessionState::Active || st == score::SessionState::Rest) {
        mLiveSeen = true;
        rebuild(s);
        return;
    }
    if (!mLiveSeen && mHasLeaving && st == mLeaving) {
        return;   // sent before the service handled our intent
    }
    ScreenManager::instance().goTo(st == score::SessionState::Paused ? ScreenId::Pause : ScreenId::Home);
}

void ActiveScreen::onUndoResult(bool ok, bool made)
{
    showToast(!ok ? "Nothing to undo" : (made ? "Make removed" : "Miss removed"));
}

void ActiveScreen::onSuspend()
{
    mKeys.cancel();
}
