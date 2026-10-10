/**
 ******************************************************************************
 * @file    GuiApp.cpp
 * @brief   Hoops GUI entry: builds the model and shows the first screen.
 *
 * una_lvgl_app_init() is the hook the SDK's LVGL entry point calls once LVGL
 * and the display are up. Objects are constructed here, not as globals, so
 * nothing touches the kernel before main() has bound it.
 *
 * Built with HOOPS_SELFTEST defined, it also plays a scripted walk through
 * the screens (button codes sent to the active screen, as the kernel's would
 * arrive) and logs the model's snapshot after each step. Off by default: the
 * block compiles to nothing.
 ******************************************************************************
 */

#include <new>

#include "gui/Assets.hpp"
#include "gui/model/Model.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace
{

alignas(Model) uint8_t sModelStorage[sizeof(Model)];
Model* sModel = nullptr;

} // namespace

#ifdef HOOPS_SELFTEST

#define LOG_MODULE_PRX      "SelfTest"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace
{

struct SelfTestStep {
    uint32_t    delayMs;   ///< wait before sending the key
    char        key;       ///< SDK::GUI::Button code
    const char* note;
};

// Two sessions: the first is played, changes spot, pauses, resumes and is
// saved (Results); the second is discarded through the double SELECT.
const SelfTestStep kSelfTest[] = {
    { 1500, '3', "Home: SELECT Open Session -> SpotPicker" },
    {  500, '2', "SpotPicker: DOWN" },
    {  500, '1', "SpotPicker: UP" },
    {  500, '3', "SpotPicker: SELECT -> startOpen, Active" },
    { 1200, '3', "Active: make" },
    {  400, '3', "Active: make" },
    {  400, '3', "Active: make" },
    {  400, '4', "Active: miss" },
    {  600, '1', "Active: undo (toast)" },
    { 2500, 'w', "Active: DOWN press (toast gone)" },
    {  800, 's', "Active: DOWN release after hold -> SpotPicker (change spot)" },
    {  600, '2', "SpotPicker: DOWN" },
    {  500, '3', "SpotPicker: SELECT -> changeSpot, Active" },
    {  800, '3', "Active: make" },
    {  600, '2', "Active: DOWN -> pause, Pause" },
    { 1500, '3', "Pause: SELECT Resume -> Active" },
    { 1200, '2', "Active: DOWN -> pause, Pause" },
    {  800, '2', "Pause: DOWN -> End & save" },
    {  500, '3', "Pause: SELECT End & save -> SAVED -> Results" },
    { 1500, '4', "Results: BACK -> Home" },
    { 1000, '3', "Home: SELECT Open Session -> SpotPicker" },
    {  500, '3', "SpotPicker: SELECT -> startOpen, Active" },
    { 1000, '3', "Active: make" },
    {  600, '2', "Active: DOWN -> pause, Pause" },
    {  800, '2', "Pause: DOWN" },
    {  400, '2', "Pause: DOWN -> Discard" },
    {  500, '3', "Pause: SELECT Discard -> confirm" },
    { 3500, '3', "Pause: SELECT after the 3 s window -> confirm again" },
    {  500, '3', "Pause: SELECT -> discard, Home" },
    { 1500, '4', "Home: BACK -> exit app" },
};
constexpr uint32_t kSelfTestCount = sizeof(kSelfTest) / sizeof(kSelfTest[0]);
uint32_t sSelfTestIndex = 0;

void selfTestStep(lv_timer_t* t)
{
    if (sModel->hasSnapshot()) {
        const score::Snapshot& s = sModel->snapshot();
        LOG_INFO("snapshot: state %u mode %u spot %u spot %u/%u total %u/%u last5 %u:%02x t %lu s\n",
                 s.state, s.mode, s.spot, s.spotMakes, s.spotAttempts, s.totalMakes, s.totalAttempts,
                 s.last5Count, s.last5Bits, static_cast<unsigned long>(s.elapsed_s));
    }
    if (sSelfTestIndex >= kSelfTestCount) {
        LOG_INFO("done\n");
        lv_timer_delete(t);
        return;
    }
    const SelfTestStep& step = kSelfTest[sSelfTestIndex++];
    LOG_INFO("step %lu key '%c' on screen %u: %s\n", static_cast<unsigned long>(sSelfTestIndex), step.key,
             static_cast<unsigned>(ScreenManager::instance().current()), step.note);
    uint32_t key = static_cast<uint32_t>(step.key);
    lv_obj_send_event(lv_screen_active(), LV_EVENT_KEY, &key);
    if (sSelfTestIndex < kSelfTestCount) {
        lv_timer_set_period(t, kSelfTest[sSelfTestIndex].delayMs);
    }
}

void startSelfTest()
{
    lv_timer_create(&selfTestStep, kSelfTest[0].delayMs, nullptr);
}

} // namespace

#endif // HOOPS_SELFTEST

/**
 * @brief LVGL's default font (LV_FONT_DEFAULT in the SDK's lv_conf.h).
 *
 * Returning one of the app's own faces keeps LVGL's built-in Montserrat out
 * of the link. Every label sets its font explicitly, so this only backs
 * widgets created without one.
 */
extern "C" const lv_font_t* una_lvgl_default_font(void)
{
    return &poppins_regular_18;
}

extern "C" void una_lvgl_app_init(void)
{
    Theme::init();
    sModel = new (sModelStorage) Model();
    ScreenManager::instance().start(*sModel, ScreenId::Home);
#ifdef HOOPS_SELFTEST
    startSelfTest();
#endif
}
