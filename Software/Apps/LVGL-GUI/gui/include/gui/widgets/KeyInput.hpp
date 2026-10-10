/**
 ******************************************************************************
 * @file    KeyInput.hpp
 * @brief   Turns the kernel's press / release / click codes into Click and
 *          Hold actions on the four logical buttons.
 *
 * Buttons: L1 = Up, L2 = Down, R1 = Select, R2 = Back (SDK/GUI/Button.hpp).
 * The kernel (and the simulator's keys 1-4) emit PRESS on key down, RELEASE
 * on key up, and CLICK only when the hold was shorter than 500 ms.
 *
 * - CLICK codes come back from feed() synchronously as {Click, btn}.
 * - A PRESS starts a kHoldMs timer; if the button is still down when it
 *   fires, {Hold, btn} is delivered through the Callback given to the
 *   constructor (feed() has long returned by then). A Hold suppresses the
 *   Click that might follow for the same button.
 * - A RELEASE stops the timer. kHoldMs equals the kernel's click limit, so
 *   every press ends as either a Click or a Hold.
 *
 * feed() returns {None, None} for every code that is not a Click. Call it
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
    enum class Kind : uint8_t { None, Click, Hold };
    struct Action { Kind kind; Btn btn; };

    /// Receives Hold actions (from the LVGL timer, on the GUI thread). A
    /// Hold with no callback set is dropped with a one-time log warning.
    using Callback = void (*)(void* ctx, Action a);

    explicit KeyInput(Callback cb = nullptr, void* ctx = nullptr);
    ~KeyInput();

    KeyInput(const KeyInput&)            = delete;
    KeyInput& operator=(const KeyInput&) = delete;

    /// Feed one SDK::GUI::Button code; returns the Click it makes, if any.
    Action feed(uint8_t code);

    /// Drop a pending hold (button state unknown, e.g. on suspend).
    void cancel();

    static constexpr uint32_t kHoldMs = 500;

private:
    void stopTimer();
    static void timerCb(lv_timer_t* t);

    lv_timer_t* mTimer     = nullptr;
    Btn         mPressed   = Btn::None;   ///< button held down, if any
    Btn         mSuppress  = Btn::None;   ///< its next Click is swallowed (a Hold fired)
    bool        mHoldFired = false;
    Callback    mCb;
    void*       mCtx;
    Action      mPending   = { Kind::None, Btn::None };   ///< the Hold being delivered
};

#endif // KEY_INPUT_HPP
