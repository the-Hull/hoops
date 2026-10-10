/**
 ******************************************************************************
 * @file    SelfTest.cpp
 * @brief   Scripted walk through the screens, for headless simulator runs.
 ******************************************************************************
 */

#include "gui/SelfTest.hpp"

#ifdef HOOPS_SELFTEST

#include <cstdint>

#include "lvgl.h"

#include "gui/model/Model.hpp"
#include "gui/screens/ScreenManager.hpp"

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

// Two sessions: the first is played, changes spot (hold DOWN, navigation on
// the release), pauses, resumes and is saved (Results); the second is
// discarded through the double SELECT.
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

void tick(lv_timer_t* t)
{
    logSnapshot();
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
