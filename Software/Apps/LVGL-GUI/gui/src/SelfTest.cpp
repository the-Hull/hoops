/**
 ******************************************************************************
 * @file    SelfTest.cpp
 * @brief   Scripted walk through the screens, for headless simulator runs.
 ******************************************************************************
 */

#include "gui/SelfTest.hpp"

#ifdef HOOPS_SELFTEST

#include <cstdint>
#include <cstring>

#include "lvgl.h"

#include "gui/model/Model.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

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
    LOG_INFO("snapshot: state %u mode %u spot %u spot %u/%u total %u/%u last5 %u:%02x t %lu s\n",
             s.state, s.mode, s.spot, s.spotMakes, s.spotAttempts, s.totalMakes, s.totalAttempts,
             s.last5Count, s.last5Bits, static_cast<unsigned long>(s.elapsed_s));
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
