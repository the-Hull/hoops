# Hoops (ScoreTrack) — UNA Watch app

Basketball shooting tracker for the UNA Watch, built on the UNA SDK v1.5.0 with the
**LVGL** GUI (no TouchGFX — the developer is on Ubuntu, where TouchGFX Designer
doesn't exist). Scaffolded from the SDK tutorial `Docs/Tutorials/Buttons` (LVGL variant: minimal
service + LVGL GUI + simulator) with fonts from `Examples/Apps/RunLVGL`; RunLVGL remains the reference for the
Model + ScreenManager + Theme pattern.

Competition: UNA App Competition, deadline **30 Oct 2026, 16:00 UK**. Judged on
Usefulness, Originality, UX/UI (10 pts each). Must be published on
apps.unawatch.com and submitted via the form. A table-tennis app may follow as a
second entry (max 2 per person) — keep the scoring engine reusable.

- Full feature spec: `docs/feature-spec.md`
- Screen mockups (14 screens): `docs/mockups/*.dc.html`. These are design-canvas source files — they do not render
  standalone in a browser, but the markup holds the exact layout, colours, text and button labels (the black
  300×300 circle is the 240×240 screen at 1.25×). Rendered canvas: https://claude.ai/artifact/EZBpMUafRTmC5CggUc3uvM
  Order: Main (home), SpotPicker, Workouts, WorkoutPreview, StepBuilder, ActiveOpen, ActiveWorkout, UndoToast,
  Rest, Pause, ResultsCourt, ResultsCategories, ResultsHighlights, History.

## Environment

- Ubuntu, VS Code. Folder layout: `~/una-sdk` (SDK v1.6.0-rc1+, `UNA_SDK` points here — exported in `~/.bashrc`,
  so non-login shells may need `export UNA_SDK=$HOME/una-sdk`), `~/work/006_una_hoops` (this repo).
- Python venv in `.venv/` — the watch build is configured with
  `-DUNA_PYTHON_EXECUTABLE=.venv/bin/python3` so the SDK's pack scripts use it.
- ARM compiler: STM32CubeCLT 1.22, installed to `/opt/st/stm32cubeclt_*/GNU-tools-for-STM32/bin`
  (verify with `find /opt/st -name arm-none-eabi-gcc`). Do NOT use Ubuntu's `gcc-arm-none-eabi`.
- `setup-una-dev.sh` installs/configures everything (apt packages, SDK clone,
  toolchain detection, `~/.bashrc` block, venv, `.vscode/` tasks + debug config).
  Re-runnable; backs up existing `.vscode/*.json` as `*.bak`.

## Build & run

```bash
# Simulator (needs libsdl2-dev; run from build/bin — the mock file system is relative to it)
cd Software/Apps/LVGL-GUI/simulator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build
(cd build/bin && ./HoopsSimulator)          # headless smoke: SDL_VIDEODRIVER=dummy

# Watch app (.uapp) — the SDK's copy into Output/ can fail; copy build/*.uapp by hand if so
cd Software/Apps/Hoops-CMake
cmake -S . -B build -DUNA_PYTHON_EXECUTABLE=$PWD/../../../.venv/bin/python3 && cmake --build build -j
# -> build/Hoops_<version>.uapp (also copied to Output/); version comes from git tags

# Fonts: Poppins C files copied from RunLVGL (large faces carry digits only — see RunLVGL's gen_assets.py
# for ranges). No gen_assets.py here yet; copy/adapt RunLVGL's when new glyphs or images are needed.

# Host unit tests (from $UNA_SDK)
cmake -S Tests/Host -B build-host -DCMAKE_BUILD_TYPE=Debug && cmake --build build-host && ctest --test-dir build-host --output-on-failure
```

VS Code tasks (NOT yet present — no `.vscode/` or `setup-una-dev.sh` in this repo): `Sim: build` (Ctrl+Shift+B), `Sim: run`, `Watch: build .uapp`,
`Assets: regenerate fonts/images`; F5 = gdb debug of the simulator.

Simulator keys: `1/2/3/4` = UP/DOWN/SELECT/BACK click (L1/L2/R1/R2);
`q/w/e/r` press, `a/s/d/f` release (use for long-press); `5` = wrist raise.
Vibration/buzzer only appear as `Mock.Vibro::play` / `Mock.Buzzer::play` log lines.
Accelerometer/gyro are NOT simulated — sensor features need a real watch or recorded data.
Sensor sim config: `Software/Apps/LVGL-GUI/simulator/ConfigurationSimulator.hpp`.

