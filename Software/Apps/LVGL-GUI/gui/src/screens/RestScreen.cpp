/**
 ******************************************************************************
 * @file    RestScreen.cpp
 * @brief   Rest between workout steps, and the "workout done" summary.
 ******************************************************************************
 */

#include "gui/screens/RestScreen.hpp"

#include <cstdio>

#include "score/Spots.hpp"
#include "score/Stats.hpp"
#include "score/Workout.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

// Layout, docs/mockup-layout.md §9 (vertical centres).
constexpr int32_t kHeaderY    = 55;
constexpr int32_t kCountdownY = 93;
constexpr int32_t kDividerY   = 131;
constexpr int32_t kDividerW   = 120;
constexpr int32_t kNextLabelY = 144;
constexpr int32_t kNextStepY  = 162;
constexpr int32_t kNextStepW  = 180;
constexpr int32_t kResultY    = 186;
constexpr int32_t kResultLeft  = 44;    // clear of the DOWN badge at (31,183)
constexpr int32_t kResultRight = 200;   // inside the ring at y186 (BACK has no hint here)

// Done mode
constexpr int32_t kTotalY     = 93;
constexpr int32_t kPctY       = 136;
constexpr int32_t kSaveY      = 162;
constexpr int32_t kKeepY      = 184;

constexpr Font kNextFonts[]   = { Font::SemiBold20, Font::Medium18, Font::Regular16 };
constexpr Font kResultFonts[] = { Font::Regular16, Font::Regular14 };

bool resting(const Model& m)
{
    return m.hasSnapshot() && static_cast<score::SessionState>(m.snapshot().state) == score::SessionState::Rest;
}

} // namespace

RestScreen::RestScreen(Model& model)
    : Screen(model)
    , mKeys(&KeyInput::noHold, nullptr)
{
}

void RestScreen::build()
{
    mRing = std::make_unique<Ring>(mRoot);
    mRing->setProgress(0, Theme::kAmber);   // its arcs first, under everything else
    mContent   = Theme::container(mRoot, 0, 0, 240, 240);
    mCountdown = Theme::label(mRoot, Font::SemiBold60, "", 20, Theme::topFor(Font::SemiBold60, kCountdownY), 200);
    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Dot, Hints::PauseIcon, Hints::Dot, Hints::Off);
    buildContent();
}

void RestScreen::onShow()
{
    // Entered from Pause on a running snapshot: Paused ones still on their
    // way predate the resume, so they are ignored until a live one.
    mIgnorePaused      = mModel.resumeEntry;
    mModel.resumeEntry = false;
    mModel.requestSnapshot();
}

// --- State -------------------------------------------------------------------

bool RestScreen::doneMode() const
{
    return mModel.hasStepDone() && mModel.lastStepDone().workoutDone != 0;
}

RestScreen::Info RestScreen::resolve() const
{
    Info in {};
    const score::Snapshot* s = mModel.hasSnapshot() ? &mModel.snapshot() : nullptr;
    in.stepCount = s ? s->stepCount : 0;

    // During Rest the snapshot's stepIndex is still the finished step's, so a
    // STEP_DONE for another step is an old one.
    const CustomMessage::StepDone& d = mModel.lastStepDone();
    if (mModel.hasStepDone() && (!s || d.stepIndex == s->stepIndex)) {
        in.stepIndex     = d.stepIndex;
        in.makes         = d.makes;
        in.attempts      = d.attempts;
        in.rest_s        = d.rest_s;
        in.nextSpot      = d.nextSpot;
        in.nextGoalType  = d.nextGoalType;
        in.nextGoalValue = d.nextGoalValue;
        in.hasNext       = d.workoutDone == 0;
        return in;
    }
    if (!s) {
        return in;
    }
    // No STEP_DONE for this rest (the GUI was away when the step ended):
    // the step's counts are in the snapshot, the next step in the workout.
    in.stepIndex = s->stepIndex;
    in.makes     = s->stepMakes;
    in.attempts  = s->stepAttempts;
    in.rest_s    = s->restRemaining_s;
    in.nextSpot  = s->nextSpot;
    in.hasNext   = s->stepIndex + 1 < s->stepCount;
    const score::Workout* w = mModel.workoutById(static_cast<uint8_t>(s->workoutId));
    if (w && s->stepIndex + 1 < w->stepCount) {
        const score::Step& next = w->steps[s->stepIndex + 1];
        in.rest_s        = w->steps[s->stepIndex].rest_s;
        in.nextSpot      = next.spot;
        in.nextGoalType  = next.goal.type;
        in.nextGoalValue = next.goal.value;
    }
    return in;
}

