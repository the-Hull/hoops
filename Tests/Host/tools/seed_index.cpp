/**
 * @file  seed_index.cpp
 * @brief Host tool: writes a worst-case store for the self-test's phase 3.
 *
 * Usage: hoops-seed-index <store dir> [--light]   (the simulator's store is Software/Output)
 *
 * Writes, framed exactly as Storage does ({magic, size, FNV-1a} + raw POD):
 *  - sessions/<id>.bin  50 ended sessions ('HSS1'), oldest id 1780000000, one a day;
 *  - sessions/index.bin a full index of them ('HIX2', 50 rows, per-spot records);
 *  - workouts.bin       10 user workouts ('HWK1', ids 100..109);
 *  - settings.bin       with --light only: the defaults with the Light scheme ('HST1'),
 *                       so the GUI starts by rebuilding its first screen.
 * Existing files of those names are overwritten; nothing else is touched.
 */

#include "score/History.hpp"
#include "score/Serializer.hpp"
#include "score/Session.hpp"
#include "score/Settings.hpp"
#include "score/Spots.hpp"
#include "score/Stats.hpp"
#include "score/Workout.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

using namespace score;

namespace {

bool writeFile(const std::string& path, uint32_t magic, const void* obj, size_t size)
{
    std::vector<uint8_t> buf(sizeof(ser::Header) + size);
    const size_t n = ser::encode(magic, obj, size, buf.data(), buf.size());
    FILE* f = n ? std::fopen(path.c_str(), "wb") : nullptr;
    if (!f) {
        std::fprintf(stderr, "cannot write %s\n", path.c_str());
        return false;
    }
    const bool ok = std::fwrite(buf.data(), 1, n, f) == n;
    return (std::fclose(f) == 0) && ok;
}

uint32_t ymdOf(uint32_t unixTime)
{
    const std::time_t t = static_cast<std::time_t>(unixTime);
    std::tm tm{};
    gmtime_r(&t, &tm);
    return static_cast<uint32_t>(tm.tm_year + 1900) * 10000u + static_cast<uint32_t>(tm.tm_mon + 1) * 100u +
           static_cast<uint32_t>(tm.tm_mday);
}

/// Session @p i (0 = oldest): 10..40 shots over three spots, a workout id
/// cycling open / presets / user workouts, makes in a fixed pattern.
void makeSession(uint8_t i, Session& s)
{
    std::memset(&s, 0, sizeof s);
    s.id        = 1780000000u + static_cast<uint32_t>(i) * 86400u;
    s.workoutId = (i % 4 == 0) ? 0 : (i % 4 == 1) ? static_cast<uint16_t>(1 + i % kPresetCount)
                                                   : static_cast<uint16_t>(kFirstUserWorkoutId + i % kMaxUserWorkouts);
    s.count     = static_cast<uint16_t>(10 + (i % 7) * 5);
    s.state     = static_cast<uint8_t>(SessionState::Ended);
    s.spot      = static_cast<uint8_t>(i % kSpotCount);
    for (uint16_t k = 0; k < s.count; ++k) {
        Shot& shot = s.shots[k];
        shot.spot  = static_cast<uint8_t>((i + k / 5) % kSpotCount);
        shot.made  = static_cast<uint8_t>(((k * 7 + i * 3) % 10) < 3 + i % 5);
        shot.t_s   = static_cast<uint16_t>(k * 12);
    }
    s.elapsed_s = static_cast<uint32_t>(s.count) * 12u;
}

} // namespace

int main(int argc, char** argv)
{
    const bool light = argc == 3 && std::strcmp(argv[2], "--light") == 0;
    if (argc != 2 && !light) {
        std::fprintf(stderr, "usage: %s <store dir> [--light]\n", argv[0]);
        return 2;
    }
    const std::string dir = argv[1];
    std::error_code ec;
    std::filesystem::create_directories(dir + "/sessions", ec);
    if (ec) {
        std::fprintf(stderr, "cannot create %s/sessions: %s\n", dir.c_str(), ec.message().c_str());
        return 1;
    }

    static SessionIndex index;   // ~1.9 KB; static keeps it off the stack and zeroed
    static Session      s;
    for (uint8_t i = 0; i < kIndexCapacity; ++i) {
        makeSession(i, s);
        if (!writeFile(dir + "/sessions/" + std::to_string(s.id) + ".bin", ser::kMagicSession, &s, sizeof s)) return 1;
        if (index.push(summarize(s, ymdOf(s.id))) != 0) {
            std::fprintf(stderr, "unexpected eviction at %u\n", i);
            return 1;
        }
        index.recordSpots(SessionStats::compute(s).spot);
    }
    if (!index.valid() || index.count != kIndexCapacity) return 1;
    if (!writeFile(dir + "/sessions/index.bin", ser::kMagicIndex, &index, sizeof index)) return 1;

    static WorkoutStore store;
    for (uint8_t i = 0; i < kMaxUserWorkouts; ++i) {
        Workout w{};
        std::snprintf(w.name, sizeof w.name, "Seed %u", static_cast<unsigned>(i + 1));
        w.stepCount = 2;
        for (uint8_t k = 0; k < w.stepCount; ++k) {
            w.steps[k].spot       = static_cast<uint8_t>((i + k * 5) % kSpotCount);
            w.steps[k].rest_s     = 30;
            w.steps[k].goal.type  = static_cast<uint8_t>(GoalType::Attempts);
            w.steps[k].goal.value = 10;
        }
        uint8_t id = 0;
        if (!store.add(w, id) || id != kFirstUserWorkoutId + i) {
            std::fprintf(stderr, "workout %u not added\n", i);
            return 1;
        }
    }
    if (!store.valid()) return 1;
    if (!writeFile(dir + "/workouts.bin", ser::kMagicWorkouts, &store, sizeof store)) return 1;

    if (light) {
        Settings st = Settings::defaults();
        st.scheme   = static_cast<uint8_t>(Scheme::Light);
        if (!st.valid() || !writeFile(dir + "/settings.bin", ser::kMagicSettings, &st, sizeof st)) return 1;
    }

    std::printf("seeded %s: %u sessions (ids %u..%u), %u workouts%s\n", dir.c_str(), index.count,
                static_cast<unsigned>(index.items[0].id), static_cast<unsigned>(index.items[kIndexCapacity - 1].id),
                store.count, light ? ", settings.bin (Light)" : "");
    return 0;
}
