/**
 ******************************************************************************
 * @file    ActiveScreen.hpp
 * @brief   The shooting screen: log makes and misses, undo, pause.
 *
 * docs/mockup-layout.md §6 (open session) and §8 (undo toast). Everything
 * shown comes from the service's Snapshot; the screen counts nothing itself.
 * Keys: SELECT make, BACK miss, UP undo, DOWN pause; holding DOWN in an
 * open session opens the spot picker to change spot, on the button's
 * release so that release cannot reach the picker.
 *
 * rebuild() branches on Snapshot::mode; the workout layout (§7) is added by
 * a later task in rebuildWorkout().
 ******************************************************************************
 */

#ifndef ACTIVE_SCREEN_HPP
#define ACTIVE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/Ring.hpp"

class ActiveScreen : public Screen
{
public:
    explicit ActiveScreen(Model& model);
    ~ActiveScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

    void onSnapshot(const score::Snapshot& s) override;
    void onUndoResult(bool ok, bool made) override;
    void onSuspend() override;

    static constexpr uint32_t kToastMs = 2000;

protected:
    void build() override;

private:
    static constexpr uint8_t kLast5 = 5;

    static void toastTimerCb(lv_timer_t* t);

    void changeSpot();

    /// Redraw everything from @p s; branches on open / workout mode.
    void rebuild(const score::Snapshot& s);
    void rebuildOpen(const score::Snapshot& s);
    void rebuildWorkout(const score::Snapshot& s);

    void showHeader(const char* text);
    void showCount(uint16_t makes, uint16_t attempts);
    void showPct(uint16_t makes, uint16_t attempts, bool full);
    void showLast5(uint8_t count, uint8_t bits);
    void showFooter(uint32_t elapsed_s, uint8_t hr);

    void showToast(const char* text);
    void hideToast();

    KeyInput               mKeys;
    std::unique_ptr<Ring>  mRing;
    std::unique_ptr<Hints> mHints;

    lv_obj_t* mBody    = nullptr;   ///< header, count and pct; moves up under the toast
    lv_obj_t* mHeader  = nullptr;
    lv_obj_t* mMakes   = nullptr;
    lv_obj_t* mOfTotal = nullptr;   ///< "/11"
    lv_obj_t* mPct     = nullptr;
    lv_obj_t* mDots[kLast5] = {};
    lv_obj_t* mNewest  = nullptr;   ///< white ring round the newest dot
    lv_obj_t* mTime    = nullptr;
    lv_obj_t* mHeart   = nullptr;
    lv_obj_t* mHr      = nullptr;
    lv_obj_t* mToast   = nullptr;
    lv_obj_t* mToastText = nullptr;
    lv_timer_t* mToastTimer = nullptr;

    // Snapshots the service sent before it handled the intent that brought
    // us here (RESUME, START_OPEN) still show the state we left: Paused,
    // Idle or Ended. Until the first live snapshot, those are ignored.
    bool                mLiveSeen   = false;
    bool                mHasLeaving = false;
    score::SessionState mLeaving    = score::SessionState::Idle;
};

#endif // ACTIVE_SCREEN_HPP
