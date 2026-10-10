/**
 ******************************************************************************
 * @file    SelfTest.cpp
 * @brief   Scripted walk through the screens, for headless simulator runs.
 ******************************************************************************
 */

#include "gui/SelfTest.hpp"

#ifdef HOOPS_SELFTEST

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "lvgl.h"

#include "gui/model/Model.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/widgets/Ring.hpp"

#define LOG_MODULE_PRX      "SelfTest"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{

Model* sModel = nullptr;

using Check = bool (*)(const Model& m);

struct Step {
    uint32_t    delayMs;   ///< wait before this step
    Check       check;     ///< runs first (on the state the earlier steps left), or nullptr
    const char* checkNote;
    char        key;       ///< SDK::GUI::Button code then sent
    const char* note;
};

bool onScreen(ScreenId id) { return ScreenManager::instance().current() == id; }

bool stillActive(const Model&) { return onScreen(ScreenId::Active); }

bool pickerOnCurrentSpot(const Model& m)
{
    return onScreen(ScreenId::SpotPicker) && m.pickerMode == Model::PickerMode::ChangeSpot && m.hasSnapshot()
        && m.pickerSpot == m.snapshot().spot;
}

bool backOnActive(const Model&) { return onScreen(ScreenId::Active); }
bool onPause(const Model&)      { return onScreen(ScreenId::Pause); }
bool onResults(const Model&)    { return onScreen(ScreenId::Results); }
bool onHome(const Model&)       { return onScreen(ScreenId::Home); }

/// A label under @p o whose text equals (or, with @p contains, holds)
/// @p text; @p color 0 = any colour.
bool findLabel(lv_obj_t* o, const char* text, uint32_t color = 0, bool contains = false)
{
    if (lv_obj_check_type(o, &lv_label_class)) {
        const char* t = lv_label_get_text(o);
        const bool match = contains ? strstr(t, text) != nullptr : strcmp(t, text) == 0;
        if (match && (color == 0 || lv_color_eq(lv_obj_get_style_text_color(o, LV_PART_MAIN), lv_color_hex(color)))) {
            return true;
        }
    }
    const uint32_t n = lv_obj_get_child_count(o);
    for (uint32_t i = 0; i < n; ++i) {
        if (findLabel(lv_obj_get_child(o, i), text, color, contains)) {
            return true;
        }
    }
    return false;
}

bool onScreenLabel(const char* text, uint32_t color = 0, bool contains = false)
{
    return findLabel(lv_screen_active(), text, color, contains);
}

// Session 3: 3PT Top 3/5 (three makes in a row), Mid L-Baseline 1/2, FT 1/2
// = 5/9, 56 %. Labels hold the text as drawn (" · " -> " - ").
bool resultsPage1(const Model& m)
{
    return onScreen(ScreenId::Results) && m.resultsPage == 0 && onScreenLabel("5/9 - ") && onScreenLabel("56 %")
        && onScreenLabel("SESSION - 1 / 4");
}
bool resultsPage2(const Model& m)
{
    return onScreen(ScreenId::Results) && m.resultsPage == 1 && onScreenLabel("3/5") && onScreenLabel("Top 3/5");
}
bool resultsPage3(const Model& m)
{
    return onScreen(ScreenId::Results) && m.resultsPage == 2 && onScreenLabel("3", Theme::kAccent)
        && onScreenLabel("STREAK");
}
// Trend on 3PT: this session 3/5 = 60 % against session 1's 4/4 = 100 %.
bool resultsPage4(const Model& m)
{
    return onScreen(ScreenId::Results) && m.resultsPage == 3 && onScreenLabel("TREND - 4 / 4")
        && onScreenLabel("3PT", Theme::kAccent) && onScreenLabel("60 %") && onScreenLabel("-40 vs last", Theme::kBad);
}
// The newest row is session 3, selected: "d.m. · Open" / "5/9 · 56 %".
bool historyLists(const Model& m)
{
    return onScreen(ScreenId::History) && m.hasHistory() && m.history().count > 0
        && onScreenLabel(" - Open", 0, true) && onScreenLabel("5/9 - 56 %");
}
bool resultsFromHistory(const Model& m)
{
    return resultsPage1(m) && m.resultsFrom == Model::ResultsFrom::History && m.hasResults()
        && m.results().sessionId == m.resultsSessionId && m.history().items[0].id == m.resultsSessionId;
}
bool onHistory(const Model&) { return onScreen(ScreenId::History); }

