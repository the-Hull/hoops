/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of Hoops and its link to the service process.
 *
 * Same shape as RunLVGL's Model: registers with SDK::LVGL::Port for the
 * kernel's lifecycle and the service's custom messages, keeps the latest copy
 * of what the service sent (Commands.hpp), and turns screen intents into
 * messages. It computes no statistics, touches no files and drives no
 * haptics: the service does all of that. It also remembers navigation state
 * (list positions, the workout draft) so a screen reopens where it was left.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>
#include <ctime>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Utils/Utils.hpp"
#include "SDK/GUI/Config.hpp"

#include "Commands.hpp"
#include "score/SessionEngine.hpp"
#include "score/Workout.hpp"

namespace App::Config
{
constexpr uint32_t kFrameRate       = SDK::GUI::Config::kFrameRate;
constexpr uint32_t kIdleTimeoutTicks = SDK::Utils::secToTicks(60, kFrameRate);   // 60 s
} // namespace App::Config

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    /// The screen that receives model events; nullptr for none.
    void bind(ModelListener* l);

    // --- Intents (GUI -> service) -------------------------------------------
    void startOpen(uint8_t spot);
    void startWorkout(uint8_t id);
    void shot(bool made);
    void undo();
    void pause();
    void resume();
    void skipRest();
    void extendRest();                    ///< +15 s
    void changeSpot(uint8_t spot);
    void endSession(bool save);
    void requestSnapshot();
    void requestWorkouts();               ///< onWorkouts() once the list is in
    void saveWorkout(const score::Workout& w);
    void deleteWorkout(uint8_t id);
    void requestResults(uint32_t sessionId);   ///< 0 = the session just ended
    /// Forget the results held, so the Results screen requests fresh ones.
    void clearResults() { mHasResults = false; }
    /// Ask for the whole index; the service streams it in pages of
    /// kTrendCount rows, newest first (onHistory() fires per page).
    void requestHistory();
    /// Ask for the per-spot last / best records (onSpotStats()).
    void requestSpotStats();
    void exitApp();
    /// Store @p s, apply its scheme here at once (applyScheme()) and send it
    /// to the service, which answers with SETTINGS.
    void setSettings(const score::Settings& s);

    // --- State (latest from the service) ------------------------------------
    bool hasSnapshot() const { return mHasSnapshot; }
    const score::Snapshot& snapshot() const { return mSnapshot; }
    /// The latest snapshot shows a session in progress (Active, Paused or Rest).
    bool sessionRunning() const
    {
        return mHasSnapshot && score::isRunning(static_cast<score::SessionState>(mSnapshot.state));
    }

    uint8_t userWorkoutCount() const { return mUserCount; }
    const score::Workout& userWorkout(uint8_t i) const;
    bool workoutsLoaded() const { return mWorkoutsLoaded; }

    /// A preset or a user workout (from the list held) by id; nullptr if none.
    const score::Workout* workoutById(uint8_t id) const;

    /// The last STEP_DONE of the running workout; hasStepDone() is false
    /// until one arrives, and again after startWorkout() / startOpen().
    const CustomMessage::StepDone& lastStepDone() const { return mStepDone; }
    bool hasStepDone() const { return mHasStepDone; }

    /// The last RESULTS reply. hasResults() is true only when the reply to
    /// the latest requestResults() named a session (sessionId != 0); a reply
    /// with sessionId 0 means "none" and still raises onResults().
    const CustomMessage::Results& results() const { return mResults; }
    bool hasResults() const { return mHasResults; }

    /// The history index, newest first, as far as its pages have arrived.
    /// hasHistory(): the first page of the latest requestHistory() is in;
    /// historyCount(): rows received (contiguous from the newest);
    /// historyTotal(): rows in the index; historyComplete(): all of them in.
    bool    hasHistory() const { return mHasHistory; }
    bool    historyComplete() const { return mHasHistory && mHistoryLoaded == mHistoryTotal; }
    uint8_t historyCount() const { return mHistoryLoaded; }
    uint8_t historyTotal() const { return mHistoryTotal; }
    /// Row @p i (0 = newest), clamped to the last row received; a zero
    /// summary while none is.
    const score::SessionSummary& historyItem(uint8_t i) const;

    /// The index's per-spot records (SPOT_STATS), indexed by score::Spot:
    /// spotLast() = the newest session's makes/attempts there (attempts 0 =
    /// never shot), spotBest() = the best session with at least
    /// kMinAttemptsForRank attempts (attempts 0 = none). Zero for a bad spot.
    bool hasSpotStats() const { return mHasSpotStats; }
    const score::SpotStat& spotLast(uint8_t spot) const;
    const score::SpotStat& spotBest(uint8_t spot) const;

    /// Local time from the service's LOCAL_TIME (tm_year since 1900, tm_mon 0-11).
    const std::tm& time() const { return mTime; }
    /// Today as YYYYMMDD; 0 before the clock is known.
    uint32_t todayYmd() const;

    /// The settings the service last sent (SETTINGS) or setSettings() set;
    /// the defaults until then (hasSettings() false).
    const score::Settings& settings() const { return mSettings; }
    bool hasSettings() const { return mHasSettings; }

    // --- Navigation memory ---------------------------------------------------
    enum class PickerMode : uint8_t { StartOpen, ChangeSpot, StepSpot };
    PickerMode pickerMode = PickerMode::StartOpen;
    uint8_t    pickerSpot = 1;

    enum class ResultsFrom : uint8_t { Session, History };
    ResultsFrom resultsFrom = ResultsFrom::Session;
    uint32_t    resultsSessionId = 0;   ///< saved session to show when resultsFrom == History

    uint8_t homeIndex = 0;   ///< HomeScreen's selected action (its Action enum), not a row
    uint8_t workoutsIndex = 0, historyIndex = 0, resultsPage = 0;
    uint8_t settingsIndex = 0;   ///< SettingsScreen's selected row
    uint8_t selectedWorkoutId = 1;   ///< preview / start

    /// Set by Pause when it routes to Rest / Active after the session runs
    /// again; that screen then ignores Paused snapshots until a live one
    /// (they predate the resume) and clears it.
    bool resumeEntry = false;

    /// A message for the Home screen to show once on arrival (a toast), e.g.
    /// "Save failed" after End & save; Home clears it. A string literal.
    const char* homeToast = nullptr;

    score::Workout& draft() { return mDraft; }
    uint8_t draftStep  = 0;
    bool    draftIsNew = true;
    /// The step builder's field to open on (0 Spot .. 3 Rest); the
    /// SpotPicker sets 1 (Goal type) after a pick, the builder resets it.
    uint8_t draftField = 0;

    // --- Controls --------------------------------------------------------------
    /// Every button code passes here first (Screen's key callback).
    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();

