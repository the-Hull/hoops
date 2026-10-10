# Mockup layout reference

Extracted from `docs/mockups/*.dc.html`. All coordinates are 240×240 screen px
(canvas px ÷ 1.25); x is the horizontal centre (≈120 unless stated); y is ±3 px.
Mockup colours are richer than the watch's 64-colour palette — map them as in the
"Colour mapping" section at the end.

Common elements:
- Ring: centre (120,120), r=112, fills clockwise from 12 o'clock. Track #555555 w4.8; fill w8, round caps.
- Button-hint dots: r≈4 at TL (30,57) UP, BL (30,183) DOWN, TR (210,57) SELECT, BR (210,183) BACK. White unless noted.
- Shooting-screen badges: 17.6 px circles at TR (209,55) green ✓, BR (209,183) red ✗, TL (31,55) white undo, BL (31,183) white pause.
- Headers: condensed caps, letter-spaced, teal #00AAAA (or amber/grey as noted).
- Selected menu item: teal pill, black bold text; unselected items white/grey.

## 1 Main (home)
Dots TR, BR. Header "HOOPS" 12px teal y47 (with ball icon). Items: "Open Session" 18px bold in pill 168×38 y80; "Workouts" 16px y115; "History" 16px y141; "Settings" 16px grey y166; "Last · 6.10. · 58 %" 10px grey y193.
Hints: UP scroll up · DOWN scroll down · SELECT open · BACK exit app.

## 2 SpotPicker
Dots: all 4, TR green. "CHOOSE SPOT" 11px teal y37. Half court 106×71 at x67–173, y47–119 (viewBox 200×135). Court lines #555555: baseline y10 x2–198; lane rect (73,10) 54×64; FT half-arc r20 (80,74)→(120,74) bulging down; 3PT `M22 10 L22 45 A80.5 80.5 0 0 0 178 45 L178 10`; hoop circle (100,25) r5 amber.
Spot unit coords: 3PT L-Corner (12,28), L-Wing (36,92), Top (100,118), R-Wing (164,92), R-Corner (188,28); Mid L-Baseline (44,26), L-Elbow (68,76), Top-Key (100,94), R-Elbow (132,76), R-Baseline (156,26); FT (100,77); Layup L (82,34), R (118,34). Unselected r5 #555555; selected r8 teal + ring r13 teal w3.
List: prev "3PT · L-Corner" 13px grey y129; selected "3PT · L-Wing" 18px bold pill 176×33 y165; next "3PT · Top" 13px grey y192; "Last 40 % · best 46 %" 10px grey y211.
Hints: UP prev · DOWN next · SELECT start shooting · BACK home.

## 3 Workouts
Dots all 4. "WORKOUTS" 11px teal y49. prev "Free Throw 50" 13px grey y69; selected pill 182×47 y106: "Game Shots" 18px bold + "4 steps · 40 shots" 11px; "Around the World" 15px y144; "Mikan Layups" 15px y168; "+ New workout" 13px teal y190.
Hints: UP/DOWN scroll · SELECT preview · BACK home.

## 4 WorkoutPreview
Dots TR green, BL, BR. "GAME SHOTS" 11px teal y46. Step rows x38–202, gap 5.6: number circle 17.6 #555555 at x46, label 14px bold at x63, count "×10" 16px cond grey right-aligned x202. Rows y69, 94, 119, 143. "Rest 30 s · about 12 min" 10px grey y166. START pill green 70×22 y189, text 12px bold black.
Hints: UP — · DOWN edit workout · SELECT start · BACK workouts.

## 5 StepBuilder
Dots all 4. "NEW STEP 3" 11px teal y44. Rows x44–196: "Spot | Free Throw" y62, "Goal | Attempts" y82 (label 12px grey, value 12px bold white). Value box 152×84 border 1.6 teal r14 centre y139: ▲ teal y106, "10" 45px cond bold y132, "shots" 11px grey y159, ▼ y169. Row "Rest | 30 s" y196 (both grey, not active).
Hints: UP +5 · DOWN −5 · SELECT next field (after Rest: add step / save) · BACK previous field.

## 6 ActiveOpen
Ring track + green fill (7/11 = 63.6 %). 4 badges. "3PT · L-WING" 13px cond teal y54. "7" 67px cond bold white + "/11" 45px grey, y97. "64 %" 26px cond bold green y142. Last-5 dots 8.8px gap 4.8 y167 at x93,106,120,134,147 (green/green/red/green/green; last has white outline ring). "12:41 · ♥148" 12px grey y186 (heart red).
Hints: UP undo last · DOWN pause (hold: change spot) · SELECT MAKE (1 short buzz) · BACK MISS (2 short buzzes).

