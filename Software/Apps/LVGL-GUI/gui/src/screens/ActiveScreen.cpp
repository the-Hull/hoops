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
#include "score/Workout.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

// Layout, docs/mockup-layout.md §6 / §7 / §8 (vertical centres).
struct Layout {
    int32_t headerY, countY, pctY, footerY;
    Font    footerFont;
    const Font* headerFonts;   ///< tried in order until the header fits
    uint8_t nHeaderFonts;
};
constexpr Font kOpenHeaderFonts[]    = { Font::SemiBold20, Font::Medium18, Font::Regular16 };
constexpr Font kWorkoutHeaderFonts[] = { Font::Regular16, Font::Regular14 };
constexpr Layout kOpen    = { 54, 97, 142, 186, Font::Regular16, kOpenHeaderFonts, 3 };
constexpr Layout kWorkout = { 64, 105, 148, 191, Font::Regular14, kWorkoutHeaderFonts, 2 };

constexpr int32_t kHeaderMaxW = 150;   // clear of the corner badges
constexpr int32_t kStepLineY = 48;     // workout: "STEP 2 / 4"
constexpr int32_t kProgressY = 174;    // workout: "3 shots left"
constexpr int32_t kLast5Y    = 167;
constexpr int32_t kLast5X[5] = { 93, 106, 120, 134, 147 };
constexpr int32_t kDotR      = 4;     // 8.8 px dots
constexpr int32_t kNewestR   = 6;     // ring round the newest dot
constexpr int32_t kHeartR    = 3;
constexpr int32_t kBodyLift  = 7;     // header / count / pct move up under the toast

constexpr int32_t kToastY    = 171;
constexpr int32_t kToastMinW = 129;
constexpr int32_t kToastH    = 27;
constexpr int32_t kToastPad  = 12;
constexpr int32_t kToastMaxW = 196;   // inside the round edge at y 158..185
constexpr Font kToastFonts[] = { Font::SemiBold20, Font::Medium18, Font::Regular16 };

const Layout& layoutFor(bool workout) { return workout ? kWorkout : kOpen; }

} // namespace

ActiveScreen::ActiveScreen(Model& model)
    : Screen(model)
    , mKeys(&ActiveScreen::onHold, this)
{
}

ActiveScreen::~ActiveScreen()
{
    if (mToastTimer) {
        lv_timer_delete(mToastTimer);
        mToastTimer = nullptr;
    }
    stopLiveTimer();
}

void ActiveScreen::build()
{
    mRing = std::make_unique<Ring>(mRoot);
    mRing->setProgress(0, Theme::kDim);

    mBody     = Theme::container(mRoot, 0, 0, 240, 240);
    mStepLine = Theme::label(mBody, Font::Regular14, "", 20, Theme::topFor(Font::Regular14, kStepLineY), 200,
                             LV_TEXT_ALIGN_CENTER, Theme::kDim);
    mHeader   = Theme::label(mBody, Font::SemiBold20, "", 20, 0, 200, LV_TEXT_ALIGN_CENTER, Theme::kAccent);

    // Makes and "/attempts" share a baseline (applyLayout() sets the y);
    // showCount() centres the pair.
    mMakes   = Theme::label(mBody, Font::SemiBold60, "", 0, 0, 1, LV_TEXT_ALIGN_LEFT, Theme::kText);
    mOfTotal = Theme::label(mBody, Font::SemiBold40, "", 0, 0, 1, LV_TEXT_ALIGN_LEFT, Theme::kDim);

    mPct = Theme::label(mBody, Font::SemiBold30, "", 20, 0, 200);

    for (uint8_t i = 0; i < kLast5; ++i) {
        mDots[i] = Theme::dot(mRoot, kLast5X[i], kLast5Y, kDotR, Theme::kLine);
    }
    mNewest = Theme::arc(mRoot, kLast5X[kLast5 - 1], kLast5Y, kNewestR, 1, 0, 360, Theme::kText, false);
    Theme::setHidden(mNewest, true);

    mProgress = Theme::label(mRoot, Font::Medium18, "", 20, Theme::topFor(Font::Medium18, kProgressY), 200,
                             LV_TEXT_ALIGN_CENTER, Theme::kText);

    mTime  = Theme::label(mRoot, Font::Regular16, "", 0, 0, 1, LV_TEXT_ALIGN_LEFT, Theme::kDim);
    mHeart = Theme::dot(mRoot, 0, 0, kHeartR, Theme::kBad);
    mHr    = Theme::label(mRoot, Font::Regular16, "", 0, 0, 1, LV_TEXT_ALIGN_LEFT, Theme::kDim);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Undo, Hints::PauseIcon, Hints::Make, Hints::Miss);

    mToast = Theme::box(mRoot, 120 - kToastMinW / 2, kToastY - kToastH / 2, kToastMinW, kToastH, Theme::kText,
                        kToastH / 2);
    mToastText = Theme::label(mRoot, Font::SemiBold20, "", 20, Theme::topFor(Font::SemiBold20, kToastY), 200,
                              LV_TEXT_ALIGN_CENTER, Theme::kBg);
    Theme::setHidden(mToast, true);
    Theme::setHidden(mToastText, true);

    const bool workout = mModel.hasSnapshot() && mModel.snapshot().mode == score::Snapshot::kModeWorkout;
    mWorkoutLayout = !workout;   // force the first applyLayout()
    applyLayout(workout);
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
    if (mModel.resumeEntry) {
        // Entered from Pause on a running snapshot: a Paused one still on
        // its way predates the resume.
        mModel.resumeEntry = false;
        mHasLeaving        = true;
        mLeaving           = score::SessionState::Paused;
    }
    if (mModel.hasSnapshot() && score::isRunning(st)) {
        rebuild(mModel.snapshot());
    }
    mModel.requestSnapshot();
    stopLiveTimer();
    mLiveTimer = lv_timer_create(&ActiveScreen::liveTimerCb, kLiveWaitMs, this);
}

