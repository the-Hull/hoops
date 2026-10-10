#include "Service.hpp"

#include <ctime>

#include "SDK/Messages/MessageGuard.hpp"

#include "Commands.hpp"
#include "score/Stats.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using Effect = SDK::Message::RequestVibroPlay::Effect;
using Event  = score::SessionEngine::Event;

Service::Service(SDK::Kernel& kernel)
    : mKernel(kernel)
    , mTimeTracker(kernel.sys)
{}

void Service::run()
{
    mTimeTracker.init();
    mProcessedUtc = mTimeTracker.getExpectedUTC();
    LOG_INFO("thread started\n");

    while (true) {
        SDK::MessageBase* msg;
        if (mKernel.comm.getMessage(msg, 100)) {
            switch (msg->getType()) {
                case SDK::MessageType::COMMAND_APP_STOP:
                    LOG_INFO("Force exit from the application\n");
                    // We must release message because this is the last event.
                    mKernel.comm.releaseMessage(msg);
                    return;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                    LOG_INFO("GUI is now running\n");
                    mGuiStarted = true;
                    sendLocalTime();
                    pushSnapshot();
                    LOG_INFO("snapshot pushed (state %u)\n", static_cast<unsigned>(mEngine.state()));
                    break;

                case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                    LOG_INFO("GUI has stopped\n");
                    mGuiStarted = false;
                    break;

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
    }
}

void Service::handleMessage(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::START_OPEN: {
            const auto* m = static_cast<CustomMessage::StartOpen*>(msg);
            if (!mEngine.startOpen(static_cast<score::Spot>(m->spot), nowUnix()))
                LOG_WARNING("START_OPEN refused (spot %u)\n", m->spot);
            pushSnapshot();
        } break;

        case CustomMessage::START_WORKOUT: {
            const uint8_t id = static_cast<CustomMessage::StartWorkout*>(msg)->workoutId;
            const score::Workout* w = score::findPreset(id);
            if (!w) w = mWorkouts.find(id);
            if (!w || !mEngine.startWorkout(*w, nowUnix()))
                LOG_WARNING("START_WORKOUT refused (id %u)\n", id);
            pushSnapshot();
        } break;

        case CustomMessage::SHOT: {
            const bool made = static_cast<CustomMessage::Shot*>(msg)->made != 0;
            const uint16_t before = mEngine.session().count;
            const Event ev = mEngine.shot(made);
            if (mEngine.session().count != before) {
                vibro(made ? Effect::SHARP_CLICK_100 : Effect::SHORT_DOUBLE_CLICK_STRONG_1_100);
                backlightOn();
            }
            pushSnapshot();
            onEvent(ev);
        } break;

        case CustomMessage::UNDO: {
            bool made = false;
            const bool ok = mEngine.undo(made);
            if (ok) {
                vibro(Effect::SHARP_TICK_1_100, 3);
                backlightOn();
            }
            sendUndoResult(ok, made);
            pushSnapshot();
        } break;

        case CustomMessage::PAUSE:
            mEngine.pause();
            pushSnapshot();
            break;

        case CustomMessage::RESUME:
            mEngine.resume();
            pushSnapshot();
            break;

        case CustomMessage::SKIP_REST:
            if (mEngine.skipRest()) {
                vibro(Effect::ALERT_750MS_100);
                backlightOn();
            }
            pushSnapshot();
            break;

        case CustomMessage::EXTEND_REST:
            mEngine.extendRest(static_cast<CustomMessage::ExtendRest*>(msg)->seconds);
            pushSnapshot();
            break;

        case CustomMessage::CHANGE_SPOT:
            mEngine.changeSpot(static_cast<score::Spot>(static_cast<CustomMessage::ChangeSpot*>(msg)->spot));
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
            const score::Workout& w = static_cast<CustomMessage::SaveWorkout*>(msg)->workout;
            uint8_t id = w.id;
            const bool ok = (w.id == 0) ? mWorkouts.add(w, id) : mWorkouts.replace(w);
            if (!ok) {
                LOG_WARNING("SAVE_WORKOUT refused (id %u)\n", w.id);
                id = 0;
            }
            // Task 7: persist the store here.
            SDK::send_msg<CustomMessage::WorkoutSaved>(mKernel, id);
            sendWorkouts();
        } break;

        case CustomMessage::DELETE_WORKOUT: {
            const uint8_t id = static_cast<CustomMessage::DeleteWorkout*>(msg)->workoutId;
            if (!mWorkouts.remove(id)) LOG_WARNING("DELETE_WORKOUT: no workout %u\n", id);
            // Task 7: persist the store here.
            sendWorkouts();
        } break;

        case CustomMessage::GET_RESULTS:
            sendResults(static_cast<CustomMessage::GetResults*>(msg)->sessionId);
            break;

        case CustomMessage::GET_HISTORY:
            sendHistory();
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
    pushSnapshot();
    onEvent(ev);
}

void Service::pushSnapshot()
{
    if (!mGuiStarted) return;
    SDK::send_msg<CustomMessage::SnapshotMsg>(mKernel, mEngine.snapshot());
}

void Service::onEvent(Event ev)
{
    if (ev == Event::None) return;
    vibro(Effect::ALERT_750MS_100);
    backlightOn();
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
    } else if (sessionId != 0 && mLoaded.id == sessionId) {   // Task 7 loads mLoaded from storage
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

void Service::sendHistory()
{
    auto m = SDK::make_msg<CustomMessage::History>(mKernel);
    if (!m) return;
    for (uint8_t i = 0; i < score::kTrendCount; ++i) {
        const score::SessionSummary* row = mIndex.newest(i);
        if (!row) break;
        m->items[m->count++] = *row;
    }
    m.send();
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
}

void Service::finishSession(bool save)
{
    if (!mEngine.isActive()) {
        LOG_WARNING("END_SESSION with no active session\n");
        pushSnapshot();
        return;
    }
    const score::Session& s = mEngine.end();
    if (save) {
        mLastEndedId = s.id;
        // Task 7: write the session file and push its summary into mIndex here.
        vibro(Effect::STRONG_CLICK_100);
        backlightOn();
        SDK::send_msg<CustomMessage::Saved>(mKernel, s.id);
        LOG_INFO("session %lu ended: %u shots\n", static_cast<unsigned long>(s.id), s.count);
    } else {
        LOG_INFO("session %lu discarded\n", static_cast<unsigned long>(s.id));
    }
    pushSnapshot();
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

uint32_t Service::nowUnix() const
{
    return static_cast<uint32_t>(std::time(nullptr));
}

uint32_t Service::todayYmd()
{
    const std::tm t = mTimeTracker.getLocalTime(std::time(nullptr));
    return static_cast<uint32_t>((t.tm_year + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday);
}
