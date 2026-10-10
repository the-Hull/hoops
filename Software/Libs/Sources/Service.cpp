#include "Service.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserWristMotion.hpp"

#include "Commands.hpp"
#include "score/Stats.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using Effect = SDK::Message::RequestVibroPlay::Effect;
using Event  = score::SessionEngine::Event;

namespace {

/// One row of the haptics table (spec §5): the effect per strength, played `count` times.
struct HapticEntry {
    Effect  strong;
    Effect  light;
    uint8_t count;
};

/// Indexed by Service::HapticEvent.
constexpr HapticEntry kHaptics[] = {
    {Effect::SHARP_CLICK_100,                 Effect::SHARP_TICK_1_100,              1},   // Make
    {Effect::SHORT_DOUBLE_CLICK_STRONG_1_100, Effect::SHORT_DOUBLE_SHARP_TICK_1_100, 1},   // Miss
    {Effect::SHARP_TICK_1_100,                Effect::SHARP_TICK_1_100,              3},   // Undo
    {Effect::ALERT_750MS_100,                 Effect::ALERT_750MS_100,               1},   // Alert
    {Effect::STRONG_CLICK_100,                Effect::MEDIUM_CLICK_1_100,            1},   // Saved
    {Effect::DOUBLE_CLICK_100,                Effect::SHORT_DOUBLE_SHARP_TICK_1_100, 1},   // Streak5
    {Effect::STRONG_CLICK_100,                Effect::MEDIUM_CLICK_1_100,            3},   // Streak10
};
static_assert(sizeof(kHaptics) / sizeof(kHaptics[0]) ==
                  static_cast<size_t>(Service::HapticEvent::Streak10) + 1u,
              "one haptics row per HapticEvent");

} // namespace

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
    , mStorage(kernel)
    , mTimeTracker(kernel.sys)
{}

void Service::run()
{
    mTimeTracker.init();
    mProcessedUtc = mTimeTracker.getExpectedUTC();
    LOG_INFO("thread started\n");

    if (mStorage.loadWorkouts(mWorkouts) && !mWorkouts.valid()) {
        LOG_WARNING("workouts.bin out of range, starting empty\n");
        mWorkouts = score::WorkoutStore{};
    }
    if (mStorage.loadIndex(mIndex) && !mIndex.valid()) {
        LOG_WARNING("sessions/index.bin out of range, starting empty\n");
        mIndex = score::SessionIndex{};
    }
    if (!mStorage.loadSettings(mSettings) || !mSettings.valid()) mSettings = score::Settings::defaults();
    LOG_INFO("loaded %u workouts, %u sessions\n", mWorkouts.count, mIndex.count);
    restoreCurrent();

    while (true) {
        SDK::MessageBase* msg;
        if (mKernel.comm.getMessage(msg, 100)) {
            switch (msg->getType()) {
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    autosave();   // the running session as of now, not of the last 5 s tick
                    mSensorHr.disconnect();
                    mSensorWrist.disconnect();
                    // We must release message because this is the last event.
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    LOG_INFO("GUI is now running\n");
                    mGuiStarted = true;
                    if (!mSensorWrist.connect()) LOG_WARNING("wrist motion connect failed\n");
                    sendSettings();
                    sendLocalTime();
                    pushSnapshot();
                    LOG_INFO("snapshot pushed (state %u)\n", static_cast<unsigned>(mEngine.state()));
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    LOG_INFO("GUI has stopped\n");
                    mGuiStarted = false;
                    mSensorWrist.disconnect();
                    break;

                case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
                    auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
                    SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
                    handleSensorsData(event->handle, batch);
                } break;

                default:
                    handleMessage(msg);
                    break;
            }
            mKernel.comm.releaseMessage(msg);
        }

        // One onSecond() per elapsed wall-clock second; capped so a long stall can't spin.
        const std::time_t utc = mTimeTracker.getExpectedUTC();
        uint8_t catchUp = 0;
        while (mProcessedUtc != 0 && utc > mProcessedUtc && catchUp < 5) {
            onSecond();
            ++mProcessedUtc;
            ++catchUp;
        }
        mProcessedUtc = utc;

        // Once a minute while the GUI runs: a new local date is sent as LOCAL_TIME.
        if (mGuiStarted && utc / 60 != mLastMinute) {
            mLastMinute = utc / 60;
            if (todayYmd() != mLastYmd) sendLocalTime();
        }
    }
}

