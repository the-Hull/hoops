/**
 ******************************************************************************
 * @file    SettingsScreen.cpp
 * @brief   The user settings: twelve rows, SELECT steps a row to its next value.
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
    "Backlight on shot", "Heart rate", "Colour scheme", "CSV export", "Clear history", "About",
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

static_assert(sizeof(kNames) / sizeof(kNames[0]) == 12, "one name per row");

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
    mModel.requestHistory();   // for the Clear history count
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

    const uint8_t total = mModel.historyTotal();
    if (mConfirmTimer) {
        text(kClear, "SELECT again to confirm");
    } else if (!mModel.hasHistory()) {
        text(kClear, "Loading\xE2\x80\xA6");
    } else if (total > 0 || mCleared) {
        put(kClear, total == 1 ? "%u session" : "%u sessions", total);
    } else {
        text(kClear, "Nothing to clear");
    }
    text(kAbout, kAboutText);

    mMenu->refresh();
}

void SettingsScreen::change(uint8_t row)
{
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
    if (!mConfirmTimer) {
        mConfirmTimer = lv_timer_create(&SettingsScreen::confirmTimerCb, kConfirmMs, this);
        refresh();
        return;
    }
    stopConfirm();
    mAwaitClear = true;
    mModel.clearHistory();   // the empty HISTORY page -> onHistory()
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
    if (mAwaitClear && mModel.historyTotal() == 0) {
        mAwaitClear = false;
        mCleared    = true;
        mToast->show("History cleared");
    }
    refresh();
}

void SettingsScreen::onSuspend()
{
    mKeys.cancel();
}
