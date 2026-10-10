/**
 ******************************************************************************
 * @file    Screen.hpp
 * @brief   Base class of every Hoops screen.
 *
 * A Screen owns one LVGL screen object (lv_obj_create(nullptr)), receives the
 * kernel's button codes through LV_EVENT_KEY on that object, and is the
 * Model's listener while it is on display. This plays the role TouchGFX's
 * View + Presenter pair play in a TouchGFX app, collapsed into one class:
 * onShow() is the presenter's activate(), onHide() its deactivate().
 ******************************************************************************
 */

#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"
#include "gui/widgets/KeyInput.hpp"

class Screen : public ModelListener
{
public:
    explicit Screen(Model& model);
    virtual ~Screen();

    Screen(const Screen&)            = delete;
    Screen& operator=(const Screen&) = delete;

    /// Create the LVGL screen object and its widgets (calls build()).
    void create();

    /// Delete the LVGL screen object and everything on it.
    void destroy();

    /// The LVGL screen object, or nullptr before create() / after destroy().
    lv_obj_t* root() const { return mRoot; }

    /// Called after this screen has become the active one and the Model is bound.
    virtual void onShow() {}

    /// Called just before the screen is replaced; store navigation state here.
    virtual void onHide() {}

    /// Called with an SDK::GUI::Button code (click, press or release).
    virtual void onKey(uint8_t code) { (void)code; }

    /// 60 s without a button: exit the app unless a session is in progress.
    /// Screens of a session in progress (and the step builder, which holds an
    /// unsaved draft) override this with a no-op.
    void onIdleTimeout() override;

protected:
    /// Populate mRoot with widgets. mRoot exists and is styled as the background.
    virtual void build() = 0;

    /// A KeyInput action: what onKey()'s feed() returned, or one delivered
    /// later through keyAction() (Hold, or a HoldRelease that waited for a
    /// late CLICK). Screens act on KeyInput::isActivate() actions here.
    virtual void onAction(KeyInput::Action a) { (void)a; }

    /// KeyInput::Callback for a screen's KeyInput (ctx = the Screen):
    /// forwards to onAction().
    static void keyAction(void* ctx, KeyInput::Action a);

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;

private:
    static void keyEventCb(lv_event_t* e);
};

#endif // SCREEN_HPP
