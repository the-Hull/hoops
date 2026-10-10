/**
 ******************************************************************************
 * @file    HistoryScreen.cpp
 * @brief   Every saved session, newest first; SELECT opens one's results.
 ******************************************************************************
 */

#include "gui/screens/HistoryScreen.hpp"

#include <cstdio>

#include "score/Workout.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

constexpr int32_t kHeaderY = 49;
constexpr int32_t kEmptyY  = 106;   // where the pill would be

} // namespace

HistoryScreen::HistoryScreen(Model& model)
    : Screen(model)
    , mKeys(&Screen::keyAction, this)
{
}

void HistoryScreen::build()
{
    Theme::header(mRoot, "HISTORY", kHeaderY);

    mMenu = std::make_unique<ListMenu>(mRoot, ListMenu::workoutsGeometry());
    mMenu->setCircular(false);   // newest first: nothing above the newest
    mEmpty = Theme::label(mRoot, Theme::Font::Regular16, "", 20, Theme::topFor(Theme::Font::Regular16, kEmptyY), 200,
                          LV_TEXT_ALIGN_CENTER, Theme::kDim);

    mHints = std::make_unique<Hints>(mRoot);

    updateItems();
}

void HistoryScreen::onShow()
{
    mModel.requestHistory();    // the rows held are dropped: "Loading…"
    mModel.requestWorkouts();   // user workout names
    mWant = mModel.historyIndex;
    updateItems();
}

void HistoryScreen::onHide()
{
    if (mWant != kNoWant) {
        mModel.historyIndex = static_cast<uint8_t>(mWant);   // its row never arrived
    } else if (mCount > 0) {
        mModel.historyIndex = static_cast<uint8_t>(mMenu->selected());
    }
}

void HistoryScreen::workoutName(char* out, size_t n, uint16_t id) const
{
    if (id == 0) {
        snprintf(out, n, "Open");
        return;
    }
    if (const score::Workout* p = score::findPreset(static_cast<uint8_t>(id <= 0xFF ? id : 0))) {
        snprintf(out, n, "%.*s", static_cast<int>(score::kNameLen), p->name);
        return;
    }
    if (mModel.workoutsLoaded()) {
        for (uint8_t i = 0; i < mModel.userWorkoutCount(); ++i) {
            const score::Workout& w = mModel.userWorkout(i);
            if (w.id == id) {
                snprintf(out, n, "%.*s", static_cast<int>(score::kNameLen), w.name);
                return;
            }
        }
    }
    snprintf(out, n, "Custom");
}

void HistoryScreen::updateItems()
{
    const bool loaded = mModel.hasHistory();
    // Keep the selection as pages arrive (they only append rows).
    const uint16_t prev = mCount > 0 ? mMenu->selected() : 0;
    const uint8_t  n    = loaded ? mModel.historyCount() : 0;
    mCount = n < kMaxRows ? n : kMaxRows;

    for (uint8_t i = 0; i < mCount; ++i) {
        const score::SessionSummary& s = mModel.historyItem(i);
        char date[12];
        char name[score::kNameLen + 1];
        Theme::fmtDate(date, sizeof(date), s.ymd);
        workoutName(name, sizeof(name), s.workoutId);
        snprintf(mLine1[i], sizeof(mLine1[i]), "%s \xC2\xB7 %s", date, name);

        // The summary keeps per-category counts; the line shows their sum.
        unsigned makes = 0;
        unsigned att   = 0;
        for (uint8_t c = 0; c < score::kCategoryCount; ++c) {
            makes += s.catMakes[c];
            att   += s.catAtt[c];
        }
        snprintf(mLine2[i], sizeof(mLine2[i]), "%u/%u \xC2\xB7 %u %%", makes, att, static_cast<unsigned>(s.totalPct));
        mItems[i] = { mLine1[i], mLine2[i], 0 };
    }

    uint16_t sel = prev < mCount ? prev : 0;
    if (mWant != kNoWant && mCount > 0) {
        // The row to return to may still be on its way: select the nearest
        // one meanwhile, and stop waiting once it is placed or all is in.
        const uint16_t want = static_cast<uint16_t>(mWant);
        sel = want < mCount ? want : static_cast<uint16_t>(mCount - 1);
        if (want < mCount || mModel.historyComplete()) {
            mWant = kNoWant;
        }
    } else if (mWant != kNoWant && loaded && mModel.historyComplete()) {
        mWant = kNoWant;   // the index is empty
    }
    mMenu->setItems(mItems, mCount, sel);

    Theme::setText(mEmpty, loaded ? "No sessions yet" : "Loading\xE2\x80\xA6");
    Theme::setHidden(mEmpty, mCount > 0);

    const Hints::Style move = mCount > 1 ? Hints::Dot : Hints::Off;
    mHints->set(move, move, mCount > 0 ? Hints::Dot : Hints::Off, Hints::Dot);
}

void HistoryScreen::open()
{
    if (mCount == 0) {
        return;
    }
    const uint16_t sel       = mMenu->selected();
    mModel.historyIndex      = static_cast<uint8_t>(sel);
    mModel.resultsFrom       = Model::ResultsFrom::History;
    mModel.resultsSessionId  = mModel.historyItem(static_cast<uint8_t>(sel)).id;
    mModel.resultsPage       = 0;
    mModel.clearResults();   // Results requests this session on show
    ScreenManager::instance().goTo(ScreenId::Results);
}

void HistoryScreen::onKey(uint8_t code)
{
    onAction(mKeys.feed(code));
}

void HistoryScreen::onAction(KeyInput::Action a)
{
    if (!KeyInput::isActivate(a)) {
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:     if (mCount > 0) { mWant = kNoWant; mMenu->prev(); } break;
        case KeyInput::Btn::Down:   if (mCount > 0) { mWant = kNoWant; mMenu->next(); } break;
        case KeyInput::Btn::Select: open(); break;
        case KeyInput::Btn::Back:   ScreenManager::instance().goTo(ScreenId::Home); break;
        case KeyInput::Btn::None:   break;
    }
}

void HistoryScreen::onHistory()
{
    updateItems();
}

void HistoryScreen::onWorkouts()
{
    if (mModel.hasHistory()) {
        updateItems();
    }
}

void HistoryScreen::onSuspend()
{
    mKeys.cancel();
}
