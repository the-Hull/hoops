/**
 ******************************************************************************
 * @file    HistoryScreen.hpp
 * @brief   The newest saved sessions, newest first; SELECT opens one's
 *          results.
 *
 * docs/mockup-layout.md §3 list geometry with the HISTORY header. Rows come
 * from the service's HISTORY reply (up to score::kTrendCount): line 1
 * "6.10. · Game Shots" (preset names from score::findPreset, user workouts
 * by id from the WORKOUT_ITEM list, "Open" for free shooting, "Custom" for a
 * workout not known), line 2 "m/a · p %" (the summary's category counts and
 * totalPct). History and workouts are requested on show. UP / DOWN move,
 * SELECT opens Results on page 1 for that session, BACK returns Home. The
 * selection is kept in Model::historyIndex.
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
    static constexpr uint8_t kMaxRows = score::kTrendCount;

    /// Rebuild the rows from Model::history(); "Loading…" before the reply,
    /// "No sessions yet" when it is empty.
    void updateItems();
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
};

#endif // HISTORY_SCREEN_HPP
