/**
 ******************************************************************************
 * @file    HistoryScreen.hpp
 * @brief   Every saved session, newest first; SELECT opens one's results.
 *
 * docs/mockup-layout.md §3 list geometry with the HISTORY header. Rows come
 * from the service's HISTORY pages (Model::historyItem(), up to
 * score::kIndexCapacity) and render as the pages arrive: line 1
 * "6.10. · Game Shots" (preset names from score::findPreset, user workouts
 * by id from the WORKOUT_ITEM list, "Open" for free shooting, "Custom" for a
 * workout not known), line 2 "m/a · p %" (the summary's category counts and
 * totalPct). History and workouts are requested on show. UP / DOWN move,
 * SELECT opens Results on page 1 for that session, BACK returns Home. The
 * selection is kept in Model::historyIndex; on return it is restored as soon
 * as its row has arrived (until then the nearest loaded row is selected).
 ******************************************************************************
 */

#ifndef HISTORY_SCREEN_HPP
#define HISTORY_SCREEN_HPP

#include <memory>

#include "score/History.hpp"

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"

class HistoryScreen : public Screen
{
public:
    explicit HistoryScreen(Model& model);

    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onAction(KeyInput::Action a) override;

    void onHistory() override;
    void onWorkouts() override;
    void onSuspend() override;

protected:
    void build() override;

private:
    static constexpr uint8_t kMaxRows = score::kIndexCapacity;
    static constexpr int16_t kNoWant  = -1;

    /// Rebuild the rows from Model::historyItem(); "Loading…" before the
    /// first page, "No sessions yet" when the index is empty.
    void updateItems();
    /// Ask for the whole index (after the user workouts are in).
    void startHistory();
    /// Workout name for a summary's workoutId into @p out.
    void workoutName(char* out, size_t n, uint16_t id) const;
    void open();

    KeyInput                  mKeys;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Hints>    mHints;
    lv_obj_t*                 mEmpty = nullptr;   ///< "Loading…" / "No sessions yet"

    ListMenu::Item mItems[kMaxRows]    = {};
    char           mLine1[kMaxRows][48] = {};
    char           mLine2[kMaxRows][24] = {};
    uint8_t        mCount = 0;
    bool           mStarted = false;   ///< this visit's history stream asked for
    /// Model::historyIndex to restore once that row has arrived; kNoWant
    /// once it is placed, the user moved or the history is complete.
    int16_t        mWant  = kNoWant;
};

#endif // HISTORY_SCREEN_HPP
