/**
 ******************************************************************************
 * @file    PauseScreen.hpp
 * @brief   The paused session: time, a summary line and Resume / End & save /
 *          Discard.
 *
 * docs/mockup-layout.md §10. UP / DOWN move (not circular), SELECT confirms,
 * BACK resumes. Discard asks twice: the first SELECT turns the item into
 * "Discard? / SELECT again" for 3 s, a second SELECT within that time ends the
 * session without saving. End & save waits for the service's SAVED and then
 * opens the session's results. A running snapshot after a Paused one (the
 * session resumed elsewhere) returns to the shooting screen.
 ******************************************************************************
 */

#ifndef PAUSE_SCREEN_HPP
#define PAUSE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"
#include "gui/widgets/Ring.hpp"

class PauseScreen : public Screen
{
public:
    explicit PauseScreen(Model& model);
    ~PauseScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

    void onSnapshot(const score::Snapshot& s) override;
    void onSaved(uint32_t id) override;
    void onSuspend() override;

    static constexpr uint32_t kConfirmMs = 3000;

protected:
    void build() override;

private:
    enum Item : uint8_t { kResume, kEndSave, kDiscard, kItemCount };

    static void confirmTimerCb(lv_timer_t* t);

    void show(const score::Snapshot& s);
    void select();
    void resume();
    void setConfirm(bool on);

    KeyInput                  mKeys;
    std::unique_ptr<Ring>     mRing;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Hints>    mHints;

    lv_obj_t* mTime    = nullptr;
    lv_obj_t* mSummary = nullptr;

    ListMenu::Item mItems[kItemCount] = {};
    lv_timer_t*    mConfirmTimer = nullptr;   ///< Discard confirmation window
    bool           mConfirming   = false;
    bool           mEnding       = false;     ///< END_SESSION sent
    bool           mSaved        = false;     ///< SAVED arrived; Results on its way
    bool           mSeenPaused   = false;     ///< a Paused snapshot arrived here
};

#endif // PAUSE_SCREEN_HPP
