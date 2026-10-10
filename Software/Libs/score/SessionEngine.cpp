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

bool SessionEngine::startWorkout(const Workout& w, uint32_t nowUnix)
{
    if (w.stepCount == 0 || w.stepCount > kMaxSteps) return false;
    if (!startOpen(static_cast<Spot>(w.steps[0].spot), nowUnix)) return false;
    mWorkout           = w;
    mHasWorkout        = true;
    mSession.workoutId = w.id;
    return true;
}

bool SessionEngine::restore(const Session& saved, const Workout* w)
{
    const SessionState s = static_cast<SessionState>(saved.state);
    if (s != SessionState::Active && s != SessionState::Paused && s != SessionState::Rest) return false;
    // Reject records that would index outside the shot log or step list.
    if (saved.count > kMaxShots || saved.stepStartCount > saved.count) return false;
    if (w && (w->stepCount == 0 || w->stepCount > kMaxSteps || saved.stepIndex >= w->stepCount)) return false;

    SessionState from = (s == SessionState::Paused) ? static_cast<SessionState>(saved.pausedFrom) : s;
    if (from != SessionState::Active && from != SessionState::Rest) from = SessionState::Active;
    // Rest on the last step never happens; advancing from it would run past the step list.
    if (w && from == SessionState::Rest && saved.stepIndex + 1 >= w->stepCount) return false;

    mSession = saved;
    if (w) {
        mWorkout    = *w;
        mHasWorkout = true;
    } else {
        // Workout definition gone: continue as an open session at the current spot.
        mWorkout                 = Workout{};
        mHasWorkout              = false;
        mSession.workoutId       = 0;
        mSession.workoutDone     = 0;
        mSession.stepIndex       = 0;
        mSession.stepElapsed_s   = 0;
        mSession.restRemaining_s = 0;
        mSession.stepStartCount  = mSession.count;
        if (from == SessionState::Rest) from = SessionState::Active;
    }
    mSession.pausedFrom = st(from);
    mSession.state      = st(SessionState::Paused);
    mResumable          = true;
    mLastStep           = StepResult{};
    mLastHr             = 0;
    return true;
}

bool SessionEngine::skipRest()
{
    if (state() != SessionState::Rest) return false;
    advanceStep();
    return true;
}

bool SessionEngine::extendRest(uint16_t seconds)
{
    if (state() != SessionState::Rest) return false;
    const uint32_t r = static_cast<uint32_t>(mSession.restRemaining_s) + seconds;
    mSession.restRemaining_s = static_cast<uint16_t>(r > UINT16_MAX ? UINT16_MAX : r);
    return true;
}

SessionEngine::Event SessionEngine::completeStep()
{
    const uint8_t rest = step().rest_s;
    mLastStep = { mSession.stepIndex, stepMakes(),
                  static_cast<uint16_t>(mSession.count - mSession.stepStartCount), rest };
    if (mSession.stepIndex + 1 >= mWorkout.stepCount) {
        mSession.workoutDone    = 1;
        mSession.stepStartCount = mSession.count;   // only extra shots can be undone
        return Event::WorkoutDone;
    }
    if (rest > 0) {
        mSession.state           = st(SessionState::Rest);
        mSession.restRemaining_s = rest;
    } else {
        advanceStep();
    }
    return Event::StepDone;
}

void SessionEngine::advanceStep()
{
    ++mSession.stepIndex;
    mSession.spot            = step().spot;
    mSession.stepStartCount  = mSession.count;
    mSession.stepElapsed_s   = 0;
    mSession.restRemaining_s = 0;
    mSession.state           = st(SessionState::Active);
}

SessionEngine::Event SessionEngine::shot(bool made)
{
    if (state() != SessionState::Active || mSession.count >= kMaxShots) return Event::None;
    mSession.shots[mSession.count++] = { mSession.spot, static_cast<uint8_t>(made ? 1 : 0),
                                         static_cast<uint16_t>(mSession.elapsed_s) };
    if (!mHasWorkout || mSession.workoutDone) return Event::None;

    const Goal& g = step().goal;
    const uint16_t attempts = mSession.count - mSession.stepStartCount;
    if ((g.type == static_cast<uint8_t>(GoalType::Attempts) && attempts >= g.value) ||
        (g.type == static_cast<uint8_t>(GoalType::Makes) && stepMakes() >= g.value))
        return completeStep();
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
    mResumable     = false;
    return true;
}

SessionEngine::Event SessionEngine::tick()
{
    switch (state()) {
    case SessionState::Active:
        ++mSession.elapsed_s;
        ++mSession.stepElapsed_s;
        if (mHasWorkout && !mSession.workoutDone &&
            step().goal.type == static_cast<uint8_t>(GoalType::Time) &&
            mSession.stepElapsed_s >= step().goal.value)
            return completeStep();
        break;
    case SessionState::Rest:
        ++mSession.elapsed_s;
        if (mSession.restRemaining_s > 0) --mSession.restRemaining_s;
        if (mSession.restRemaining_s == 0) {
            advanceStep();
            return Event::RestOver;
        }
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
    s.nextSpot  = mSession.spot;
    if (mHasWorkout) {
        s.stepCount = mWorkout.stepCount;
        s.goalType  = step().goal.type;
        s.goalValue = step().goal.value;
        const bool resting = state() == SessionState::Rest ||
            (state() == SessionState::Paused && mSession.pausedFrom == st(SessionState::Rest));
        if (resting && mSession.stepIndex + 1 < mWorkout.stepCount)
            s.nextSpot = mWorkout.steps[mSession.stepIndex + 1].spot;
    }

    const uint16_t n = mSession.count < kMaxShots ? mSession.count : kMaxShots;
    for (uint16_t i = 0; i < n; ++i) {
        const Shot& sh = mSession.shots[i];
        s.totalMakes += sh.made;
        if (sh.spot == mSession.spot) { ++s.spotAttempts; s.spotMakes += sh.made; }
    }
    s.totalAttempts = n;
    s.stepAttempts  = n > mSession.stepStartCount ? n - mSession.stepStartCount : 0;
    s.stepMakes     = stepMakes();
    for (uint16_t i = mSession.stepStartCount, b = 0; i < n && b < 64; ++i, ++b)
        if (mSession.shots[i].made) s.stepBits |= uint64_t{1} << b;

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