void Service::handleMessage(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::START_OPEN: {
            const auto* m = static_cast<CustomMessage::StartOpen*>(msg);
            if (mEngine.startOpen(static_cast<score::Spot>(m->spot), nowUnix())) {
                autosave();
                connectHr();
            } else if (mEngine.isActive()) {
                // The GUI should not ask while a session runs; the snapshot tells it so.
                LOG_INFO("START_OPEN refused: a session is running\n");
            } else {
                LOG_WARNING("START_OPEN refused (spot %u)\n", m->spot);
            }
            pushSnapshot();
        } break;

        case CustomMessage::START_WORKOUT: {
            const uint8_t id = static_cast<CustomMessage::StartWorkout*>(msg)->workoutId;
            const score::Workout* w = score::findPreset(id);
            if (!w) w = mWorkouts.find(id);
            if (w && mEngine.startWorkout(*w, nowUnix())) {
                autosave();
                connectHr();
            } else if (mEngine.isActive()) {
                LOG_INFO("START_WORKOUT refused: a session is running\n");
            } else {
                LOG_WARNING("START_WORKOUT refused (id %u)\n", id);
            }
            pushSnapshot();
        } break;

        case CustomMessage::SHOT: {
            const bool made = static_cast<CustomMessage::Shot*>(msg)->made != 0;
            const uint16_t before = mEngine.session().count;
            const Event ev = mEngine.shot(made);
            bool alerted = false;   // one haptic per shot: Alert replaces Make/Miss/Streak
            if (mEngine.session().count != before) {
                // Open session target: once, on the shot that reaches it (spec §2.3).
                const bool targetHit = mEngine.workout() == nullptr && mSettings.openTarget > 0 &&
                                       mEngine.session().count == mSettings.openTarget;
                if (targetHit || ev == Event::StepDone || ev == Event::WorkoutDone) {
                    haptic(HapticEvent::Alert);
                    backlightOn();
                    alerted = true;
                } else {
                    if (made) {
                        const uint8_t streak = mEngine.currentStreak();
                        haptic(streak == 5                          ? HapticEvent::Streak5
                               : (streak >= 10 && streak % 10 == 0) ? HapticEvent::Streak10
                                                                    : HapticEvent::Make);
                    } else {
                        haptic(HapticEvent::Miss);
                    }
                    if (mSettings.backlightOnShot) backlightOn();
                }
                autosave();
            }
            pushSnapshot();
            onEvent(ev, !alerted);
        } break;

        case CustomMessage::UNDO: {
            bool made = false;
            const bool ok = mEngine.undo(made);
            if (ok) {
                haptic(HapticEvent::Undo);
                if (mSettings.backlightOnShot) backlightOn();
                autosave();
            }
            sendUndoResult(ok, made);
            pushSnapshot();
        } break;

        case CustomMessage::PAUSE:
            if (mEngine.pause()) {
                autosave();                // ticks don't save while paused
                mSensorHr.disconnect();    // a paused session counts no heart rate
            }
            pushSnapshot();
            break;

        case CustomMessage::RESUME:
            if (mEngine.resume()) connectHr();
            pushSnapshot();
            break;

        case CustomMessage::SKIP_REST:
            if (mEngine.skipRest()) {
                haptic(HapticEvent::Alert);
                backlightOn();
            }
            pushSnapshot();
            break;

        case CustomMessage::EXTEND_REST:
            mEngine.extendRest(static_cast<CustomMessage::ExtendRest*>(msg)->seconds);
            pushSnapshot();
            break;

        case CustomMessage::CHANGE_SPOT:
            if (mEngine.changeSpot(static_cast<score::Spot>(static_cast<CustomMessage::ChangeSpot*>(msg)->spot)))
                autosave();
            pushSnapshot();
            break;

        case CustomMessage::END_SESSION:
            finishSession(static_cast<CustomMessage::EndSession*>(msg)->save != 0);
            break;

        case CustomMessage::GET_SNAPSHOT:
            pushSnapshot();
            break;

        case CustomMessage::GET_WORKOUTS:
            sendWorkouts();
            break;

        case CustomMessage::SAVE_WORKOUT: {
            score::Workout w = static_cast<CustomMessage::SaveWorkout*>(msg)->workout;
            w.name[score::kNameLen - 1] = '\0';   // NUL-terminated whatever the GUI sent
            uint8_t id = w.id;
            if (!w.validSteps()) {
                // Stored as sent and later run by the engine: out-of-range steps
                // would index past the spot table or never complete.
                LOG_WARNING("SAVE_WORKOUT refused: steps out of range (id %u, %u steps)\n", w.id, w.stepCount);
                SDK::send_msg<CustomMessage::WorkoutSaved>(mKernel, static_cast<uint8_t>(0));
                break;
            }
            const bool ok = (w.id == 0) ? mWorkouts.add(w, id) : mWorkouts.replace(w);
            if (!ok) {
                LOG_WARNING("SAVE_WORKOUT refused (id %u)\n", w.id);
                id = 0;
            } else {
                mStorage.saveWorkouts(mWorkouts);
            }
            // The reply only: WORKOUT_SAVED plus a 10-item list would overflow the
            // GUI's 10-message queue, so the GUI asks for the list (GET_WORKOUTS).
            SDK::send_msg<CustomMessage::WorkoutSaved>(mKernel, id);
        } break;

        case CustomMessage::DELETE_WORKOUT: {
            const uint8_t id = static_cast<CustomMessage::DeleteWorkout*>(msg)->workoutId;
            if (mEngine.isActive() && mEngine.session().workoutId == id)
                LOG_WARNING("DELETE_WORKOUT refused: workout %u is running\n", id);
            else if (!mWorkouts.remove(id))
                LOG_WARNING("DELETE_WORKOUT: no workout %u\n", id);
            else
                mStorage.saveWorkouts(mWorkouts);
            sendWorkouts();
        } break;

        case CustomMessage::GET_RESULTS:
            sendResults(static_cast<CustomMessage::GetResults*>(msg)->sessionId);
            break;

        case CustomMessage::GET_HISTORY: {
            const auto* m = static_cast<CustomMessage::GetHistory*>(msg);
            sendHistory(m->offset, m->seq);
        } break;

        case CustomMessage::GET_SETTINGS:
            // The GUI lost (or never got) the start-up SETTINGS / LOCAL_TIME.
            LOG_INFO("GET_SETTINGS: settings and local time sent again\n");
            sendSettings();
            sendLocalTime();
            break;

        case CustomMessage::GET_SPOT_STATS:
            sendSpotStats();
            break;

        case CustomMessage::CLEAR_HISTORY:
            clearHistory();
            break;

        case CustomMessage::SET_SETTINGS:
            applySettings(static_cast<CustomMessage::SetSettings*>(msg)->settings);
            break;

        default:
            LOG_DEBUG("unhandled message 0x%lx\n", static_cast<unsigned long>(msg->getType()));
            break;
    }
}

