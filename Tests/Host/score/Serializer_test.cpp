#include <gtest/gtest.h>
#include <cstring>
#include "score/Serializer.hpp"
#include "score/Session.hpp"
using namespace score;

namespace {
constexpr size_t kFrame = sizeof(ser::Header) + sizeof(Session);
uint8_t buf[kFrame + 16];

Session sample() {
    Session s{};
    s.id = 42; s.workoutId = 3; s.elapsed_s = 125;
    s.shots[0] = { 1, 1, 5 };
    s.shots[1] = { 10, 0, 9 };
    s.shots[2] = { 11, 1, 14 };
    s.count = 3;
    return s;
}
} // namespace

TEST(Serializer, ChecksumIsFnv1a) {
    EXPECT_EQ(ser::checksum("", 0), 0x811C9DC5u);
    EXPECT_EQ(ser::checksum("a", 1), 0xE40C292Cu);
}
TEST(Serializer, RoundTripSession) {
    const Session in = sample();
    ASSERT_EQ(ser::encode(ser::kMagicSession, &in, sizeof in, buf, sizeof buf), kFrame);
    Session out{};
    ASSERT_TRUE(ser::decode(ser::kMagicSession, buf, kFrame, &out, sizeof out));
    EXPECT_EQ(std::memcmp(&in, &out, sizeof in), 0);
    EXPECT_EQ(out.count, 3);
    EXPECT_EQ(out.shots[2].spot, 11);
}
TEST(Serializer, CapTooSmallWritesNothing) {
    const Session in = sample();
    EXPECT_EQ(ser::encode(ser::kMagicSession, &in, sizeof in, buf, kFrame - 1), 0u);
    EXPECT_EQ(ser::encode(ser::kMagicSession, &in, sizeof in, buf, 0), 0u);
}
TEST(Serializer, TruncatedBufferFails) {
    const Session in = sample();
    ASSERT_EQ(ser::encode(ser::kMagicSession, &in, sizeof in, buf, sizeof buf), kFrame);
    Session out{}; out.id = 7;
    EXPECT_FALSE(ser::decode(ser::kMagicSession, buf, kFrame - 1, &out, sizeof out));
    EXPECT_FALSE(ser::decode(ser::kMagicSession, buf, 4, &out, sizeof out));
    EXPECT_EQ(out.id, 7u);   // untouched
}
TEST(Serializer, WrongMagicFails) {
    const Session in = sample();
    ASSERT_EQ(ser::encode(ser::kMagicSession, &in, sizeof in, buf, sizeof buf), kFrame);
    Session out{};
    EXPECT_FALSE(ser::decode(ser::kMagicIndex, buf, kFrame, &out, sizeof out));
    EXPECT_EQ(out.id, 0u);
}
TEST(Serializer, FlippedPayloadByteFails) {
    const Session in = sample();
    ASSERT_EQ(ser::encode(ser::kMagicSession, &in, sizeof in, buf, sizeof buf), kFrame);
    buf[sizeof(ser::Header) + 20] ^= 0x01;
    Session out{};
    EXPECT_FALSE(ser::decode(ser::kMagicSession, buf, kFrame, &out, sizeof out));
    EXPECT_EQ(out.id, 0u);
}
TEST(Serializer, SizeMismatchFails) {
    struct Small { uint32_t a, b; };
    const Small in{ 1, 2 };
    uint8_t small[sizeof(ser::Header) + sizeof(Small)];
    ASSERT_EQ(ser::encode(ser::kMagicSession, &in, sizeof in, small, sizeof small), sizeof small);
    struct Smaller { uint32_t a; };
    Smaller out{ 9 };
    // length matches Smaller's frame + 4, header.size says 8
    EXPECT_FALSE(ser::decode(ser::kMagicSession, small, sizeof small, &out, sizeof out));
    EXPECT_FALSE(ser::decode(ser::kMagicSession, small, sizeof(ser::Header) + sizeof out, &out, sizeof out));
    EXPECT_EQ(out.a, 9u);
}
