/**
 ******************************************************************************
 * @file    ScreenManager.cpp
 * @brief   Owns the active screen and switches between them.
 ******************************************************************************
 */

#include "gui/screens/ScreenManager.hpp"

#include "lvgl.h"

#define LOG_MODULE_PRX      "ScreenManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "gui/model/Model.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/screens/ActiveScreen.hpp"
#include "gui/screens/HistoryScreen.hpp"
#include "gui/screens/HomeScreen.hpp"
#include "gui/screens/PauseScreen.hpp"
#include "gui/screens/RestScreen.hpp"
#include "gui/screens/ResultsScreen.hpp"
#include "gui/screens/SettingsScreen.hpp"
#include "gui/screens/SpotPickerScreen.hpp"
#include "gui/screens/StepBuilderScreen.hpp"
#include "gui/screens/WorkoutPreviewScreen.hpp"
#include "gui/screens/WorkoutsScreen.hpp"

ScreenManager& ScreenManager::instance()
{
    static ScreenManager sInstance;
    return sInstance;
}

void ScreenManager::start(Model& model, ScreenId first)
{
    mModel = &model;
    switchNow(first);
}

void ScreenManager::goTo(ScreenId id)
{
    mPending = id;
    if (!mPendingSet) {
        if (lv_async_call(&ScreenManager::asyncCb, this) != LV_RESULT_OK) {
            // LVGL's pool had no room for the request. Leave the flag clear
            // so the next goTo() tries again rather than waiting for a call
            // that will never come.
            LOG_ERROR("Switch to screen %u could not be scheduled\n", static_cast<unsigned>(id));
            return;
        }
        mPendingSet = true;
    }
}

void ScreenManager::rebuild()
{
    if (mModel && !mPendingSet) {
        goTo(mCurrentId);
    }
}

void ScreenManager::asyncCb(void* user)
{
    auto* self = static_cast<ScreenManager*>(user);
    self->mPendingSet = false;
    self->switchNow(self->mPending);
}

void ScreenManager::switchNow(ScreenId id)
{
    Screen* next = create(id);
    if (!next) {
        LOG_WARNING("No screen for id %u\n", static_cast<unsigned>(id));
        return;
    }

    // resumeEntry is meant for the Active / Rest screen Pause routes to; a
    // switch anywhere else (e.g. a snapshot sends Pause home) must not leave
    // it set for a later, unrelated entry.
    if (id != ScreenId::Active && id != ScreenId::Rest) {
        mModel->resumeEntry = false;
    }

    Screen* old = mCurrent;
    if (old) {
        old->onHide();
        mModel->bind(nullptr);
    }

    // Build and show the new screen, then free the old one. Both widget trees
    // exist for the duration of the switch, so the pool must hold the largest
    // such pair; the peak logged below is what to size it by.
    next->create();
    lv_screen_load(next->root());
    mCurrent   = next;
    mCurrentId = id;

    if (old) {
        // ~Screen() runs the derived destructors (which stop timers) before
        // it deletes the LVGL tree, so a widget destructor may still touch
        // its objects.
        delete old;
    }

    mModel->bind(next);
    next->onShow();

#if defined(SIMULATOR) || (LOG_LEVEL >= LOG_LEVEL_DEBUG && LOG_MODULE_LEVEL >= LOG_LEVEL_DEBUG)
    // Peak use of LVGL's static pool (LV_MEM_SIZE), logged per screen so the
    // pool can be sized to what the app actually needs. A development aid:
    // on the watch only when this module logs at DEBUG.
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    LOG_INFO("LVGL pool: %u/%u B used, peak %u%%, frag %u%% (screen %u)\n",
             static_cast<unsigned>(mon.total_size - mon.free_size),
             static_cast<unsigned>(mon.total_size),
             static_cast<unsigned>(mon.max_used) * 100u / static_cast<unsigned>(mon.total_size),
             static_cast<unsigned>(mon.frag_pct),
             static_cast<unsigned>(id));
#endif
}

Screen* ScreenManager::create(ScreenId id)
{
    Model& m = *mModel;

    switch (id) {
        case ScreenId::Home:
            return new HomeScreen(m);
        case ScreenId::SpotPicker:
            return new SpotPickerScreen(m);
        case ScreenId::Active:
            return new ActiveScreen(m);
        case ScreenId::Pause:
            return new PauseScreen(m);
        case ScreenId::Results:
            return new ResultsScreen(m);
        case ScreenId::History:
            return new HistoryScreen(m);
        case ScreenId::Workouts:
            return new WorkoutsScreen(m);
        case ScreenId::WorkoutPreview:
            return new WorkoutPreviewScreen(m);
        case ScreenId::Rest:
            return new RestScreen(m);
        case ScreenId::StepBuilder:
            return new StepBuilderScreen(m);
        case ScreenId::Settings:
            return new SettingsScreen(m);
    }
    return nullptr;
}