// --- Workouts -----------------------------------------------------------------

/// The Rest screen's result line, in either of its forms ("Step 1 done ·
/// 7/10 · " when it fits, else "Step 1 · 7/10 · "), before the coloured pct.
bool restResult(unsigned step, const char* ma)
{
    char full[40];
    char brief[40];
    snprintf(full, sizeof(full), "Step %u done - %s - ", step, ma);
    snprintf(brief, sizeof(brief), "Step %u - %s - ", step, ma);
    return onScreenLabel(full) || onScreenLabel(brief);
}

/// The bezel ring's arcs (read back from the screen's Ring), by colour.
struct RingArcs {
    Ring::Look look = Ring::Look::None;
    uint32_t total = 0, good = 0, bad = 0, line = 0, accent = 0, amber = 0;
};

RingArcs ringArcs()
{
    RingArcs r;
    const Ring* ring = Ring::find(lv_screen_active());
    if (ring) {
        r.look  = ring->look();
        r.total = ring->arcCount();
        for (uint8_t i = 0; i < ring->arcCount(); ++i) {
            const uint32_t c = ring->arcColor(i);
            r.good   += c == Theme::kGood ? 1 : 0;
            r.bad    += c == Theme::kBad ? 1 : 0;
            r.line   += c == Theme::kLine ? 1 : 0;
            r.accent += c == Theme::kAccent ? 1 : 0;
            r.amber  += c == Theme::kAmber ? 1 : 0;
        }
    }
    static const char* const kLooks[] = { "none", "progress", "segments", "dashed" };
    LOG_INFO("ring: %s, %lu arcs (good %lu bad %lu grey %lu teal %lu amber %lu)\n",
             kLooks[static_cast<uint8_t>(r.look)], static_cast<unsigned long>(r.total),
             static_cast<unsigned long>(r.good), static_cast<unsigned long>(r.bad), static_cast<unsigned long>(r.line),
             static_cast<unsigned long>(r.accent), static_cast<unsigned long>(r.amber));
    return r;
}