// --- Drawing -----------------------------------------------------------------

void RestScreen::buildContent()
{
    lv_obj_clean(mContent);
    mDone = doneMode();
    Theme::setHidden(mCountdown, mDone);
    if (mDone) {
        buildDone();
        return;
    }
    const Info in = resolve();
    buildRest(in);
    if (mTotal < in.rest_s) {
        mTotal = in.rest_s;
    }
    showCountdown(resting(mModel) ? mModel.snapshot().restRemaining_s : in.rest_s);
}

void RestScreen::buildRest(const Info& in)
{
    Theme::header(mContent, "REST", kHeaderY, Theme::kAmber);
    Theme::hline(mContent, 120 - kDividerW / 2, kDividerY - 1, kDividerW, Theme::kLine);

    char buf[48];
    if (in.hasNext) {
        snprintf(buf, sizeof(buf), "NEXT%sSTEP %u / %u", Theme::kMidDot, static_cast<unsigned>(in.stepIndex + 2),
                 static_cast<unsigned>(in.stepCount));
        Theme::label(mContent, Font::Regular14, buf, 20, Theme::topFor(Font::Regular14, kNextLabelY), 200,
                     LV_TEXT_ALIGN_CENTER, Theme::kDim);

        // "Free Throw x10"; the spot without its category when too wide.
        const char* name = in.nextSpot < score::kSpotCount ? score::spotInfo(in.nextSpot).name : "";
        char goal[12] = "";
        if (in.nextGoalValue > 0) {
            Theme::fmtGoal(goal, sizeof(goal), in.nextGoalType, in.nextGoalValue);
        }
        char full[40];
        char brief[40];
        snprintf(full, sizeof(full), "%s%s%s", name, goal[0] ? " " : "", goal);
        snprintf(brief, sizeof(brief), "%s%s%s", Theme::spotShort(name), goal[0] ? " " : "", goal);
        const char* texts[] = { full, brief };
        Theme::fitLabel(mContent, texts, 2, kNextFonts, 3, 120 - kNextStepW / 2, kNextStepW, kNextStepY,
                        LV_TEXT_ALIGN_CENTER, Theme::kText);
    }

    // "Step 2 done · 7/10 · " dim + "70 %" coloured; the smaller face, then
    // a shorter form, when it would not fit between the DOWN badge and the
    // ring.
    char pct[8];
    Theme::fmtPct(pct, sizeof(pct), in.makes, in.attempts);
    char forms[2][40];
    snprintf(forms[0], sizeof(forms[0]), "Step %u done%s%u/%u%s", static_cast<unsigned>(in.stepIndex + 1),
             Theme::kMidDot, static_cast<unsigned>(in.makes), static_cast<unsigned>(in.attempts), Theme::kMidDot);
    snprintf(forms[1], sizeof(forms[1]), "Step %u%s%u/%u%s", static_cast<unsigned>(in.stepIndex + 1), Theme::kMidDot,
             static_cast<unsigned>(in.makes), static_cast<unsigned>(in.attempts), Theme::kMidDot);
    // Centred when it clears the badge, else moved right just enough.
    const char* text = forms[1];
    Font f = Font::Regular14;
    int32_t w = 0;
    bool fits = false;
    for (const char* t : forms) {
        for (Font c : kResultFonts) {
            const int32_t tw = Theme::textWidth(c, t) + Theme::textWidth(c, pct);
            if (!fits && tw <= kResultRight - kResultLeft) {
                text = t, f = c, w = tw, fits = true;
            }
        }
    }
    if (!fits) {
        w = Theme::textWidth(f, text) + Theme::textWidth(f, pct);
    }
    const int32_t cx = 120 - w / 2 >= kResultLeft ? 120 : kResultLeft + w / 2;
    Theme::pair(mContent, f, text, Theme::kDim, f, pct, Theme::pctColor(in.makes, in.attempts), kResultY, 0, cx);
}

