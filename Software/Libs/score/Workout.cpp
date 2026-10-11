#include "score/Workout.hpp"
#include "score/PresetsTable.hpp"

namespace score {

namespace {

constexpr uint32_t kSecondsPerShot = 12;

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
