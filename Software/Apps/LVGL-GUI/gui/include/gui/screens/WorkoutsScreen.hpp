/**
 ******************************************************************************
 * @file    WorkoutsScreen.hpp
 * @brief   The workout list: presets, the user's workouts, "+ New workout".
 *
 * docs/mockup-layout.md §3. UP / DOWN scroll (circular), SELECT opens the
 * workout's preview (or, on "+ New workout", the step builder with a fresh
 * one-step draft), BACK goes home. The user's workouts are requested on
 * show when the Model does not hold them yet; the list is rebuilt when they
 * arrive. The position is kept in Model::workoutsIndex.
 ******************************************************************************
 */

#ifndef WORKOUTS_SCREEN_HPP
#define WORKOUTS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"

#include "score/Workout.hpp"

class WorkoutsScreen : public Screen
{
public:
    explicit WorkoutsScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    void onWorkouts() override;
    void onSuspend() override;

protected:
    void build() override;

private:
    static constexpr uint8_t kMaxItems = score::kPresetCount + score::kMaxUserWorkouts + 1;
    static constexpr uint8_t kLine2Len = 32;

    /// Fill the items from the presets and the Model's user workouts.
    void rebuildItems(uint16_t selected);
    void select();
    void newWorkout();

    KeyInput                  mKeys;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Hints>    mHints;

    ListMenu::Item mItems[kMaxItems]              = {};
    uint8_t        mIds[kMaxItems]                = {};   ///< workout id per item; 0 = "+ New workout"
    char           mLine2[kMaxItems][kLine2Len]   = {};
    uint8_t        mCount                         = 0;
};

#endif // WORKOUTS_SCREEN_HPP
