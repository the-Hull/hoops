/**
 ******************************************************************************
 * @file    KeyInput.hpp
 * @brief   Turns the kernel's press / release / click codes into Click,
 *          Hold and HoldRelease actions on the four logical buttons.
 *
 * Buttons: L1 = Up, L2 = Down, R1 = Select, R2 = Back (SDK/GUI/Button.hpp).
 * The kernel (and the simulator's keys 1-4) emit PRESS on key down, CLICK
 * when the hold was shorter than 500 ms, then RELEASE on key up (in that
 * order: SDK LvglHost, "press/click/release" in the port docs).
 *
 * Every press ends as exactly one of Click or HoldRelease:
 * - CLICK codes come back from feed() synchronously as {Click, btn}.
 * - The RELEASE of a press for which no Click was delivered comes back as
 *   {HoldRelease, btn}, whether or not the hold timer fired: a long press,
 *   or a short one the kernel did not report as a click (its timing and
 *   ours differ by the frames a code waits in the queue). A slow press must
 *   not be lost, so screens act on isActivate() (Click or HoldRelease), not
 *   on Click alone.
 * - A PRESS starts a kHoldMs timer; if the button is still down (and not
 *   clicked) when it fires, {Hold, btn} is delivered through the Callback
 *   given to the constructor (feed() has long returned by then). A Hold
 *   swallows a Click that still comes for the same press; the RELEASE then
 *   gives the HoldRelease. A screen that navigates on a hold does so on
 *   the HoldRelease, not the Hold: the release then cannot reach the next
 *   screen.
 * - A RELEASE (or CLICK) with no PRESS before it on this screen (the press
 *   happened on the previous screen) is ignored, as is a lone RELEASE.

 * feed() returns {None, None} for every other code. Call it
 * from Screen::onKey(); call cancel() from onSuspend() so a hold in progress
 * does not fire while the GUI is off screen.
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

    /// Receives Hold actions (from the LVGL timer, on the GUI thread). A
    /// Hold with no callback set is dropped with a one-time log warning.
    using Callback = void (*)(void* ctx, Action a);

    explicit KeyInput(Callback cb = nullptr, void* ctx = nullptr);

    /// Callback for screens that use no Hold: ignores it (and keeps the
    /// "Hold dropped" warning for screens that forgot a callback).
    static void noHold(void* ctx, Action a);
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

    static constexpr uint32_t kHoldMs = 500;

private:
    void stopTimer();
    static void timerCb(lv_timer_t* t);

    lv_timer_t* mTimer     = nullptr;
    Btn         mPressed   = Btn::None;   ///< button held down, if any
    Btn         mSuppress  = Btn::None;   ///< its next Click is swallowed (a Hold fired)
    bool        mClicked   = false;       ///< a Click was delivered for the press in mPressed
    Callback    mCb;
    void*       mCtx;
    Action      mPending   = { Kind::None, Btn::None };   ///< the Hold being delivered
};

#endif // KEY_INPUT_HPP
