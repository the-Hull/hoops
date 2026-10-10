/**
 ******************************************************************************
 * @file    HomeScreen.cpp
 * @brief   Home: "HOOPS" header, the main menu and the last session's line.
 ******************************************************************************
 */

#include "gui/screens/HomeScreen.hpp"

#include <cstdio>

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

constexpr int32_t kFooterY = 193;
constexpr int32_t kHeaderY = 47;
// A session in progress: the header moves up to make room for the hint
// line, both clear of the menu pill (top at y 61).
constexpr int32_t kHeaderUpY = 32;
constexpr int32_t kHintY     = 50;

} // namespace

HomeScreen::HomeScreen(Model& model)
    : Screen(model)
    , mKeys(&Screen::keyAction, this)
{
}

void HomeScreen::build()
{
    mHeader = Theme::header(mRoot, "HOOPS", kHeaderY);
    mHint   = Theme::label(mRoot, Theme::Font::Regular14, "Session in progress", 20,
                           Theme::topFor(Theme::Font::Regular14, kHintY), 200, LV_TEXT_ALIGN_CENTER, Theme::kAmber);
    Theme::setHidden(mHint, true);

    mMenu = std::make_unique<ListMenu>(mRoot, ListMenu::homeGeometry());

    mFooter = Theme::label(mRoot, Theme::Font::Regular14, "", 20, Theme::topFor(Theme::Font::Regular14, kFooterY),
                           200, LV_TEXT_ALIGN_CENTER, Theme::kDim);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Off, Hints::Off, Hints::Dot, Hints::Dot);
    mToast = std::make_unique<Toast>(mRoot);

    updateItems();
    updateFooter();
}

void HomeScreen::onShow()
{
    if (mModel.homeToast) {
        mToast->show(mModel.homeToast);
        mModel.homeToast = nullptr;
    }
    mModel.requestSnapshot();
    mModel.requestHistory();
}

void HomeScreen::onHide()
{
    // The action, not the row: the rows shift when "Resume session" comes or
    // goes (e.g. a stale running snapshot on return after a discard).
    mModel.homeIndex = static_cast<uint8_t>(mActions[mMenu->selected()]);
}

bool HomeScreen::sessionInProgress() const
{
    if (!mModel.hasSnapshot()) {
        return false;
    }
    return score::isRunning(static_cast<score::SessionState>(mModel.snapshot().state));
}

void HomeScreen::updateItems()
{
    const bool resume = sessionInProgress();
    if (mCount > 0 && resume == mHasResume) {
        return;
    }

    // Keep the selected action across the change; the first build selects
    // the action in homeIndex. Without that action (Resume gone): the first
    // row. A session restored after a crash (kResumable, until resumed)
    // selects Resume; a stale running snapshot (e.g. after a discard) must
    // not move the selection.
    Action current = mCount > 0 ? mActions[mMenu->selected()] : static_cast<Action>(mModel.homeIndex);
    if (resume && !mHasResume && (mModel.snapshot().flags & score::Snapshot::kResumable)) {
        current = Action::Resume;
    }

    mCount     = 0;
    mHasResume = resume;
    auto add = [this](Action a, const char* text, uint32_t color) {
        mItems[mCount]   = { text, nullptr, color };
        mActions[mCount] = a;
        ++mCount;
    };
    if (resume) {
        add(Action::Resume, "Resume session", Theme::kText);
    }
    add(Action::Open,     "Open Session", Theme::kText);
    add(Action::Workouts, "Workouts",     Theme::kText);
    add(Action::History,  "History",      Theme::kText);
    add(Action::Settings, "Settings",     Theme::kDim);

    uint16_t sel = 0;
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mActions[i] == current) {
            sel = i;
        }
    }
    mMenu->setItems(mItems, mCount, sel);

    Theme::setHidden(mHint, !resume);
    lv_obj_set_y(mHeader, Theme::topFor(Theme::Font::Regular14, resume ? kHeaderUpY : kHeaderY));
}

void HomeScreen::updateFooter()
{
    if (!mModel.hasHistory() || mModel.history().count == 0) {
        Theme::setHidden(mFooter, true);
        return;
    }
    const score::SessionSummary& last = mModel.history().items[0];
    char date[12];
    Theme::fmtDate(date, sizeof(date), last.ymd);
    char buf[40];
    snprintf(buf, sizeof(buf), "Last \xC2\xB7 %s \xC2\xB7 %u %%", date, static_cast<unsigned>(last.totalPct));
    Theme::setText(mFooter, buf);
    Theme::setHidden(mFooter, false);
}

void HomeScreen::activate(Action a)
{
    ScreenManager& sm = ScreenManager::instance();
    switch (a) {
        case Action::Resume:
            switch (static_cast<score::SessionState>(mModel.snapshot().state)) {
                case score::SessionState::Paused: sm.goTo(ScreenId::Pause); break;
                case score::SessionState::Rest:   sm.goTo(ScreenId::Rest); break;
                default:                          sm.goTo(ScreenId::Active); break;
            }
            break;
        case Action::Open:
            mModel.pickerMode = Model::PickerMode::StartOpen;
            sm.goTo(ScreenId::SpotPicker);
            break;
        case Action::Workouts:
            sm.goTo(ScreenId::Workouts);
            break;
        case Action::History:
            sm.goTo(ScreenId::History);
            break;
        case Action::Settings:
            break;
    }
}

void HomeScreen::onKey(uint8_t code)
{
    onAction(mKeys.feed(code));
}

void HomeScreen::onAction(KeyInput::Action a)
{
    if (!KeyInput::isActivate(a)) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:     mMenu->prev(); break;
        case KeyInput::Btn::Down:   mMenu->next(); break;
        case KeyInput::Btn::Select: activate(mActions[mMenu->selected()]); break;
        case KeyInput::Btn::Back:
            if (sessionInProgress()) {
                // The pause menu is the way out of a session; make it true.
                if (static_cast<score::SessionState>(mModel.snapshot().state) != score::SessionState::Paused) {
                    mModel.pause();
                }
                ScreenManager::instance().goTo(ScreenId::Pause);
            } else {
                mModel.exitApp();
            }
            break;
        case KeyInput::Btn::None:   break;
    }
}

void HomeScreen::onSnapshot(const score::Snapshot& s)
{
    (void)s;
    updateItems();
}

void HomeScreen::onHistory()
{
    updateFooter();
}

void HomeScreen::onSuspend()
{
    mKeys.cancel();
}