## 7 ActiveWorkout
Segmented ring: 10 segments w8, each ~32.7°, gaps ~3.3°, starting at 12 o'clock; colours by shot (green make, red miss, #555555 remaining). "STEP 2 / 4" 10px grey y48. "MID · R-ELBOW" 16px cond teal y64. "5" 64px + "/7" 43px y105. "71 %" 24px green y148. "3 shots left" 13px bold white y174. "18:05 · ♥156" 11px grey y191.
Hints: as ActiveOpen (no hold-to-change-spot); BACK note "10th shot: long buzz, step done".

## 8 UndoToast (ActiveOpen after undo)
Same as ActiveOpen, text shifted up: spot y47, "6/10" y90, "60 %" y135. Toast: white pill 129×27 r14 y171 with undo icon + "Make removed" 13px bold black. Note "3 buzzes · hides after 2 s".

## 9 Rest
Ring track + amber fill (remaining). Dots TL, BL, TR. "REST" 13px cond amber y55. "0:24" 62px cond bold y93. Divider 120×1.6 #555555 y131. "NEXT · STEP 3 / 4" 10px grey y144. "Free Throw ×10" 18px bold y162. "Step 2 done · 7/10 · 70 %" 11px grey (pct green) y186.
Hints: UP +15 s · DOWN pause · SELECT skip rest (long buzz at 0:00) · BACK —.

## 10 Pause
Dashed amber ring r112 w3.2 (dash 11/8). Dots all 4. "PAUSED" 13px amber y47. "18:02" 32px cond bold y76. "Step 2 / 4 · 12/17 · 71 %" 11px grey y105. "Resume" 17px bold pill 160×33 y137; "End & save" 15px y168; "Discard" 15px red y192.
Hints: UP/DOWN scroll · SELECT confirm (Discard asks twice) · BACK resume.

## 11 ResultsCourt (1/4)
Dots all 4. "SESSION · 1 / 4" 10px grey y27. "84/140 · 60 %" 24px cond bold y48 (pct green). Court 181×122 at x30–210, y63–186, lines w2; spot dots r7 units (~6.3px) coloured by % (green ≥50, amber 35–49, red <35; spots with no attempts #555555); selected spot white ring r12 w2.5. Caption "3PT · Top 5/10 · 50 %" 14px y197. Legend y214 8.8px grey: ●≥50 ●35–49 ●<35.
Hints: UP/DOWN prev/next spot · SELECT next page · BACK done (session already saved).

## 12 ResultsCategories (2/4)
Dots all 4. "BY CATEGORY · 2 / 4" 10px grey y37. Rows x35–205, gap 8: name 14px bold left; right: "18/50" 10px grey + "36 %" 16px cond bold coloured; bar 6.4 tall r3.2 track #555555 fill coloured. Rows (label y / bar y): 3PT 61/76, Mid-range 97/112, Free throw 133/148, Layup 169/184. "Total 84/140 · 60 %" 11px y202.
Hints: UP/DOWN prev/next category (shows its spots) · SELECT next page · BACK done.

## 13 ResultsHighlights (3/4)
Dots TR, BR. "HIGHLIGHTS · 3 / 4" y37. 2×2 cards 85×65 border 1.6 #555555 r9.6, col centres x74/166, row centres y83/155. Title 8.8px grey, value 27px cond bold, sub 10px grey. STREAK "9" teal "in a row · FT"; BEST SPOT "100 %" green "Layup R"; WORK ON "30 %" red "3PT corners"; TIME "38:20" white "3.7 shots/min". "♥ avg 141 · max 172 bpm" 11px grey y202.
Hints: UP — · DOWN — · SELECT next page · BACK done.

## 14 History / Trend (4/4)
Dots all 4. "TREND · 4 / 4" y39. "FREE THROW" 14px cond teal y58. "87 %" 35px cond bold + "+6 vs last" 12px bold green, y90. Bar chart x38–202, plot height 77, baseline 1.6 #555555 y190; 5 bars 19px wide, top r3.2, centres x50,85,120,155,190; value label 8.8px above; date label 8.8px y202. Past bars grey #AAAAAA, today's teal with white bold labels.
Hints: UP/DOWN prev/next category · SELECT back to page 1 · BACK done.

## Colour mapping (mockup → SDK 64-colour palette, `SDK/GUI/Color.hpp`)
- #000000 → BLACK · #FFFFFF → WHITE (0xC0C0C0) · #AAAAAA → GRAY (0x808080) · #555555 → GRAY_DARK (0x404040)
- #00AAAA teal → 0x008080 (TEAL) · #55FF55 → 0x40C040 or GREEN 0x00C000 · #FF5555 → 0xC04040 or RED 0xC00000 · #FFAA00 → 0xC08000 (AMBER)
Check the exact constants in Color.hpp when building the theme.
