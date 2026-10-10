#include "Storage.hpp"

#include <cstdio>
#include <cstring>
#include <memory>

#include "SDK/Interfaces/IFileSystem.hpp"

#define LOG_MODULE_PRX      "Storage"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace {
constexpr const char* kWorkoutsPath = "workouts.bin";
constexpr const char* kCurrentPath  = "current.bin";
constexpr const char* kIndexPath    = "sessions/index.bin";
} // namespace

Storage::Storage(const SDK::Kernel& kernel)
    : mKernel(kernel)
    , mBuf{}
{}

bool Storage::loadWorkouts(score::WorkoutStore& out)
{
    return read(kWorkoutsPath, score::ser::kMagicWorkouts, &out, sizeof(out));
}

bool Storage::saveWorkouts(const score::WorkoutStore& in)
{
    return write(kWorkoutsPath, score::ser::kMagicWorkouts, &in, sizeof(in));
}

bool Storage::loadIndex(score::SessionIndex& out)
{
    return read(kIndexPath, score::ser::kMagicIndex, &out, sizeof(out));
}

bool Storage::saveIndex(const score::SessionIndex& in)
{
    return write(kIndexPath, score::ser::kMagicIndex, &in, sizeof(in));
}

bool Storage::saveCurrent(const score::Session& s)
{
    return write(kCurrentPath, score::ser::kMagicSession, &s, sizeof(s));
}

bool Storage::loadCurrent(score::Session& out)
{
    return read(kCurrentPath, score::ser::kMagicSession, &out, sizeof(out));
}

void Storage::clearCurrent()
{
    remove(kCurrentPath);
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
}

void Storage::sessionPath(uint32_t id, char* out, size_t cap)
{
    snprintf(out, cap, "sessions/%lu.bin", static_cast<unsigned long>(id));
}

bool Storage::write(const char* path, uint32_t magic, const void* obj, size_t size)
{
    const size_t n = score::ser::encode(magic, obj, size, mBuf, sizeof(mBuf));
    if (n == 0) {
        LOG_WARNING("%s: %u B do not fit the buffer\n", path, static_cast<unsigned>(size));
        return false;
    }

    // Create the parent directory, as SettingsSerializer::save does.
    const char* slash = strrchr(path, '/');
    if (slash) {
        char dir[SDK::Interface::IFileSystem::skMaxPathLen]{};
        snprintf(dir, sizeof(dir), "%.*s", static_cast<int>(slash - path), path);
        if (!mKernel.fs.mkdir(dir)) {
            LOG_WARNING("%s: mkdir failed\n", dir);
            return false;
        }
    }

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
