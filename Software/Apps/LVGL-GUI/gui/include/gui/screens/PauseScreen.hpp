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
 * opens the session's results; SAVED with id 0 (nothing stored: an empty
 * session, or the save failed) goes Home with a toast there ("Nothing to
 * save" / "Save failed"). Without a reply in kEndMs the keys come back with
 * a "No reply" toast. Resume sends RESUME and waits for the
 * running snapshot that follows a Paused one: Rest (or a finished
 * workout's done summary) returns to the Rest screen, Active to the
 * shooting screen. A resume elsewhere routes the same way.
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
#include "gui/widgets/Toast.hpp"

class PauseScreen : public Screen
{
public:
    explicit PauseScreen(Model& model);
    ~PauseScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onAction(KeyInput::Action a) override;

    void onSnapshot(const score::Snapshot& s) override;
    void onSaved(uint32_t id) override;
    void onSuspend() override;
    void onIdleTimeout() override {}   // the way out of a session: never exit on idle

    static constexpr uint32_t kConfirmMs = 3000;
    static constexpr uint32_t kEndMs     = 3000;   ///< End & save reply watchdog

protected:
    void build() override;

private:
    enum Item : uint8_t { kResume, kEndSave, kDiscard, kItemCount };

    static void confirmTimerCb(lv_timer_t* t);
    static void endTimerCb(lv_timer_t* t);

    void endAndSave();
    void stopEndTimer();

    void show(const score::Snapshot& s);
    void select();
    void resume();
    void setConfirm(bool on);

    KeyInput                  mKeys;
    std::unique_ptr<Ring>     mRing;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Hints>    mHints;
    std::unique_ptr<Toast>    mToast;

    lv_obj_t* mTime    = nullptr;
    lv_obj_t* mSummary = nullptr;

    ListMenu::Item mItems[kItemCount] = {};
    lv_timer_t*    mConfirmTimer = nullptr;   ///< Discard confirmation window
    lv_timer_t*    mEndTimer     = nullptr;   ///< End & save watchdog (kEndMs)
    bool           mEndEmpty     = false;     ///< the session had no shots at End & save
    bool           mConfirming   = false;
    bool           mEnding       = false;     ///< END_SESSION sent
    bool           mSaved        = false;     ///< SAVED arrived; Results on its way
    bool           mSeenPaused   = false;     ///< a Paused snapshot arrived here
};

#endif // PAUSE_SCREEN_HPP
