/**
 ******************************************************************************
 * @file    HomeScreen.hpp
 * @brief   Home: "HOOPS" header and the main menu (placeholder until the
 *          menu actions exist; SELECT does nothing yet).
 *
 * UP / DOWN move the selection, BACK exits the app. With HOOPS_WIDGET_DEMO
 * set to 1 it also draws a Ring, a Court and badge Hints behind the menu, to
 * check the shared widgets construct and render.
 ******************************************************************************
 */

#ifndef HOME_SCREEN_HPP
#define HOME_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"

#ifndef HOOPS_WIDGET_DEMO
#define HOOPS_WIDGET_DEMO 0
#endif

#if HOOPS_WIDGET_DEMO
#include "gui/widgets/Court.hpp"
#include "gui/widgets/Ring.hpp"
#endif

class HomeScreen : public Screen
{
public:
    explicit HomeScreen(Model& model);

    void onHide() override;
    void onKey(uint8_t code) override;
    void onSuspend() override;

protected:
    void build() override;

private:
    KeyInput                  mKeys;
    std::unique_ptr<Hints>    mHints;
    std::unique_ptr<ListMenu> mMenu;
#if HOOPS_WIDGET_DEMO
    std::unique_ptr<Ring>     mRing;
    std::unique_ptr<Court>    mCourt;
#endif
};

#endif // HOME_SCREEN_HPP
