/**
 ******************************************************************************
 * @file    WorkoutsScreen.cpp
 * @brief   The workout list: presets, the user's workouts, "+ New workout".
 ******************************************************************************
 */

#include "gui/screens/WorkoutsScreen.hpp"

#include <cstdio>

#include "score/Spots.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

constexpr int32_t kHeaderY = 49;

} // namespace

WorkoutsScreen::WorkoutsScreen(Model& model)
    : Screen(model)
    , mKeys(&KeyInput::noHold, nullptr)
{
}

void WorkoutsScreen::build()
{
    Theme::header(mRoot, "WORKOUTS", kHeaderY);
    mMenu = std::make_unique<ListMenu>(mRoot, ListMenu::workoutsGeometry());
    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Dot, Hints::Dot, Hints::Dot, Hints::Dot);
    fillItems();
    showItems(mModel.workoutsIndex);
}

void WorkoutsScreen::onShow()
{
    if (!mModel.workoutsLoaded()) {
        mModel.requestWorkouts();
    }
}

void WorkoutsScreen::onHide()
{
    mModel.workoutsIndex = static_cast<uint8_t>(mMenu->selected());
}

void WorkoutsScreen::fillItems()
{
    mCount = 0;
    auto add = [this](const score::Workout& w) {
        snprintf(mLine2[mCount], kLine2Len, "%u step%s%s%u shots", static_cast<unsigned>(w.stepCount),
                 w.stepCount == 1 ? "" : "s", Theme::kMidDot, static_cast<unsigned>(w.totalShots()));
        mItems[mCount] = { w.name, mLine2[mCount], 0 };
        mIds[mCount]   = w.id;
        ++mCount;
    };
    for (uint8_t i = 0; i < score::kPresetCount; ++i) {
        add(score::preset(i));
    }
    for (uint8_t i = 0; i < mModel.userWorkoutCount() && mCount < kMaxItems - 1; ++i) {
        add(mModel.userWorkout(i));
    }
    mItems[mCount] = { "+ New workout", nullptr, Theme::kAccent };
    mIds[mCount]   = 0;
    ++mCount;
}

void WorkoutsScreen::showItems(uint16_t selected)
{
    mMenu->setItems(mItems, mCount, selected < mCount ? selected : static_cast<uint16_t>(mCount - 1));
}

void WorkoutsScreen::newWorkout()
{
    // A one-step draft: Free Throw, 10 attempts, 30 s rest.
    score::Workout& d = mModel.draft();
    d = score::Workout {};
    snprintf(d.name, sizeof(d.name), "Custom %u", static_cast<unsigned>(mModel.userWorkoutCount() + 1));
    d.stepCount = 1;
    d.steps[0]  = { static_cast<uint8_t>(score::Spot::Ft), 30,
                    { static_cast<uint8_t>(score::GoalType::Attempts), 0, 10 } };
    mModel.draftStep  = 0;
    mModel.draftField = 0;
    mModel.draftIsNew = true;
    ScreenManager::instance().goTo(ScreenId::StepBuilder);
}

void WorkoutsScreen::select()
{
    const uint8_t id = mIds[mMenu->selected()];
    if (id == 0) {
        newWorkout();
        return;
    }
    mModel.selectedWorkoutId = id;
    ScreenManager::instance().goTo(ScreenId::WorkoutPreview);
}

void WorkoutsScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind != KeyInput::Kind::Click) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:     mMenu->prev(); break;
        case KeyInput::Btn::Down:   mMenu->next(); break;
        case KeyInput::Btn::Select: select(); break;
        case KeyInput::Btn::Back:   ScreenManager::instance().goTo(ScreenId::Home); break;
        case KeyInput::Btn::None:   break;
    }
}

void WorkoutsScreen::onWorkouts()
{
    // Keep the selected workout selected if it is still in the list.
    const uint8_t id = mIds[mMenu->selected()];
    uint16_t sel = mMenu->selected();
    fillItems();
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mIds[i] == id) {
            sel = i;
            break;
        }
    }
    showItems(sel);
}

void WorkoutsScreen::onSuspend()
{
    mKeys.cancel();
}
