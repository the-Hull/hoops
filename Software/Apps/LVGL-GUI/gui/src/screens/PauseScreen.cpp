/**
 ******************************************************************************
 * @file    PauseScreen.cpp
 * @brief   The paused session: time, a summary line and Resume / End & save /
 *          Discard.
 ******************************************************************************
 */

#include "gui/screens/PauseScreen.hpp"

#include <cstdio>

#include "score/Spots.hpp"
#include "score/Stats.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

constexpr int32_t kHeaderY   = 47;
constexpr int32_t kTimeY     = 76;
constexpr int32_t kSummaryY  = 105;
constexpr int32_t kSummaryMaxW = 200;

const ListMenu::Item kDiscardItem = { "Discard", nullptr, Theme::kBad };
const ListMenu::Item kConfirmItem = { "Discard?", "SELECT again", Theme::kBad };

} // namespace

PauseScreen::PauseScreen(Model& model)
    : Screen(model)
    , mKeys(&KeyInput::noHold, nullptr)
{
    mItems[kResume]  = { "Resume", nullptr, 0 };
    mItems[kEndSave] = { "End & save", nullptr, 0 };
    mItems[kDiscard] = kDiscardItem;
}

PauseScreen::~PauseScreen()
{
    if (mConfirmTimer) {
        lv_timer_delete(mConfirmTimer);
        mConfirmTimer = nullptr;
    }
}

void PauseScreen::build()
{
    mRing = std::make_unique<Ring>(mRoot);
    mRing->setDashed(Theme::kAmber);

    Theme::header(mRoot, "PAUSED", kHeaderY, Theme::kAmber);
    mTime    = Theme::label(mRoot, Font::SemiBold35, "", 20, Theme::topFor(Font::SemiBold35, kTimeY), 200);
    mSummary = Theme::label(mRoot, Font::Regular16, "", 20, Theme::topFor(Font::Regular16, kSummaryY), 200,
                            LV_TEXT_ALIGN_CENTER, Theme::kDim);

    mMenu = std::make_unique<ListMenu>(mRoot, ListMenu::pauseGeometry());
    mMenu->setCircular(false);
    mMenu->setItems(mItems, kItemCount, kResume);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Dot, Hints::Dot, Hints::Dot, Hints::Dot);

    if (mModel.hasSnapshot()) {
        show(mModel.snapshot());
    }
}

void PauseScreen::onShow()
{
    mModel.requestSnapshot();
}

void PauseScreen::show(const score::Snapshot& s)
{
    char buf[48];
    Theme::fmtDuration(buf, sizeof(buf), s.elapsed_s);
    Theme::setText(mTime, buf);

    // Open: the current spot's numbers; workout: the session's so far.
    if (s.mode == score::Snapshot::kModeWorkout) {
        snprintf(buf, sizeof(buf), "Step %u / %u \xC2\xB7 %u/%u \xC2\xB7 %u %%",
                 static_cast<unsigned>(s.stepIndex + 1), static_cast<unsigned>(s.stepCount),
                 static_cast<unsigned>(s.totalMakes), static_cast<unsigned>(s.totalAttempts),
                 static_cast<unsigned>(score::pct(s.totalMakes, s.totalAttempts)));
    } else {
        snprintf(buf, sizeof(buf), "%s \xC2\xB7 %u/%u \xC2\xB7 %u %%",
                 s.spot < score::kSpotCount ? score::spotInfo(s.spot).name : "",
                 static_cast<unsigned>(s.spotMakes), static_cast<unsigned>(s.spotAttempts),
                 static_cast<unsigned>(score::pct(s.spotMakes, s.spotAttempts)));
    }
    const Font f = Theme::textWidth(Font::Regular16, buf) <= kSummaryMaxW ? Font::Regular16 : Font::Regular14;
    lv_obj_set_style_text_font(mSummary, Theme::font(f), LV_PART_MAIN);
    lv_obj_set_y(mSummary, Theme::topFor(f, kSummaryY));
    Theme::setText(mSummary, buf);
}

// --- Actions -----------------------------------------------------------------

void PauseScreen::resume()
{
    // Back to where the session was paused from: the running snapshot that
    // answers RESUME says where (onSnapshot()).
    mModel.resume();
}

void PauseScreen::setConfirm(bool on)
{
    if (mConfirmTimer) {
        lv_timer_delete(mConfirmTimer);
        mConfirmTimer = nullptr;
    }
    mConfirming      = on;
    mItems[kDiscard] = on ? kConfirmItem : kDiscardItem;
    mMenu->refresh();
    if (on) {
        mConfirmTimer = lv_timer_create(&PauseScreen::confirmTimerCb, kConfirmMs, this);
    }
}

void PauseScreen::confirmTimerCb(lv_timer_t* t)
{
    static_cast<PauseScreen*>(lv_timer_get_user_data(t))->setConfirm(false);   // deletes the timer
}

void PauseScreen::select()
{
    switch (mMenu->selected()) {
        case kResume:
            resume();
            break;
        case kEndSave:
            mEnding = true;
            mModel.endSession(true);   // SAVED -> onSaved() -> Results
            break;
        case kDiscard:
            if (!mConfirming) {
                setConfirm(true);
                break;
            }
            setConfirm(false);
            mEnding = true;
            mModel.endSession(false);
            ScreenManager::instance().goTo(ScreenId::Home);
            break;
        default:
            break;
    }
}

void PauseScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind != KeyInput::Kind::Click || mEnding) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:
            mMenu->prev();
            if (mConfirming) {
                setConfirm(false);
            }
            break;
        case KeyInput::Btn::Down:
            mMenu->next();
            if (mConfirming) {
                setConfirm(false);
            }
            break;
        case KeyInput::Btn::Select: select(); break;
        case KeyInput::Btn::Back:   resume(); break;
        case KeyInput::Btn::None:   break;
    }
}

// --- Model events ------------------------------------------------------------

void PauseScreen::onSnapshot(const score::Snapshot& s)
{
    const auto st = static_cast<score::SessionState>(s.state);
    if (st == score::SessionState::Ended || st == score::SessionState::Idle) {
        // After End & save the service sends SAVED before this snapshot, so
        // mSaved is set and Results is on its way. Otherwise the session is
        // gone (discarded, or there was none): back to Home.
        if (!mSaved) {
            ScreenManager::instance().goTo(ScreenId::Home);
        }
        return;
    }
    if (st == score::SessionState::Paused) {
        mSeenPaused = true;
    } else if (mSeenPaused && !mEnding) {
        // Active / Rest after Paused (our RESUME, or a resume elsewhere): the
        // session runs again. Running snapshots before the first Paused one
        // predate our PAUSE and are only shown. Rest, and a finished workout
        // (its done summary), return to the Rest screen; the rest to Active.
        const bool doneSummary = mModel.hasStepDone() && mModel.lastStepDone().workoutDone
            && (s.flags & score::Snapshot::kWorkoutDone);
        mModel.resumeEntry = true;
        ScreenManager::instance().goTo(st == score::SessionState::Rest || doneSummary ? ScreenId::Rest
                                                                                     : ScreenId::Active);
        return;
    }
    show(s);
}

void PauseScreen::onSaved(uint32_t id)
{
    (void)id;
    // Results requests the session just ended (GET_RESULTS 0) on show.
    mSaved              = true;
    mModel.resultsFrom  = Model::ResultsFrom::Session;
    mModel.resultsPage  = 0;
    mModel.clearResults();
    ScreenManager::instance().goTo(ScreenId::Results);
}

void PauseScreen::onSuspend()
{
    mKeys.cancel();
}
