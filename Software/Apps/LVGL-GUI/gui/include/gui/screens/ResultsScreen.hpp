/**
 ******************************************************************************
 * @file    ResultsScreen.hpp
 * @brief   Session results in four pages: court, categories, highlights,
 *          trend.
 *
 * docs/mockup-layout.md §11-§14. Shows Model::results() (the service's
 * SessionStats for one session); the GUI adds nothing up, it only formats.
 * On show, when no results are held, it requests them: the session just
 * ended (resultsFrom == Session) or Model::resultsSessionId (History), and
 * shows "Loading…" until the reply; a reply naming no session shows
 * "No data".
 *
 * Keys: SELECT next page (4 wraps to 1), BACK to History or Home (where the
 * results were opened from), UP / DOWN per page: page 1 previous / next
 * spot with attempts, page 2 previous / next category, page 3 nothing,
 * page 4 previous / next trend category. The page is kept in
 * Model::resultsPage. Every change rebuilds the page container.
 ******************************************************************************
 */

#ifndef RESULTS_SCREEN_HPP
#define RESULTS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Court.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"

class ResultsScreen : public Screen
{
public:
    explicit ResultsScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;

    void onResults() override;
    void onHistory() override;
    void onSuspend() override;

    static constexpr uint8_t kPages = 4;

protected:
    void build() override;

private:
    enum Page : uint8_t { kCourt, kCategories, kHighlights, kTrend };

    /// Clear the page container and draw the current page (or the
    /// Loading / No data message), its header and its hints.
    void rebuild();
    void pageHeader(const char* title);
    void message(const char* text);

    void buildCourt();
    void buildCategories();
    void buildHighlights();
    void buildTrend();

    void setPage(uint8_t page);
    void step(int8_t dir);   ///< UP (-1) / DOWN (+1) on the current page
    /// The spot with attempts after (dir +1) or before (dir -1) @p from,
    /// wrapping; Court::kNone when there is none.
    uint8_t spotWithAttempts(uint8_t from, int8_t dir) const;

    KeyInput               mKeys;
    std::unique_ptr<Court> mCourt;
    std::unique_ptr<Hints> mHints;
    lv_obj_t*              mPageObj = nullptr;   ///< everything drawn; cleaned on rebuild

    uint8_t mPage     = kCourt;
    uint8_t mSpot     = Court::kNone;   ///< page 1 selection
    uint8_t mCat      = 0;              ///< page 2 selection (score::Category)
    uint8_t mTrendCat = 0;              ///< page 4: 0-3 score::Category, 4 total
    bool    mWaiting  = false;          ///< RESULTS requested, no reply yet
};

#endif // RESULTS_SCREEN_HPP
