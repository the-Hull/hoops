/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of Hoops and its link to the service process.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include <cstring>

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/GUI/Button.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

namespace
{
constexpr size_t kStepDonePayload = 12;   ///< CustomMessage::StepDone after its MessageBase
} // namespace

Model::Model()
    : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);

#if defined(SIMULATOR)
    LOG_INFO("Keys: 1 = UP, 2 = DOWN, 3 = SELECT, 4 = BACK (hold >= 0.5 s = long press), Esc = close\n");
#endif
}

void Model::bind(ModelListener* l)
{
    mListener = l;
    if (l) {
        l->model = this;
    }
}

// --- Intents ---------------------------------------------------------------

void Model::startOpen(uint8_t spot)
{
    mHasStepDone = false;
    resumeEntry  = false;
    SDK::send_msg<CustomMessage::StartOpen>(mKernel, spot);
}

void Model::startWorkout(uint8_t id)
{
    mHasStepDone = false;
    resumeEntry  = false;
    SDK::send_msg<CustomMessage::StartWorkout>(mKernel, id);
}

void Model::shot(bool made)                { SDK::send_msg<CustomMessage::Shot>(mKernel, static_cast<uint8_t>(made)); }
void Model::undo()                         { SDK::send_msg<CustomMessage::Undo>(mKernel); }
void Model::pause()                        { SDK::send_msg<CustomMessage::Pause>(mKernel); }
void Model::resume()                       { SDK::send_msg<CustomMessage::Resume>(mKernel); }
void Model::skipRest()                     { SDK::send_msg<CustomMessage::SkipRest>(mKernel); }
void Model::extendRest()                   { SDK::send_msg<CustomMessage::ExtendRest>(mKernel, static_cast<uint16_t>(15)); }
void Model::changeSpot(uint8_t spot)       { SDK::send_msg<CustomMessage::ChangeSpot>(mKernel, spot); }
void Model::endSession(bool save)          { SDK::send_msg<CustomMessage::EndSession>(mKernel, static_cast<uint8_t>(save)); }
void Model::requestSnapshot()              { SDK::send_msg<CustomMessage::GetSnapshot>(mKernel); }
void Model::saveWorkout(const score::Workout& w) { SDK::send_msg<CustomMessage::SaveWorkout>(mKernel, w); }
void Model::deleteWorkout(uint8_t id)      { SDK::send_msg<CustomMessage::DeleteWorkout>(mKernel, id); }

void Model::resetWorkouts()
{
    // The reply is the workout list: count it as the one in flight, so a
    // requestWorkouts() in the same frame doesn't double the burst.
    mWorkoutsLoaded  = false;
    mWorkoutsPending = true;
    mWorkoutsAskedAt = mFrame;
    mWorkoutsRetried = false;
    SDK::send_msg<CustomMessage::ResetWorkouts>(mKernel);
}

void Model::requestWorkouts()
{
    mWorkoutsLoaded = false;   // the list already held stays readable meanwhile
    // One list in flight: a second request in the same frame (e.g. the
    // WORKOUT_SAVED handler and the preview's onShow) would double the burst
    // past the GUI's 10-message queue. A list lost on the way is asked for
    // again by the next request after kWorkoutsRetryFrames.
    if (mWorkoutsPending && mFrame - mWorkoutsAskedAt < kWorkoutsRetryFrames) {
        return;
    }
    mWorkoutsRetried = false;   // a fresh request gets its own automatic re-ask
    sendGetWorkouts();
}

void Model::sendGetWorkouts()
{
    mWorkoutsPending = true;
    mWorkoutsAskedAt = mFrame;
    SDK::send_msg<CustomMessage::GetWorkouts>(mKernel);
}

void Model::requestResults(uint32_t sessionId)
{
    mHasResults = false;
    SDK::send_msg<CustomMessage::GetResults>(mKernel, sessionId);
}

