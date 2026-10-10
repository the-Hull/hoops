#include "score/Settings.hpp"

#include "score/Session.hpp"

namespace score {

namespace {

constexpr uint16_t kMaxGoalValue = 999;

bool isFlag(uint8_t v) { return v <= 1; }

bool isRest(uint8_t r) { return r == 0 || r == 15 || r == 30 || r == 60; }

} // namespace

Settings Settings::defaults()
{
    Settings s{};
    s.goalType        = static_cast<uint8_t>(GoalType::Attempts);
    s.rest_s          = 30;
    s.goalValue       = 10;
    s.openTarget      = 0;
    s.holdDown        = static_cast<uint8_t>(HoldDown::ChangeSpot);
    s.haptics         = static_cast<uint8_t>(Haptics::Strong);
    s.backlightOnShot = 1;
    s.heartRate       = 1;
    s.scheme          = static_cast<uint8_t>(Scheme::Dark);
    s.csvExport       = 1;
    return s;
}

bool Settings::valid() const
{
    return goalType <= static_cast<uint8_t>(GoalType::Time)
        && isRest(rest_s)
        && goalValue >= 1 && goalValue <= kMaxGoalValue
        && openTarget <= kMaxShots
        && holdDown <= static_cast<uint8_t>(HoldDown::Pause)
        && haptics <= static_cast<uint8_t>(Haptics::Strong)
        && isFlag(backlightOnShot) && isFlag(heartRate) && isFlag(csvExport)
        && scheme <= static_cast<uint8_t>(Scheme::Light);
}

} // namespace score
