# Hoops — store listing

Text for the app's page on apps.unawatch.com. Everything above "Maintainer notes" is written for store
visitors; the last section is for the developer.

## Name

Hoops

## Tagline

Your basketball shooting coach on the wrist.

## Short description

Count makes and misses with one button press, at 13 court spots. Workouts, rest timers, court map, streaks
and trends.

<!-- 118 characters; the limit is 160. -->

## Long description

Hoops turns your UNA Watch into a shooting log for the court. Shoot, press a button, keep shooting: the right
top button counts a make, the right bottom button a miss, and a short buzz confirms each one, so you never have
to look at the screen between shots.

**Open sessions.** Pick one of 13 spots (five three-point spots, five mid-range spots, the free-throw line and
left and right layups) and start shooting. The screen shows makes and attempts, your last shots, the time and
your heart rate, and the ring shows your percentage as you go, coloured green from 50 %, amber from 35 % and red
below. Change spot at any time by holding the bottom left button, or
set an open-session target (25 to 200 shots) and watch the ring fill up towards it.

**Workouts.** Fifteen built-in workouts from 50 to 200 shots — five drills in small and large sizes (Around
the World, Free Throw, Mikan Layups, Mid-range Ladder; Game Shots in small and medium) plus warm-up,
free-throw sets, corners, elbows and a 200-shot session. Each step has a spot and a goal (a number of shots, a number of makes, or a time) followed by
a rest countdown you can extend or skip. Build your own workouts right on the watch, up to eight steps each, and
keep up to ten of them.

**Plan workouts on your computer.** The Hoops workout builder is a single web page that works offline and
reads and writes the watch's custom workout file:
1. Connect the watch over USB and import `Apps/Hoops/workouts.bin` into the builder (skip this if you have no
   custom workouts yet; the file you download replaces the whole custom list).
2. Add or edit workouts — spot, goal and rest for up to eight steps — and press Download workouts.bin. A
   half-court map on each workout shows the spot of the step you are editing, just like the watch's spot
   picker; click a spot on the map to move the step there.
3. With Hoops closed, copy the file back to `Apps/Hoops/workouts.bin`, eject the watch and open Hoops.
To start over from the fifteen built-in workouts, use Settings → Restore workouts on the watch (it asks twice
and deletes every custom workout).
<!-- Builder: tools/workout-builder.html in the source; link the hosted page here once it is published. -->

**Mistakes happen.** Pressed the wrong button? The top left button undoes the last shot. The bottom left
button pauses; from the pause menu you resume, end and save, or discard. If the watch restarts mid-session,
Hoops picks up where you left off.

**Results.** Every saved session gets four pages: a court map with your percentage at every spot you shot from,
your numbers by category (3PT, mid-range, free throws, layups), highlights (longest streak and where you made
it, best spot, the spot to work on, time and shots per minute, average and maximum heart rate) and a trend of
your last five sessions per category.

**History.** The last 50 sessions are kept on the watch, newest first; open any of them to see its results
again. The spot picker shows your last and best percentage at each spot. Turn on CSV export and every saved
session is also written as a spreadsheet file you can copy off the watch over USB.

**Made for the court.** Five makes in a row and every ten after that earn their own buzz and a toast. The
backlight comes on with each shot and when you raise your wrist. Choose a dark or a light colour scheme (light
reads better in bright sunshine), light or strong haptics or none, and whether heart rate is recorded.

## Features

- One-press scoring: right top = make, right bottom = miss, with distinct buzzes
- 13 court spots across three-point, mid-range, free throw and layups
- Open sessions with a live percentage ring and an optional shot target (25 / 50 / 100 / 200)
- Fifteen built-in workouts from 50 to 200 shots — five drills in small and large sizes plus warm-up, free-throw sets,
  corners, elbows and a 200-shot session
- Up to ten custom workouts (up to eight steps each), built on the watch or in the offline workout builder on
  your computer; one setting deletes them all to start over from the built-in fifteen
- Goals by attempts, makes or time, with a rest countdown (+15 s or skip)
- Undo, pause, resume after a restart
- Results: court map, categories, highlights, five-session trend
- History of the last 50 sessions, plus last and best percentage per spot
- Streak haptics at 5 makes in a row and every 10
- Heart rate (average and maximum per session), optional
- CSV export of every saved session over USB mass storage, optional
- Dark and light colour schemes; haptics off / light / strong
- Works on the watch alone: no phone needed while you shoot

## Controls

| Button | In menus | While shooting | During rest |
|---|---|---|---|
| Top left (UP) | Move up | Undo last shot | Add 15 s |
| Bottom left (DOWN) | Move down | Pause (hold: change spot) | Pause |
| Top right (SELECT) | Open / confirm | Make | Skip rest |
| Bottom right (BACK) | Back | Miss | — |