void ActiveScreen::stopLiveTimer()
{
    if (mLiveTimer) {
        lv_timer_delete(mLiveTimer);
        mLiveTimer = nullptr;
    }
}

void ActiveScreen::liveTimerCb(lv_timer_t* t)
{
    // Backstop: the snapshots ignored as "sent before our intent" may be all
    // there is (e.g. a START refused because a session already runs, which
    // leaves the old Paused one). No live one in kLiveWaitMs: go where the
    // last snapshot says instead of logging shots into nothing.
    auto* self = static_cast<ActiveScreen*>(lv_timer_get_user_data(t));
    self->stopLiveTimer();   // one shot
    if (self->mLiveSeen) {
        return;
    }
    Model& m = self->mModel;
    m.requestSnapshot();
    const auto st = m.hasSnapshot() ? static_cast<score::SessionState>(m.snapshot().state) : score::SessionState::Idle;
    ScreenManager& sm = ScreenManager::instance();
    switch (st) {
        case score::SessionState::Active:
            self->mLiveSeen = true;   // running, and nothing says it predates us
            self->rebuild(m.snapshot());
            break;
        case score::SessionState::Rest:   sm.goTo(ScreenId::Rest); break;
        case score::SessionState::Paused: sm.goTo(ScreenId::Pause); break;
        default:                          sm.goTo(ScreenId::Home); break;
    }
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

void ActiveScreen::applyLayout(bool workout)
{
    if (workout == mWorkoutLayout) {
        return;
    }
    mWorkoutLayout = workout;
    const Layout& L = layoutFor(workout);

    const int32_t makesTop = Theme::topFor(Font::SemiBold60, L.countY);
    const lv_font_t* big   = Theme::font(Font::SemiBold60);
    const int32_t baseline = makesTop + lv_font_get_line_height(big) - big->base_line;
    lv_obj_set_y(mMakes, makesTop);
    lv_obj_set_y(mOfTotal, Theme::topForBaseline(Font::SemiBold40, baseline));
    lv_obj_set_y(mPct, Theme::topFor(Font::SemiBold30, L.pctY));

    const int32_t footTop = Theme::topFor(L.footerFont, L.footerY);
    lv_obj_set_style_text_font(mTime, Theme::font(L.footerFont), LV_PART_MAIN);
    lv_obj_set_style_text_font(mHr, Theme::font(L.footerFont), LV_PART_MAIN);
    lv_obj_set_y(mTime, footTop);
    lv_obj_set_y(mHr, footTop);
    lv_obj_set_y(mHeart, L.footerY - kHeartR);

    for (lv_obj_t* d : mDots) {
        Theme::setHidden(d, workout);
    }
    if (workout) {
        Theme::setHidden(mNewest, true);
    }
    Theme::setHidden(mStepLine, !workout);
    Theme::setHidden(mProgress, !workout);
}

void ActiveScreen::showBody(const score::Snapshot& s, uint16_t makes, uint16_t attempts)
{
    showHeader(s.spot < score::kSpotCount ? score::spotInfo(s.spot).header : "");
    showCount(makes, attempts);
    showPct(makes, attempts, (s.flags & score::Snapshot::kFull) != 0);
    showFooter(s.elapsed_s, s.hr);
}

void ActiveScreen::rebuildOpen(const score::Snapshot& s)
{
    applyLayout(false);
    showBody(s, s.spotMakes, s.spotAttempts);
    const uint8_t pct = score::pct(s.spotMakes, s.spotAttempts);
    mRing->setProgress(static_cast<uint16_t>(pct) * 10u, Theme::pctColor(s.spotMakes, s.spotAttempts));
    showLast5(s.last5Count, s.last5Bits);
}

void ActiveScreen::rebuildWorkout(const score::Snapshot& s)
{
    // After the last step the counts are the extra shots (the engine starts
    // a new "step" for them); the session totals are on the Rest screen.
    applyLayout(true);
    char buf[24];
    if (s.flags & score::Snapshot::kWorkoutDone) {
        Theme::setText(mStepLine, "WORKOUT DONE");
        lv_obj_set_style_text_color(mStepLine, Theme::rgb(Theme::kGood), LV_PART_MAIN);
    } else {
        snprintf(buf, sizeof(buf), "STEP %u / %u", static_cast<unsigned>(s.stepIndex + 1),
                 static_cast<unsigned>(s.stepCount));
        Theme::setText(mStepLine, buf);
        lv_obj_set_style_text_color(mStepLine, Theme::rgb(Theme::kDim), LV_PART_MAIN);
    }
    showBody(s, s.stepMakes, s.stepAttempts);
    showProgressLine(s);
    showWorkoutRing(s);
}

void ActiveScreen::showProgressLine(const score::Snapshot& s)
{
    char buf[24];
    const auto goal = static_cast<score::GoalType>(s.goalType);
    if (s.flags & score::Snapshot::kWorkoutDone) {
        snprintf(buf, sizeof(buf), "extra shots");
    } else if (goal == score::GoalType::Time) {
        char t[12];
        Theme::fmtDuration(t, sizeof(t), s.goalValue > s.stepElapsed_s ? s.goalValue - s.stepElapsed_s : 0u);
        snprintf(buf, sizeof(buf), "%s left", t);
    } else {
        const bool makes   = goal == score::GoalType::Makes;
        const uint16_t did = makes ? s.stepMakes : s.stepAttempts;
        const unsigned left = s.goalValue > did ? static_cast<unsigned>(s.goalValue - did) : 0u;
        if (left == 0) {
            snprintf(buf, sizeof(buf), "step done");   // shown until the Rest screen takes over
        } else if (!makes && left == 1) {
            snprintf(buf, sizeof(buf), "last shot!");
        } else {
            snprintf(buf, sizeof(buf), "%u %s left", left,
                     makes ? (left == 1 ? "make" : "makes") : "shots");
        }
    }
    Theme::setText(mProgress, buf);
}

void ActiveScreen::showWorkoutRing(const score::Snapshot& s)
{
    if (s.flags & score::Snapshot::kWorkoutDone) {
        mRing->setProgress(1000, Theme::kGood);
        return;
    }
    const auto goal = static_cast<score::GoalType>(s.goalType);
    if (goal == score::GoalType::Attempts && s.goalValue > 0 && s.goalValue <= Ring::kMaxSegments) {
        // One segment per shot of the step, coloured by the engine's bits.
        uint8_t states[Ring::kMaxSegments];
        for (uint8_t i = 0; i < s.goalValue; ++i) {
            states[i] = i >= s.stepAttempts ? 0 : (((s.stepBits() >> i) & 1u) ? 1 : 2);
        }
        mRing->setSegments(static_cast<uint8_t>(s.goalValue), states);
        return;
    }
    uint32_t done = s.stepAttempts;
    if (goal == score::GoalType::Makes) {
        done = s.stepMakes;
    } else if (goal == score::GoalType::Time) {
        done = s.stepElapsed_s;
    }
    const uint32_t permille = s.goalValue == 0 ? 1000u : (done >= s.goalValue ? 1000u : done * 1000u / s.goalValue);
    mRing->setProgress(static_cast<uint16_t>(permille), Theme::kAccent);
}

void ActiveScreen::showHeader(const char* text)
{
    // Long spot names ("MID · L-BASELINE") step down a face to stay clear
    // of the corner badges.
    const Layout& L = layoutFor(mWorkoutLayout);
    Font f = L.headerFonts[0];
    for (uint8_t i = 0; i < L.nHeaderFonts; ++i) {
        f = L.headerFonts[i];
        if (Theme::textWidth(f, text) <= kHeaderMaxW) {
            break;
        }
    }
    lv_obj_set_style_text_font(mHeader, Theme::font(f), LV_PART_MAIN);
    lv_obj_set_y(mHeader, Theme::topFor(f, L.headerY));
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
    Theme::fmtPct(buf, sizeof(buf), makes, attempts);
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
    Theme::setHidden(mNewest, count == 0 || mWorkoutLayout);
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
    const Font f        = layoutFor(mWorkoutLayout).footerFont;
    const int32_t wt    = Theme::textWidth(f, t);
    const int32_t wh    = Theme::textWidth(f, h);
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

void ActiveScreen::showToast(const char* text, uint32_t ms)
{
    // The first face whose pill fits the round screen, else the smallest.
    Font f    = kToastFonts[0];
    int32_t w = 0;
    for (const Font cand : kToastFonts) {
        f = cand;
        w = Theme::textWidth(f, text) + 2 * kToastPad;
        if (w <= kToastMaxW) {
            break;
        }
    }
    if (w < kToastMinW) {
        w = kToastMinW;
    }
    lv_obj_set_x(mToast, 120 - w / 2);
    lv_obj_set_width(mToast, w);
    lv_obj_set_style_text_font(mToastText, Theme::font(f), LV_PART_MAIN);
    lv_obj_set_y(mToastText, Theme::topFor(f, kToastY));
    Theme::setText(mToastText, text);
    Theme::setHidden(mToast, false);
    Theme::setHidden(mToastText, false);
    lv_obj_set_y(mBody, -kBodyLift);

    if (mToastTimer) {
        lv_timer_set_period(mToastTimer, ms);
        lv_timer_reset(mToastTimer);
    } else {
        mToastTimer = lv_timer_create(&ActiveScreen::toastTimerCb, ms, this);
    }
}

void ActiveScreen::hideToast()
{
    mHoldToast = false;
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

void ActiveScreen::pauseNow()
{
    mModel.pause();
    ScreenManager::instance().goTo(ScreenId::Pause);
}

void ActiveScreen::onHold(void* ctx, KeyInput::Action a)
{
    // Open session, DOWN held past 500 ms: say what the release will do.
    auto* self = static_cast<ActiveScreen*>(ctx);
    if (a.btn == KeyInput::Btn::Down && !self->mWorkoutLayout) {
        self->showToast("Release to change spot", kHoldToastMs);
        self->mHoldToast = true;
    }
}

void ActiveScreen::onKey(uint8_t code)
{
    if (mHoldToast) {
        hideToast();   // any key ends the hold hint: the release, or another button
    }
    const KeyInput::Action a = mKeys.feed(code);
    if (!KeyInput::isActivate(a)) {
        return;
    }
    if (a.kind == KeyInput::Kind::HoldRelease && a.btn == KeyInput::Btn::Down) {
        // Open session: change spot. A workout has no spot to change, so a
        // hold pauses like a click.
        if (mWorkoutLayout) {
            pauseNow();
        } else {
            changeSpot();
        }
        return;
    }
    // A slow press of SELECT / BACK / UP (HoldRelease) still logs its shot
    // or undo: it must not be lost.
    switch (a.btn) {
        case KeyInput::Btn::Select: mModel.shot(true); break;
        case KeyInput::Btn::Back:   mModel.shot(false); break;
        case KeyInput::Btn::Up:     mModel.undo(); break;
        case KeyInput::Btn::Down:   pauseNow(); break;
        case KeyInput::Btn::None:   break;
    }
}

// --- Model events ------------------------------------------------------------

void ActiveScreen::onSnapshot(const score::Snapshot& s)
{
    const auto st = static_cast<score::SessionState>(s.state);
    if (st == score::SessionState::Active) {
        mLiveSeen = true;
        rebuild(s);
        return;
    }
    if (st == score::SessionState::Rest) {
        // A step just ended (its STEP_DONE follows this snapshot), or the
        // session was found resting: the Rest screen shows it either way.
        mLiveSeen = true;
        rebuild(s);
        ScreenManager::instance().goTo(ScreenId::Rest);
        return;
    }
    if (!mLiveSeen && mHasLeaving && st == mLeaving) {
        return;   // sent before the service handled our intent
    }
    ScreenManager::instance().goTo(st == score::SessionState::Paused ? ScreenId::Pause : ScreenId::Home);
}

void ActiveScreen::onStepDone(const CustomMessage::StepDone& d)
{
    // The Model keeps d for the Rest screen.
    if (d.workoutDone || d.rest_s > 0) {
        ScreenManager::instance().goTo(ScreenId::Rest);
        return;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "Step %u done%s%u/%u", static_cast<unsigned>(d.stepIndex + 1), Theme::kMidDot,
             static_cast<unsigned>(d.makes), static_cast<unsigned>(d.attempts));
    showToast(buf, kStepToastMs);
}

void ActiveScreen::onUndoResult(bool ok, bool made)
{
    showToast(!ok ? "Nothing to undo" : (made ? "Make removed" : "Miss removed"), kToastMs);
}

void ActiveScreen::onSuspend()
{
    mKeys.cancel();
    if (mHoldToast) {
        hideToast();
    }
}
