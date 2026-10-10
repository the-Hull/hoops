/**
 ******************************************************************************
 * @file    SpotPickerScreen.cpp
 * @brief   Choose one of the 13 shooting spots on a half court.
 ******************************************************************************
 */

#include "gui/screens/SpotPickerScreen.hpp"

#include <cstdio>

#include "score/Stats.hpp"

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
constexpr int32_t kFooterY    = 211;

} // namespace

SpotPickerScreen::SpotPickerScreen(Model& model)
    : Screen(model)
    , mKeys(&Screen::keyAction, this)
{
    for (uint8_t i = 0; i < score::kSpotCount; ++i) {
        mItems[i] = { score::spotInfo(i).name, nullptr, 0 };
    }
}

void SpotPickerScreen::build()
{
    Theme::header(mRoot, "CHOOSE SPOT", 37);

    mCourt = std::make_unique<Court>(mRoot, kCourtX, kCourtY, kCourtScale, kCourtLineW, kCourtSpotR);

    mMenu = std::make_unique<ListMenu>(mRoot, ListMenu::pickerGeometry());
    mMenu->setCircular(true);
    const uint8_t start = mModel.pickerSpot < score::kSpotCount ? mModel.pickerSpot : 0;
    mMenu->setItems(mItems, score::kSpotCount, start);

    mFooter = Theme::label(mRoot, Theme::Font::Regular14, "", 40, Theme::topFor(Theme::Font::Regular14, kFooterY),
                           160, LV_TEXT_ALIGN_CENTER, Theme::kDim);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Dot, Hints::Dot, Hints::Dot, Hints::Dot);
    mToast = std::make_unique<Toast>(mRoot);

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
    updateFooter();
}

void SpotPickerScreen::updateFooter()
{
    const CustomMessage::History& h = mModel.history();
    if (!mModel.hasHistory() || h.count == 0) {
        Theme::setHidden(mFooter, true);
        return;
    }
    const score::Category cat = score::spotInfo(static_cast<uint8_t>(mMenu->selected())).category;
    const uint8_t c = static_cast<uint8_t>(cat);
    const score::SessionSummary& last = h.items[0];
    if (c >= score::kCategoryCount || last.catAtt[c] == 0) {
        Theme::setHidden(mFooter, true);
        return;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "Last %s \xC2\xB7 %u %%", score::categoryShort(cat),
             static_cast<unsigned>(score::pct(last.catMakes[c], last.catAtt[c])));
    Theme::setText(mFooter, buf);
    Theme::setHidden(mFooter, false);
}

void SpotPickerScreen::onShow()
{
    mModel.requestHistory();
}

void SpotPickerScreen::onHistory()
{
    updateFooter();
}

void SpotPickerScreen::select()
{
    const uint8_t spot = static_cast<uint8_t>(mMenu->selected());
    ScreenManager& sm  = ScreenManager::instance();
    switch (mModel.pickerMode) {
        case Model::PickerMode::StartOpen:
            if (mModel.sessionRunning()) {
                // One session at a time: the service would refuse the start.
                mToast->show("Session in progress");
                break;
            }
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
            mModel.draftField = 1;   // on to the goal type
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
    onAction(mKeys.feed(code));
}

void SpotPickerScreen::onAction(KeyInput::Action a)
{
    if (!KeyInput::isActivate(a)) {
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
