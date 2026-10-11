/**
 ******************************************************************************
 * @file    WorkoutPreviewScreen.cpp
 * @brief   One workout's steps before it starts.
 ******************************************************************************
 */

#include "gui/screens/WorkoutPreviewScreen.hpp"

#include <cstdio>
#include <cstring>

#include "score/Spots.hpp"
#include "score/Workout.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

// Layout, docs/mockup-layout.md §4 (vertical centres).
constexpr int32_t kHeaderY      = 46;
constexpr int32_t kRowY[4]      = { 69, 94, 119, 143 };
// Five rows, 19 px apart, starting low enough that row 1 clears the SELECT
// badge (centre y 55, r 9); the counts also stop short of it (x 200).
constexpr int32_t kDenseTop        = 68;
constexpr int32_t kDensePitch      = 19;
constexpr int32_t kDenseCountRight = 196;
constexpr uint8_t kMaxRows      = 5;
constexpr int32_t kNumX         = 46;
constexpr int32_t kNumR         = 9;
constexpr int32_t kLabelX       = 63;
constexpr int32_t kCountRight   = 202;
constexpr int32_t kRowGap       = 6;     // label .. count
constexpr int32_t kFooterY      = 166;
constexpr int32_t kStartY       = 189;
constexpr int32_t kStartW       = 70;
constexpr int32_t kStartH       = 22;

constexpr Font kLabelFonts[] = { Font::Medium18, Font::Regular16, Font::Regular14 };

int32_t rowY(uint8_t i, uint8_t rows)
{
    return rows <= 4 ? kRowY[i] : kDenseTop + i * kDensePitch;
}

int32_t countRight(uint8_t rows)
{
    return rows <= 4 ? kCountRight : kDenseCountRight;
}

} // namespace

WorkoutPreviewScreen::WorkoutPreviewScreen(Model& model)
    : Screen(model)
    , mKeys(&Screen::keyAction, this)
{
}

void WorkoutPreviewScreen::build()
{
    mContent = Theme::container(mRoot, 0, 0, 240, 240);

    // START pill (black text on green).
    Theme::box(mRoot, 120 - kStartW / 2, kStartY - kStartH / 2, kStartW, kStartH, Theme::kGood, kStartH / 2);
    const Font sf = Theme::textWidth(Font::SemiBold20, "START") <= kStartW - 8 ? Font::SemiBold20 : Font::Regular14;
    Theme::label(mRoot, sf, "START", 120 - kStartW / 2, Theme::topFor(sf, kStartY), kStartW, LV_TEXT_ALIGN_CENTER,
                 Theme::kBg);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Off, Hints::Dot, Hints::Make, Hints::Dot);
    mShown = show();
    mToast = std::make_unique<Toast>(mRoot);   // last: on top of the rows
}

void WorkoutPreviewScreen::onShow()
{
    // A user workout needs the list; presets are always there.
    if (!mModel.workoutById(mModel.selectedWorkoutId) && !mModel.workoutsLoaded()) {
        mModel.requestWorkouts();
    }
}

