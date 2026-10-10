#include "Storage.hpp"

#include <cstdio>
#include <cstring>
#include <memory>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "score/Spots.hpp"
#include "score/Workout.hpp"

#define LOG_MODULE_PRX      "Storage"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace {
constexpr const char* kWorkoutsPath = "workouts.bin";
constexpr const char* kSettingsPath = "settings.bin";
constexpr const char* kCurrentPath  = "current.bin";
constexpr const char* kIndexPath    = "sessions/index.bin";
} // namespace

Storage::Storage(const SDK::Kernel& kernel)
    : mKernel(kernel)
    , mBuf{}
{}

bool Storage::loadWorkouts(score::WorkoutStore& out)
{
    return readAtomic(kWorkoutsPath, score::ser::kMagicWorkouts, &out, sizeof(out));
}

bool Storage::saveWorkouts(const score::WorkoutStore& in)
{
    return write(kWorkoutsPath, score::ser::kMagicWorkouts, &in, sizeof(in), true);
}

bool Storage::loadSettings(score::Settings& out)
{
    return readAtomic(kSettingsPath, score::ser::kMagicSettings, &out, sizeof(out));
}

bool Storage::saveSettings(const score::Settings& in)
{
    return write(kSettingsPath, score::ser::kMagicSettings, &in, sizeof(in), true);
}

bool Storage::loadIndex(score::SessionIndex& out)
{
    return readAtomic(kIndexPath, score::ser::kMagicIndex, &out, sizeof(out));
}

bool Storage::saveIndex(const score::SessionIndex& in)
{
    return write(kIndexPath, score::ser::kMagicIndex, &in, sizeof(in), true);
}

bool Storage::saveCurrent(const score::Session& s)
{
    // Atomic as workouts / index: a crash mid-write must not destroy the
    // only copy of the running session.
    return write(kCurrentPath, score::ser::kMagicSession, &s, sizeof(s), true);
}

bool Storage::loadCurrent(score::Session& out)
{
    return readAtomic(kCurrentPath, score::ser::kMagicSession, &out, sizeof(out));
}

void Storage::clearCurrent()
{
    // The .tmp too: readAtomic() would bring an ended session back from it.
    char tmp[SDK::Interface::IFileSystem::skMaxPathLen];
    tmpPath(kCurrentPath, tmp, sizeof(tmp));
    remove(kCurrentPath);
    remove(tmp);
}

bool Storage::saveSession(const score::Session& s)
{
    char path[SDK::Interface::IFileSystem::skMaxPathLen];
    sessionPath(s.id, path, sizeof(path));
    return write(path, score::ser::kMagicSession, &s, sizeof(s));
}

bool Storage::loadSession(uint32_t id, score::Session& out)
{
    char path[SDK::Interface::IFileSystem::skMaxPathLen];
    sessionPath(id, path, sizeof(path));
    if (!read(path, score::ser::kMagicSession, &out, sizeof(out))) return false;
    if (out.id != id || out.count > score::kMaxShots) {
        LOG_WARNING("%s: bad record (id %lu, %u shots)\n", path, static_cast<unsigned long>(out.id),
                    static_cast<unsigned>(out.count));
        out = score::Session{};
        return false;
    }
    return true;
}

void Storage::removeSession(uint32_t id)
{
    char path[SDK::Interface::IFileSystem::skMaxPathLen];
    sessionPath(id, path, sizeof(path));
    remove(path);
    csvPath(id, path, sizeof(path));
    remove(path);
}

