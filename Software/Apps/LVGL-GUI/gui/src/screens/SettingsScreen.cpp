/**
 ******************************************************************************
 * @file    SettingsScreen.cpp
 * @brief   The user settings: thirteen rows, SELECT steps a row to its next value.
 ******************************************************************************
 */

#include "gui/screens/SettingsScreen.hpp"

#include <cstdio>

#include "score/Workout.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/screens/StepValues.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

constexpr int32_t kHeaderY = 49;

constexpr const char* kNames[] = {
    "Goal type", "Goal value", "Rest", "Open target", "Hold DOWN", "Haptics",
    "Backlight on shot", "Heart rate", "Colour scheme", "CSV export", "Clear history",
    "Restore workouts", "About",
};

constexpr uint16_t kOpenTargets[] = { 0, 25, 50, 100, 200 };
constexpr StepValues::Values kTargets = { kOpenTargets, sizeof(kOpenTargets) / sizeof(kOpenTargets[0]) };

#ifdef BUILD_VERSION
constexpr const char* kAboutText = "Hoops v" BUILD_VERSION;
#else
constexpr const char* kAboutText = "Hoops dev";
#endif

/// The entry after @p cur, wrapping to the first (a value not in the list
/// steps onto the next one above it).
uint16_t cycle(const StepValues::Values& l, uint16_t cur)
{
    const uint16_t next = StepValues::stepValue(l, cur, +1);
    return next != cur ? next : l.v[0];
}

/// @p cur + 1 modulo @p n (an out-of-range value restarts at 0).
uint8_t cycle(uint8_t cur, uint8_t n)
{
    return static_cast<uint8_t>(cur + 1 < n ? cur + 1 : 0);
}

const char* onOff(uint8_t v)
{
    return v ? "On" : "Off";
}

} // namespace

static_assert(sizeof(kNames) / sizeof(kNames[0]) == 13, "one name per row");

SettingsScreen::SettingsScreen(Model& model)
    : Screen(model)
    , mKeys(&Screen::keyAction, this)
    , mEdit(model.settings())
{
}

SettingsScreen::~SettingsScreen()
{
    stopConfirm();
}

void SettingsScreen::build()
{
    Theme::header(mRoot, "SETTINGS", kHeaderY);
    mMenu  = std::make_unique<ListMenu>(mRoot, ListMenu::workoutsGeometry());
    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Dot, Hints::Dot, Hints::Dot, Hints::Dot);
    mToast = std::make_unique<Toast>(mRoot);   // last: on top

    for (uint8_t i = 0; i < kRowCount; ++i) {
        mItems[i] = { kNames[i], mValue[i], 0 };
    }
    mEdit = mModel.settings();
    mMenu->setItems(mItems, kRowCount, mModel.settingsIndex < kRowCount ? mModel.settingsIndex : 0);
    refresh();
}

void SettingsScreen::onShow()
{
    // The first page carries the total (the Clear history count); the user
    // workouts (the Restore workouts count) follow it in onHistory(): never
    // both replies in one frame (10 items + the page overflow the GUI queue).
    mModel.requestHistory(false);
    if (!mModel.hasSettings()) {
        mModel.requestSettings();   // SELECT waits for them (change())
    }
    refresh();
}

void SettingsScreen::onHide()
{
    mModel.settingsIndex = static_cast<uint8_t>(mMenu->selected());
}

void SettingsScreen::refresh()
{
    const score::Settings& s = mEdit;
    auto put = [this](uint8_t row, const char* fmt, unsigned v) {
        snprintf(mValue[row], sizeof(mValue[row]), fmt, v);
    };
    auto text = [this](uint8_t row, const char* t) { snprintf(mValue[row], sizeof(mValue[row]), "%s", t); };

    text(kGoalType, StepValues::goalName(s.goalType));
    switch (static_cast<score::GoalType>(s.goalType)) {
        case score::GoalType::Time:  put(kGoalValue, "%u s", s.goalValue); break;
        case score::GoalType::Makes: put(kGoalValue, "%u makes", s.goalValue); break;
        default:                     put(kGoalValue, "%u shots", s.goalValue); break;
    }
    put(kRest, "%u s", s.rest_s);
    if (s.openTarget == 0) {
        text(kOpenTarget, "Off");
    } else {
        put(kOpenTarget, "%u shots", s.openTarget);
    }
    text(kHoldDown, static_cast<score::HoldDown>(s.holdDown) == score::HoldDown::Pause ? "Pause" : "Change spot");
    switch (static_cast<score::Haptics>(s.haptics)) {
        case score::Haptics::Off:   text(kHaptics, "Off"); break;
        case score::Haptics::Light: text(kHaptics, "Light"); break;
        default:                    text(kHaptics, "Strong"); break;
    }
    text(kBacklight, onOff(s.backlightOnShot));
    text(kHeartRate, onOff(s.heartRate));
    text(kScheme, static_cast<score::Scheme>(s.scheme) == score::Scheme::Light ? "Light" : "Dark");
    text(kCsv, onOff(s.csvExport));
    if (!mModel.hasSettings()) {
        // Not known yet: no value to show, and none to step from (change()).
        for (uint8_t row = kGoalType; row <= kCsv; ++row) {
            text(row, "Loading\xE2\x80\xA6");
        }
    }

    const uint8_t total = mModel.historyTotal();
    if (mConfirmTimer && mConfirmRow == kClear) {
        text(kClear, "SELECT again to confirm");
    } else if (!mModel.hasHistory()) {
        text(kClear, "Loading\xE2\x80\xA6");
    } else if (total > 0 || mCleared) {
        put(kClear, total == 1 ? "%u session" : "%u sessions", total);
    } else {
        text(kClear, "Nothing to clear");
    }
    const uint8_t custom = mModel.userWorkoutCount();
    if (mConfirmTimer && mConfirmRow == kReset) {
        text(kReset, "SELECT again to confirm");
    } else if (!mModel.workoutsLoaded()) {
        text(kReset, "Loading\xE2\x80\xA6");
    } else if (customRunning()) {
        text(kReset, "Not while running");
    } else if (custom == 0) {
        text(kReset, "No custom workouts");
    } else {
        put(kReset, "Deletes %u custom", custom);   // "1 custom" reads as well as "3 custom"
    }
    text(kAbout, kAboutText);

    mMenu->refresh();
}