void Service::onSecond()
{
    if (!mEngine.isActive()) return;
    const Event ev = mEngine.tick();
    // Running time only: a paused session was saved on PAUSE and doesn't change.
    if (ev != Event::None ||
        (mEngine.state() != score::SessionState::Paused && ++mSaveTicks >= kAutosaveTicks))
        autosave();
    pushSnapshot();
    onEvent(ev);
}

void Service::pushSnapshot()
{
    if (!mGuiStarted) return;
    SDK::send_msg<CustomMessage::SnapshotMsg>(mKernel, mEngine.snapshot());
}

void Service::onEvent(Event ev, bool alert)
{
    if (ev == Event::None) return;
    if (alert) {
        haptic(HapticEvent::Alert);
        backlightOn();
    }
    if (ev == Event::RestOver || !mGuiStarted) return;

    // StepDone / WorkoutDone: the finished step's result plus the step that follows.
    auto m = SDK::make_msg<CustomMessage::StepDone>(mKernel);
    if (!m) return;
    const score::SessionEngine::StepResult& r = mEngine.lastStep();
    m->stepIndex   = r.stepIndex;
    m->makes       = r.makes;
    m->attempts    = r.attempts;
    m->rest_s      = r.rest_s;
    m->workoutDone = (ev == Event::WorkoutDone) ? 1 : 0;
    const score::Workout* w = mEngine.workout();
    if (!m->workoutDone && w && r.stepIndex + 1 < w->stepCount) {
        const score::Step& next = w->steps[r.stepIndex + 1];
        m->nextSpot      = next.spot;
        m->nextGoalType  = next.goal.type;
        m->nextGoalValue = next.goal.value;
    }
    m.send();
}