bool Storage::saveSessionCsv(const score::Session& s, uint32_t ymd, const char* workoutName)
{
    char path[SDK::Interface::IFileSystem::skMaxPathLen];
    csvPath(s.id, path, sizeof(path));
    if (!mkParent(path)) return false;

    // The name is a CSV field: a ',' (or a line break) would split the row.
    char name[score::kNameLen];   // workout names are at most kNameLen - 1 chars; keeps a row < 96 B
    snprintf(name, sizeof(name), "%s", workoutName ? workoutName : "Open");
    for (char* c = name; *c; ++c)
        if (*c == ',' || *c == '\n' || *c == '\r') *c = ' ';

    std::unique_ptr<SDK::Interface::IFile> file = mKernel.fs.file(path);
    if (!file) {
        LOG_WARNING("%s: no file handle\n", path);
        return false;
    }
    if (!file->open(true, true)) {
        LOG_WARNING("%s: open for write failed\n", path);
        return false;
    }

    // Each line is formatted into `line`, then collected in mBuf, which goes out
    // whenever the next line would not fit (about 10 writes for 400 shots).
    char   line[96];
    size_t used = 0, total = 0;
    bool   ok   = true;
    auto flushBuf = [&]() {
        if (!ok || used == 0) return;
        size_t bw = 0;
        ok = file->write(reinterpret_cast<const char*>(mBuf), used, bw) && bw == used;
        if (!ok) LOG_WARNING("%s: write failed (%u/%u B)\n", path, static_cast<unsigned>(bw),
                             static_cast<unsigned>(used));
        total += bw;
        used = 0;
    };
    auto append = [&](int len) {   // len = snprintf's result for `line`
        if (!ok) return;
        if (len < 0 || static_cast<size_t>(len) >= sizeof(line)) {   // snprintf failed or truncated
            LOG_WARNING("%s: line too long\n", path);
            ok = false;
            return;
        }
        if (used + static_cast<size_t>(len) > sizeof(mBuf)) flushBuf();
        if (!ok) return;
        memcpy(mBuf + used, line, static_cast<size_t>(len));
        used += static_cast<size_t>(len);
    };

    append(snprintf(line, sizeof(line), "session_id,date,workout,shot,spot,category,made,t_s\n"));
    const uint16_t count = s.count <= score::kMaxShots ? s.count : score::kMaxShots;
    for (uint16_t i = 0; i < count && ok; ++i) {
        const score::Shot&     shot = s.shots[i];
        const score::SpotInfo& info = score::spotInfo(shot.spot);
        append(snprintf(line, sizeof(line), "%lu,%04u-%02u-%02u,%s,%u,%s,%s,%u,%u\n",
                        static_cast<unsigned long>(s.id), static_cast<unsigned>(ymd / 10000u),
                        static_cast<unsigned>((ymd / 100u) % 100u), static_cast<unsigned>(ymd % 100u), name,
                        static_cast<unsigned>(i + 1), info.name, score::categoryShort(info.category),
                        shot.made ? 1u : 0u, static_cast<unsigned>(shot.t_s)));
    }
    flushBuf();
    const bool flushed = file->flush();
    const bool closed  = file->close();
    if (!ok || !flushed || !closed) {
        LOG_WARNING("%s: not written (%u B)\n", path, static_cast<unsigned>(total));
        remove(path);   // a torn copy would look complete to the user
        return false;
    }
    LOG_INFO("%s: %u shots, %u B\n", path, static_cast<unsigned>(count), static_cast<unsigned>(total));
    return true;
}

void Storage::sessionPath(uint32_t id, char* out, size_t cap)
{
    snprintf(out, cap, "sessions/%lu.bin", static_cast<unsigned long>(id));
}

void Storage::csvPath(uint32_t id, char* out, size_t cap)
{
    snprintf(out, cap, "sessions/%lu.csv", static_cast<unsigned long>(id));
}

bool Storage::mkParent(const char* path)
{
    // Create the parent directory, as SettingsSerializer::save does.
    const char* slash = strrchr(path, '/');
    if (!slash) return true;
    char dir[SDK::Interface::IFileSystem::skMaxPathLen]{};
    snprintf(dir, sizeof(dir), "%.*s", static_cast<int>(slash - path), path);
    if (!mKernel.fs.mkdir(dir)) {
        LOG_WARNING("%s: mkdir failed\n", dir);
        return false;
    }
    return true;
}