bool onWorkouts(const Model&) { return onScreen(ScreenId::Workouts) && onScreenLabel("WORKOUTS"); }
bool workoutsOnGameShots(const Model&)
{
    return onScreen(ScreenId::Workouts) && onScreenLabel("Game Shots") && onScreenLabel("4 steps - 40 shots");
}
// 40 shots x 12 s + 4 x 30 s rest = 600 s.
bool previewGameShots(const Model& m)
{
    return onScreen(ScreenId::WorkoutPreview) && m.selectedWorkoutId == 5 && onScreenLabel("GAME SHOTS", Theme::kAccent)
        && onScreenLabel("Rest 30 s - about 10 min") && onScreenLabel("START") && onScreenLabel("x10");
}
bool workoutStep(const Model& m, unsigned step, unsigned steps)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "STEP %u / %u", step, steps);
    return onScreen(ScreenId::Active) && m.hasSnapshot() && m.snapshot().mode == score::Snapshot::kModeWorkout
        && m.snapshot().stepIndex + 1u == step && onScreenLabel(buf, Theme::kDim);
}
bool gsStep1Start(const Model& m)
{
    const RingArcs r = ringArcs();
    return workoutStep(m, 1, 4) && onScreenLabel("10 shots left") && r.look == Ring::Look::Segments
        && r.total == 10 && r.line == 10;
}
// M M X M M: four green, one red, five grey segments; bits 0b11011.
bool gsStep1Mid(const Model& m)
{
    const RingArcs r = ringArcs();
    return workoutStep(m, 1, 4) && onScreenLabel("5 shots left") && m.snapshot().stepBits == 0x1Bu
        && r.look == Ring::Look::Segments && r.total == 10 && r.good == 4 && r.bad == 1 && r.line == 5;
}
bool gsStep1Last(const Model& m) { return workoutStep(m, 1, 4) && onScreenLabel("last shot!"); }
bool gsRest1(const Model& m)
{
    return onScreen(ScreenId::Rest) && m.snapshot().state == static_cast<uint8_t>(score::SessionState::Rest)
        && onScreenLabel("REST", Theme::kAmber) && onScreenLabel("NEXT - STEP 2 / 4")
        && onScreenLabel("R-Elbow x10", 0, true) && restResult(1, "7/10")
        && onScreenLabel("70 %", Theme::kGood) && m.hasStepDone() && m.lastStepDone().rest_s == 30;
}
bool restExtended(const Model& m)
{
    if (!onScreen(ScreenId::Rest) || m.snapshot().state != static_cast<uint8_t>(score::SessionState::Rest)) {
        return false;
    }
    char buf[12];
    Theme::fmtDuration(buf, sizeof(buf), m.snapshot().restRemaining_s);
    LOG_INFO("rest countdown after +15 s: %s\n", buf);
    return m.snapshot().restRemaining_s >= 40 && onScreenLabel(buf);
}
bool gsStep2(const Model& m) { return workoutStep(m, 2, 4) && onScreenLabel("MID - R-ELBOW", Theme::kAccent); }
bool gsRest2(const Model& m)
{
    return onScreen(ScreenId::Rest) && onScreenLabel("NEXT - STEP 3 / 4") && onScreenLabel("Free Throw x10")
        && restResult(2, "5/10");
}
bool restAfterResume(const Model& m)
{
    return onScreen(ScreenId::Rest) && m.snapshot().state == static_cast<uint8_t>(score::SessionState::Rest)
        && onScreenLabel("NEXT - STEP 3 / 4");
}
bool gsStep3(const Model& m) { return workoutStep(m, 3, 4) && onScreenLabel("FREE THROW", Theme::kAccent); }
bool gsRest3(const Model& m) { return onScreen(ScreenId::Rest) && onScreenLabel("NEXT - STEP 4 / 4"); }
bool gsStep4(const Model& m) { return workoutStep(m, 4, 4); }
// 7 + 5 + 8 + 6 makes of 40.
bool gsDone(const Model& m)
{
    return onScreen(ScreenId::Rest) && onScreenLabel("WORKOUT DONE", Theme::kGood) && onScreenLabel("26")
        && onScreenLabel("/40") && m.snapshot().totalAttempts == 40 && m.hasStepDone() && m.lastStepDone().workoutDone
        && onScreenLabel("UP - keep shooting");
}
bool activeDone(const Model& m)
{
    return onScreen(ScreenId::Active) && onScreenLabel("WORKOUT DONE", Theme::kGood) && onScreenLabel("extra shots")
        && (m.snapshot().flags & score::Snapshot::kWorkoutDone);
}
bool extraShot(const Model& m)
{
    return activeDone(m) && m.snapshot().totalAttempts == 41 && m.snapshot().stepAttempts == 1;
}
// Categories 3PT 7/10, Mid 5/10, FT 8/10, Layup 7/11 (with the extra make).
bool gsResultsCats(const Model& m)
{
    if (!onScreen(ScreenId::Results) || m.resultsPage != 1 || !m.hasResults()) {
        return false;
    }
    const score::SessionStats& st = m.results().stats;
    LOG_INFO("categories: %u/%u %u/%u %u/%u %u/%u\n", st.cat[0].makes, st.cat[0].attempts, st.cat[1].makes,
             st.cat[1].attempts, st.cat[2].makes, st.cat[2].attempts, st.cat[3].makes, st.cat[3].attempts);
    return st.cat[0].attempts == 10 && st.cat[1].attempts == 10 && st.cat[2].attempts == 10
        && st.cat[3].attempts == 11 && onScreenLabel("7/10") && onScreenLabel("5/10") && onScreenLabel("8/10")
        && onScreenLabel("7/11");
}
bool previewLadder(const Model& m)
{
    return onScreen(ScreenId::WorkoutPreview) && m.selectedWorkoutId == 4
        && onScreenLabel("MID-RANGE LADDER", Theme::kAccent) && onScreenLabel("5 mk");
}
// Makes goal: a progress ring (grey track + teal fill), not segments.
bool ladderProgressRing(const Model& m)
{
    const RingArcs r = ringArcs();
    return workoutStep(m, 1, 5) && onScreenLabel("2 makes left") && r.look == Ring::Look::Progress && r.total == 2
        && r.line == 1 && r.accent == 1;
}
bool ladderRest(const Model& m)
{
    const RingArcs r = ringArcs();
    return onScreen(ScreenId::Rest) && restResult(1, "5/7") && onScreenLabel("NEXT - STEP 2 / 5")
        && m.lastStepDone().attempts == 7 && r.look == Ring::Look::Progress && r.amber == 1;
}
bool previewMikan(const Model& m)
{
    return onScreen(ScreenId::WorkoutPreview) && m.selectedWorkoutId == 3 && onScreenLabel("MIKAN LAYUPS")
        && onScreenLabel("No rest - about 4 min");
}
// Free Throw 50: the largest segmented ring (50 arcs), then the switch to
// Pause, the costliest pair of trees for LVGL's pool.
bool ft50Segments(const Model& m)
{
    const RingArcs r = ringArcs();
    return workoutStep(m, 1, 1) && r.look == Ring::Look::Segments && r.total == 50 && r.good == 2 && r.bad == 1
        && r.line == 47
        && onScreenLabel("47 shots left");
}
bool onPauseFt50(const Model& m) { return onPause(m) && onScreenLabel("Step 1 / 1 - 2/3 - ", 0, true); }

