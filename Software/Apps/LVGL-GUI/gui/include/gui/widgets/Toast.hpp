/**
 ******************************************************************************
 * @file    Toast.hpp
 * @brief   A short message in a white pill near the bottom of the screen.
 *
 * The shooting screen's toast (docs/mockup-layout.md §8) for the menu
 * screens: black text on a white pill centred on y 171, in the first face
 * that fits the round screen there. show() replaces any message on display
 * and restarts its timer; it hides itself after the given time. Create it
 * after the screen's other widgets so it draws on top.
 ******************************************************************************
 */

#ifndef TOAST_HPP
#define TOAST_HPP

#include <cstdint>

#include "lvgl.h"

class Toast
{
public:
    explicit Toast(lv_obj_t* parent);
    ~Toast();

    Toast(const Toast&)            = delete;
    Toast& operator=(const Toast&) = delete;

    void show(const char* text, uint32_t ms = kDefaultMs);
    void hide();
    bool visible() const { return mTimer != nullptr; }

    static constexpr uint32_t kDefaultMs = 2000;

private:
    static void timerCb(lv_timer_t* t);

    lv_obj_t*   mBox   = nullptr;
    lv_obj_t*   mText  = nullptr;
    lv_timer_t* mTimer = nullptr;
};

#endif // TOAST_HPP