void Service::sendUndoResult(bool ok, bool made)
{
    SDK::send_msg<CustomMessage::UndoResult>(mKernel, static_cast<uint8_t>(ok), static_cast<uint8_t>(made));
}

void Service::sendWorkouts()
{
    if (mWorkouts.count == 0) {
        SDK::send_msg<CustomMessage::WorkoutItem>(mKernel);   // {0, 0, {}}: empty list
        return;
    }
    // Stateless: every GET_WORKOUTS resends the whole list from index 0.
    for (uint8_t i = 0; i < mWorkouts.count; ++i) {
        auto m = SDK::make_msg<CustomMessage::WorkoutItem>(mKernel);
        if (!m) {
            LOG_WARNING("sendWorkouts: no message block, list cut at %u/%u\n", i, mWorkouts.count);
            return;
        }
        m->index   = i;
        m->count   = mWorkouts.count;
        m->workout = mWorkouts.items[i];
        m.send();
    }
}

void Service::sendResults(uint32_t sessionId)
{
    const score::Session& live = mEngine.session();
    const bool liveEnded = mEngine.state() == score::SessionState::Ended && mLastEndedId != 0 &&
                           live.id == mLastEndedId;

    const score::Session* s = nullptr;
    if (liveEnded && (sessionId == 0 || sessionId == live.id)) {
        s = &live;
    } else if (sessionId != 0 && (mLoaded.id == sessionId || mStorage.loadSession(sessionId, mLoaded))) {
        s = &mLoaded;
    }
    auto m = SDK::make_msg<CustomMessage::Results>(mKernel);
    if (!m) return;
    if (!s) {   // always answer: sessionId 0 with zeroed stats means "none"
        LOG_WARNING("GET_RESULTS: session %lu not available\n", static_cast<unsigned long>(sessionId));
        m.send();
        return;
    }
    const score::SessionSummary* row = mIndex.find(s->id);
    m->sessionId = s->id;
    m->ymd       = row ? row->ymd : todayYmd();
    m->workoutId = s->workoutId;
    m->stats     = score::SessionStats::compute(*s);
    m.send();
}

void Service::sendHistory(uint8_t offset, uint8_t seq)
{
    // One page per GET_HISTORY (the GUI pulls the next): rows offset.. newest
    // first, count 0 past the end ({0, 0, 0} for an empty index).
    auto m = SDK::make_msg<CustomMessage::History>(mKernel);
    if (!m) {
        LOG_WARNING("sendHistory: no message block (offset %u, seq %u)\n", offset, seq);
        return;
    }
    m->total  = mIndex.count;
    m->offset = offset;
    m->seq    = seq;
    for (uint8_t i = 0; i < score::kTrendCount; ++i) {
        const score::SessionSummary* row = mIndex.newest(static_cast<uint8_t>(offset + i));
        if (!row) break;
        m->items[m->count++] = *row;
    }
    m.send();
}

