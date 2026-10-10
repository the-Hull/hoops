/**
 ******************************************************************************
 * @file    SpotPickerScreen.hpp
 * @brief   Choose one of the 13 shooting spots on a half court.
 *
 * docs/mockup-layout.md §2: the court with the selected spot ringed and in
 * teal, and the spot wheel below it. UP / DOWN move (circular). What SELECT
 * and BACK do depends on Model::pickerMode: start an open session, change
 * the spot of the running one, or set a workout step's spot. The selection
 * is kept in Model::pickerSpot.
 ******************************************************************************
 */

#ifndef SPOT_PICKER_SCREEN_HPP
#define SPOT_PICKER_SCREEN_HPP

#include <memory>

#include "score/Spots.hpp"

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Court.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"

class SpotPickerScreen : public Screen
{
public:
    explicit SpotPickerScreen(Model& model);

    void onKey(uint8_t code) override;
    void onSuspend() override;

protected:
    void build() override;

private:
    static void holdCb(void* ctx, KeyInput::Action a);

    /// Court highlight and Model::pickerSpot follow the list selection.
    void showSelection();
    void select();
    void back();

    KeyInput                  mKeys;
    std::unique_ptr<Court>    mCourt;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Hints>    mHints;

    ListMenu::Item mItems[score::kSpotCount] = {};
    uint8_t        mShown = Court::kNone;   ///< spot currently drawn as selected
};

#endif // SPOT_PICKER_SCREEN_HPP