In the pause menu, BACK resumes. Holding DOWN can be set to pause instead of changing spot (Settings).

## Requirements

- UNA Watch with kernel firmware 1.4.0 or later
- No extra hardware and no phone connection needed. Heart rate uses the watch's own sensor.

## What's new

**Version 0.2.0** — first release: open sessions at 13 spots; fifteen built-in workouts from 50 to 200 shots
(five drills in small and large sizes — Game Shots in small and medium — plus Warm-up 50, FT Sets 5x10, Elbows &
Key, Corner 3s and Pro Session 200); custom workouts built on the watch or in the workout builder, a web page
for making them on your computer — with a half-court map of each step — and copying them to the watch over
USB; a Settings row that restores the built-in list; undo and pause; results with court map, categories,
highlights and trend; 50-session history, per-spot records, streak haptics, CSV export, settings, dark and
light colour schemes.

## Category and tags

- Category: Activity (Sports)
- Tags: basketball, shooting, training, workout, sports, free throws, statistics

## Previews

Screenshots from the watch display (240 × 240, shown at 2×), in `previews/`:

| File | Shows |
|---|---|
| `home.png` | Home menu with the last session's result |
| `spot-picker.png` | Choosing one of the 13 spots, with last and best percentage |
| `active-open.png` | An open session: makes / attempts, percentage, last shots, time, heart rate |
| `active-workout.png` | A workout step in progress: step 1 of 5, shots left |
| `rest.png` | Rest countdown between workout steps |
| `pause.png` | Pause menu: resume, end and save, discard |
| `results-court.png` | Results page 1: court map, percentage per spot |
| `results-categories.png` | Results page 2: 3PT, mid-range, free throws, layups |
| `results-highlights.png` | Results page 3: streak, best spot, work on, time, heart rate |
| `results-trend.png` | Results page 4: trend over the last five sessions |
| `history.png` | History list of saved sessions |
| `workouts.png` | The workout list: built-in workouts, then your own |
| `workout-preview.png` | A workout's steps before starting (Game Shots S, five steps) |
| `step-builder.png` | Building a custom workout step on the watch |
| `settings.png` | Settings: goals, target, haptics, scheme, CSV export and more |
| `home-light.png` | Home in the light colour scheme |

---

## Maintainer notes

Not part of the listing.

**App ID.** `store/app-manifest.json` `"id"` must equal `APP_ID` in `Software/Apps/Hoops-CMake/CMakeLists.txt`.
Both hold the local placeholder `13F039C6FE024E86` until the real ID comes from apps.unawatch.com;
`tools/package_store.sh` refuses to package while it is the placeholder (unless `--allow-placeholder`, for a
test package) and when the two differ.

**Icons.** `tools/make_icons.py` (run with `.venv/bin/python3`) regenerates `Resources/icon_60x60.png`,
`Resources/icon_30x30.png` and `store/icon.png`. Previews come from the self-test's screenshots
(`HOOPS_SHOTS_DIR` + `tools/ppm2png.py`, CLAUDE.md "Self-test"); regenerate them after any visible change.

**Builder hosting.** The listing's builder paragraph needs a public URL. The page is one self-contained file
(`tools/workout-builder.html`), so any static host works: GitHub Pages (push the repo, Settings → Pages →
deploy from `main`, link `…/tools/workout-builder.html`), or any web space. Replace the HTML comment under the
builder paragraph with the link before pasting the listing.

**Submission checklist** — deadline **30 Oct 2026, 16:00 UK** (UNA App Competition):

1. apps.unawatch.com → Add New → name "Hoops" + short description → copy the App ID.
2. Put the App ID in both places: `APP_ID` in `Software/Apps/Hoops-CMake/CMakeLists.txt` and `"id"` in
   `store/app-manifest.json`. Commit.
3. Tag the build you submit (`git tag -a apps-vX.Y.Z -m "…"` on the commit with the real App ID; the SDK
   reads only `apps-vX.Y.Z` tags — `apps-v0.0.1`, `apps-v0.1.0` and `apps-v0.2.0` exist) so the build version
   (the `.uapp` name and `appVersion`) is clean; commits after a tag add `-<n>-<hash>`, a dirty tree `-dirty`.
4. `export UNA_SDK=$HOME/una-sdk && tools/package_store.sh` → `dist/Hoops-<ver>.zip` (builds the watch app,
   fills `binary`/`appVersion`, stamps and checks `minKernelVersion`).
5. On the app page: Version → Upload New → upload the zip; paste the listing text above (description,
   previews).
6. Release → Confirm and Publish.
7. Submit the competition form with the store link.