void RestScreen::buildDone()
{
    // The session totals: after the last step the snapshot's step counts are
    // the extra shots only.
    const score::Snapshot s = mModel.hasSnapshot() ? mModel.snapshot() : score::Snapshot {};
    Theme::header(mContent, "WORKOUT DONE", kHeaderY, Theme::kGood);

    char makes[8];
    char ofTotal[8];
    snprintf(makes, sizeof(makes), "%u", static_cast<unsigned>(s.totalMakes));
    snprintf(ofTotal, sizeof(ofTotal), "/%u", static_cast<unsigned>(s.totalAttempts));
    Theme::pair(mContent, Font::SemiBold60, makes, Theme::kText, Font::SemiBold40, ofTotal, Theme::kDim, kTotalY, 0);

    char pct[8];
    Theme::fmtPct(pct, sizeof(pct), s.totalMakes, s.totalAttempts);
    const uint32_t color = Theme::pctColor(s.totalMakes, s.totalAttempts);
    Theme::label(mContent, Font::SemiBold30, pct, 20, Theme::topFor(Font::SemiBold30, kPctY), 200,
                 LV_TEXT_ALIGN_CENTER, color);

    char line[32];
    snprintf(line, sizeof(line), "SELECT%ssave", Theme::kMidDot);
    Theme::label(mContent, Font::Regular14, line, 20, Theme::topFor(Font::Regular14, kSaveY), 200,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);
    snprintf(line, sizeof(line), "UP%skeep shooting", Theme::kMidDot);
    Theme::label(mContent, Font::Regular14, line, 20, Theme::topFor(Font::Regular14, kKeepY), 200,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);

    mRing->setProgress(static_cast<uint16_t>(score::pct(s.totalMakes, s.totalAttempts)) * 10u, color);
}

void RestScreen::showCountdown(uint16_t remaining)
{
    if (remaining > mTotal) {
        mTotal = remaining;   // +15 s makes the ring's scale grow
    }
    char buf[12];
    Theme::fmtDuration(buf, sizeof(buf), remaining);
    Theme::setText(mCountdown, buf);
    const uint32_t permille = mTotal == 0 ? 0u : static_cast<uint32_t>(remaining) * 1000u / mTotal;
    mRing->setProgress(static_cast<uint16_t>(permille), Theme::kAmber);
}

// --- Actions -----------------------------------------------------------------

void RestScreen::save()
{
    mEnding = true;
    mModel.endSession(true);   // SAVED -> onSaved() -> Results
}

void RestScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind != KeyInput::Kind::Click || mEnding) {
        return;
    }
    ScreenManager& sm = ScreenManager::instance();
    switch (a.btn) {
        case KeyInput::Btn::Up:
            if (mDone) {
                sm.goTo(ScreenId::Active);   // keep shooting: extra shots
            } else {
                mModel.extendRest();
            }
            break;
        case KeyInput::Btn::Select:
            if (mDone) {
                save();
            } else {
                mModel.skipRest();           // the Active snapshot switches back
            }
            break;
        case KeyInput::Btn::Down:
            mModel.pause();
            sm.goTo(ScreenId::Pause);
            break;
        case KeyInput::Btn::Back:
        case KeyInput::Btn::None:
            break;
    }
}

// --- Model events ------------------------------------------------------------

void RestScreen::onSnapshot(const score::Snapshot& s)
{
    const auto st = static_cast<score::SessionState>(s.state);
    ScreenManager& sm = ScreenManager::instance();
    if (st == score::SessionState::Ended || st == score::SessionState::Idle) {
        // After save the service sends SAVED before this snapshot, so mSaved
        // is set and Results is on its way.
        if (!mSaved) {
            sm.goTo(ScreenId::Home);
        }
        return;
    }
    if (st == score::SessionState::Paused) {
        if (!(mIgnorePaused && !mLiveSeen)) {
            sm.goTo(ScreenId::Pause);
        }
        return;
    }
    mLiveSeen = true;   // Active or Rest
    if (mDone) {
        // The service keeps the finished workout Active (for extra shots);
        // no shot can be logged from here, so the totals shown stand.
        return;
    }
    if (st == score::SessionState::Active) {
        sm.goTo(ScreenId::Active);   // rest over or skipped
        return;
    }
    showCountdown(s.restRemaining_s);
}

void RestScreen::onStepDone(const CustomMessage::StepDone& d)
{
    (void)d;   // the Model holds it; resolve() reads it from there
    buildContent();
}

void RestScreen::onSaved(uint32_t id)
{
    (void)id;
    // As PauseScreen's End & save: Results requests the session just ended.
    mSaved             = true;
    mModel.resultsFrom = Model::ResultsFrom::Session;
    mModel.resultsPage = 0;
    mModel.clearResults();
    ScreenManager::instance().goTo(ScreenId::Results);
}

void RestScreen::onSuspend()
{
    mKeys.cancel();
}
