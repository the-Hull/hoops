/**
 ******************************************************************************
 * @file    ActiveScreen.hpp
 * @brief   The shooting screen: log makes and misses, undo, pause.
 *
 * docs/mockup-layout.md §6 (open session), §7 (workout) and §8 (undo
 * toast). Everything shown comes from the service's Snapshot; the screen
 * counts nothing itself (the workout ring's per-shot colours are the
 * engine's Snapshot::stepBits). Keys: SELECT make, BACK miss, UP undo,
 * DOWN pause; holding DOWN in an open session opens the spot picker to
 * change spot, on the button's release so that release cannot reach the
 * picker (in a workout a hold pauses, like a click). While DOWN is held
 * past the hold time a toast says "Release to change spot"; any key hides it.
 * A long press of SELECT / BACK / UP counts like a click (KeyInput's
 * HoldRelease).
 *
 * Snapshots that still show the state we came from are ignored until the
 * first live (Active / Rest) one; if none arrives within kLiveWaitMs of
 * onShow() the screen goes where the latest snapshot says (Pause, Rest or
 * Home), so a refused start can never leave it logging into nothing.
 *
 * rebuild() branches on Snapshot::mode; both layouts share the number
 * group, the percentage and the footer (showBody()). A finished workout
 * step goes to the Rest screen (STEP_DONE with rest, or the last step), or
 * shows a 1.5 s toast when the next step starts at once (no rest).
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
    void onStepDone(const CustomMessage::StepDone& d) override;
    void onUndoResult(bool ok, bool made) override;
    void onSuspend() override;

    static constexpr uint32_t kToastMs     = 2000;   ///< undo
    static constexpr uint32_t kStepToastMs = 1500;   ///< step done, no rest
    static constexpr uint32_t kHoldToastMs = 10000;  ///< "Release to change spot", until a key
    static constexpr uint32_t kLiveWaitMs  = 1500;   ///< first live snapshot backstop

protected:
    void build() override;

private:
    static constexpr uint8_t kLast5 = 5;

    static void toastTimerCb(lv_timer_t* t);
    static void liveTimerCb(lv_timer_t* t);
    static void onHold(void* ctx, KeyInput::Action a);

    void stopLiveTimer();

    void changeSpot();
    void pauseNow();

    /// Redraw everything from @p s; branches on open / workout mode.
    void rebuild(const score::Snapshot& s);
    void rebuildOpen(const score::Snapshot& s);
    void rebuildWorkout(const score::Snapshot& s);

    /// Move the shared widgets to the open (§6) or workout (§7) positions.
    void applyLayout(bool workout);
    /// Header, number group, percentage and footer: the part both modes share.
    void showBody(const score::Snapshot& s, uint16_t makes, uint16_t attempts);
    void showWorkoutRing(const score::Snapshot& s);
    void showProgressLine(const score::Snapshot& s);

    void showHeader(const char* text);
    void showCount(uint16_t makes, uint16_t attempts);
    void showPct(uint16_t makes, uint16_t attempts, bool full);
    void showLast5(uint8_t count, uint8_t bits);
    void showFooter(uint32_t elapsed_s, uint8_t hr);

    void showToast(const char* text, uint32_t ms);
    void hideToast();

    KeyInput               mKeys;
    std::unique_ptr<Ring>  mRing;
    std::unique_ptr<Hints> mHints;

    lv_obj_t* mBody    = nullptr;   ///< step line, header, count and pct; moves up under the toast
    lv_obj_t* mStepLine = nullptr;  ///< "STEP 2 / 4" (workout)
    lv_obj_t* mHeader  = nullptr;
    lv_obj_t* mMakes   = nullptr;
    lv_obj_t* mOfTotal = nullptr;   ///< "/11"
    lv_obj_t* mPct     = nullptr;
    lv_obj_t* mDots[kLast5] = {};
    lv_obj_t* mNewest  = nullptr;   ///< white ring round the newest dot
    lv_obj_t* mTime    = nullptr;
    lv_obj_t* mHeart   = nullptr;
    lv_obj_t* mHr      = nullptr;
    lv_obj_t* mProgress = nullptr;  ///< "3 shots left" (workout)
    lv_obj_t* mToast   = nullptr;
    lv_obj_t* mToastText = nullptr;
    lv_timer_t* mToastTimer = nullptr;
    lv_timer_t* mLiveTimer  = nullptr;   ///< kLiveWaitMs after onShow()
    bool        mHoldToast  = false;   ///< the toast shown is the hold hint
    bool        mWorkoutLayout = false;

    // Snapshots the service sent before it handled the intent that brought
    // us here (RESUME, START_OPEN) still show the state we left: Paused,
    // Idle or Ended. Until the first live snapshot, those are ignored.
    bool                mLiveSeen   = false;
    bool                mHasLeaving = false;
    score::SessionState mLeaving    = score::SessionState::Idle;
};

#endif // ACTIVE_SCREEN_HPP
