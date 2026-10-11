/**
 ******************************************************************************
 * @file    SettingsScreen.hpp
 * @brief   The user settings: thirteen rows, SELECT steps a row to its next value.
 *
 * Spec v2 §2.4. Header "SETTINGS", ListMenu::workoutsGeometry() (circular):
 * line 1 the setting's name, line 2 (the selected row only) its value.
 * UP / DOWN move, SELECT = next value (cycles, wraps), BACK = Home. Every
 * change goes to the service at once (Model::setSettings() from the local
 * copy mEdit); the SETTINGS echo (onSettings()) overwrites the copy. The
 * colour scheme row rebuilds the screen through the Model; the selection is
 * kept in Model::settingsIndex. Until the service's SETTINGS is in
 * (Model::hasSettings(); asked for on show with GET_SETTINGS) the setting
 * rows read "Loading…" and SELECT does nothing, so the defaults never
 * overwrite the stored settings.
 *
 * Clear history: line 2 "N sessions" (Model::historyTotal(), requested on
 * show) or "Nothing to clear"; the first SELECT asks "SELECT again to
 * confirm" for 3 s, a second one within them sends CLEAR_HISTORY, and the
 * empty HISTORY page that follows shows the toast "History cleared".
 * Restore workouts: line 2 "Deletes N custom" (Model::userWorkoutCount();
 * the list, if not held, is requested once the history page is in), "No custom workouts", "Not while running"
 * (a user workout is in progress) or "Loading…"; the same two-SELECT confirm
 * sends RESET_WORKOUTS, and the empty list that follows (onWorkouts()) shows
 * the toast "Custom workouts deleted". The presets are code and stay.
 * About: "Hoops v<BUILD_VERSION>" ("Hoops dev" without the define).
 ******************************************************************************
 */

#ifndef SETTINGS_SCREEN_HPP
#define SETTINGS_SCREEN_HPP

#include <memory>

#include "score/Settings.hpp"

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"
#include "gui/widgets/Toast.hpp"

class SettingsScreen : public Screen
{
public:
    explicit SettingsScreen(Model& model);
    ~SettingsScreen() override;

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onAction(KeyInput::Action a) override;

    void onSettings() override;
    void onHistory() override;
    void onWorkouts() override;
    void onSuspend() override;

protected:
    void build() override;

private:
    enum Row : uint8_t {
        kGoalType, kGoalValue, kRest, kOpenTarget, kHoldDown, kHaptics, kBacklight, kHeartRate, kScheme, kCsv,
        kClear, kReset, kAbout, kRowCount
    };

    static constexpr uint32_t kConfirmMs = 3000;

    /// Step the selected row to its next value and send the settings.
    void change(uint8_t row);
    void clearHistory();
    void resetWorkouts();
    /// A user workout is in progress: the service would refuse RESET_WORKOUTS.
    bool customRunning() const;
    /// Rewrite every row's value text and redraw the menu.
    void refresh();
    void stopConfirm();
    static void confirmTimerCb(lv_timer_t* t);

    KeyInput                  mKeys;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Hints>    mHints;
    std::unique_ptr<Toast>    mToast;

    score::Settings mEdit;
    ListMenu::Item  mItems[kRowCount]     = {};
    char            mValue[kRowCount][28] = {};
    lv_timer_t*     mConfirmTimer         = nullptr;   ///< the "SELECT again" window
    uint8_t         mConfirmRow           = kClear;    ///< the row it belongs to (kClear / kReset)
    bool            mAwaitClear           = false;     ///< CLEAR_HISTORY sent, empty page not yet in
    bool            mCleared              = false;     ///< cleared here: "0 sessions", not "Nothing to clear"
    bool            mAwaitReset           = false;     ///< RESET_WORKOUTS sent, its list not yet in
};

#endif // SETTINGS_SCREEN_HPP
