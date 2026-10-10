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
    void requestHistory();
    void exitApp();

    // --- State (latest from the service) ------------------------------------
    bool hasSnapshot() const { return mHasSnapshot; }
    const score::Snapshot& snapshot() const { return mSnapshot; }

    uint8_t userWorkoutCount() const { return mUserCount; }
    const score::Workout& userWorkout(uint8_t i) const;
    bool workoutsLoaded() const { return mWorkoutsLoaded; }

    /// The last RESULTS reply. hasResults() is true only when the reply to
    /// the latest requestResults() named a session (sessionId != 0); a reply
    /// with sessionId 0 means "none" and still raises onResults().
    const CustomMessage::Results& results() const { return mResults; }
    bool hasResults() const { return mHasResults; }

    /// The last HISTORY reply (count may be 0); hasHistory() once it arrived.
    const CustomMessage::History& history() const { return mHistory; }
    bool hasHistory() const { return mHasHistory; }

    /// Local time from the service's LOCAL_TIME (tm_year since 1900, tm_mon 0-11).
    const std::tm& time() const { return mTime; }
    /// Today as YYYYMMDD; 0 before the clock is known.
    uint32_t todayYmd() const;

    // --- Navigation memory ---------------------------------------------------
    enum class PickerMode : uint8_t { StartOpen, ChangeSpot, StepSpot };
    PickerMode pickerMode = PickerMode::StartOpen;
    uint8_t    pickerSpot = 1;

    enum class ResultsFrom : uint8_t { Session, History };
    ResultsFrom resultsFrom = ResultsFrom::Session;
    uint32_t    resultsSessionId = 0;   ///< saved session to show when resultsFrom == History

    uint8_t homeIndex = 0, workoutsIndex = 0, historyIndex = 0, resultsPage = 0;
    uint8_t selectedWorkoutId = 1;   ///< preview / start

    score::Workout& draft() { return mDraft; }
    uint8_t draftStep  = 0;
    bool    draftIsNew = true;

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
    CustomMessage::History mHistory;
    bool                   mHasHistory = false;

    std::tm        mTime {};
    score::Workout mDraft {};
};

#endif // MODEL_HPP