void Service::sendSpotStats()
{
    auto m = SDK::make_msg<CustomMessage::SpotStats>(mKernel);
    if (!m) {
        LOG_WARNING("sendSpotStats: no message block\n");
        return;
    }
    std::memcpy(m->last, mIndex.last, sizeof(m->last));
    std::memcpy(m->best, mIndex.best, sizeof(m->best));
    m.send();
}

void Service::clearHistory()
{
    // The emptied index goes to flash before any file is removed: a crash or a
    // failed write leaves at worst orphan files, never rows whose file is gone.
    uint32_t ids[score::kIndexCapacity];
    const uint8_t n = mIndex.count;
    for (uint8_t i = 0; i < n; ++i) ids[i] = mIndex.items[i].id;
    mIndex.clear();
    mLoaded      = score::Session{};
    mLastEndedId = 0;
    if (mStorage.saveIndex(mIndex)) {
        for (uint8_t i = 0; i < n; ++i) mStorage.removeSession(ids[i]);   // .bin and .csv
        LOG_INFO("history cleared: %u sessions removed\n", n);
    } else {
        LOG_WARNING("history cleared in memory only: index not written, %u session files kept\n", n);
    }
    sendSpotStats();   // the GUI asks for the (empty) history page itself
}

void Service::sendLocalTime()
{
    const std::tm t = mTimeTracker.getLocalTime(std::time(nullptr));
    CustomMessage::Clock c{};
    c.year = static_cast<uint16_t>(t.tm_year + 1900);
    c.mon  = static_cast<uint8_t>(t.tm_mon + 1);
    c.mday = static_cast<uint8_t>(t.tm_mday);
    c.hour = static_cast<uint8_t>(t.tm_hour);
    c.min  = static_cast<uint8_t>(t.tm_min);
    c.sec  = static_cast<uint8_t>(t.tm_sec);
    c.wday = static_cast<uint8_t>(t.tm_wday);
    SDK::send_msg<CustomMessage::LocalTime>(mKernel, c);
    mLastYmd = static_cast<uint32_t>(c.year * 10000u + c.mon * 100u + c.mday);
}

void Service::sendSettings()
{
    SDK::send_msg<CustomMessage::SettingsMsg>(mKernel, mSettings);
}

void Service::applySettings(const score::Settings& s)
{
    if (!s.valid()) {
        LOG_WARNING("SET_SETTINGS refused: out of range\n");
        sendSettings();   // the GUI resyncs to what is in force
        return;
    }
    mSettings = s;
    if (!mStorage.saveSettings(mSettings)) LOG_WARNING("settings.bin not written\n");

    if (!mSettings.heartRate)
        mSensorHr.disconnect();
    else if (mEngine.isActive() && mEngine.state() != score::SessionState::Paused)
        connectHr();
    sendSettings();
}