Deploy to watch: USB mass storage → create `Apps/Hoops/` → copy `.uapp` → eject → reboot watch.
Publish: apps.unawatch.com → Add New → copy App ID into `APP_ID` in the `*-CMake/CMakeLists.txt`
→ rebuild → `app-manifest.json` + `python3 $UNA_SDK/Utilities/Scripts/app_packer/min_kernel_version.py --stamp app-manifest.json`
→ zip `.uapp` + `icon.png` + manifest → Version → Upload New → Release.

## Hardware & SDK facts (v1.5.0 docs)

- Display: 1.2" round MIP LCD, 240×240, 64 colours (ABGR2222), LED backlight, no touch.
- Buttons: L1 top-left (UP), L2 bottom-left (DOWN), R1 top-right (SELECT/START/STOP), R2 bottom-right (BACK).
  LVGL: key events via `LV_EVENT_KEY` → `Screen::onKey(code)`; press/release/click codes exist. Long-press is not
  an SDK event — derive it from press→release timing. Double-press is app-implemented.
- Sensors (`SDK::Sensor::Type`): ACCELEROMETER (m/s², incl. gravity), GYROSCOPE (deg/s), FUSION, HEART_RATE,
  WRIST_MOTION, MOTION_DETECT, ACTIVITY_RECOGNITION, GESTURE_RECOGNITION (undocumented), TOUCH_DETECT (worn).
  `SDK::Sensor::Connection(type, period, latency)`; batched `EventData`.
- Feedback messages: `SDK::Message::RequestVibroPlay`, `RequestBuzzerPlay`, `RequestBacklightSet` (usage undocumented — check SDK headers/examples).
- Docs: https://www.developers.unawatch.com/sdk-v1.5.0/ (index: genindex.html). When reading docs, pick pages by title, don't read everything.

## Decided UX (mockups are the source of truth where the spec differs)

- Menus: UP/DOWN scroll, SELECT open/confirm, BACK back.
- Shooting: **SELECT = MAKE** (1 short buzz), **BACK = MISS** (2 short buzzes), **UP = undo** (3 buzzes + 2 s toast
  "Make removed"), **DOWN = pause**, **hold DOWN = change spot** (open session).
- No exit mid-session: leaving only via the pause screen (Resume / End & save / Discard; Discard asks twice; BACK = resume).
- Workout step done → long buzz → rest screen (countdown ring, UP +15 s, SELECT skip).
- Results pages via SELECT: 1 court map (UP/DOWN move between spots) → 2 categories → 3 highlights → 4 trend. BACK = done (already saved).
- Colours: green ≥50 %, amber 35–49 %, red <35 %, teal accent `#00AAAA`, black background. Keep to the 64-colour palette.
- Auto-save after every input.
- Spots (13): 3PT L-Corner, L-Wing, Top, R-Wing, R-Corner · Mid L-Baseline, L-Elbow, Top-Key, R-Elbow, R-Baseline · FT · Layup L, R.
- Presets: Around the World, Free Throw 50, Mikan Layups, Mid-range Ladder, Game Shots (3PT/Mid/FT/Layup ×10).

## Architecture guidance

- Keep the scoring/session/workout engine as plain C++ with no GUI or SDK dependencies (`Software/Libs/score/` or similar)
  so it is unit-testable on the host and reusable for table tennis.
- One LVGL screen class per screen under `LVGL-GUI/gui/.../screens/`, following the RunLVGL pattern (Model + ScreenManager).
- When adding sources, check whether the simulator CMake and the app CMake both pick them up (`lvgl-gui.cmake` globs the GUI).

## Status / next steps

1. ~~Setup; scaffold builds for simulator and watch~~ (done 2026-10-10: `Hoops_0.0.0-dev.uapp`, `HoopsSimulator`
   smoke-tested headless). App ID `13F039C6FE024E86` is a local md5 placeholder — replace with the portal ID.
2. New icons in `Resources/` (currently the tutorial's).
3. Verify in the simulator: long-press via press/release keys, vibro/buzzer message usage, screen-timeout behaviour.
4. Build the score engine + tests, then screens in mockup order: Home → Spot picker → Shooting → Pause → Results.
