# Hoops — basketball shooting tracker for the UNA Watch

Count makes and misses with one button press at 13 court spots, run built-in or custom workouts with rest
timers, and read your results as a court map, category stats, highlights and a trend — all on the watch.
Built on the [UNA SDK](https://github.com/UNAWatch/una-sdk) with the LVGL GUI.

- Store text and screenshots: [docs/store/listing.md](docs/store/listing.md), [docs/store/previews/](docs/store/previews/)
- Workout builder (offline web page that reads and writes the watch's `workouts.bin`):
  [tools/workout-builder.html](tools/workout-builder.html)
- Built-in workouts: [Software/Libs/score/presets.json](Software/Libs/score/presets.json)
- Developer reference (architecture, build, self-test, decisions): [CLAUDE.md](CLAUDE.md)

## Layout

```
Software/Libs/score/      scoring engine: plain C++17, host-tested (Tests/Host)
Software/Libs/Sources/    service process: session engine, storage, haptics, heart rate
Software/Apps/LVGL-GUI/   GUI process (LVGL) and the desktop simulator
Software/Apps/Hoops-CMake watch build (.uapp)
tools/                    gen_presets.py, workout-builder.html, make_icons.py, package_store.sh
docs/                     feature spec, mockups, store listing, design specs and plans
```

## Build

Needs the UNA SDK checkout in `$UNA_SDK`, the ST GNU tools for STM32, CMake, Ninja, SDL2 and a Python venv in
`.venv/` (see CLAUDE.md "Environment").

```bash
export UNA_SDK=$HOME/una-sdk
# host tests (engine)
cmake -S Tests/Host -B build-host -DCMAKE_BUILD_TYPE=Debug && cmake --build build-host && ctest --test-dir build-host
# desktop simulator
cd Software/Apps/LVGL-GUI/simulator && cmake -S . -B build -G Ninja && cmake --build build && (cd build/bin && ./HoopsSimulator)
# watch app
cd Software/Apps/Hoops-CMake && cmake -S . -B build -DUNA_PYTHON_EXECUTABLE=$PWD/../../../.venv/bin/python3 && cmake --build build -j
```

Simulator keys: `1`/`2`/`3`/`4` = UP / DOWN / SELECT / BACK, `Esc` quits. Install on the watch by copying the
`.uapp` to `Apps/Hoops/` over USB mass storage.

## Versions

Releases are git tags of the form `apps-vX.Y.Z` (the SDK derives the app version from them):
`apps-v0.0.1` MVP + settings/history/CSV, `apps-v0.1.0` 15 presets + workout builder, `apps-v0.2.0` restore
workouts + builder court map.
