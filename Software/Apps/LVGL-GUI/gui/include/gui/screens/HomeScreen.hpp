/**
 ******************************************************************************
 * @file    HomeScreen.hpp
 * @brief   Home: "HOOPS" header, the main menu and the last session's line.
 *
 * Menu (docs/mockup-layout.md §1): "Resume session" first while the service
 * reports a session in progress, then Open Session, Workouts, History and
 * Settings (dim, no action yet); with it an amber "Session in progress" line
 * under the header (which moves up for it). UP / DOWN move, SELECT opens, BACK exits
 * the app, or goes to Pause while a session is in progress. The footer
 * "Last · d.m. · pp %" comes from the service's history (newest first).
 * Model::homeToast, when set (e.g. "Save failed"), shows as a toast on arrival.
 ******************************************************************************
 */

#ifndef HOME_SCREEN_HPP
#define HOME_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"
#include "gui/widgets/Toast.hpp"

class HomeScreen : public Screen
{
public:
    explicit HomeScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    void onSnapshot(const score::Snapshot& s) override;
    void onHistory() override;
    void onIdleTimeout() override;
    void onSuspend() override;

protected:
    void build() override;

private:
    enum class Action : uint8_t { Resume, Open, Workouts, History, Settings };

    static constexpr uint8_t kMaxItems = 5;


    /// True while the service reports a session Active, Paused or in Rest.
    bool sessionInProgress() const;

    /// Rebuild the menu items (with or without "Resume session"), keeping
    /// the selected action where it still exists.
    void updateItems();
    void updateFooter();
    void activate(Action a);

    KeyInput                  mKeys;
    std::unique_ptr<Hints>    mHints;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Toast>    mToast;
    lv_obj_t*                 mFooter = nullptr;
    lv_obj_t*                 mHeader = nullptr;
    lv_obj_t*                 mHint   = nullptr;   ///< "Session in progress"

    ListMenu::Item mItems[kMaxItems]   = {};
    Action         mActions[kMaxItems] = {};
    uint8_t        mCount     = 0;
    bool           mHasResume = false;
};

#endif // HOME_SCREEN_HPP