void Service::finishSession(bool save)
{
    if (!mEngine.isActive()) {
        LOG_WARNING("END_SESSION with no active session\n");
        pushSnapshot();
        return;
    }
    const score::Session& s = mEngine.end();
    mSensorHr.disconnect();
    mSaveTicks = 0;
    if (save && s.count == 0) {
        // Nothing to keep: dropped like a discard; SAVED{0} sends the GUI Home.
        LOG_INFO("session %lu ended empty, not saved\n", static_cast<unsigned long>(s.id));
        mStorage.clearCurrent();
        SDK::send_msg<CustomMessage::Saved>(mKernel, static_cast<uint32_t>(0));
        pushSnapshot();
        return;
    }
    if (!save) {
        mStorage.clearCurrent();
        LOG_INFO("session %lu discarded\n", static_cast<unsigned long>(s.id));
        pushSnapshot();
        return;
    }

    // Order matters for a crash at any point: session file -> index row ->
    // index file -> evicted file -> current.bin. current.bin goes last, so
    // until the session is safely stored a restart still resumes it (and
    // restoreCurrent() drops it once its id is in the saved index).
    if (!mStorage.saveSession(s)) {
        // No row (History must not list what Results can't load), no "saved"
        // buzz; current.bin stays, so the session comes back on the next start
        // (unless a new session overwrites it first).
        LOG_WARNING("session %lu: file not written, not indexed\n", static_cast<unsigned long>(s.id));
        SDK::send_msg<CustomMessage::Saved>(mKernel, static_cast<uint32_t>(0));   // 0 = save failed
        pushSnapshot();
        return;
    }
    const uint32_t ymd = todayYmd();
    const score::SessionStats st = score::SessionStats::compute(s);   // 88 B
    const uint32_t evicted = mIndex.push(score::summarize(s, ymd));
    mIndex.recordSpots(st.spot);
    // The CSV before the index file: a crash in between leaves at worst an orphan CSV.
    if (mSettings.csvExport) {
        char name[score::kNameLen + 1];
        workoutNameOf(s.workoutId, name);
        if (!mStorage.saveSessionCsv(s, ymd, name))
            LOG_WARNING("session %lu: CSV not written\n", static_cast<unsigned long>(s.id));
    }
    if (evicted != 0 && mLoaded.id == evicted) mLoaded = score::Session{};
    if (mStorage.saveIndex(mIndex)) {
        // The evicted row's files only once no index on flash lists it (as in
        // clearHistory()): a crash before this leaves an orphan, never a dead row.
        if (evicted != 0) mStorage.removeSession(evicted);
        mStorage.clearCurrent();
    } else {
        LOG_WARNING("index not written: current.bin kept for session %lu\n", static_cast<unsigned long>(s.id));
    }
    mLastEndedId = s.id;
    haptic(HapticEvent::Saved);
    backlightOn();
    SDK::send_msg<CustomMessage::Saved>(mKernel, s.id);
    LOG_INFO("session %lu ended: %u shots\n", static_cast<unsigned long>(s.id), s.count);
    pushSnapshot();
}

void Service::haptic(HapticEvent ev)
{
    const auto mode = static_cast<score::Haptics>(mSettings.haptics);
    if (mode == score::Haptics::Off) return;
    const HapticEntry& e = kHaptics[static_cast<size_t>(ev)];
    vibro(mode == score::Haptics::Light ? e.light : e.strong, e.count);
}

void Service::vibro(Effect effect, uint8_t count, uint16_t silenceMs)
{
    if (count == 0) {
        return;
    }

    // A series of N effects needs 2*N-1 notes (effects + silences between them).
    // Cap to what fits in skMaxNotes: max count = (skMaxNotes + 1) / 2 = 4.
    const uint8_t maxCount = (SDK::Message::RequestVibroPlay::skMaxNotes + 1u) / 2u;
    if (count > maxCount) {
        count = maxCount;
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestVibroPlay>();
    if (msg) {
        uint8_t n = 0;
        for (uint8_t i = 0; i < count; ++i) {
            msg->notes[n].effect = static_cast<uint8_t>(effect);
            msg->notes[n].pause  = 0;
            ++n;
            if (i < count - 1u) {
                msg->notes[n].effect = static_cast<uint8_t>(Effect::NO_EFFECT);
                msg->notes[n].pause  = silenceMs;
                ++n;
            }
        }
        msg->notesCount = n;
        mKernel.comm.sendMessage(msg);
        mKernel.comm.releaseMessage(msg);
    }
}

void Service::backlightOn(uint32_t ms)
{
    auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel);
    if (bl) {
        bl->brightness       = 100;
        bl->autoOffTimeoutMs = ms;
        bl.send();
    }
}

