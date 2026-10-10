/**
 ******************************************************************************
 * @file    KeyInput.cpp
 * @brief   Press / release / click codes -> Click, Hold and HoldRelease actions.
 ******************************************************************************
 */

#include "gui/widgets/KeyInput.hpp"

#include "SDK/GUI/Button.hpp"

#define LOG_MODULE_PRX      "KeyInput"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{

enum class Code : uint8_t { Other, Click, Press, Release };

struct Decoded {
    Code          code;
    KeyInput::Btn btn;
};

Decoded decode(uint8_t c)
{
    namespace B = SDK::GUI::Button;
    using Btn   = KeyInput::Btn;
    switch (c) {
        case B::L1:         return { Code::Click,   Btn::Up };
        case B::L2:         return { Code::Click,   Btn::Down };
        case B::R1:         return { Code::Click,   Btn::Select };
        case B::R2:         return { Code::Click,   Btn::Back };
        case B::L1_PRESS:   return { Code::Press,   Btn::Up };
        case B::L2_PRESS:   return { Code::Press,   Btn::Down };
        case B::R1_PRESS:   return { Code::Press,   Btn::Select };
        case B::R2_PRESS:   return { Code::Press,   Btn::Back };
        case B::L1_RELEASE: return { Code::Release, Btn::Up };
        case B::L2_RELEASE: return { Code::Release, Btn::Down };
        case B::R1_RELEASE: return { Code::Release, Btn::Select };
        case B::R2_RELEASE: return { Code::Release, Btn::Back };
        default:            return { Code::Other,   Btn::None };
    }
}

constexpr KeyInput::Action kNone = { KeyInput::Kind::None, KeyInput::Btn::None };

} // namespace

KeyInput::LastHoldRelease KeyInput::sLastHoldRelease = { Btn::None, 0 };

void KeyInput::noteHoldRelease(Btn btn)
{
    sLastHoldRelease = { btn, lv_tick_get() };
}

bool KeyInput::lateClickAfterHoldRelease(Btn btn)
{
    if (sLastHoldRelease.btn != btn || lv_tick_elaps(sLastHoldRelease.tick) > kLateClickMs) {
        return false;
    }
    sLastHoldRelease.btn = Btn::None;   // one CLICK per press
    return true;
}

KeyInput::KeyInput(Callback cb, void* ctx)
    : mCb(cb)
    , mCtx(ctx)
{
}

KeyInput::~KeyInput()
{
    stopTimer();
    stopReleaseTimer();
}

KeyInput::Action KeyInput::feed(uint8_t code)
{
    const Decoded d = decode(code);
    switch (d.code) {
        case Code::Press:
            flushRelease();   // the previous press ends before this one starts
            sLastHoldRelease.btn = Btn::None;   // a new press: no late CLICK can be pending
            stopTimer();
            mPressed   = d.btn;
            mSuppress  = Btn::None;
            mClicked   = false;
            mHoldFired = false;
            mTimer     = lv_timer_create(&KeyInput::timerCb, kHoldMs, this);
            return kNone;

        case Code::Release: {
            if (d.btn != mPressed) {
                return kNone;   // pressed on another screen, or a stray release
            }
            stopTimer();
            mPressed = Btn::None;
            const bool clicked = mClicked;
            mClicked = false;
            if (clicked) {
                return kNone;   // the CLICK came first and was the press's action
            }
            if (mHoldFired) {
                // >= kHoldMs: the kernel sends no CLICK for it (a stray one
                // stays swallowed by mSuppress until the next PRESS).
                mHoldFired = false;
                noteHoldRelease(d.btn);
                return { Kind::HoldRelease, d.btn };
            }
            // Short, and no CLICK yet: it may still come after this RELEASE.
            // Wait kReleaseGraceMs for it; without it the press ends as a
            // HoldRelease, delivered through the callback.
            stopReleaseTimer();
            mReleaseBtn   = d.btn;
            mReleaseTimer = lv_timer_create(&KeyInput::releaseTimerCb, kReleaseGraceMs, this);
            return kNone;
        }

        case Code::Click:
            if (d.btn == mReleaseBtn && mReleaseTimer) {
                stopReleaseTimer();   // the CLICK after the RELEASE: one Click, no HoldRelease
                return { Kind::Click, d.btn };
            }
            flushRelease();           // another button: the pending press ends first
            if (lateClickAfterHoldRelease(d.btn)) {
                return kNone;         // that press already ended as a HoldRelease (maybe on another screen)
            }
            if (d.btn == mSuppress) {
                return kNone;         // the Hold fired: the RELEASE reports this press
            }
            if (d.btn == mPressed) {
                mClicked = true;
                stopTimer();          // the press is over; no Hold may follow its Click
            }
            return { Kind::Click, d.btn };

        case Code::Other:
            break;
    }
    return kNone;
}

void KeyInput::cancel()
{
    stopTimer();
    stopReleaseTimer();   // dropped, not delivered: the GUI is going off screen
    mPressed   = Btn::None;
    mSuppress  = Btn::None;
    mClicked   = false;
    mHoldFired = false;
}

void KeyInput::stopTimer()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
}

void KeyInput::stopReleaseTimer()
{
    if (mReleaseTimer) {
        lv_timer_delete(mReleaseTimer);
        mReleaseTimer = nullptr;
    }
    mReleaseBtn = Btn::None;
}

void KeyInput::flushRelease()
{
    if (!mReleaseTimer) {
        return;
    }
    const Btn btn = mReleaseBtn;
    stopReleaseTimer();
    mSuppress = btn;          // a CLICK still to come on this screen is swallowed...
    noteHoldRelease(btn);     // ...and on the next one, within kLateClickMs
    deliver({ Kind::HoldRelease, btn });
}

void KeyInput::deliver(Action a)
{
    mPending = a;
    if (mCb) {
        mCb(mCtx, mPending);
        return;
    }
    static bool sWarned = false;
    if (!sWarned) {
        sWarned = true;
        LOG_WARNING("Action %u on button %u dropped: no callback set\n", static_cast<unsigned>(a.kind),
                    static_cast<unsigned>(a.btn));
    }
}

void KeyInput::releaseTimerCb(lv_timer_t* t)
{
    // No CLICK within kReleaseGraceMs of the RELEASE: the press ends as a
    // HoldRelease. (The callback may switch screens: touch nothing after.)
    static_cast<KeyInput*>(lv_timer_get_user_data(t))->flushRelease();
}

void KeyInput::timerCb(lv_timer_t* t)
{
    auto* self = static_cast<KeyInput*>(lv_timer_get_user_data(t));
    self->stopTimer();   // one shot; deleting a timer from its own callback is allowed
    if (self->mPressed == Btn::None || self->mClicked) {
        return;
    }
    self->mHoldFired = true;
    self->mSuppress  = self->mPressed;
    self->deliver({ Kind::Hold, self->mPressed });
}
