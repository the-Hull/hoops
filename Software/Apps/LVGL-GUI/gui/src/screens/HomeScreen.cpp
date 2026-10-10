/**
 ******************************************************************************
 * @file    HomeScreen.cpp
 * @brief   Home: "HOOPS" header and the main menu.
 ******************************************************************************
 */

#include "gui/screens/HomeScreen.hpp"

#include "gui/theme/Theme.hpp"

namespace
{

const ListMenu::Item kItems[] = {
    { "Open Session", nullptr, Theme::kText },
    { "Workouts",     nullptr, Theme::kText },
    { "History",      nullptr, Theme::kText },
    { "Settings",     nullptr, Theme::kDim },
};
constexpr uint16_t kItemCount = sizeof(kItems) / sizeof(kItems[0]);

} // namespace

HomeScreen::HomeScreen(Model& model)
    : Screen(model)
{
}

void HomeScreen::build()
{
#if HOOPS_WIDGET_DEMO
    mRing = std::make_unique<Ring>(mRoot);
    mRing->setProgress(636, Theme::kGood);
    mRing->setDashed(Theme::kAmber);
    const uint8_t states[10] = { 1, 1, 2, 1, 1, 0, 0, 0, 0, 0 };
    mRing->setSegments(10, states);

    mCourt = std::make_unique<Court>(mRoot, 67, 0, 0.53f, 1, 3);
    for (uint8_t s = 0; s < score::kSpotCount; ++s) {
        mCourt->setSpotColor(s, s % 3 == 0 ? Theme::kGood : (s % 3 == 1 ? Theme::kAmber : Theme::kBad));
    }
    mCourt->setSelected(1);
#endif

    Theme::header(mRoot, "HOOPS", 47);

    mMenu = std::make_unique<ListMenu>(mRoot, ListMenu::homeGeometry());
    mMenu->setItems(kItems, kItemCount, mModel.homeIndex);

    mHints = std::make_unique<Hints>(mRoot);
#if HOOPS_WIDGET_DEMO
    mHints->set(Hints::Undo, Hints::PauseIcon, Hints::Make, Hints::Miss);
#else
    mHints->set(Hints::Off, Hints::Off, Hints::Dot, Hints::Dot);
#endif
}

void HomeScreen::onHide()
{
    mModel.homeIndex = static_cast<uint8_t>(mMenu->selected());
}

void HomeScreen::onSuspend()
{
    mKeys.cancel();
}

void HomeScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind != KeyInput::Kind::Click) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:     mMenu->prev(); break;
        case KeyInput::Btn::Down:   mMenu->next(); break;
        case KeyInput::Btn::Select: break;   // menu actions arrive with their screens
        case KeyInput::Btn::Back:   mModel.exitApp(); break;
        case KeyInput::Btn::None:   break;
    }
}
