/**
 ******************************************************************************
 * @file    KeyInput.hpp
 * @brief   Turns the kernel's press / release / click codes into Click,
 *          Hold and HoldRelease actions on the four logical buttons.
 *
 * Buttons: L1 = Up, L2 = Down, R1 = Select, R2 = Back (SDK/GUI/Button.hpp).
 * The kernel (and the simulator's keys 1-4) emit PRESS on key down, RELEASE
 * on key up, and CLICK when the hold was shorter than 500 ms. The simulator
 * sends CLICK before RELEASE; the order on the watch is not documented, so
 * nothing here depends on it.
 *
 * Every press ends as exactly one of Click or HoldRelease:
 * - A CLICK while the button is down (or within kReleaseGraceMs after its
 *   RELEASE) is the press's action: {Click, btn}, returned from feed().
 * - A RELEASE after the hold timer fired (>= kHoldMs; the kernel sends no
 *   CLICK then) returns {HoldRelease, btn} from feed() at once.
 * - A short RELEASE with no CLICK yet waits kReleaseGraceMs: a CLICK in that
 *   window is returned as the Click (and nothing else follows); otherwise
 *   {HoldRelease, btn} is delivered through the Callback when the wait
 *   ends (a press the kernel did not count as a click: its timing and ours
 *   differ by the frames a code waits in the queue). The next PRESS, or a
 *   CLICK of another button, delivers a waiting HoldRelease first.
 * - A PRESS starts a kHoldMs timer; if the button is still down (and not
 *   clicked) when it fires, {Hold, btn} goes through the Callback. A Hold
 *   swallows any CLICK for that press, until the next PRESS.
 * - A RELEASE with no PRESS before it on this screen (the press happened on
 *   the previous screen) is ignored.
 *
 * So a slow press is never lost and a short one never counts twice: screens
 * act on isActivate() (Click or HoldRelease), from feed()'s result and from
 * the Callback alike (Screen::onAction()). A screen that navigates on a hold
 * does so on the HoldRelease, not the Hold, so the release cannot reach the
 * next screen.
 *
 * feed() returns {None, None} for every other code. Call it from
 * Screen::onKey(); call cancel() from onSuspend() so a hold in progress does
 * not fire (and a waiting HoldRelease is dropped) while the GUI is off screen.
 ******************************************************************************
 */

#ifndef KEY_INPUT_HPP
#define KEY_INPUT_HPP

#include <cstdint>

#include "lvgl.h"

class KeyInput
{
public:
    enum class Btn : uint8_t { Up, Down, Select, Back, None };
    enum class Kind : uint8_t { None, Click, Hold, HoldRelease };
    struct Action { Kind kind; Btn btn; };

    /// Receives Hold and the delayed HoldRelease actions (from LVGL timers,
    /// on the GUI thread, or from inside feed() when a PRESS / another
    /// button's CLICK ends a waiting press). With no callback set they are
    /// dropped with a one-time log warning.
    using Callback = void (*)(void* ctx, Action a);

    explicit KeyInput(Callback cb = nullptr, void* ctx = nullptr);

    ~KeyInput();

    KeyInput(const KeyInput&)            = delete;
    KeyInput& operator=(const KeyInput&) = delete;

    /// Feed one SDK::GUI::Button code; returns the Click or HoldRelease it
    /// makes, if any.
    Action feed(uint8_t code);

    /// The press is complete and should act: a Click or a HoldRelease.
    static bool isActivate(Action a) { return a.kind == Kind::Click || a.kind == Kind::HoldRelease; }

    /// Drop a pending hold (button state unknown, e.g. on suspend).
    void cancel();

    static constexpr uint32_t kHoldMs      = 500;
    /// How long a short RELEASE without CLICK waits for a late CLICK before
    /// it becomes a HoldRelease. The port delivers one key code per GUI
    /// frame (100 ms at SDK::GUI::Config::kFrameRate 10), before LVGL's
    /// timers run, so a CLICK queued right behind its RELEASE reaches us
    /// one frame later: the grace covers a full frame plus jitter.
    static constexpr uint32_t kReleaseGraceMs = 150;

private:
    void stopTimer();
    void stopReleaseTimer();
    /// Deliver the waiting HoldRelease now, if any.
    void flushRelease();
    void deliver(Action a);
    static void timerCb(lv_timer_t* t);
    static void releaseTimerCb(lv_timer_t* t);

    lv_timer_t* mTimer        = nullptr;   ///< kHoldMs after PRESS
    lv_timer_t* mReleaseTimer = nullptr;   ///< kReleaseGraceMs after a short RELEASE without CLICK
    Btn         mReleaseBtn   = Btn::None; ///< the button whose CLICK may still come
    Btn         mPressed   = Btn::None;   ///< button held down, if any
    Btn         mSuppress  = Btn::None;   ///< its next Click is swallowed (a Hold fired)
    bool        mClicked   = false;       ///< a Click was delivered for the press in mPressed
    bool        mHoldFired = false;       ///< the Hold fired for the press in mPressed
    Callback    mCb;
    void*       mCtx;
    Action      mPending   = { Kind::None, Btn::None };   ///< the action being delivered
};

#endif // KEY_INPUT_HPP