void Service::restoreCurrent()
{
    // mLoaded doubles as scratch: a 1.6 KB Session is too big for the 10 KB service stack.
    if (!mStorage.loadCurrent(mLoaded)) {
        mStorage.clearCurrent();   // drops a corrupt file; no-op when absent
        return;
    }
    if (mIndex.find(mLoaded.id)) {
        // Saved already: the crash came after the index was written but
        // before current.bin was removed (finishSession()).
        LOG_INFO("current.bin: session %lu is in the index already, deleting\n",
                 static_cast<unsigned long>(mLoaded.id));
        mStorage.clearCurrent();
        mLoaded = score::Session{};
        return;
    }
    const uint16_t wid = mLoaded.workoutId;
    const score::Workout* w = nullptr;
    if (wid != 0 && wid <= UINT8_MAX) {
        w = score::findPreset(static_cast<uint8_t>(wid));
        if (!w) w = mWorkouts.find(static_cast<uint8_t>(wid));
    }
    if (mEngine.restore(mLoaded, w)) {
        LOG_INFO("session %lu restored (%u shots, workout %u%s)\n", static_cast<unsigned long>(mLoaded.id),
                 mLoaded.count, wid, (wid != 0 && !w) ? " gone, continuing open" : "");
        autosave();   // persist the Paused state (and any open-session fallback)
        // No heart rate until it is resumed (RESUME connects it).
    } else {
        LOG_WARNING("current.bin rejected by restore, deleting\n");
        mStorage.clearCurrent();
    }
    mLoaded = score::Session{};
}

void Service::workoutNameOf(uint16_t id, char (&out)[score::kNameLen + 1]) const
{
    const score::Workout* w = nullptr;
    if (id != 0 && id <= UINT8_MAX) {
        w = score::findPreset(static_cast<uint8_t>(id));
        if (!w) w = mWorkouts.find(static_cast<uint8_t>(id));
    }
    if (w) {
        std::memcpy(out, w->name, score::kNameLen);   // name[] need not be NUL-terminated
        out[score::kNameLen] = '\0';
    } else {
        std::snprintf(out, sizeof(out), "%s", id == 0 ? "Open" : "Custom");
    }
}

void Service::autosave()
{
    mSaveTicks = 0;
    if (mEngine.isActive()) mStorage.saveCurrent(mEngine.session());
}

void Service::connectHr()
{
    if (!mSettings.heartRate) return;
    mHrSeen = false;
    if (!mSensorHr.isConnected() && !mSensorHr.connect()) LOG_WARNING("heart rate connect failed\n");
}

void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (data.size() == 0) return;
    if (mSensorHr.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRateEx parser(data[0]);
        // Active and Rest only: a paused session counts no heart rate.
        if (!parser.isDataValid() || parser.getTrustLevel() <= 0.f || !mEngine.isActive() ||
            mEngine.state() == score::SessionState::Paused)
            return;
        const float bpm = parser.getBpm();
        if (bpm < 30.f || bpm > 220.f) return;
        mEngine.addHr(static_cast<uint8_t>(bpm));
        if (!mHrSeen) {
            mHrSeen = true;
            LOG_INFO("first HR sample: %u bpm (trust %.1f)\n", static_cast<unsigned>(bpm),
                     static_cast<double>(parser.getTrustLevel()));
        }
    } else if (mSensorWrist.matchesDriver(handle)) {
        SDK::SensorDataParser::WristMotion parser(data[0]);
        if (parser.isDataValid() && mEngine.isActive()) backlightOn();
    }
}

uint32_t Service::nowUnix() const
{
    return static_cast<uint32_t>(std::time(nullptr));
}

uint32_t Service::todayYmd()
{
    const std::tm t = mTimeTracker.getLocalTime(std::time(nullptr));
    return static_cast<uint32_t>((t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday);
}
