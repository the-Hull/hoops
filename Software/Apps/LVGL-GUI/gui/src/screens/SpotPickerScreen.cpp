/**
 ******************************************************************************
 * @file    SpotPickerScreen.cpp
 * @brief   Choose one of the 13 shooting spots on a half court.
 ******************************************************************************
 */

#include "gui/screens/SpotPickerScreen.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

// Court box 106 x 71 at (67, 47): the 200 x 135 mockup units at 0.528 px.
constexpr int32_t kCourtX     = 67;
constexpr int32_t kCourtY     = 47;
constexpr float   kCourtScale = 0.528f;
constexpr int32_t kCourtLineW = 1;
constexpr int32_t kCourtSpotR = 3;

} // namespace

SpotPickerScreen::SpotPickerScreen(Model& model)
    : Screen(model)
    , mKeys(&SpotPickerScreen::holdCb, this)
{
    for (uint8_t i = 0; i < score::kSpotCount; ++i) {
        mItems[i] = { score::spotInfo(i).name, nullptr, 0 };
    }
}

void SpotPickerScreen::holdCb(void* ctx, KeyInput::Action a)
{
    // No hold actions here; the callback is set so none is reported lost.
    (void)ctx;
    (void)a;
}

void SpotPickerScreen::build()
{
    Theme::header(mRoot, "CHOOSE SPOT", 37);

    mCourt = std::make_unique<Court>(mRoot, kCourtX, kCourtY, kCourtScale, kCourtLineW, kCourtSpotR);

    mMenu = std::make_unique<ListMenu>(mRoot, ListMenu::pickerGeometry());
    mMenu->setCircular(true);
    const uint8_t start = mModel.pickerSpot < score::kSpotCount ? mModel.pickerSpot : 0;
    mMenu->setItems(mItems, score::kSpotCount, start);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Dot, Hints::Dot, Hints::Dot, Hints::Dot);

    showSelection();
}

void SpotPickerScreen::showSelection()
{
    const uint8_t spot = static_cast<uint8_t>(mMenu->selected());
    if (mShown != Court::kNone) {
        mCourt->setSpotColor(mShown, Theme::kLine);
    }
    mCourt->setSpotColor(spot, Theme::kAccent);
    mCourt->setSelected(spot);
    mShown            = spot;
    mModel.pickerSpot = spot;
}

void SpotPickerScreen::select()
{
    const uint8_t spot = static_cast<uint8_t>(mMenu->selected());
    ScreenManager& sm  = ScreenManager::instance();
    switch (mModel.pickerMode) {
        case Model::PickerMode::StartOpen:
            mModel.startOpen(spot);
            sm.goTo(ScreenId::Active);
            break;
        case Model::PickerMode::ChangeSpot:
            mModel.changeSpot(spot);
            sm.goTo(ScreenId::Active);
            break;
        case Model::PickerMode::StepSpot:
            if (mModel.draftStep < score::kMaxSteps) {
                mModel.draft().steps[mModel.draftStep].spot = spot;
            }
            sm.goTo(ScreenId::StepBuilder);
            break;
    }
}

void SpotPickerScreen::back()
{
    ScreenManager& sm = ScreenManager::instance();
    switch (mModel.pickerMode) {
        case Model::PickerMode::StartOpen:  sm.goTo(ScreenId::Home); break;
        case Model::PickerMode::ChangeSpot: sm.goTo(ScreenId::Active); break;
        case Model::PickerMode::StepSpot:   sm.goTo(ScreenId::StepBuilder); break;
    }
}

void SpotPickerScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind != KeyInput::Kind::Click) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:     mMenu->prev(); showSelection(); break;
        case KeyInput::Btn::Down:   mMenu->next(); showSelection(); break;
        case KeyInput::Btn::Select: select(); break;
        case KeyInput::Btn::Back:   back(); break;
        case KeyInput::Btn::None:   break;
    }
}

void SpotPickerScreen::onSuspend()
{
    mKeys.cancel();
}