private:
    // IGuiLifeCycleCallback
    void onStart() override;
    void onFrame() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler
    bool customMessageHandler(SDK::MessageBase* message) override;

    /// Make mSettings' scheme the active palette; when it changed and a
    /// screen is on display, rebuild that screen in it.
    void applyScheme();

    ModelListener*     mListener = nullptr;
    const SDK::Kernel& mKernel;

    bool     mIsRunning = false;
    uint32_t mIdleTimer = 0;

    score::Snapshot mSnapshot {};
    bool            mHasSnapshot = false;

    score::Workout mUserWorkouts[score::kMaxUserWorkouts] {};
    uint8_t        mUserCount      = 0;
    bool           mWorkoutsLoaded = false;

    CustomMessage::Results mResults;
    bool                   mHasResults = false;
    score::SessionSummary mHistory[score::kIndexCapacity] {};   ///< newest first
    uint8_t               mHistoryTotal  = 0;
    uint8_t               mHistoryLoaded = 0;
    bool                  mHasHistory    = false;

    score::SpotStat mSpotLast[score::kSpotCount] {};
    score::SpotStat mSpotBest[score::kSpotCount] {};
    bool            mHasSpotStats = false;

    CustomMessage::StepDone mStepDone;
    bool                    mHasStepDone = false;

    score::Settings mSettings    = score::Settings::defaults();
    bool            mHasSettings = false;

    std::tm        mTime {};
    score::Workout mDraft {};
};

#endif // MODEL_HPP
