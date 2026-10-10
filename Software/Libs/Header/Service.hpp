#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include <cstdint>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/Timer/Timer.hpp"

#include "score/History.hpp"
#include "score/Session.hpp"
#include "score/SessionEngine.hpp"
#include "score/Workout.hpp"

/// Background process: owns the session engine, answers the GUI, drives haptics.
class Service
{
public:
    explicit Service(SDK::Kernel& kernel);

    void run();

private:
    void handleMessage(SDK::MessageBase* msg);
    void onSecond();
    void pushSnapshot();
    void onEvent(score::SessionEngine::Event ev);
    void sendUndoResult(bool ok, bool made);
    void sendWorkouts();
    void sendResults(uint32_t sessionId);
    void sendHistory();
    void sendLocalTime();
    void finishSession(bool save);
    void vibro(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1, uint16_t silenceMs = 120);
    void backlightOn(uint32_t ms = 5000);
    uint32_t nowUnix() const;
    uint32_t todayYmd();

    SDK::Kernel&         mKernel;
    bool                 mGuiStarted = false;
    score::SessionEngine mEngine;
    score::WorkoutStore  mWorkouts{};
    score::SessionIndex  mIndex{};
    score::Session       mLoaded{};        ///< a past session loaded for GET_RESULTS (storage: Task 7)
    uint32_t             mLastEndedId = 0; ///< id of the last session ended with save
    SDK::Timer           mSecond;
    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
};

#endif