// No rest after step 1: a toast on Active, step 2 already running.
bool mikanToast(const Model& m)
{
    return workoutStep(m, 2, 2) && onScreenLabel("Step 1 done - 6/10") && m.lastStepDone().rest_s == 0;
}

// Three sessions: the first is played, changes spot (hold DOWN, navigation
// on the release), pauses, resumes and is saved (Results); the second is
// discarded through the double SELECT; the third covers three spots and is
// saved, its results are paged through, then reopened from History.
const Step kSteps[] = {
    { 1500, nullptr, nullptr, '3', "Home: SELECT Open Session -> SpotPicker" },
    {  500, nullptr, nullptr, '2', "SpotPicker: DOWN" },
    {  500, nullptr, nullptr, '1', "SpotPicker: UP" },
    {  500, nullptr, nullptr, '3', "SpotPicker: SELECT -> startOpen, Active" },
    { 1200, nullptr, nullptr, '3', "Active: make" },
    {  400, nullptr, nullptr, '3', "Active: make" },
    {  400, nullptr, nullptr, '3', "Active: make" },
    {  400, nullptr, nullptr, '4', "Active: miss" },
    {  600, nullptr, nullptr, '1', "Active: undo (toast)" },
    { 2500, nullptr, nullptr, 'w', "Active: DOWN press" },
    {  600, stillActive, "hold fired, no switch before the release",
                             's', "Active: DOWN release after the hold -> SpotPicker (change spot)" },
    {  600, pickerOnCurrentSpot, "picker open in ChangeSpot mode on the current spot",
                             '2', "SpotPicker: DOWN" },
    {  500, nullptr, nullptr, '3', "SpotPicker: SELECT -> changeSpot, Active" },
    {  800, backOnActive, "back on Active",
                             '3', "Active: make" },
    {  600, nullptr, nullptr, '2', "Active: DOWN -> pause, Pause" },
    { 1500, onPause, "on Pause",
                             '3', "Pause: SELECT Resume -> Active" },
    { 1200, backOnActive, "resumed onto Active (stale Paused ignored)",
                             '2', "Active: DOWN -> pause, Pause" },
    {  800, nullptr, nullptr, '2', "Pause: DOWN -> End & save" },
    {  500, nullptr, nullptr, '3', "Pause: SELECT End & save -> SAVED -> Results" },
    { 1500, onResults, "on Results after SAVED",
                             '4', "Results: BACK -> Home" },
    { 1000, nullptr, nullptr, '3', "Home: SELECT Open Session -> SpotPicker" },
    {  500, nullptr, nullptr, '3', "SpotPicker: SELECT -> startOpen, Active" },
    { 1000, backOnActive, "new session on Active (stale Ended ignored)",
                             '3', "Active: make" },
    {  600, nullptr, nullptr, '2', "Active: DOWN -> pause, Pause" },
    {  800, nullptr, nullptr, '2', "Pause: DOWN" },
    {  400, nullptr, nullptr, '2', "Pause: DOWN -> Discard" },
    {  500, nullptr, nullptr, '3', "Pause: SELECT Discard -> confirm" },
    { 3500, onPause, "still on Pause (confirm window lapsed)",
                             '3', "Pause: SELECT after the 3 s window -> confirm again" },
    {  500, nullptr, nullptr, '3', "Pause: SELECT -> discard, Home" },
    { 1500, onHome, "on Home after discard",
                             '3', "Home: SELECT Open Session -> SpotPicker" },
    // Session 3: three spots in three categories, a 3-make streak, End & save.
    {  500, nullptr, nullptr, '3', "SpotPicker: SELECT (3PT Top) -> Active" },
    { 1200, nullptr, nullptr, '3', "Active: make" },
    {  400, nullptr, nullptr, '3', "Active: make" },
    {  400, nullptr, nullptr, '3', "Active: make" },
    {  400, nullptr, nullptr, '4', "Active: miss" },
    {  400, nullptr, nullptr, '4', "Active: miss" },
    {  600, nullptr, nullptr, 'w', "Active: DOWN press" },
    {  600, nullptr, nullptr, 's', "Active: DOWN release -> SpotPicker" },
    {  600, nullptr, nullptr, '2', "SpotPicker: DOWN" },
    {  300, nullptr, nullptr, '2', "SpotPicker: DOWN" },
    {  300, nullptr, nullptr, '2', "SpotPicker: DOWN (Mid L-Baseline)" },
    {  400, nullptr, nullptr, '3', "SpotPicker: SELECT -> changeSpot, Active" },
    {  800, nullptr, nullptr, '3', "Active: make" },
    {  400, nullptr, nullptr, '4', "Active: miss" },
    {  600, nullptr, nullptr, 'w', "Active: DOWN press" },
    {  600, nullptr, nullptr, 's', "Active: DOWN release -> SpotPicker" },
    {  600, nullptr, nullptr, '2', "SpotPicker: DOWN" },
    {  300, nullptr, nullptr, '2', "SpotPicker: DOWN" },
    {  300, nullptr, nullptr, '2', "SpotPicker: DOWN" },
    {  300, nullptr, nullptr, '2', "SpotPicker: DOWN" },
    {  300, nullptr, nullptr, '2', "SpotPicker: DOWN (Free Throw)" },
    {  400, nullptr, nullptr, '3', "SpotPicker: SELECT -> changeSpot, Active" },
    {  800, nullptr, nullptr, '4', "Active: miss" },
    {  400, nullptr, nullptr, '3', "Active: make" },
    {  600, nullptr, nullptr, '2', "Active: DOWN -> pause, Pause" },
    {  800, nullptr, nullptr, '2', "Pause: DOWN -> End & save" },
    {  500, nullptr, nullptr, '3', "Pause: SELECT End & save -> Results" },
    { 1500, resultsPage1, "Results page 1: total 5/9 - 56 %",
                             '1', "Results 1: UP (previous spot)" },
    {  500, nullptr, nullptr, '2', "Results 1: DOWN (next spot)" },
    {  500, nullptr, nullptr, '3', "Results 1: SELECT -> page 2" },
    {  500, resultsPage2, "Results page 2: 3PT 3/5, footer Top 3/5",
                             '2', "Results 2: DOWN (Mid-range)" },
    {  500, nullptr, nullptr, '1', "Results 2: UP (3PT)" },
    {  500, nullptr, nullptr, '3', "Results 2: SELECT -> page 3" },
    {  500, resultsPage3, "Results page 3: STREAK 3",
                             '3', "Results 3: SELECT -> page 4" },
    {  500, resultsPage4, "Results page 4: trend",
                             '1', "Results 4: UP" },
    {  500, nullptr, nullptr, '2', "Results 4: DOWN" },
    {  500, nullptr, nullptr, '3', "Results 4: SELECT -> page 1" },
    {  500, resultsPage1, "Results wrapped to page 1",
                             '4', "Results: BACK -> Home" },
    { 1000, onHome, "on Home after Results",
                             '2', "Home: DOWN (Workouts)" },
    {  400, nullptr, nullptr, '2', "Home: DOWN (History)" },
    {  400, nullptr, nullptr, '3', "Home: SELECT -> History" },
    { 1000, historyLists, "History lists the saved session",
                             '3', "History: SELECT -> Results of that session" },
    { 1500, resultsFromHistory, "Results page 1 of the session opened from History",
                             '4', "Results: BACK -> History" },
    { 1000, onHistory, "back on History",
                             '4', "History: BACK -> Home" },
    { 1000, onHome, "on Home after History",
                             '1', "Home: UP (Workouts)" },
    {  400, nullptr, nullptr, '3', "Home: SELECT -> Workouts" },
    { 1000, onWorkouts, "on Workouts",
                             '2', "Workouts: DOWN" },
    {  300, nullptr, nullptr, '2', "Workouts: DOWN" },
    {  300, nullptr, nullptr, '2', "Workouts: DOWN" },
    {  300, nullptr, nullptr, '2', "Workouts: DOWN (Game Shots)" },
    {  400, workoutsOnGameShots, "Workouts: Game Shots, 4 steps - 40 shots",
                             '3', "Workouts: SELECT -> Preview" },
    {  800, previewGameShots, "Preview: GAME SHOTS, rest 30 s, about 10 min",
                             '3', "Preview: SELECT START -> Active (workout)" },
    { 1200, gsStep1Start, "Active workout: STEP 1 / 4, 10 grey segments",
                             '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  500, gsStep1Mid, "segments: 4 green, 1 red, 5 grey (stepBits 0x1B)",
                             '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  500, gsStep1Last, "one shot left: last shot!",
                             '3', "Active: make (10th: step done)" },
    {  800, gsRest1, "Rest header REST (amber), NEXT - STEP 2 / 4, Step 1 (done) - 7/10 - 70 %",
                             '1', "Rest: UP +15 s" },
    {  800, restExtended, "countdown after +15 s >= 0:40",
                             '3', "Rest: SELECT skip -> Active step 2" },
    {  800, gsStep2, "Active: STEP 2 / 4 at MID - R-ELBOW",
                             '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  800, gsRest2, "Rest: NEXT - STEP 3 / 4, Free Throw x10",
                             '2', "Rest: DOWN -> pause, Pause" },
    { 1000, onPause, "on Pause from Rest",
                             '3', "Pause: SELECT Resume" },
    { 1000, restAfterResume, "resumed back into Rest",
                             '3', "Rest: SELECT skip -> Active step 3" },
    {  800, gsStep3, "Active: STEP 3 / 4 at FREE THROW",
                             '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  800, gsRest3, "Rest: NEXT - STEP 4 / 4",
                             '3', "Rest: SELECT skip -> Active step 4" },
    {  800, gsStep4, "Active: STEP 4 / 4",
                             '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  800, gsDone, "done mode: WORKOUT DONE (green), 26/40",
                             '1', "Rest (done): UP keep shooting -> Active" },
    {  800, activeDone, "Active: WORKOUT DONE header, extra shots",
                             '3', "Active: make (extra shot)" },
    {  600, extraShot, "extra shot counted: total 41",
                             '2', "Active: DOWN -> pause, Pause" },
    {  800, nullptr, nullptr, '2', "Pause: DOWN -> End & save" },
    {  500, nullptr, nullptr, '3', "Pause: SELECT End & save -> Results" },
    { 1500, onResults, "on Results after the workout",
                             '3', "Results 1: SELECT -> page 2" },
    {  600, gsResultsCats, "Results page 2: 4 categories, attempts 10/10/10/11",
                             '4', "Results: BACK -> Home" },
    // Mid-range Ladder: Makes goals, a progress ring.
    { 1000, onHome, "on Home after the workout",
                             '3', "Home: SELECT -> Workouts" },
    { 1000, nullptr, nullptr, '1', "Workouts: UP (Mid-range Ladder)" },
    {  400, nullptr, nullptr, '3', "Workouts: SELECT -> Preview" },
    {  800, previewLadder, "Preview: MID-RANGE LADDER, 5 mk rows",
                             '3', "Preview: SELECT START -> Active" },
    { 1200, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  500, ladderProgressRing, "Makes goal: progress ring (track + teal fill), 2 makes left",
                             '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  800, ladderRest, "Rest after 5 makes in 7 shots, amber ring",
                             '2', "Rest: DOWN -> pause, Pause" },
    {  800, nullptr, nullptr, '2', "Pause: DOWN" },
    {  400, nullptr, nullptr, '2', "Pause: DOWN -> Discard" },
    {  400, nullptr, nullptr, '3', "Pause: SELECT Discard -> confirm" },
    {  400, nullptr, nullptr, '3', "Pause: SELECT -> discard, Home" },
    // Mikan Layups: no rest between the steps -> toast.
    { 1500, onHome, "on Home after discarding the ladder",
                             '3', "Home: SELECT -> Workouts" },
    { 1000, nullptr, nullptr, '1', "Workouts: UP (Mikan Layups)" },
    {  400, nullptr, nullptr, '3', "Workouts: SELECT -> Preview" },
    {  800, previewMikan, "Preview: MIKAN LAYUPS, no rest",
                             '3', "Preview: SELECT START -> Active" },
    { 1200, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  500, mikanToast, "zero-rest step: toast Step 1 done - 6/10, STEP 2 / 2",
                             '2', "Active: DOWN -> pause, Pause" },
    {  800, nullptr, nullptr, '2', "Pause: DOWN" },
    {  400, nullptr, nullptr, '2', "Pause: DOWN -> Discard" },
    {  400, nullptr, nullptr, '3', "Pause: SELECT Discard -> confirm" },
    {  400, nullptr, nullptr, '3', "Pause: SELECT -> discard, Home" },
    // Free Throw 50: 50 segments, then Pause.
    { 1500, onHome, "on Home after discarding Mikan Layups",
                             '3', "Home: SELECT -> Workouts" },
    { 1000, nullptr, nullptr, '1', "Workouts: UP (Free Throw 50)" },
    {  400, nullptr, nullptr, '3', "Workouts: SELECT -> Preview" },
    {  800, nullptr, nullptr, '3', "Preview: SELECT START -> Active" },
    { 1200, nullptr, nullptr, '3', "Active: make" },
    {  300, nullptr, nullptr, '4', "Active: miss" },
    {  300, nullptr, nullptr, '3', "Active: make" },
    {  500, ft50Segments, "Free Throw 50: 50 segments (2 green, 1 red)",
                             '2', "Active: DOWN -> pause, Pause" },
    { 1000, onPauseFt50, "on Pause from a 50-segment Active",
                             '2', "Pause: DOWN" },
    {  400, nullptr, nullptr, '2', "Pause: DOWN -> Discard" },
    {  400, nullptr, nullptr, '3', "Pause: SELECT Discard -> confirm" },
    {  400, nullptr, nullptr, '3', "Pause: SELECT -> discard, Home" },
    { 1500, onHome, "on Home after the workouts",
                             '4', "Home: BACK -> exit app" },
};
constexpr uint32_t kCount = sizeof(kSteps) / sizeof(kSteps[0]);

