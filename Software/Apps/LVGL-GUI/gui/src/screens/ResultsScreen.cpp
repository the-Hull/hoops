/**
 ******************************************************************************
 * @file    ResultsScreen.cpp
 * @brief   Session results (placeholder: header and "Coming soon").
 ******************************************************************************
 */

#include "gui/screens/ResultsScreen.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

ResultsScreen::ResultsScreen(Model& model)
    : Screen(model)
    , mKeys(&ResultsScreen::holdCb, this)
{
}

void ResultsScreen::holdCb(void* ctx, KeyInput::Action a)
{
    // No hold actions here; the callback is set so none is reported lost.
    (void)ctx;
    (void)a;
}

void ResultsScreen::build()
{
    Theme::header(mRoot, "RESULTS", 47);
    Theme::label(mRoot, Theme::Font::Regular18, "Coming soon", 20, Theme::topFor(Theme::Font::Regular18, 120), 200,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Off, Hints::Off, Hints::Off, Hints::Dot);
}

void ResultsScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (a.kind == KeyInput::Kind::Click && a.btn == KeyInput::Btn::Back) {
        ScreenManager::instance().goTo(ScreenId::Home);
    }
}

void ResultsScreen::onSuspend()
{
    mKeys.cancel();
}
