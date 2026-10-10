#ifndef __STORAGE_HPP__
#define __STORAGE_HPP__

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "SDK/Kernel/Kernel.hpp"

#include "score/History.hpp"
#include "score/Serializer.hpp"
#include "score/Session.hpp"

/// Flash files of the app, framed by score::ser. The only place that touches the file system.
/// Paths are relative to the app directory:
///   workouts.bin          user workouts (WorkoutStore); written via workouts.bin.tmp + rename
///   current.bin           the running session, for crash resume; written via current.bin.tmp + rename
///   sessions/index.bin    session summaries (SessionIndex); written via index.bin.tmp + rename
///   sessions/<id>.bin     one finished session each
/// Every failure is logged at WARNING and returned as false; nothing asserts.
class Storage
{
public:
    explicit Storage(const SDK::Kernel& kernel);

    bool loadWorkouts(score::WorkoutStore& out);
    bool saveWorkouts(const score::WorkoutStore& in);

    bool loadIndex(score::SessionIndex& out);
    bool saveIndex(const score::SessionIndex& in);

    bool saveCurrent(const score::Session& s);
    bool loadCurrent(score::Session& out);
    void clearCurrent();   ///< removes current.bin and its .tmp; no-op when absent

    bool saveSession(const score::Session& s);
    bool loadSession(uint32_t id, score::Session& out);   ///< false (out zeroed) on id/count mismatch
    void removeSession(uint32_t id);                      ///< no-op when absent

private:
    /// @p atomic: write <path>.tmp, then rename it over <path> (workouts, index, current).
    bool write(const char* path, uint32_t magic, const void* obj, size_t size, bool atomic = false);
    bool writeFile(const char* path, size_t n);   ///< the first n bytes of mBuf
    bool read(const char* path, uint32_t magic, void* obj, size_t size);
    /// read(), falling back to <path>.tmp when <path> is absent (a crash mid-replace);
    /// a .tmp read that way is renamed to <path>.
    bool readAtomic(const char* path, uint32_t magic, void* obj, size_t size);
    void remove(const char* path);
    static void sessionPath(uint32_t id, char* out, size_t cap);
    static void tmpPath(const char* path, char* out, size_t cap);

    /// Largest persisted payload; SessionIndex (1804 B) outgrows Session (1632 B).
    static constexpr size_t kMaxPayload =
        std::max({sizeof(score::Session), sizeof(score::SessionIndex), sizeof(score::WorkoutStore)});

    const SDK::Kernel& mKernel;
    uint8_t            mBuf[sizeof(score::ser::Header) + kMaxPayload];
};

#endif
