#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__

#include <cstdint>
#include <ctime>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include "Storage.hpp"
#include "score/History.hpp"
#include "score/Session.hpp"
#include "score/SessionEngine.hpp"
#include "score/Settings.hpp"
#include "score/Workout.hpp"

/// Background process: owns the session engine, answers the GUI, drives haptics.
class Service
{
public:
    /// What a vibration means; mapped to an effect per `Settings::haptics` (spec §5).
    enum class HapticEvent : uint8_t { Make, Miss, Undo, Alert, Saved, Streak5, Streak10 };

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
    void sendSettings();
    void applySettings(const score::Settings& s);
    void finishSession(bool save);
    void restoreCurrent();
    void autosave();
    void connectHr();
    void handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data);
    void haptic(HapticEvent ev);
    void vibro(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1, uint16_t silenceMs = 120);
    void backlightOn(uint32_t ms = 5000);
    uint32_t nowUnix() const;
    uint32_t todayYmd();

    static constexpr uint8_t kAutosaveTicks = 5;   ///< save current.bin every N running seconds

    SDK::Kernel&         mKernel;
    Storage              mStorage;
    bool                 mGuiStarted = false;
    uint8_t              mSaveTicks = 0;           ///< running seconds since the last autosave
    bool                 mHrSeen = false;          ///< first HR sample of the session logged
    SDK::Sensor::Connection mSensorHr{SDK::Sensor::Type::HEART_RATE_EX, 1000, 1000};
    SDK::Sensor::Connection mSensorWrist{SDK::Sensor::Type::WRIST_MOTION};
    score::SessionEngine mEngine;
    score::WorkoutStore  mWorkouts{};
    score::SessionIndex  mIndex{};
    score::Settings      mSettings = score::Settings::defaults();
    score::Session       mLoaded{};        ///< a past session loaded for GET_RESULTS (also boot scratch)
    uint32_t             mLastEndedId = 0; ///< id of the last session ended with save
    std::time_t          mProcessedUtc = 0;  ///< last second handed to onSecond()
    uint32_t             mLastYmd = 0;       ///< date of the last LOCAL_TIME sent
    std::time_t          mLastMinute = 0;    ///< utc / 60 of the last date check
    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
};

#endif