bool Storage::write(const char* path, uint32_t magic, const void* obj, size_t size, bool atomic)
{
    const size_t n = score::ser::encode(magic, obj, size, mBuf, sizeof(mBuf));
    if (n == 0) {
        LOG_WARNING("%s: %u B do not fit the buffer\n", path, static_cast<unsigned>(size));
        return false;
    }

    if (!mkParent(path)) return false;

    if (!atomic) return writeFile(path, n);

    // Atomic: the whole file goes to <path>.tmp, which then replaces <path>.
    // A crash leaves either the old file or a complete .tmp (see readAtomic).
    char tmp[SDK::Interface::IFileSystem::skMaxPathLen];
    tmpPath(path, tmp, sizeof(tmp));
    remove(tmp);   // stale, from an interrupted write
    if (!writeFile(tmp, n)) return false;
    if (!mKernel.fs.rename(tmp, path)) {
        // POSIX rename (the simulator) replaces the target; FatFS-style
        // file systems refuse an existing one: drop it and retry.
        remove(path);
        if (!mKernel.fs.rename(tmp, path)) {
            LOG_WARNING("%s: rename to %s failed\n", tmp, path);
            return false;
        }
    }
    return true;
}

bool Storage::writeFile(const char* path, size_t n)
{
    std::unique_ptr<SDK::Interface::IFile> file = mKernel.fs.file(path);
    if (!file) {
        LOG_WARNING("%s: no file handle\n", path);
        return false;
    }
    if (!file->open(true, true)) {
        LOG_WARNING("%s: open for write failed\n", path);
        return false;
    }
    size_t bw = 0;
    const bool written = file->write(reinterpret_cast<const char*>(mBuf), n, bw) && bw == n;
    const bool flushed = file->flush();
    const bool closed  = file->close();
    if (!written || !flushed || !closed) {
        LOG_WARNING("%s: write failed (%u/%u B)\n", path, static_cast<unsigned>(bw), static_cast<unsigned>(n));
        return false;
    }
    return true;
}

bool Storage::readAtomic(const char* path, uint32_t magic, void* obj, size_t size)
{
    if (mKernel.fs.exist(path)) return read(path, magic, obj, size);
    // A crash between remove(path) and rename() leaves only the complete
    // .tmp; decode() rejects a torn one.
    char tmp[SDK::Interface::IFileSystem::skMaxPathLen];
    tmpPath(path, tmp, sizeof(tmp));
    if (!mKernel.fs.exist(tmp)) return false;
    LOG_WARNING("%s: absent, recovering from %s\n", path, tmp);
    if (!read(tmp, magic, obj, size)) return false;
    // Promote it: the next write's remove(tmp) must not delete the only copy.
    if (!mKernel.fs.rename(tmp, path)) LOG_WARNING("%s: rename to %s failed\n", tmp, path);
    return true;
}

void Storage::tmpPath(const char* path, char* out, size_t cap)
{
    snprintf(out, cap, "%s.tmp", path);
}

bool Storage::read(const char* path, uint32_t magic, void* obj, size_t size)
{
    if (!mKernel.fs.exist(path)) {
        LOG_DEBUG("%s: absent\n", path);   // first run: not an error
        return false;
    }
    std::unique_ptr<SDK::Interface::IFile> file = mKernel.fs.file(path);
    if (!file) {
        LOG_WARNING("%s: no file handle\n", path);
        return false;
    }
    if (!file->open(false, false)) {
        LOG_WARNING("%s: open for read failed\n", path);
        return false;
    }
    const size_t len = file->size();
    if (len == 0 || len > sizeof(mBuf)) {
        LOG_WARNING("%s: bad size %u B\n", path, static_cast<unsigned>(len));
        file->close();
        return false;
    }
    size_t br = 0;
    const bool ok = file->read(reinterpret_cast<char*>(mBuf), len, br) && br == len;
    file->close();
    if (!ok) {
        LOG_WARNING("%s: read failed (%u/%u B)\n", path, static_cast<unsigned>(br), static_cast<unsigned>(len));
        return false;
    }
    if (!score::ser::decode(magic, mBuf, len, obj, size)) {
        LOG_WARNING("%s: corrupt (%u B)\n", path, static_cast<unsigned>(len));
        return false;
    }
    return true;
}

void Storage::remove(const char* path)
{
    if (!mKernel.fs.exist(path)) return;
    if (!mKernel.fs.remove(path)) LOG_WARNING("%s: remove failed\n", path);
}
