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

KeyInput::KeyInput(Callback cb, void* ctx)
    : mCb(cb)
    , mCtx(ctx)
{
}

void KeyInput::noHold(void* ctx, Action a)
{
    (void)ctx;
    (void)a;
}

KeyInput::~KeyInput()
{
    stopTimer();
}

KeyInput::Action KeyInput::feed(uint8_t code)
{
    const Decoded d = decode(code);
    switch (d.code) {
        case Code::Press:
            stopTimer();
            mPressed  = d.btn;
            mSuppress = Btn::None;
            mClicked  = false;
            mTimer    = lv_timer_create(&KeyInput::timerCb, kHoldMs, this);
            return kNone;

        case Code::Release:
            if (d.btn != mPressed) {
                return kNone;   // pressed on another screen, or a stray release
            }
            stopTimer();
            mPressed  = Btn::None;
            mSuppress = Btn::None;   // the CLICK, if any, came before this RELEASE
            if (mClicked) {
                mClicked = false;
                return kNone;
            }
            // No Click for this press: a hold, or a press the kernel did not
            // count as a click. Either way it ends here, once.
            return { Kind::HoldRelease, d.btn };

        case Code::Click:
            if (d.btn == mSuppress) {
                mSuppress = Btn::None;   // the Hold fired: the RELEASE reports this press
                return kNone;
            }
            if (d.btn == mPressed) {
                mClicked = true;
                stopTimer();   // the press is over; no Hold may follow its Click
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
    mPressed  = Btn::None;
    mSuppress = Btn::None;
    mClicked  = false;
}

void KeyInput::stopTimer()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
}

void KeyInput::timerCb(lv_timer_t* t)
{
    auto* self = static_cast<KeyInput*>(lv_timer_get_user_data(t));
    self->stopTimer();   // one shot; deleting a timer from its own callback is allowed
    if (self->mPressed == Btn::None || self->mClicked) {
        return;
    }
    self->mSuppress  = self->mPressed;
    self->mPending   = { Kind::Hold, self->mPressed };
    if (self->mCb) {
        self->mCb(self->mCtx, self->mPending);
    } else {
        static bool sWarned = false;
        if (!sWarned) {
            sWarned = true;
            LOG_WARNING("Hold on button %u dropped: no callback set\n", static_cast<unsigned>(self->mPending.btn));
        }
    }
}