void SettingsScreen::change(uint8_t row)
{
    // Until the service's SETTINGS is in, mEdit holds the defaults: a change
    // would store defaults + one step over the user's settings.bin.
    if (!mModel.hasSettings()) {
        return;
    }
    score::Settings& s = mEdit;
    switch (row) {
        case kGoalType:
            s.goalType  = cycle(s.goalType, 3);
            s.goalValue = StepValues::nearest(StepValues::valuesFor(s.goalType), s.goalValue);
            break;
        case kGoalValue:
            s.goalValue = cycle(StepValues::valuesFor(s.goalType), s.goalValue);
            break;
        case kRest:
            s.rest_s = static_cast<uint8_t>(cycle(StepValues::kRests, s.rest_s));
            break;
        case kOpenTarget: s.openTarget      = cycle(kTargets, s.openTarget); break;
        case kHoldDown:   s.holdDown        = cycle(s.holdDown, 2); break;
        case kHaptics:    s.haptics         = cycle(s.haptics, 3); break;
        case kBacklight:  s.backlightOnShot = s.backlightOnShot ? 0 : 1; break;
        case kHeartRate:  s.heartRate       = s.heartRate ? 0 : 1; break;
        case kScheme:     s.scheme          = cycle(s.scheme, 2); break;
        case kCsv:        s.csvExport       = s.csvExport ? 0 : 1; break;
        case kClear:      clearHistory(); return;
        case kReset:      resetWorkouts(); return;
        default:          return;   // About
    }
    // A scheme change rebuilds this screen (asynchronously) in the new palette.
    mModel.setSettings(mEdit);
    refresh();
}

void SettingsScreen::clearHistory()
{
    if (mAwaitClear || !mModel.hasHistory() || mModel.historyTotal() == 0) {
        return;
    }
    if (!mConfirmTimer || mConfirmRow != kClear) {
        stopConfirm();
        mConfirmRow   = kClear;
        mConfirmTimer = lv_timer_create(&SettingsScreen::confirmTimerCb, kConfirmMs, this);
        refresh();
        return;
    }
    stopConfirm();
    mAwaitClear = true;
    mModel.clearHistory();   // the empty HISTORY page -> onHistory()
    refresh();
}

bool SettingsScreen::customRunning() const
{
    return mModel.sessionRunning() && mModel.snapshot().workoutId >= score::kFirstUserWorkoutId;
}

void SettingsScreen::resetWorkouts()
{
    if (mAwaitReset || !mModel.workoutsLoaded() || mModel.userWorkoutCount() == 0 || customRunning()) {
        return;
    }
    if (!mConfirmTimer || mConfirmRow != kReset) {
        stopConfirm();
        mConfirmRow   = kReset;
        mConfirmTimer = lv_timer_create(&SettingsScreen::confirmTimerCb, kConfirmMs, this);
        refresh();
        return;
    }
    stopConfirm();
    mAwaitReset = true;
    mModel.resetWorkouts();   // the (empty) list -> onWorkouts()
    refresh();
}

void SettingsScreen::stopConfirm()
{
    if (mConfirmTimer) {
        lv_timer_delete(mConfirmTimer);
        mConfirmTimer = nullptr;
    }
}

void SettingsScreen::confirmTimerCb(lv_timer_t* t)
{
    auto* self = static_cast<SettingsScreen*>(lv_timer_get_user_data(t));
    self->stopConfirm();
    self->refresh();
}

void SettingsScreen::onKey(uint8_t code)
{
    onAction(mKeys.feed(code));
}

void SettingsScreen::onAction(KeyInput::Action a)
{
    if (!KeyInput::isActivate(a)) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:
        case KeyInput::Btn::Down:
            if (mConfirmTimer) {   // moving off the row drops the confirm window
                stopConfirm();
                refresh();
            }
            if (a.btn == KeyInput::Btn::Up) {
                mMenu->prev();
            } else {
                mMenu->next();
            }
            break;
        case KeyInput::Btn::Select: change(static_cast<uint8_t>(mMenu->selected())); break;
        case KeyInput::Btn::Back:   ScreenManager::instance().goTo(ScreenId::Home); break;
        case KeyInput::Btn::None:   break;
    }
}

void SettingsScreen::onSettings()
{
    mEdit = mModel.settings();   // the echo wins over the local copy
    refresh();
}

void SettingsScreen::onHistory()
{
    if (!mModel.workoutsLoaded()) {
        mModel.requestWorkouts();   // a list held is current: saves and deletes refresh it
    }
    if (mAwaitClear && mModel.historyTotal() == 0) {
        mAwaitClear = false;
        mCleared    = true;
        mToast->show("History cleared");
    }
    refresh();
}

void SettingsScreen::onWorkouts()
{
    if (mAwaitReset) {
        mAwaitReset = false;   // a refused reset (a race with a start) answers with the full list
        if (mModel.userWorkoutCount() == 0) {
            mToast->show("Custom workouts deleted");
        }
    }
    refresh();
}

void SettingsScreen::onSuspend()
{
    mKeys.cancel();
}
