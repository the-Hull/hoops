/**
 * @file  Commands.hpp
 * @brief Service <-> GUI IPC contract: custom message ids and payload structs.
 *
 * Payloads are the score:: PODs copied verbatim; the GUI never computes stats.
 * Every struct must fit the kernel's 256-byte message block and carries explicit pads
 * instead of implicit padding, so its bytes are fully defined (asserted at the bottom).
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "score/History.hpp"
#include "score/SessionEngine.hpp"
#include "score/Settings.hpp"
#include "score/Stats.hpp"
#include "score/Workout.hpp"

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// GUI --> Service
constexpr SDK::MessageType::Type START_OPEN     = 0x0101;
constexpr SDK::MessageType::Type START_WORKOUT  = 0x0102;
constexpr SDK::MessageType::Type SHOT           = 0x0103;
constexpr SDK::MessageType::Type UNDO           = 0x0104;
constexpr SDK::MessageType::Type PAUSE          = 0x0105;
constexpr SDK::MessageType::Type RESUME         = 0x0106;
constexpr SDK::MessageType::Type SKIP_REST      = 0x0107;
constexpr SDK::MessageType::Type EXTEND_REST    = 0x0108;
constexpr SDK::MessageType::Type CHANGE_SPOT    = 0x0109;
constexpr SDK::MessageType::Type END_SESSION    = 0x010A;
constexpr SDK::MessageType::Type GET_SNAPSHOT   = 0x010B;
constexpr SDK::MessageType::Type GET_WORKOUTS   = 0x010C;
constexpr SDK::MessageType::Type SAVE_WORKOUT   = 0x010D;
constexpr SDK::MessageType::Type DELETE_WORKOUT = 0x010E;
constexpr SDK::MessageType::Type GET_RESULTS    = 0x010F;
constexpr SDK::MessageType::Type GET_HISTORY    = 0x0110;
constexpr SDK::MessageType::Type SET_SETTINGS   = 0x0111;
constexpr SDK::MessageType::Type GET_SPOT_STATS = 0x0112;
constexpr SDK::MessageType::Type CLEAR_HISTORY  = 0x0113;

// Service --> GUI
constexpr SDK::MessageType::Type SNAPSHOT      = 0x0201;
constexpr SDK::MessageType::Type UNDO_RESULT   = 0x0202;
constexpr SDK::MessageType::Type STEP_DONE     = 0x0203;
constexpr SDK::MessageType::Type WORKOUT_ITEM  = 0x0204;
constexpr SDK::MessageType::Type RESULTS       = 0x0205;
constexpr SDK::MessageType::Type HISTORY       = 0x0206;
constexpr SDK::MessageType::Type LOCAL_TIME    = 0x0207;
constexpr SDK::MessageType::Type SAVED         = 0x0208;
constexpr SDK::MessageType::Type WORKOUT_SAVED = 0x0209;
constexpr SDK::MessageType::Type SETTINGS      = 0x020A;
constexpr SDK::MessageType::Type SPOT_STATS    = 0x020B;

/// Local wall-clock time; padding-free POD.
struct Clock {
    uint16_t year;   ///< e.g. 2026
    uint8_t  mon;    ///< 1..12
    uint8_t  mday;   ///< 1..31
    uint8_t  hour, min, sec;
    uint8_t  wday;   ///< 0 = Sunday
};
static_assert(sizeof(Clock) == 8 && std::has_unique_object_representations<Clock>::value, "Clock: no padding");

// ---------------------------------------------------------------- GUI --> Service

struct StartOpen : public SDK::MessageBase {
    uint8_t spot;   ///< score::Spot
    uint8_t pad[3] = {};
    explicit StartOpen(uint8_t s = 0) : SDK::MessageBase(START_OPEN), spot(s) {}
};

struct StartWorkout : public SDK::MessageBase {
    uint8_t workoutId;   ///< preset 1..kPresetCount or user id >= kFirstUserWorkoutId
    uint8_t pad[3] = {};
    explicit StartWorkout(uint8_t id = 0) : SDK::MessageBase(START_WORKOUT), workoutId(id) {}
};

struct Shot : public SDK::MessageBase {
    uint8_t made;   ///< 1 = make, 0 = miss
    uint8_t pad[3] = {};
    explicit Shot(uint8_t m = 0) : SDK::MessageBase(SHOT), made(m) {}
};

struct Undo : public SDK::MessageBase {
    Undo() : SDK::MessageBase(UNDO) {}
};

struct Pause : public SDK::MessageBase {
    Pause() : SDK::MessageBase(PAUSE) {}
};

struct Resume : public SDK::MessageBase {
    Resume() : SDK::MessageBase(RESUME) {}
};

struct SkipRest : public SDK::MessageBase {
    SkipRest() : SDK::MessageBase(SKIP_REST) {}
};

struct ExtendRest : public SDK::MessageBase {
    uint16_t seconds;
    uint8_t  pad[2] = {};
    explicit ExtendRest(uint16_t s = 15) : SDK::MessageBase(EXTEND_REST), seconds(s) {}
};

struct ChangeSpot : public SDK::MessageBase {
    uint8_t spot;   ///< score::Spot
    uint8_t pad[3] = {};
    explicit ChangeSpot(uint8_t s = 0) : SDK::MessageBase(CHANGE_SPOT), spot(s) {}
};

struct EndSession : public SDK::MessageBase {
    uint8_t save;   ///< 1 = keep the session, 0 = discard
    uint8_t pad[3] = {};
    explicit EndSession(uint8_t s = 1) : SDK::MessageBase(END_SESSION), save(s) {}
};

struct GetSnapshot : public SDK::MessageBase {
    GetSnapshot() : SDK::MessageBase(GET_SNAPSHOT) {}
};

struct GetWorkouts : public SDK::MessageBase {
    GetWorkouts() : SDK::MessageBase(GET_WORKOUTS) {}
};

struct SaveWorkout : public SDK::MessageBase {
    score::Workout workout;   ///< id 0 = new, else replaces that user workout
    uint8_t        pad[2] = {};
    explicit SaveWorkout(const score::Workout& w = {}) : SDK::MessageBase(SAVE_WORKOUT), workout(w) {}
};

struct DeleteWorkout : public SDK::MessageBase {
    uint8_t workoutId;
    uint8_t pad[3] = {};
    explicit DeleteWorkout(uint8_t id = 0) : SDK::MessageBase(DELETE_WORKOUT), workoutId(id) {}
};

struct GetResults : public SDK::MessageBase {
    uint32_t sessionId;   ///< 0 = the session that just ended
    explicit GetResults(uint32_t id = 0) : SDK::MessageBase(GET_RESULTS), sessionId(id) {}
};

struct GetHistory : public SDK::MessageBase {
    GetHistory() : SDK::MessageBase(GET_HISTORY) {}
};

/// Store and apply; the service answers with SETTINGS (an invalid one is ignored, answered with the current).
struct SetSettings : public SDK::MessageBase {
    score::Settings settings;
    explicit SetSettings(const score::Settings& s = score::Settings::defaults())
        : SDK::MessageBase(SET_SETTINGS), settings(s) {}
};

struct GetSpotStats : public SDK::MessageBase {
    GetSpotStats() : SDK::MessageBase(GET_SPOT_STATS) {}
};

/// Delete every indexed session (files, CSVs, index, spot records); a running session is unaffected.
struct ClearHistory : public SDK::MessageBase {
    ClearHistory() : SDK::MessageBase(CLEAR_HISTORY) {}
};

// ---------------------------------------------------------------- Service --> GUI

struct SnapshotMsg : public SDK::MessageBase {
    score::Snapshot snapshot;
    explicit SnapshotMsg(const score::Snapshot& s = {}) : SDK::MessageBase(SNAPSHOT), snapshot(s) {}
};

struct UndoResult : public SDK::MessageBase {
    uint8_t ok;     ///< 1 = a shot was removed
    uint8_t made;   ///< the removed shot was a make
    uint8_t pad[2] = {};
    explicit UndoResult(uint8_t o = 0, uint8_t m = 0) : SDK::MessageBase(UNDO_RESULT), ok(o), made(m) {}
};

/// A workout step finished: its result plus what comes next (zeros when workoutDone).
struct StepDone : public SDK::MessageBase {
    uint8_t  stepIndex     = 0;
    uint8_t  workoutDone   = 0;
    uint8_t  nextSpot      = 0;   ///< score::Spot
    uint8_t  nextGoalType  = 0;   ///< score::GoalType
    uint16_t makes         = 0;
    uint16_t attempts      = 0;
    uint16_t rest_s        = 0;
    uint16_t nextGoalValue = 0;
    StepDone() : SDK::MessageBase(STEP_DONE) {}
};

/// One user workout per message; a single {0, 0, {}} means the list is empty.
struct WorkoutItem : public SDK::MessageBase {
    uint8_t        index   = 0;
    uint8_t        count   = 0;
    score::Workout workout = {};
    WorkoutItem() : SDK::MessageBase(WORKOUT_ITEM) {}
};

/// sessionId 0 (stats zeroed) = no such session.
struct Results : public SDK::MessageBase {
    uint32_t            sessionId = 0;
    uint32_t            ymd       = 0;
    uint16_t            workoutId = 0;
    uint8_t             pad[2]    = {};
    score::SessionStats stats     = {};
    Results() : SDK::MessageBase(RESULTS) {}
};

/// One page of the index, newest first: items[0] is row `offset` of `total`; count rows here.
/// GET_HISTORY streams ceil(total / kTrendCount) pages (one {0, 0, 0} page when the index is empty).
struct History : public SDK::MessageBase {
    uint8_t               total  = 0;
    uint8_t               offset = 0;
    uint8_t               count  = 0;
    uint8_t               pad    = 0;
    score::SessionSummary items[score::kTrendCount] = {};
    History() : SDK::MessageBase(HISTORY) {}
};

struct LocalTime : public SDK::MessageBase {
    Clock clock;
    explicit LocalTime(const Clock& c = {}) : SDK::MessageBase(LOCAL_TIME), clock(c) {}
};

struct Saved : public SDK::MessageBase {
    uint32_t sessionId;   ///< 0 = nothing stored (the session was empty, or the save failed)
    explicit Saved(uint32_t id = 0) : SDK::MessageBase(SAVED), sessionId(id) {}
};

struct WorkoutSaved : public SDK::MessageBase {
    uint8_t workoutId;   ///< 0 = the save was refused
    uint8_t pad[3] = {};
    explicit WorkoutSaved(uint8_t id = 0) : SDK::MessageBase(WORKOUT_SAVED), workoutId(id) {}
};

/// The stored settings: sent on GUI start (before LOCAL_TIME and the first snapshot) and after every SET_SETTINGS.
struct SettingsMsg : public SDK::MessageBase {
    score::Settings settings;
    explicit SettingsMsg(const score::Settings& s = score::Settings::defaults())
        : SDK::MessageBase(SETTINGS), settings(s) {}
};

/// The index's per-spot records (SessionIndex::last / best), indexed by score::Spot.
struct SpotStats : public SDK::MessageBase {
    score::SpotStat last[score::kSpotCount] = {};
    score::SpotStat best[score::kSpotCount] = {};
    SpotStats() : SDK::MessageBase(SPOT_STATS) {}
};

static_assert(sizeof(StartOpen)     <= 256, "StartOpen exceeds the message block");
static_assert(sizeof(StartWorkout)  <= 256, "StartWorkout exceeds the message block");
static_assert(sizeof(Shot)          <= 256, "Shot exceeds the message block");
static_assert(sizeof(Undo)          <= 256, "Undo exceeds the message block");
static_assert(sizeof(Pause)         <= 256, "Pause exceeds the message block");
static_assert(sizeof(Resume)        <= 256, "Resume exceeds the message block");
static_assert(sizeof(SkipRest)      <= 256, "SkipRest exceeds the message block");
static_assert(sizeof(ExtendRest)    <= 256, "ExtendRest exceeds the message block");
static_assert(sizeof(ChangeSpot)    <= 256, "ChangeSpot exceeds the message block");
static_assert(sizeof(EndSession)    <= 256, "EndSession exceeds the message block");
static_assert(sizeof(GetSnapshot)   <= 256, "GetSnapshot exceeds the message block");
static_assert(sizeof(GetWorkouts)   <= 256, "GetWorkouts exceeds the message block");
static_assert(sizeof(SaveWorkout)   <= 256, "SaveWorkout exceeds the message block");
static_assert(sizeof(DeleteWorkout) <= 256, "DeleteWorkout exceeds the message block");
static_assert(sizeof(GetResults)    <= 256, "GetResults exceeds the message block");
static_assert(sizeof(GetHistory)    <= 256, "GetHistory exceeds the message block");
static_assert(sizeof(SetSettings)   <= 256, "SetSettings exceeds the message block");
static_assert(sizeof(GetSpotStats)  <= 256, "GetSpotStats exceeds the message block");
static_assert(sizeof(ClearHistory)  <= 256, "ClearHistory exceeds the message block");
static_assert(sizeof(SnapshotMsg)   <= 256, "SnapshotMsg exceeds the message block");
static_assert(sizeof(UndoResult)    <= 256, "UndoResult exceeds the message block");
static_assert(sizeof(StepDone)      <= 256, "StepDone exceeds the message block");
static_assert(sizeof(WorkoutItem)   <= 256, "WorkoutItem exceeds the message block");
static_assert(sizeof(Results)       <= 256, "Results exceeds the message block");
static_assert(sizeof(History)       <= 256, "History exceeds the message block");
static_assert(sizeof(LocalTime)     <= 256, "LocalTime exceeds the message block");
static_assert(sizeof(Saved)         <= 256, "Saved exceeds the message block");
static_assert(sizeof(WorkoutSaved)  <= 256, "WorkoutSaved exceeds the message block");
static_assert(sizeof(SettingsMsg)   <= 256, "SettingsMsg exceeds the message block");
static_assert(sizeof(SpotStats)     <= 256, "SpotStats exceeds the message block");

// No implicit padding: payload begins right after MessageBase and sizes add up exactly.
// (has_unique_object_representations can't apply to the messages themselves: MessageBase is
// polymorphic. The embedded score:: PODs and Clock assert it on their own.)
template <typename T>
constexpr bool payloadIs(size_t bytes) { return sizeof(T) == sizeof(SDK::MessageBase) + bytes; }

static_assert(payloadIs<StartOpen>(4) && payloadIs<StartWorkout>(4) && payloadIs<Shot>(4), "implicit padding");
static_assert(payloadIs<Undo>(0) && payloadIs<Pause>(0) && payloadIs<Resume>(0) && payloadIs<SkipRest>(0),
              "implicit padding");
static_assert(payloadIs<ExtendRest>(4) && payloadIs<ChangeSpot>(4) && payloadIs<EndSession>(4), "implicit padding");
static_assert(payloadIs<GetSnapshot>(0) && payloadIs<GetWorkouts>(0) && payloadIs<GetHistory>(0), "implicit padding");
static_assert(payloadIs<SaveWorkout>(sizeof(score::Workout) + 2), "implicit padding");
static_assert(payloadIs<DeleteWorkout>(4) && payloadIs<GetResults>(4), "implicit padding");
static_assert(payloadIs<SnapshotMsg>(sizeof(score::Snapshot)) && payloadIs<UndoResult>(4), "implicit padding");
static_assert(payloadIs<StepDone>(12), "implicit padding");
static_assert(alignof(score::Snapshot) <= 4 && alignof(SnapshotMsg) <= 4, "message blocks are only 4-byte aligned");
static_assert(payloadIs<WorkoutItem>(2 + sizeof(score::Workout)), "implicit padding");
static_assert(payloadIs<Results>(12 + sizeof(score::SessionStats)), "implicit padding");
static_assert(payloadIs<History>(4 + sizeof(score::SessionSummary) * score::kTrendCount), "implicit padding");
static_assert(payloadIs<History>(4 + 36 * 5), "History page layout changed");
static_assert(payloadIs<LocalTime>(sizeof(Clock)) && payloadIs<Saved>(4) && payloadIs<WorkoutSaved>(4),
              "implicit padding");
static_assert(payloadIs<SetSettings>(12) && payloadIs<SettingsMsg>(12), "implicit padding");
static_assert(payloadIs<GetSpotStats>(0) && payloadIs<ClearHistory>(0), "implicit padding");
static_assert(payloadIs<SpotStats>(104), "implicit padding");
static_assert(sizeof(score::SpotStat) == 4 && std::has_unique_object_representations<score::SpotStat>::value,
              "SpotStat: no padding");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
