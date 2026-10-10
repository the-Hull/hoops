/**
 ******************************************************************************
 * @file    SelfTest.hpp
 * @brief   Scripted walk through the screens, for headless simulator runs.
 *
 * Only built with HOOPS_SELFTEST defined (e.g. -DCMAKE_CXX_FLAGS=
 * -DHOOPS_SELFTEST=1); otherwise SelfTest.cpp compiles to nothing.
 * HOOPS_SELFTEST_PHASE picks the script: unset = the full walk; 1 = start an
 * open session, 5 shots (3 makes), then exit without ending it (a crash);
 * 2 = the restart: Resume session on Home, Pause with the counts intact.
 ******************************************************************************
 */

#ifndef SELF_TEST_HPP
#define SELF_TEST_HPP

#ifdef HOOPS_SELFTEST

class Model;

/// Start the script: button codes are sent to the active screen on LVGL
/// timers, as the kernel's would arrive; checks between steps log PASS /
/// FAIL, and a summary line ends the run.
void startSelfTest(Model& model);

#endif // HOOPS_SELFTEST

#endif // SELF_TEST_HPP