void Model::requestHistory(bool all)
{
    // A new stream: pages of an older one still on their way carry its seq
    // and are dropped.
    ++mHistorySeq;
    mHistoryAll    = all;
    mHasHistory    = false;
    mHistoryTotal  = 0;
    mHistoryLoaded = 0;
    SDK::send_msg<CustomMessage::GetHistory>(mKernel, static_cast<uint8_t>(0), mHistorySeq);
}

void Model::clearHistory()
{
    SDK::send_msg<CustomMessage::ClearHistory>(mKernel);
    requestHistory(false);   // handled after the clear: the empty page
}

void Model::requestSettings()
{
    SDK::send_msg<CustomMessage::GetSettings>(mKernel);
}

void Model::requestSpotStats()
{
    SDK::send_msg<CustomMessage::GetSpotStats>(mKernel);
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::setSettings(const score::Settings& s)
{
    mSettings = s;
    applyScheme();
    SDK::send_msg<CustomMessage::SetSettings>(mKernel, s);
}

void Model::applyScheme()
{
    const Theme::Scheme s = static_cast<score::Scheme>(mSettings.scheme) == score::Scheme::Light
                              ? Theme::Scheme::Light : Theme::Scheme::Dark;
    if (Theme::scheme() == s) {
        return;
    }
    Theme::setScheme(s);
    // With no screen bound (before the first one, or mid-switch) the next
    // screen is built in the new palette anyway.
    if (mListener) {
        ScreenManager::instance().rebuild();
    }
}

// --- State -----------------------------------------------------------------

const score::Workout& Model::userWorkout(uint8_t i) const
{
    return mUserWorkouts[i < score::kMaxUserWorkouts ? i : score::kMaxUserWorkouts - 1];
}

const score::Workout* Model::workoutById(uint8_t id) const
{
    if (const score::Workout* w = score::findPreset(id)) {
        return w;
    }
    for (uint8_t i = 0; i < mUserCount; ++i) {
        if (id != 0 && mUserWorkouts[i].id == id) {
            return &mUserWorkouts[i];
        }
    }
    return nullptr;
}

const score::SessionSummary& Model::historyItem(uint8_t i) const
{
    static const score::SessionSummary kNone {};
    if (mHistoryLoaded == 0) {
        return kNone;
    }
    return mHistory[i < mHistoryLoaded ? i : mHistoryLoaded - 1];
}

const score::SpotStat& Model::spotLast(uint8_t spot) const
{
    static const score::SpotStat kNone {};
    return spot < score::kSpotCount ? mSpotLast[spot] : kNone;
}

const score::SpotStat& Model::spotBest(uint8_t spot) const
{
    static const score::SpotStat kNone {};
    return spot < score::kSpotCount ? mSpotBest[spot] : kNone;
}

uint32_t Model::todayYmd() const
{
    if (mTime.tm_year + 1900 < 2020) {
        return 0;
    }
    return static_cast<uint32_t>(mTime.tm_year + 1900) * 10000u
         + static_cast<uint32_t>(mTime.tm_mon + 1) * 100u
         + static_cast<uint32_t>(mTime.tm_mday);
}

// --- Controls --------------------------------------------------------------

void Model::handleKeyEvent(uint8_t key)
{
    LOG_DEBUG("key = %c\n", static_cast<char>(key));
    if (SDK::GUI::Button::isButtonCode(key)) {
        resetIdleTimer();
    }
}

void Model::resetIdleTimer()
{
    mIdleTimer = App::Config::kIdleTimeoutTicks;
}

// --- IGuiLifeCycleCallback -------------------------------------------------

void Model::onStart()
{
    LOG_INFO("Started\n");
}

void Model::onFrame()
{
    ++mFrame;
    if (mIsRunning && mIdleTimer > 0) {
        if (--mIdleTimer == 0 && mListener) {
            mListener->onIdleTimeout();
        }
    }
}

void Model::onResume()
{
    mIsRunning = true;
    resetIdleTimer();
    requestSnapshot();   // the session may have moved on while suspended
    if (!mHasSettings) {
        requestSettings();   // the start-up SETTINGS / LOCAL_TIME may be lost (or still coming)
    }
    if (mListener) {
        mListener->onResume();
    }
}

void Model::onSuspend()
{
    mIsRunning = false;
    if (mListener) {
        mListener->onSuspend();
    }
}

void Model::onStop()
{
    LOG_INFO("Force exit from the application\n");
}

// --- ICustomMessageHandler -------------------------------------------------

bool Model::customMessageHandler(SDK::MessageBase* message)
{
    // State is always updated; with no screen bound the notifications go to a
    // listener that ignores them.
    static ModelListener sNullListener;
    ModelListener* l = mListener ? mListener : &sNullListener;

    switch (message->getType()) {
        case CustomMessage::SNAPSHOT: {
            auto* msg    = static_cast<CustomMessage::SnapshotMsg*>(message);
            if (!mHasSnapshot) {
                LOG_INFO("first snapshot (state %u)\n", static_cast<unsigned>(msg->snapshot.state));
            }
            mSnapshot    = msg->snapshot;
            mHasSnapshot = true;
            l->onSnapshot(mSnapshot);
        } break;

        case CustomMessage::STEP_DONE: {
            // Messages are not copyable (MessageBase): copy the payload, the
            // bytes after the base, which payloadIs<> proves padding-free.
            static_assert(CustomMessage::payloadIs<CustomMessage::StepDone>(kStepDonePayload), "StepDone payload");
            memcpy(reinterpret_cast<uint8_t*>(&mStepDone) + sizeof(SDK::MessageBase),
                   reinterpret_cast<const uint8_t*>(message) + sizeof(SDK::MessageBase), kStepDonePayload);
            mHasStepDone = true;
            l->onStepDone(mStepDone);
        } break;

        case CustomMessage::UNDO_RESULT: {
            auto* msg = static_cast<CustomMessage::UndoResult*>(message);
            l->onUndoResult(msg->ok != 0, msg->made != 0);
        } break;

        case CustomMessage::WORKOUT_ITEM: {
            auto* msg = static_cast<CustomMessage::WorkoutItem*>(message);
            if (msg->count == 0) {           // {0, 0, {}}: the list is empty
                mUserCount       = 0;
                mWorkoutsSeen    = 0;
                mWorkoutsLoaded  = true;
                mWorkoutsPending = false;
                mWorkoutsRetried = false;
                l->onWorkouts();
                break;
            }
            if (msg->index == 0) {
                mWorkoutsLoaded = false;     // a fresh list starts
                mWorkoutsSeen   = 0;
            }
            const uint8_t count = msg->count < score::kMaxUserWorkouts ? msg->count : score::kMaxUserWorkouts;
            mUserCount = count;
            if (msg->index < count) {
                mUserWorkouts[msg->index] = msg->workout;
                mWorkoutsSeen = static_cast<uint16_t>(mWorkoutsSeen | (1u << msg->index));
            }
            if (msg->index + 1 == msg->count) {
                // The GUI queue evicts the oldest message when a frame brings
                // more than 10 (10 items + the 1 Hz SNAPSHOT), so an early
                // item can be missing: never call such a list loaded.
                const uint16_t all  = static_cast<uint16_t>((1u << count) - 1u);
                const bool complete = (mWorkoutsSeen & all) == all;
                mWorkoutsSeen = 0;           // the next list starts clean even without its item 0
                if (complete) {
                    mWorkoutsLoaded  = true;
                    mWorkoutsPending = false;
                    mWorkoutsRetried = false;
                    l->onWorkouts();
                } else if (!mWorkoutsRetried) {
                    // Ask once more, past the 1 s guard: the reply lands
                    // a frame later, away from the burst that lost the item.
                    LOG_WARNING("workout list incomplete (%u items), asked again\n", count);
                    mWorkoutsRetried = true;
                    sendGetWorkouts();
                } else {
                    // Lost twice: give up until a screen asks again.
                    LOG_WARNING("workout list incomplete again (%u items), not loaded\n", count);
                    mWorkoutsPending = false;
                }
            }
        } break;

        case CustomMessage::RESULTS: {
            // Field by field: messages are not copyable (MessageBase).
            auto* msg           = static_cast<CustomMessage::Results*>(message);
            mResults.sessionId  = msg->sessionId;
            mResults.ymd        = msg->ymd;
            mResults.workoutId  = msg->workoutId;
            mResults.stats      = msg->stats;
            mHasResults         = msg->sessionId != 0;
            l->onResults();
        } break;

        case CustomMessage::HISTORY: {
            // One page per GET_HISTORY, newest first. Only the page asked for
            // last counts: the current stream's seq at the next offset, so the
            // rows stay contiguous from the newest. A full stream asks for the
            // next page from here: one HISTORY in flight at a time.
            auto* msg = static_cast<CustomMessage::History*>(message);
            if (msg->seq != mHistorySeq || msg->offset != mHistoryLoaded) {
                break;
            }
            if (msg->offset == 0) {
                mHistoryTotal = msg->total < score::kIndexCapacity ? msg->total : score::kIndexCapacity;
            }
            uint8_t count = msg->count < score::kTrendCount ? msg->count : score::kTrendCount;
            if (count > mHistoryTotal - mHistoryLoaded) {
                count = static_cast<uint8_t>(mHistoryTotal - mHistoryLoaded);
            }
            memcpy(&mHistory[mHistoryLoaded], msg->items, sizeof(score::SessionSummary) * count);
            mHistoryLoaded = static_cast<uint8_t>(mHistoryLoaded + count);
            mHasHistory    = true;
            if (mHistoryAll && count > 0 && mHistoryLoaded < mHistoryTotal) {
                SDK::send_msg<CustomMessage::GetHistory>(mKernel, mHistoryLoaded, mHistorySeq);
            }
            l->onHistory();
        } break;

        case CustomMessage::SPOT_STATS: {
            auto* msg = static_cast<CustomMessage::SpotStats*>(message);
            memcpy(mSpotLast, msg->last, sizeof(mSpotLast));
            memcpy(mSpotBest, msg->best, sizeof(mSpotBest));
            mHasSpotStats = true;
            l->onSpotStats();
        } break;

        case CustomMessage::LOCAL_TIME: {
            const CustomMessage::Clock& c = static_cast<CustomMessage::LocalTime*>(message)->clock;
            mTime          = std::tm {};
            mTime.tm_year  = c.year - 1900;
            mTime.tm_mon   = c.mon - 1;
            mTime.tm_mday  = c.mday;
            mTime.tm_hour  = c.hour;
            mTime.tm_min   = c.min;
            mTime.tm_sec   = c.sec;
            mTime.tm_wday  = c.wday;
        } break;

        case CustomMessage::SETTINGS:
            mSettings    = static_cast<CustomMessage::SettingsMsg*>(message)->settings;
            mHasSettings = true;
            applyScheme();
            l->onSettings();
            break;

        case CustomMessage::SAVED:
            l->onSaved(static_cast<CustomMessage::Saved*>(message)->sessionId);
            break;

        case CustomMessage::WORKOUT_SAVED: {
            // A stored workout makes the list held stale: ask for it (the
            // service sends the reply alone), onWorkouts() once it is in.
            const uint8_t id = static_cast<CustomMessage::WorkoutSaved*>(message)->workoutId;
            if (id != 0) {
                requestWorkouts();
            }
            l->onWorkoutSaved(id);
        } break;

        default:
            LOG_WARNING("unknown message 0x%04x\n", static_cast<unsigned>(message->getType()));
            return false;
    }
    return true;
}
