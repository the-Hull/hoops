#include "score/Workout.hpp"

namespace score {

namespace {

constexpr uint32_t kSecondsPerShot = 12;

constexpr Step step(Spot s, GoalType t, uint16_t v, uint8_t rest)
{
    return { static_cast<uint8_t>(s), rest, { static_cast<uint8_t>(t), 0, v } };
}

constexpr GoalType A = GoalType::Attempts;
constexpr GoalType M = GoalType::Makes;

const Workout kPresets[kPresetCount] = {
    { 1, "Around the World", 5, {
        step(Spot::LCorner3, A, 10, 15), step(Spot::LWing3, A, 10, 15), step(Spot::Top3, A, 10, 15),
        step(Spot::RWing3, A, 10, 15), step(Spot::RCorner3, A, 10, 15) } },
    { 2, "Free Throw 50", 1, {
        step(Spot::Ft, A, 50, 0) } },
    { 3, "Mikan Layups", 2, {
        step(Spot::LayupL, A, 10, 0), step(Spot::LayupR, A, 10, 0) } },
    { 4, "Mid-range Ladder", 5, {
        step(Spot::LBaselineMid, M, 5, 15), step(Spot::LElbowMid, M, 5, 15), step(Spot::TopKeyMid, M, 5, 15),
        step(Spot::RElbowMid, M, 5, 15), step(Spot::RBaselineMid, M, 5, 15) } },
    { 5, "Game Shots", 4, {
        step(Spot::Top3, A, 10, 30), step(Spot::RElbowMid, A, 10, 30), step(Spot::Ft, A, 10, 30),
        step(Spot::LayupR, A, 10, 30) } },
};

} // namespace

uint16_t Workout::totalShots() const
{
    uint16_t total = 0;
    for (uint8_t i = 0; i < stepCount && i < kMaxSteps; ++i) {
        if (steps[i].goal.type != static_cast<uint8_t>(GoalType::Time)) {
            total = static_cast<uint16_t>(total + steps[i].goal.value);
        }
    }
    return total;
}

uint32_t Workout::estimateSeconds() const
{
    uint32_t secs = totalShots() * kSecondsPerShot;
    for (uint8_t i = 0; i < stepCount && i < kMaxSteps; ++i) {
        secs += steps[i].rest_s;
        if (steps[i].goal.type == static_cast<uint8_t>(GoalType::Time)) {
            secs += steps[i].goal.value;
        }
    }
    return secs;
}

bool Workout::validSteps() const
{
    if (stepCount < 1 || stepCount > kMaxSteps) {
        return false;
    }
    for (uint8_t i = 0; i < stepCount; ++i) {
        const Step& st = steps[i];
        if (st.spot >= kSpotCount || st.goal.type > static_cast<uint8_t>(GoalType::Time) || st.goal.value == 0) {
            return false;
        }
    }
    return true;
}

const Workout& preset(uint8_t index)
{
    return kPresets[index < kPresetCount ? index : kPresetCount - 1];
}

const Workout* findPreset(uint8_t id)
{
    for (const Workout& w : kPresets) {
        if (w.id == id) {
            return &w;
        }
    }
    return nullptr;
}

} // namespace score
