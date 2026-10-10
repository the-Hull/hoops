/**
 * @file  Commands.hpp
 * @brief Service <-> GUI IPC contract: custom message ids and payload structs.
 *
 * Payloads are the score:: PODs copied verbatim; the GUI never computes stats.
 * Every struct must fit the kernel's 256-byte message block.
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <cstdint>
#include <ctime>

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "score/History.hpp"
#include "score/SessionEngine.hpp"
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

// ---------------------------------------------------------------- GUI --> Service

struct StartOpen : public SDK::MessageBase {
    uint8_t spot;   ///< score::Spot
    explicit StartOpen(uint8_t s = 0) : SDK::MessageBase(START_OPEN), spot(s) {}
};

struct StartWorkout : public SDK::MessageBase {
    uint8_t workoutId;   ///< preset 1..kPresetCount or user id >= kFirstUserWorkoutId
    explicit StartWorkout(uint8_t id = 0) : SDK::MessageBase(START_WORKOUT), workoutId(id) {}
};

struct Shot : public SDK::MessageBase {
    uint8_t made;   ///< 1 = make, 0 = miss
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
    explicit ExtendRest(uint16_t s = 15) : SDK::MessageBase(EXTEND_REST), seconds(s) {}
};

struct ChangeSpot : public SDK::MessageBase {
    uint8_t spot;   ///< score::Spot
    explicit ChangeSpot(uint8_t s = 0) : SDK::MessageBase(CHANGE_SPOT), spot(s) {}
};

struct EndSession : public SDK::MessageBase {
    uint8_t save;   ///< 1 = keep the session, 0 = discard
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
    explicit SaveWorkout(const score::Workout& w = {}) : SDK::MessageBase(SAVE_WORKOUT), workout(w) {}
};

struct DeleteWorkout : public SDK::MessageBase {
    uint8_t workoutId;
    explicit DeleteWorkout(uint8_t id = 0) : SDK::MessageBase(DELETE_WORKOUT), workoutId(id) {}
};

struct GetResults : public SDK::MessageBase {
    uint32_t sessionId;   ///< 0 = the session that just ended
    explicit GetResults(uint32_t id = 0) : SDK::MessageBase(GET_RESULTS), sessionId(id) {}
};

struct GetHistory : public SDK::MessageBase {
    GetHistory() : SDK::MessageBase(GET_HISTORY) {}
};

// ---------------------------------------------------------------- Service --> GUI

struct SnapshotMsg : public SDK::MessageBase {
    score::Snapshot snapshot;
    explicit SnapshotMsg(const score::Snapshot& s = {}) : SDK::MessageBase(SNAPSHOT), snapshot(s) {}
};

struct UndoResult : public SDK::MessageBase {
    uint8_t ok;     ///< 1 = a shot was removed
    uint8_t made;   ///< the removed shot was a make
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

struct Results : public SDK::MessageBase {
    uint32_t            sessionId = 0;
    uint32_t            ymd       = 0;
    uint16_t            workoutId = 0;
    score::SessionStats stats     = {};
    Results() : SDK::MessageBase(RESULTS) {}
};

/// Newest first.
struct History : public SDK::MessageBase {
    uint8_t               count = 0;
    score::SessionSummary items[score::kTrendCount] = {};
    History() : SDK::MessageBase(HISTORY) {}
};

struct LocalTime : public SDK::MessageBase {
    std::tm tm;
    explicit LocalTime(const std::tm& t = {}) : SDK::MessageBase(LOCAL_TIME), tm(t) {}
};

struct Saved : public SDK::MessageBase {
    uint32_t sessionId;
    explicit Saved(uint32_t id = 0) : SDK::MessageBase(SAVED), sessionId(id) {}
};

struct WorkoutSaved : public SDK::MessageBase {
    uint8_t workoutId;   ///< 0 = the save was refused
    explicit WorkoutSaved(uint8_t id = 0) : SDK::MessageBase(WORKOUT_SAVED), workoutId(id) {}
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
static_assert(sizeof(SnapshotMsg)   <= 256, "SnapshotMsg exceeds the message block");
static_assert(sizeof(UndoResult)    <= 256, "UndoResult exceeds the message block");
static_assert(sizeof(StepDone)      <= 256, "StepDone exceeds the message block");
static_assert(sizeof(WorkoutItem)   <= 256, "WorkoutItem exceeds the message block");
static_assert(sizeof(Results)       <= 256, "Results exceeds the message block");
static_assert(sizeof(History)       <= 256, "History exceeds the message block");
static_assert(sizeof(LocalTime)     <= 256, "LocalTime exceeds the message block");
static_assert(sizeof(Saved)         <= 256, "Saved exceeds the message block");
static_assert(sizeof(WorkoutSaved)  <= 256, "WorkoutSaved exceeds the message block");

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
