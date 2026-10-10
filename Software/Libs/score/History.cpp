#include "score/History.hpp"

#include "score/Spots.hpp"
#include "score/Stats.hpp"

#include <cstring>

namespace score {

SessionSummary summarize(const Session& s, uint32_t ymd)
{
    SessionSummary r;
    std::memset(&r, 0, sizeof r);   // pads zero
    r.id        = s.id;
    r.ymd       = ymd;
    r.workoutId = s.workoutId;
    r.elapsed_s = s.elapsed_s;
    uint16_t makes = 0;
    const uint16_t n = s.count <= kMaxShots ? s.count : kMaxShots;
    for (uint16_t i = 0; i < n; ++i) {
        const uint8_t c = static_cast<uint8_t>(spotInfo(s.shots[i].spot).category);
        ++r.catAtt[c];
        if (s.shots[i].made) { ++r.catMakes[c]; ++makes; }
    }
    r.totalPct = pct(makes, n);
    return r;
}

// --- SessionIndex ---

uint32_t SessionIndex::push(const SessionSummary& s)
{
    uint32_t evicted = 0;
    if (count >= kIndexCapacity) {
        evicted = items[0].id;
        for (uint8_t i = 1; i < kIndexCapacity; ++i) items[i - 1] = items[i];
        count = kIndexCapacity - 1;
    }
    items[count++] = s;
    return evicted;
}

const SessionSummary* SessionIndex::find(uint32_t id) const
{
    for (uint8_t i = 0; i < count; ++i)
        if (items[i].id == id) return &items[i];
    return nullptr;
}

const SessionSummary* SessionIndex::newest(uint8_t back) const
{
    return back < count ? &items[count - 1 - back] : nullptr;
}

bool SessionIndex::valid() const { return count <= kIndexCapacity; }

bool SessionIndex::remove(uint32_t id)
{
    for (uint8_t i = 0; i < count; ++i) {
        if (items[i].id != id) continue;
        for (uint8_t j = i + 1; j < count; ++j) items[j - 1] = items[j];
        items[--count] = SessionSummary{};
        return true;
    }
    return false;
}

// --- WorkoutStore ---

bool WorkoutStore::add(Workout w, uint8_t& idOut)
{
    if (count >= kMaxUserWorkouts) return false;
    uint8_t id = kFirstUserWorkoutId;
    while (find(id)) ++id;   // count < capacity ⇒ a free id exists in range
    w.id          = id;
    items[count++] = w;
    idOut         = id;
    return true;
}

bool WorkoutStore::replace(const Workout& w)
{
    for (uint8_t i = 0; i < count; ++i) {
        if (items[i].id != w.id) continue;
        items[i] = w;
        return true;
    }
    return false;
}

bool WorkoutStore::remove(uint8_t id)
{
    for (uint8_t i = 0; i < count; ++i) {
        if (items[i].id != id) continue;
        for (uint8_t j = i + 1; j < count; ++j) items[j - 1] = items[j];
        items[--count] = Workout{};
        return true;
    }
    return false;
}

const Workout* WorkoutStore::find(uint8_t id) const
{
    for (uint8_t i = 0; i < count; ++i)
        if (items[i].id == id) return &items[i];
    return nullptr;
}

bool WorkoutStore::valid() const
{
    if (count > kMaxUserWorkouts) return false;
    for (uint8_t i = 0; i < count; ++i) {
        const Workout& w = items[i];
        if (w.id < kFirstUserWorkoutId || w.id >= kFirstUserWorkoutId + kMaxUserWorkouts) return false;
        if (w.stepCount < 1 || w.stepCount > kMaxSteps) return false;
    }
    return true;
}

uint8_t WorkoutStore::nextCustomNumber() const { return static_cast<uint8_t>(count + 1); }

} // namespace score