bool WorkoutPreviewScreen::show()
{
    lv_obj_clean(mContent);
    const score::Workout* w = mModel.workoutById(mModel.selectedWorkoutId);
    // A user workout while the list is on its way (e.g. just saved: the
    // one held may be stale) waits for it; onWorkouts() redraws.
    const bool loading = !mModel.workoutsLoaded() && !score::findPreset(mModel.selectedWorkoutId);
    if (!w || loading) {
        Theme::header(mContent, "WORKOUT", kHeaderY);
        Theme::label(mContent, Font::Regular16, loading ? "Loading..." : "Not found", 20,
                     Theme::topFor(Font::Regular16, 106), 200, LV_TEXT_ALIGN_CENTER, Theme::kDim);
        return false;
    }

    char upper[score::kNameLen + 1];
    size_t n = strnlen(w->name, score::kNameLen);
    for (size_t i = 0; i < n; ++i) {
        const char c = w->name[i];
        upper[i] = (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    }
    upper[n] = '\0';
    Theme::header(mContent, upper, kHeaderY);

    const uint8_t steps = w->stepCount < score::kMaxSteps ? w->stepCount : score::kMaxSteps;
    const uint8_t rows  = steps <= kMaxRows ? steps : kMaxRows;
    const int32_t right = countRight(rows);
    for (uint8_t i = 0; i < rows; ++i) {
        const int32_t y = rowY(i, rows);
        if (i == kMaxRows - 1 && steps > kMaxRows) {
            char more[24];
            snprintf(more, sizeof(more), "+ %u more steps", static_cast<unsigned>(steps - (kMaxRows - 1)));
            Theme::label(mContent, Font::Regular14, more, kLabelX, Theme::topFor(Font::Regular14, y),
                         right - kLabelX, LV_TEXT_ALIGN_LEFT, Theme::kDim);
            break;
        }
        const score::Step& st = w->steps[i];

        char num[4];
        snprintf(num, sizeof(num), "%u", static_cast<unsigned>(i + 1));
        Theme::dot(mContent, kNumX, y, kNumR, Theme::kLine);
        Theme::label(mContent, Font::Regular14, num, kNumX - kNumR, Theme::topFor(Font::Regular14, y), 2 * kNumR,
                     LV_TEXT_ALIGN_CENTER, Theme::kText);

        char count[12];
        Theme::fmtGoal(count, sizeof(count), st.goal.type, st.goal.value);
        const int32_t wc = Theme::textWidth(Font::SemiBold20, count);
        Theme::label(mContent, Font::SemiBold20, count, right - wc - 1, Theme::topFor(Font::SemiBold20, y), wc + 1,
                     LV_TEXT_ALIGN_RIGHT, Theme::kDim);

        // The spot, without its category part when it does not fit.
        const char* name    = score::spotInfo(st.spot).name;
        const char* texts[] = { name, Theme::spotShort(name) };
        Theme::fitLabel(mContent, texts, 2, kLabelFonts, 3, kLabelX, right - wc - kRowGap - kLabelX, y,
                        LV_TEXT_ALIGN_LEFT, Theme::kText);
    }

    // "Rest 30 s · about 12 min": the first step's rest, the estimate rounded up.
    char foot[40];
    const unsigned minutes = static_cast<unsigned>((w->estimateSeconds() + 59u) / 60u);
    if (steps > 0 && w->steps[0].rest_s > 0) {
        snprintf(foot, sizeof(foot), "Rest %u s%sabout %u min", static_cast<unsigned>(w->steps[0].rest_s),
                 Theme::kMidDot, minutes);
    } else {
        snprintf(foot, sizeof(foot), "No rest%sabout %u min", Theme::kMidDot, minutes);
    }
    Theme::label(mContent, Font::Regular14, foot, 20, Theme::topFor(Font::Regular14, kFooterY), 200,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);
    return true;
}

void WorkoutPreviewScreen::edit()
{
    const score::Workout* w = mModel.workoutById(mModel.selectedWorkoutId);
    if (!w || !mShown) {
        return;
    }
    score::Workout& d = mModel.draft();
    d = *w;
    if (!score::findPreset(w->id)) {
        mModel.draftIsNew = false;   // edit in place
    } else {
        // A preset stays as it is: edit a copy named "<name> 2".
        d.id = 0;
        size_t n = strnlen(w->name, score::kNameLen);
        if (n > score::kNameLen - 3) {
            n = score::kNameLen - 3;
        }
        memset(d.name, 0, sizeof(d.name));
        memcpy(d.name, w->name, n);
        memcpy(d.name + n, " 2", 2);
        mModel.draftIsNew = true;
    }
    mModel.draftStep  = 0;
    mModel.draftField = 0;
    ScreenManager::instance().goTo(ScreenId::StepBuilder);
}

void WorkoutPreviewScreen::onKey(uint8_t code)
{
    onAction(mKeys.feed(code));
}

void WorkoutPreviewScreen::onAction(KeyInput::Action a)
{
    if (!KeyInput::isActivate(a)) {
        return;
    }
    ScreenManager& sm = ScreenManager::instance();
    switch (a.btn) {
        case KeyInput::Btn::Select:
            if (!mShown) {
                break;
            }
            if (mModel.sessionRunning()) {
                mToast->show("Session in progress");   // the service would refuse it
                break;
            }
            mModel.startWorkout(mModel.selectedWorkoutId);
            sm.goTo(ScreenId::Active);
            break;
        case KeyInput::Btn::Down:   edit(); break;
        case KeyInput::Btn::Back:   sm.goTo(ScreenId::Workouts); break;
        case KeyInput::Btn::Up:
        case KeyInput::Btn::None:   break;
    }
}

void WorkoutPreviewScreen::onWorkouts()
{
    mShown = show();
}

void WorkoutPreviewScreen::onSuspend()
{
    mKeys.cancel();
}
