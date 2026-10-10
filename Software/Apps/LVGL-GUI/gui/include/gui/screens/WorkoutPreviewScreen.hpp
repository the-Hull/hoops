/**
 ******************************************************************************
 * @file    WorkoutPreviewScreen.hpp
 * @brief   One workout's steps before it starts.
 *
 * docs/mockup-layout.md §4. Shows Model::selectedWorkoutId: its name, one
 * row per step (number, spot, goal), the first rest and an estimate of the
 * time, and a START pill. SELECT starts it, DOWN edits it in the step
 * builder (a user workout in place; a preset as a new copy "<name> 2"),
 * BACK returns to the list. Up to four rows at the mockup's 25 px pitch,
 * five at 20 px; with more steps the fifth row reads "+ n more".
 * A user workout whose list is still on its way (just saved in the step
 * builder: the one held may be stale) shows "Loading..." until onWorkouts().
 * START while a session is in progress shows a "Session in progress" toast.
 ******************************************************************************
 */

#ifndef WORKOUT_PREVIEW_SCREEN_HPP
#define WORKOUT_PREVIEW_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/Toast.hpp"

class WorkoutPreviewScreen : public Screen
{
public:
    explicit WorkoutPreviewScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onAction(KeyInput::Action a) override;

    void onWorkouts() override;
    void onSuspend() override;

protected:
    void build() override;

private:
    /// (Re)draw the workout into mContent; false if the id is unknown or
    /// the list is still loading.
    bool show();
    void edit();

    KeyInput               mKeys;
    std::unique_ptr<Hints> mHints;
    std::unique_ptr<Toast> mToast;
    lv_obj_t*              mContent = nullptr;
    bool                   mShown   = false;   ///< show() drew the workout: START / edit allowed
};

#endif // WORKOUT_PREVIEW_SCREEN_HPP
