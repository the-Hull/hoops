# Hoops (ScoreTrack) — UNA Watch app

Basketball shooting tracker for the UNA Watch, built on the UNA SDK v1.6.0-rc1 (checkout at ~/una-sdk) with the
**LVGL** GUI (no TouchGFX — the developer is on Ubuntu, where TouchGFX Designer
doesn't exist). Scaffolded from the SDK tutorial `Docs/Tutorials/Buttons` (LVGL variant) with fonts from
`Examples/Apps/RunLVGL`; RunLVGL remains the reference for the Model + ScreenManager + Theme pattern.

Competition: UNA App Competition, deadline **30 Oct 2026, 16:00 UK**. Judged on
Usefulness, Originality, UX/UI (10 pts each). Must be published on
apps.unawatch.com and submitted via the form. A table-tennis app may follow as a
second entry (max 2 per person) — keep the scoring engine reusable.

- Feature spec: `docs/feature-spec.md`; design spec: `docs/superpowers/specs/2026-10-10-hoops-mvp-design.md`;
  implementation plan (Tasks 1–15, all done): `docs/superpowers/plans/2026-10-10-hoops-mvp.md`.
- Screen mockups (14 screens): `docs/mockups/*.dc.html`; measured layout per screen: `docs/mockup-layout.md`.
  The `.dc.html` files are design-canvas sources — they do not render standalone, but the markup holds the exact
  layout, colours, text and labels (the black 300×300 circle is the 240×240 screen at 1.25×). Rendered canvas:
  https://claude.ai/artifact/EZBpMUafRTmC5CggUc3uvM

## Architecture (as built)

Two processes, as every UNA app:

- **Service** (`Software/Libs/Sources/Service.cpp`, `Storage.cpp`): owns the session engine, all
  statistics, storage, haptics (vibro + backlight), heart rate and wrist-raise sensors. One `onSecond()` per
  wall-clock second drives timers; `current.bin` is saved after every input and every 5 running seconds.
- **GUI** (`Software/Apps/LVGL-GUI/gui/`): `Model` (latest copy of what the service sent + navigation memory),
  `ScreenManager`, `Theme`, widgets (`ListMenu`, `Ring`, `Court`, `Hints`, `KeyInput`) and one class per
  screen. It computes no statistics and touches no files.
- **Engine** (`Software/Libs/score/`): plain C++17, no SDK/GUI dependencies, unit-tested on the host —
  `Spots`, `Session` (≤ 400 shots), `Workout` (+ 5 presets, ≤ 8 steps, ≤ 10 user workouts), `SessionEngine`
  (open/workout sessions, rest, undo, pause, crash `restore()`, `Snapshot`), `Stats`, `History`
  (`SessionIndex`, 50 rows, trend of 5), `Serializer`.
- **IPC** (`Software/Libs/Header/Commands.hpp`): padding-free POD messages. `0x01xx` GUI → Service
  (START_OPEN … GET_HISTORY), `0x02xx` Service → GUI (SNAPSHOT, UNDO_RESULT, STEP_DONE, WORKOUT_ITEM, RESULTS,
  HISTORY, LOCAL_TIME, SAVED, WORKOUT_SAVED). A RESULTS reply with `sessionId 0` means "none"; `GET_RESULTS{0}`
  = the session just ended.
- **Storage** (app directory; simulator: `Software/Output/`). Every file = 12-byte header {magic, size,
  FNV-1a checksum} + the raw POD:
  - `workouts.bin` ('HWK1', `WorkoutStore`) and `sessions/index.bin` ('HIX1', `SessionIndex`): written as
    `<path>.tmp` then renamed over the target; a lone `.tmp` is recovered on load.
  - `current.bin` ('HSS1', `Session`): the running session, for crash resume (restored → Paused, `kResumable`);
    written atomically like the others. End & save: session file → index row → evicted file → index →
    remove current.bin, so a crash at any point loses nothing (a current.bin whose id is already indexed is
    dropped on start). A restored workout session whose step no longer fits the (edited) workout continues
    as an open session.
  - `sessions/<id>.bin` ('HSS1'): one per saved session; evicted with its index row (oldest beyond 50).
- **Screens** (`ScreenId` order): Home, SpotPicker, Workouts, WorkoutPreview, StepBuilder, Active (open and
  workout mode, undo toast), Rest (also the workout-done summary), Pause, Results (4 pages: court, categories,
  highlights, trend), History.
- LVGL pool `LV_MEM_SIZE` 56 KB (both CMake files): peak ≈ 23 KB (Free Throw 50 → Pause switch). RAM: GUI
  data+bss ≈ 156 KB of 600 KB, service ≈ 19 KB of 500 KB.

## Environment

- Ubuntu, VS Code. `~/una-sdk` (SDK v1.6.0-rc1+; `UNA_SDK` points here — exported in `~/.bashrc`, so non-login
  shells may need `export UNA_SDK=$HOME/una-sdk`), `~/work/006_una_hoops` (this repo).
- Python venv in `.venv/` — the watch build is configured with `-DUNA_PYTHON_EXECUTABLE=…/.venv/bin/python3`.
- ARM compiler: STM32CubeCLT 1.22 in `/opt/st/stm32cubeclt_*/GNU-tools-for-STM32/bin`
  (`find /opt/st -name arm-none-eabi-gcc`). Do NOT use Ubuntu's `gcc-arm-none-eabi`.
- No `.vscode/` or `setup-una-dev.sh` in this repo (an external `setup-una-dev.sh` set the machine up).

## Build & run

All paths from the repo root.

```bash
# Host unit tests (engine only; fetches googletest on first configure)
cmake -S Tests/Host -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host && ctest --test-dir build-host --output-on-failure      # 66 tests

# Simulator (needs libsdl2-dev; run from build/bin — the mock file system is ../../../../../Output = Software/Output)
cd Software/Apps/LVGL-GUI/simulator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build
(cd build/bin && ./HoopsSimulator)
(cd build/bin && SDL_VIDEODRIVER=dummy timeout 6 ./HoopsSimulator)   # headless smoke: expect "LVGL pool: … (screen 0)"

# Watch app (.uapp) — the SDK's copy into Output/ can fail; copy build/*.uapp by hand if so
cd Software/Apps/Hoops-CMake
cmake -S . -B build -DUNA_PYTHON_EXECUTABLE=$PWD/../../../.venv/bin/python3 && cmake --build build -j
# -> build/Hoops_<version>.uapp; version comes from git tags. RAM check:
arm-none-eabi-size build/HoopsGUI.elf build/HoopsService.elf
```

**Self-test** (scripted key presses + PASS/FAIL checks, `gui/src/SelfTest.cpp`, compiled only with
`HOOPS_SELFTEST`). Its binary also lands in `build/bin`, so rebuild the normal one afterwards:

```bash
cd Software/Apps/LVGL-GUI/simulator
cmake -S . -B build-selftest -G Ninja -DCMAKE_CXX_FLAGS=-DHOOPS_SELFTEST=1 && cmake --build build-selftest
rm -f ../../../Output/workouts.bin ../../../Output/current.bin    # fresh store, no session in progress
(cd build/bin && SDL_VIDEODRIVER=dummy timeout 400 stdbuf -oL ./HoopsSimulator | grep -E "CHECK|done:")
#   full walk: "done: 89 checks passed, 0 failed"
# Crash resume: phase 1 starts an open session, 5 shots (3 makes), then exits without ending it
# (kill -9 it after "phase 1 ready" for a real crash); phase 2 restarts, checks that neither an open
# session nor a workout can start while it is in progress (toasts), then Resume -> Pause 3/5 (9 checks).
(cd build/bin && HOOPS_SELFTEST_PHASE=1 SDL_VIDEODRIVER=dummy timeout 60 ./HoopsSimulator >/dev/null;
                 HOOPS_SELFTEST_PHASE=2 SDL_VIDEODRIVER=dummy timeout 60 stdbuf -oL ./HoopsSimulator | grep -E "CHECK|done:")
rm -rf build-selftest build/bin/HoopsSimulator && cmake --build build     # back to the normal binary
```

Fonts: Poppins C files copied from RunLVGL (large faces carry digits only — see RunLVGL's `gen_assets.py`
for ranges). No `gen_assets.py` here yet; copy/adapt RunLVGL's when new glyphs or images are needed. The faces
have no middle dot: `Theme::setText` renders " · " as " - " (tests match the drawn text).

**Simulator keys:** `1/2/3/4` = UP/DOWN/SELECT/BACK click (L1/L2/R1/R2); `q/w/e/r` press, `a/s/d/f` release
(use for long-press, e.g. `w` … `s` = hold DOWN); `5` = wrist raise; `Esc` = clean exit.
Vibration/buzzer/backlight appear only as `Mock.Vibro::play` / `Mock.Buzzer::play` / `Mock.Backlight::on` log
lines. Accelerometer/gyro are NOT simulated. Sensor sim config: `simulator/ConfigurationSimulator.hpp`.

**Deploy to watch:** USB mass storage → create `Apps/Hoops/` → copy `.uapp` → eject → reboot watch.
**Publish:** apps.unawatch.com → Add New → copy App ID into `APP_ID` in `Software/Apps/Hoops-CMake/CMakeLists.txt`
→ rebuild → `app-manifest.json` + `python3 $UNA_SDK/Utilities/Scripts/app_packer/min_kernel_version.py --stamp app-manifest.json`
→ zip `.uapp` + `icon.png` + manifest → Version → Upload New → Release.

## Hardware & SDK facts (SDK v1.6.0-rc1 (checkout at ~/una-sdk); online docs are v1.5.0)

- Display: 1.2" round MIP LCD, 240×240, 64 colours (ABGR2222), LED backlight, no touch.
- Buttons: L1 top-left (UP), L2 bottom-left (DOWN), R1 top-right (SELECT/START/STOP), R2 bottom-right (BACK).
  LVGL: key events via `LV_EVENT_KEY` → `Screen::onKey(code)`, one code per 100 ms frame. The kernel sends PRESS,
  RELEASE and CLICK (only for holds < 500 ms); the simulator sends CLICK before RELEASE, the watch's order is not
  documented, so `KeyInput` does not depend on it: every press ends as exactly one Click or HoldRelease (a short
  RELEASE without CLICK waits `kReleaseGraceMs` = 150 ms, one 100 ms frame plus jitter, for a late CLICK, else
  becomes a HoldRelease via the callback; after a 500 ms Hold the RELEASE is a HoldRelease at once). Screens act on `KeyInput::isActivate()` in `Screen::onAction()`
  (feed() results and callback deliveries alike), so a slow press is never lost and a short one never counts
  twice; screens that navigate on a hold do it on HoldRelease, so the release can't reach the next screen.
- Sensors (`SDK::Sensor::Type`): ACCELEROMETER, GYROSCOPE, FUSION, HEART_RATE, WRIST_MOTION, MOTION_DETECT,
  ACTIVITY_RECOGNITION, GESTURE_RECOGNITION (undocumented), TOUCH_DETECT. `SDK::Sensor::Connection(type, period,
  latency)`; batched `EventData`.
- Feedback: `SDK::Message::RequestVibroPlay` (≤ 4 effects per series here), `RequestBacklightSet`
  {brightness, autoOffTimeoutMs}. Mock FS `rename` is POSIX (overwrites); the watch FS may not — Storage
  removes the target and retries.
- Docs: https://www.developers.unawatch.com/sdk-v1.5.0/ (index: genindex.html). Pick pages by title.

## Decided UX (mockups are the source of truth where the spec differs)

- Menus: UP/DOWN scroll, SELECT open/confirm, BACK back. Headers are Regular14 (mockups' 11–13 px).
- Shooting: **SELECT = MAKE** (1 buzz), **BACK = MISS** (2 buzzes), **UP = undo** (3 buzzes + 2 s toast
  "Make removed" / "Miss removed" / "Nothing to undo"), **DOWN = pause**, **hold DOWN (500 ms) = change spot**
  (open session; toast "Release to change spot" while held, picker opens on release; in a workout a hold pauses).
  Every shot / undo / step end / skip / save turns the backlight on; menu navigation does not.
- No exit mid-session: leaving only via Pause (Resume / End & save / Discard; Discard asks twice within 3 s;
  BACK = resume). BACK on Home during a session goes to Pause.
- Crash resume: a restored session is Paused; Home shows "Resume session" first (selected) and an amber
  "Session in progress" line under the header; Resume opens Pause.
- One session at a time: Open Session / Workouts still open while one is in progress, but the picker's SELECT
  and the preview's START show a 2 s "Session in progress" toast instead of starting. Active falls back to
  Pause / Home if no live snapshot arrives within 1.5 s of entry.
- End & save of a session with no shots = discard (service sends SAVED{0}; Home toast "Nothing to save").
  SAVED{0} otherwise = save failed (Home toast "Save failed"; current.bin is kept). No SAVED within 3 s:
  Pause/Rest give the keys back with a "No reply" toast.
- Workout step done → long buzz → Rest (countdown ring, UP +15 s, SELECT skip); zero-rest steps show a 1.5 s
  toast instead. After the last step Rest shows the workout-done summary (UP keep shooting = extra shots).
- Step builder: fields Spot / Goal type / Value / Rest; SELECT on Spot opens the full spot picker; after Rest a
  mini-menu Add step / Save workout / Next step / Delete step / Delete workout (asked twice). Saves time out
  after 3 s with "No reply".
- Results via SELECT: 1 court map (UP/DOWN move between spots; caption "3PT · Top 5/10 · 50 %") → 2 categories
  → 3 highlights (STREAK names the spot) → 4 trend. BACK = done (already saved).
- Heart rate is counted in Active and Rest, not while paused: the HR sensor is disconnected on PAUSE (and for a
  restored session until it is resumed) and reconnected on RESUME.
- Colours: green ≥ 50 %, amber 35–49 %, red < 35 %, teal accent `#00AAAA`, black background; 64-colour palette.
- Spots (13): 3PT L-Corner, L-Wing, Top, R-Wing, R-Corner · Mid L-Baseline, L-Elbow, Top-Key, R-Elbow,
  R-Baseline · FT · Layup L, R.
- Presets: Around the World, Free Throw 50, Mikan Layups, Mid-range Ladder, Game Shots (3PT/Mid/FT/Layup ×10).

## Working notes

- Keep `score/` free of SDK/GUI dependencies (reusable for table tennis); new engine code gets host tests.
- One screen class per screen under `gui/src/screens/` + `gui/include/gui/screens/`; register it in
  `ScreenManager::create`. `lvgl-gui.cmake` globs the GUI sources and `Software/Libs/libs.cmake` globs
  `Sources/` and `score/`, for both the simulator and the watch build (re-run cmake after adding files).
- Persisted structs and IPC messages are padding-free with size `static_assert`s — changing a layout breaks
  existing files (bump the magic).
- Watch the LVGL pool when adding widgets: the self-test logs `pool: … peak …` per step.

## Status / next steps

MVP complete (2026-10-10): engine + host tests, service (IPC, haptics, storage, crash resume, HR, wrist-raise
backlight), all 10 screens with presets, custom workouts, results, history and trend; self-test 84/84 and
crash-resume 5/5 headless; `Hoops_0.0.0-dev.uapp` builds. Final review fixes (2026-10-10): self-test 89/89,
crash-resume 9/9, 66 host tests.

Known limitations / next:
- HISTORY carries 5 rows (`kTrendCount`); the index keeps 50 — the History screen shows only the newest 5.
- Idle timeout (60 s, exits the app) only on Home and only without a session in progress.
- The date (LOCAL_TIME) is sent once at GUI start; a GUI left open past midnight shows yesterday's "today".
- Hold = 500 ms (`KeyInput::kHoldMs`) with HoldRelease semantics (see Hardware facts). A CLICK that reaches the
  GUI more than 150 ms (`kReleaseGraceMs`, one frame plus jitter) after its RELEASE would count twice — confirm on
  the watch.
- Workout ids are reused after a delete (lowest free id ≥ 100), and History rows store only the id, so an
  old row shows the name of whatever workout now has that id.
- SAVE_WORKOUT is validated in the service (steps in range, NUL-terminated name); WORKOUT_SAVED{0} → builder
  toast "Not saved". "Delete workout" is greyed while a session runs that workout.

Next:
1. Real App ID from apps.unawatch.com (`13F039C6FE024E86` is a local placeholder) and app icons in `Resources/`
   (currently the tutorial's).
2. Try it on the watch: button feel, backlight timeout, HR, flash write times (atomic rename path).
3. Settings screen (greyed on Home), FIT export, IMU "smart attempts".
4. Store listing + submission by **30 Oct 2026, 16:00 UK**.
