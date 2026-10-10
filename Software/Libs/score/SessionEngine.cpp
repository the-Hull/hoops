#include "score/SessionEngine.hpp"

namespace score {

namespace {
constexpr uint8_t st(SessionState s) { return static_cast<uint8_t>(s); }
} // namespace

bool SessionEngine::startOpen(Spot spot, uint32_t nowUnix)
{
    if (isActive() || spot >= Spot::Count) return false;
    mSession       = Session{};
    mSession.id    = nowUnix;
    mSession.spot  = static_cast<uint8_t>(spot);
    mSession.state = st(SessionState::Active);
    mHasWorkout    = false;
    mResumable     = false;
    mLastStep      = StepResult{};
    mLastHr        = 0;
    return true;
}

// Workout paths: Task 4.
bool SessionEngine::startWorkout(const Workout&, uint32_t) { return false; }
bool SessionEngine::restore(const Session&, const Workout*) { return false; }
bool SessionEngine::skipRest() { return false; }
bool SessionEngine::extendRest(uint16_t) { return false; }
SessionEngine::Event SessionEngine::completeStep() { return Event::None; }
void SessionEngine::advanceStep() {}

SessionEngine::Event SessionEngine::shot(bool made)
{
    if (state() != SessionState::Active || mSession.count >= kMaxShots) return Event::None;
    mSession.shots[mSession.count++] = { mSession.spot, static_cast<uint8_t>(made ? 1 : 0),
                                         static_cast<uint16_t>(mSession.elapsed_s) };
    return Event::None;
}

bool SessionEngine::undo(bool& madeOut)
{
    if (state() != SessionState::Active || mSession.count <= mSession.stepStartCount) return false;
    madeOut = mSession.shots[--mSession.count].made != 0;
    return true;
}

bool SessionEngine::changeSpot(Spot spot)
{
    if (mHasWorkout || state() != SessionState::Active || spot >= Spot::Count) return false;
    mSession.spot = static_cast<uint8_t>(spot);
    return true;
}

bool SessionEngine::pause()
{
    const SessionState s = state();
    if (s != SessionState::Active && s != SessionState::Rest) return false;
    mSession.pausedFrom = st(s);
    mSession.state      = st(SessionState::Paused);
    return true;
}

bool SessionEngine::resume()
{
    if (state() != SessionState::Paused) return false;
    mSession.state = mSession.pausedFrom;
    return true;
}

SessionEngine::Event SessionEngine::tick()
{
    switch (state()) {
    case SessionState::Active:
        ++mSession.elapsed_s;
        ++mSession.stepElapsed_s;
        break;
    case SessionState::Rest:
        ++mSession.elapsed_s;
        if (mSession.restRemaining_s > 0) --mSession.restRemaining_s;
        break;
    default:
        break;
    }
    return Event::None;
}

void SessionEngine::addHr(uint8_t bpm)
{
    if (bpm == 0 || !isActive()) return;
    mSession.hrSum += bpm;
    ++mSession.hrN;
    if (bpm > mSession.hrMax) mSession.hrMax = bpm;
    mLastHr = bpm;
}

const Session& SessionEngine::end()
{
    if (isActive()) mSession.state = st(SessionState::Ended);
    mResumable = false;
    return mSession;
}

SessionState SessionEngine::state() const { return static_cast<SessionState>(mSession.state); }

bool SessionEngine::isActive() const
{
    const SessionState s = state();
    return s == SessionState::Active || s == SessionState::Paused || s == SessionState::Rest;
}

const Session& SessionEngine::session() const { return mSession; }
const Workout* SessionEngine::workout() const { return mHasWorkout ? &mWorkout : nullptr; }
const SessionEngine::StepResult& SessionEngine::lastStep() const { return mLastStep; }
const Step& SessionEngine::step() const { return mWorkout.steps[mSession.stepIndex]; }

uint16_t SessionEngine::stepMakes() const
{
    uint16_t m = 0;
    for (uint16_t i = mSession.stepStartCount; i < mSession.count; ++i) m += mSession.shots[i].made;
    return m;
}

Snapshot SessionEngine::snapshot() const
{
    Snapshot s{};
    s.state     = mSession.state;
    s.mode      = mHasWorkout ? 1 : 0;
    s.spot      = mSession.spot;
    s.stepIndex = mSession.stepIndex;

    const uint16_t n = mSession.count < kMaxShots ? mSession.count : kMaxShots;
    for (uint16_t i = 0; i < n; ++i) {
        const Shot& sh = mSession.shots[i];
        s.totalMakes += sh.made;
        if (sh.spot == mSession.spot) { ++s.spotAttempts; s.spotMakes += sh.made; }
    }
    s.totalAttempts = n;
    s.stepAttempts  = n > mSession.stepStartCount ? n - mSession.stepStartCount : 0;
    s.stepMakes     = stepMakes();

    for (uint16_t i = 0; i < 5 && i < n; ++i) {
        if (mSession.shots[n - 1 - i].made) s.last5Bits |= static_cast<uint8_t>(1u << i);
        ++s.last5Count;
    }

    s.elapsed_s       = mSession.elapsed_s;
    s.restRemaining_s = mSession.restRemaining_s;
    s.stepElapsed_s   = mSession.stepElapsed_s;
    s.hr              = mLastHr;
    s.workoutId       = mSession.workoutId;
    if (mSession.count >= kMaxShots) s.flags |= Snapshot::kFull;
    if (mSession.workoutDone)        s.flags |= Snapshot::kWorkoutDone;
    if (mResumable)                  s.flags |= Snapshot::kResumable;
    return s;
}

} // namespace score