uint32_t sIndex  = 0;
uint32_t sPassed = 0;
uint32_t sFailed = 0;

void logSnapshot()
{
    if (!sModel->hasSnapshot()) {
        return;
    }
    const score::Snapshot& s = sModel->snapshot();
    LOG_INFO("snapshot: state %u mode %u spot %u spot %u/%u total %u/%u last5 %u:%02x t %lu s"
             " step %u/%u %u/%u bits %lx rest %u flags %x\n",
             s.state, s.mode, s.spot, s.spotMakes, s.spotAttempts, s.totalMakes, s.totalAttempts,
             s.last5Count, s.last5Bits, static_cast<unsigned long>(s.elapsed_s), s.stepIndex + 1, s.stepCount,
             s.stepMakes, s.stepAttempts, static_cast<unsigned long>(s.stepBits), s.restRemaining_s, s.flags);
}

void logPool()
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    LOG_INFO("pool: %u B used, peak %u B (screen %u, results page %u)\n",
             static_cast<unsigned>(mon.total_size - mon.free_size), static_cast<unsigned>(mon.max_used),
             static_cast<unsigned>(ScreenManager::instance().current()), static_cast<unsigned>(sModel->resultsPage));
}

void tick(lv_timer_t* t)
{
    logSnapshot();
    logPool();
    const Step& step = kSteps[sIndex++];
    if (step.check) {
        const bool ok = step.check(*sModel);
        ok ? ++sPassed : ++sFailed;
        LOG_INFO("CHECK %s: %s (screen %u)\n", ok ? "PASS" : "FAIL", step.checkNote,
                 static_cast<unsigned>(ScreenManager::instance().current()));
    }
    if (sIndex == kCount) {
        // The last step may exit the app: report before sending it.
        LOG_INFO("done: %lu checks passed, %lu failed\n", static_cast<unsigned long>(sPassed),
                 static_cast<unsigned long>(sFailed));
    }
    LOG_INFO("step %lu key '%c' on screen %u: %s\n", static_cast<unsigned long>(sIndex), step.key,
             static_cast<unsigned>(ScreenManager::instance().current()), step.note);
    uint32_t key = static_cast<uint32_t>(step.key);
    lv_obj_send_event(lv_screen_active(), LV_EVENT_KEY, &key);
    if (sIndex < kCount) {
        lv_timer_set_period(t, kSteps[sIndex].delayMs);
    } else {
        lv_timer_delete(t);
    }
}

} // namespace

void startSelfTest(Model& model)
{
    sModel = &model;
    lv_timer_create(&tick, kSteps[0].delayMs, nullptr);
}

#endif // HOOPS_SELFTEST
