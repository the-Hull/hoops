#include <gtest/gtest.h>
#include <cstring>
#include "score/Serializer.hpp"
#include "score/Session.hpp"
#include "score/Settings.hpp"
using namespace score;

TEST(Settings, DefaultsAreValid) {
    const Settings s = Settings::defaults();
    EXPECT_TRUE(s.valid());
    EXPECT_EQ(s.goalType, static_cast<uint8_t>(GoalType::Attempts));
    EXPECT_EQ(s.rest_s, 30);
    EXPECT_EQ(s.goalValue, 10);
    EXPECT_EQ(s.openTarget, 0);
    EXPECT_EQ(s.holdDown, static_cast<uint8_t>(HoldDown::ChangeSpot));
    EXPECT_EQ(s.haptics, static_cast<uint8_t>(Haptics::Strong));
    EXPECT_EQ(s.backlightOnShot, 1);
    EXPECT_EQ(s.heartRate, 1);
    EXPECT_EQ(s.scheme, static_cast<uint8_t>(Scheme::Dark));
    EXPECT_EQ(s.csvExport, 1);
}
TEST(Settings, SizeIs12) {
    EXPECT_EQ(sizeof(Settings), 12u);
    EXPECT_TRUE(std::has_unique_object_representations<Settings>::value);
    EXPECT_TRUE(std::is_trivially_copyable<Settings>::value);
}
TEST(Settings, RejectsBadGoalType) {
    Settings s = Settings::defaults();
    s.goalType = 3;
    EXPECT_FALSE(s.valid());
}
TEST(Settings, RejectsBadRest) {
    Settings s = Settings::defaults();
    s.rest_s = 20;
    EXPECT_FALSE(s.valid());
}
TEST(Settings, RejectsZeroGoalValue) {
    Settings s = Settings::defaults();
    s.goalValue = 0;
    EXPECT_FALSE(s.valid());
}
TEST(Settings, RejectsTargetAboveMaxShots) {
    Settings s = Settings::defaults();
    s.openTarget = kMaxShots;
    EXPECT_TRUE(s.valid());
    s.openTarget = kMaxShots + 1;   // 401
    EXPECT_FALSE(s.valid());
}
TEST(Settings, RejectsBadScheme) {
    Settings s = Settings::defaults();
    s.scheme = 2;
    EXPECT_FALSE(s.valid());
}
TEST(Settings, AcceptsEveryRestValue) {
    Settings s = Settings::defaults();
    for (uint8_t r : { 0, 15, 30, 60 }) {
        s.rest_s = r;
        EXPECT_TRUE(s.valid()) << "rest " << int(r);
    }
}
TEST(Settings, RoundTripsThroughSerializer) {
    Settings s = Settings::defaults();
    s.goalType   = static_cast<uint8_t>(GoalType::Time);
    s.goalValue  = 120;
    s.openTarget = 50;
    s.scheme     = static_cast<uint8_t>(Scheme::Light);
    uint8_t buf[sizeof(ser::Header) + sizeof(Settings)];
    ASSERT_EQ(ser::encode(ser::kMagicSettings, &s, sizeof s, buf, sizeof buf), sizeof buf);
    Settings back{};
    ASSERT_TRUE(ser::decode(ser::kMagicSettings, buf, sizeof buf, &back, sizeof back));
    EXPECT_EQ(std::memcmp(&back, &s, sizeof s), 0);
    EXPECT_TRUE(back.valid());
    Settings other{};
    EXPECT_FALSE(ser::decode(ser::kMagicWorkouts, buf, sizeof buf, &other, sizeof other));
}
