# ScoreTrack – Feature Spec (Basketball Shooting + Table Tennis)

Status: v0.1 (2026-10-09) · basketball MVP built 2026-10-10, v2 (settings, full history, CSV export, streaks, store assets) built 2026-10-10 — as-built reference: CLAUDE.md · Target: UNA App Competition (deadline **30 Oct 2026, 16:00 UK**)

> Button names: L1 = UP (top-left), L2 = DOWN (bottom-left), R1 = SELECT (top-right), R2 = BACK (bottom-right).
> Where this spec and the mockups (`docs/mockups`) differ, the mockups and CLAUDE.md "Decided UX" win.

---

## 0. Competition constraints that shape scope

- Judged on **Usefulness, Originality, UX/UI** (10 pts each).
- App must be **published on the UNA community app store** and submitted via the form; it must not already be on the store.
- Max **2 apps per person** → option: ship *ScoreTrack Hoops* and *ScoreTrack Table Tennis* as two entries, or one app with a sport picker. Decide early.
- No watch needed — simulator demos are allowed.
- ~3 weeks left → MVP must be button-driven; sensor features are the originality layer on top.

## 1. Platform capabilities used (SDK v1.5.0)

| Area | What's available | Use in ScoreTrack |
|---|---|---|
| Display | 1.2" **round** MIP LCD, 240×240, 8-bit ABGR2222 (64 colours), LED backlight. No touch | Big numerals, progress ring around edge, 3–4 semantic colours |
| Buttons | L1, L2, R1, R2. Press / release / click events. Double-press is app-implemented. Long-press not an SDK event | All scoring input. Long-press from press→release timing |
| Button hints | Colours NONE / WHITE / AMBER / RED (+ GREEN in LVGL) | Green = make/point-me, Red = miss/point-opp |
| IMU | ACCELEROMETER (m/s², incl. gravity), GYROSCOPE (deg/s), FUSION, MAGNETIC_FIELD; configurable period + batching | Shot/stroke detection, wrist-twist gesture |
| Motion events | WRIST_MOTION, MOTION_DETECT, ACTIVITY_RECOGNITION, GESTURE_RECOGNITION (types undocumented), TOUCH_DETECT | Wrist-raise to wake score view; auto-pause when still/unworn |
| Heart rate | HEART_RATE, HEART_RATE_METRICS | HR + effort in summaries |
| Feedback | `RequestVibroPlay`, `RequestBuzzerPlay`, `RequestBacklightSet` | Haptic confirmation of every input |
| Storage / sync | File system, FIT files, BLE File Transfer Service; companion-app config (Waypoint tutorial) | Session history, FIT export, workouts defined off-watch |
| UI | LVGL (`WheelMenu`, `Buttons` helpers) | WheelMenu for spot/workout picking |

**Open questions to verify:** long-press feasibility, vibro pattern API, GESTURE_RECOGNITION event types, preventing screen timeout during a session, FIT lap/custom-field support.

## 2. Global UX conventions

- **Menus:** L1/L2 scroll, R1 select, R2 back.
- **In session:** R1 = positive action, R2 = negative action, L1 = undo, L2 = pause/menu.
- **Exit only from the pause screen**, never mid-session.
- **Haptics:** short buzz = positive, double buzz = negative, long buzz = step/game done, triple = undo.
- **Undo always one press away** (L1), with a 2 s toast "Make removed".
- Auto-save the session on every input.
- Colours: Green ≥ 50 % · Amber 35–49 % · Red < 35 % · White neutral.

---

## 3. Basketball Shooting Tracker

### 3.1 Spots

| Category | Spots |
|---|---|
| 3PT (5) | L-Corner, L-Wing, Top, R-Wing, R-Corner |
| Mid-range (5) | L-Baseline, L-Elbow, Top-Key, R-Elbow, R-Baseline |
| Free Throw (1) | FT line |
| Layup (2) | Left hand, Right hand |

Optional later: Floater, Post, Deep 3.

### 3.2 Modes

**A. Open Session**
1. Start → wheel of spots (L1/L2 scroll, R1 select).
2. Shoot and log. **Hold L2 = change spot**.
3. Ends via pause → "End & save".

**B. Workouts / Circuits**
- Workout = ordered list of **steps**: `spot` + `goal` + optional `rest`.
- Goal types: **attempts** ("10 shots"), **makes** ("make 5"), **time** ("60 s").
- Step done → long buzz + rest / next-step screen.
- Presets: 15 presets generated from `Software/Libs/score/presets.json` (see CLAUDE.md).
- Custom workouts: on-watch builder (spot → goal type → value 5/10/15/20/25/50 → rest 0/15/30/60 s → repeat; up to 8 slots). Stretch: JSON workouts via BLE file transfer.

### 3.3 Input methods (layered)

| Level | Method | Notes |
|---|---|---|
| **MVP** | R1 = Make, R2 = Miss, L1 = Undo, L2 = Pause | Works everywhere, simulator-friendly |
| **Smart attempts** | IMU detects each shot attempt; user presses R1 only when made; no press within ~4 s → miss | Arm raise (accel pitch) + release flick (gyro spike), optional jump. Haptic tick on detection so false positives can be undone |
| **Gesture logging (experimental)** | Wrist twist after the shot: outward roll = Make, double flick = Miss | Needs calibration; default off |

Auto-pause when MOTION_DETECT reports no motion for 60 s or TOUCH_DETECT says unworn.

### 3.4 Screens

See `docs/mockups`. Active shooting: spot name, makes/attempts large, % colour-coded, progress ring (workout: one segment per shot), last-5-shots dots, elapsed time + HR. Wrist raise → backlight on.

### 3.5 Results

1. **Court map**: half court, each spot a dot coloured by %; UP/DOWN highlight spots ("L-Wing 7/10 · 70 %").
2. **Categories**: 3PT · Mid · FT · Layup with makes/attempts, %, bar; total FG%.
3. **Highlights**: longest streak, best spot, worst spot, duration, shots/min, avg/max HR.
4. **Trend**: last 5 sessions per category; "+6 vs last".
- Storage: session JSON in app files; FIT activity with one lap per step/spot (verify FIT support).

### 3.6 Originality hooks
- Smart attempt detection.
- Pressure FT mode: "make 2 in a row to finish".
- "Beat your last" ghost pace on the progress ring.

---

## 4. Table Tennis Scorer (later / second entry)

- Rules: game to 11, win by 2; best of 3/5/7; serve changes every 2 points, every point from 10–10; server alternates per game; change-ends reminder after each game and at 5 points in the decider; doubles rotation; editable names.
- Input: R1 = point me, R2 = point opponent, L1 = undo, L2 = pause/menu. Haptic cue when the serve changes / your serve; optional beep at game/match point.
- Match screen: game no. + games won, big score, serve dot + which serve; amber at game point; full-screen game result + long buzz.
- Stats: game scores, points won on own serve vs receive, longest run, comebacks, duration, HR.
- Training: serve practice (good/fault %), rally counter via IMU (watch on racket hand).

## 5. Build order

1. Core: session engine, button mapping, haptics, auto-save.
2. Basketball Open Session + presets + results.
3. Table tennis match engine + summary.
4. On-watch workout builder.
5. One headline sensor feature (smart attempts or rally counter).
6. History/trends, FIT export.
7. Store listing, simulator demo video, form submission.

## 6. Not in scope now
Volleyball (reuses the TT match engine: sets to 25, win by 2, tie-break to 15).
