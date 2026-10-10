/**
 ******************************************************************************
 * @file    StepBuilderScreen.cpp
 * @brief   The on-watch workout builder: one step at a time.
 ******************************************************************************
 */

#include "gui/screens/StepBuilderScreen.hpp"

#include <cstdio>

#include "score/Spots.hpp"
#include "score/Workout.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{

using Font = Theme::Font;

// Layout, docs/mockup-layout.md §5 (vertical centres).
constexpr int32_t kHeaderY   = 44;
constexpr int32_t kRowLeft   = 44;
constexpr int32_t kRowRight  = 196;
constexpr int32_t kRowGap    = 8;     // row label .. value
constexpr int32_t kSpotY     = 62;
constexpr int32_t kGoalY     = 82;
constexpr int32_t kRestY     = 196;
constexpr int32_t kBoxY      = 139;
constexpr int32_t kBoxW      = 152;
constexpr int32_t kBoxH      = 84;
constexpr int32_t kBoxBorder = 2;
constexpr int32_t kBoxRadius = 14;
constexpr int32_t kBoxInnerW = kBoxW - 12;
constexpr int32_t kUpY       = 106;
constexpr int32_t kValueY    = 132;
constexpr int32_t kUnitY     = 159;
constexpr int32_t kDownY     = 170;
constexpr int32_t kArrowHalfW = 6;
constexpr int32_t kArrowH     = 5;
constexpr int32_t kArrowLineW = 2;
constexpr int32_t kSummaryY   = 108;   // menu mode: the workout's name and size

constexpr Font kRowFonts[]  = { Font::Regular14 };
constexpr Font kSpotFonts[] = { Font::Medium18, Font::Regular16, Font::Regular14 };

// The values UP / DOWN step through.
constexpr uint16_t kShotValues[] = { 5, 10, 15, 20, 25, 30, 40, 50 };
constexpr uint16_t kTimeValues[] = { 30, 60, 90, 120, 180 };
constexpr uint16_t kRestValues[] = { 0, 15, 30, 60 };

struct Values {
    const uint16_t* v;
    uint8_t         n;
};

constexpr Values kShots = { kShotValues, sizeof(kShotValues) / sizeof(kShotValues[0]) };
constexpr Values kTimes = { kTimeValues, sizeof(kTimeValues) / sizeof(kTimeValues[0]) };
constexpr Values kRests = { kRestValues, sizeof(kRestValues) / sizeof(kRestValues[0]) };

constexpr uint8_t kGoalTypes = 3;   // score::GoalType Attempts, Makes, Time

bool isTime(uint8_t type) { return type == static_cast<uint8_t>(score::GoalType::Time); }

const Values& valuesFor(uint8_t type) { return isTime(type) ? kTimes : kShots; }

/// The next entry above (dir > 0) or below (dir < 0) @p cur, or @p cur at
/// the end of the list. A value not in the list steps onto its neighbour.
uint16_t stepValue(const Values& l, uint16_t cur, int8_t dir)
{
    if (dir > 0) {
        for (uint8_t i = 0; i < l.n; ++i) {
            if (l.v[i] > cur) {
                return l.v[i];
            }
        }
    } else {
        for (uint8_t i = l.n; i > 0; --i) {
            if (l.v[i - 1] < cur) {
                return l.v[i - 1];
            }
        }
    }
    return cur;
}

/// The entry closest to @p cur (the lower one on a tie).
uint16_t nearest(const Values& l, uint16_t cur)
{
    uint16_t best = l.v[0];
    for (uint8_t i = 1; i < l.n; ++i) {
        const int32_t d    = static_cast<int32_t>(l.v[i]) - cur;
        const int32_t dBest = static_cast<int32_t>(best) - cur;
        if ((d < 0 ? -d : d) < (dBest < 0 ? -dBest : dBest)) {
            best = l.v[i];
        }
    }
    return best;
}

const char* goalName(uint8_t type)
{
    switch (static_cast<score::GoalType>(type)) {
        case score::GoalType::Makes: return "Makes";
        case score::GoalType::Time:  return "Time";
        case score::GoalType::Attempts:
        default:                     return "Attempts";
    }
}

const char* spotName(uint8_t spot) { return spot < score::kSpotCount ? score::spotInfo(spot).name : "?"; }

/// One "Label   value" row from kRowLeft to kRowRight: the label in kAccent
/// when its field is active, else kDim; the value right-aligned, the first
/// of @p texts that fits.
void row(lv_obj_t* parent, int32_t y, const char* label, bool active, const char* const* texts, uint8_t nTexts,
         uint32_t valueColor)
{
    const int32_t lw = Theme::textWidth(Font::Regular14, label);
    Theme::label(parent, Font::Regular14, label, kRowLeft, Theme::topFor(Font::Regular14, y), lw + 1,
                 LV_TEXT_ALIGN_LEFT, active ? Theme::kAccent : Theme::kDim);
    const int32_t x = kRowLeft + lw + kRowGap;
    Theme::fitLabel(parent, texts, nTexts, kRowFonts, 1, x, kRowRight - x, y, LV_TEXT_ALIGN_RIGHT, valueColor);
}

lv_obj_t* chevron(lv_obj_t* parent, lv_point_precise_t* pts, int32_t cy, bool up)
{
    const int32_t tip  = up ? cy - kArrowH / 2 - 1 : cy + kArrowH / 2 + 1;
    const int32_t ends = up ? tip + kArrowH : tip - kArrowH;
    pts[0] = { static_cast<lv_value_precise_t>(120 - kArrowHalfW), static_cast<lv_value_precise_t>(ends) };
    pts[1] = { static_cast<lv_value_precise_t>(120), static_cast<lv_value_precise_t>(tip) };
    pts[2] = { static_cast<lv_value_precise_t>(120 + kArrowHalfW), static_cast<lv_value_precise_t>(ends) };
    lv_obj_t* l = lv_line_create(parent);
    lv_obj_set_pos(l, 0, 0);
    lv_line_set_points(l, pts, 3);
    lv_obj_set_style_line_width(l, kArrowLineW, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(l, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(l, Theme::rgb(Theme::kAccent), LV_PART_MAIN);
    return l;
}

void setChevron(lv_obj_t* l, bool hidden, bool enabled)
{
    Theme::setHidden(l, hidden);
    lv_obj_set_style_line_color(l, Theme::rgb(enabled ? Theme::kAccent : Theme::kLine), LV_PART_MAIN);
}

const ListMenu::Item kDeleteWorkoutItem    = { "Delete workout", nullptr, Theme::kBad };
const ListMenu::Item kDeleteWorkoutDimItem = { "Delete workout", nullptr, Theme::kDim };
const ListMenu::Item kConfirmItem       = { "Delete?", "SELECT again", Theme::kBad };

} // namespace

StepBuilderScreen::StepBuilderScreen(Model& model)
    : Screen(model)
    , mKeys(&KeyInput::noHold, nullptr)
{
}

StepBuilderScreen::~StepBuilderScreen()
{
    if (mConfirmTimer) {
        lv_timer_delete(mConfirmTimer);
        mConfirmTimer = nullptr;
    }
    if (mToastTimer) {
        lv_timer_delete(mToastTimer);
        mToastTimer = nullptr;
    }
    setBusy(false);
}

score::Step& StepBuilderScreen::step()
{
    return mModel.draft().steps[mModel.draftStep];
}

void StepBuilderScreen::build()
{
    // Keep the draft in range whatever seeded it.
    score::Workout& d = mModel.draft();
    if (d.stepCount == 0) {
        d.stepCount = 1;
        d.steps[0]  = { static_cast<uint8_t>(score::Spot::Ft), 30,
                        { static_cast<uint8_t>(score::GoalType::Attempts), 0, 10 } };
    } else if (d.stepCount > score::kMaxSteps) {
        d.stepCount = score::kMaxSteps;
    }
    if (mModel.draftStep >= d.stepCount) {
        mModel.draftStep = static_cast<uint8_t>(d.stepCount - 1);
    }
    // Where the SpotPicker sends us back: Goal type after a pick, else Spot.
    mField = mModel.draftField < kFieldCount ? mModel.draftField : static_cast<uint8_t>(kSpot);
    mModel.draftField = kSpot;

    mBody = Theme::container(mRoot, 0, 0, 240, 240);
    mUp   = chevron(mRoot, mUpPts, kUpY, true);
    mDown = chevron(mRoot, mDownPts, kDownY, false);

    mMenuBox = Theme::container(mRoot, 0, 0, 240, 240);
    mMenu = std::make_unique<ListMenu>(mMenuBox, ListMenu::pauseGeometry());
    mMenu->setCircular(false);
    fillItems();
    mMenu->setItems(mItems, mItemCount, 0);

    mHints = std::make_unique<Hints>(mRoot);
    mHints->set(Hints::Dot, Hints::Dot, Hints::Dot, Hints::Dot);

    render();
}

// --- Drawing -----------------------------------------------------------------

void StepBuilderScreen::render()
{
    lv_obj_clean(mBody);
    const score::Workout& d = mModel.draft();
    const score::Step& st   = step();

    char buf[40];
    snprintf(buf, sizeof(buf), "%s STEP %u", mModel.draftIsNew ? "NEW" : "EDIT",
             static_cast<unsigned>(mModel.draftStep + 1));
    Theme::header(mBody, buf, kHeaderY);

    // Spot row: the full name, else the part after " · ".
    const char* name        = spotName(st.spot);
    const char* spotTexts[] = { name, Theme::spotShort(name) };
    row(mBody, kSpotY, "Spot", !mMenuOpen && mField == kSpot, spotTexts, 2, Theme::kText);

    // Goal row: "Attempts 10", "Makes 5", "Time 60 s".
    char goal[24];
    if (isTime(st.goal.type)) {
        snprintf(goal, sizeof(goal), "Time %u s", static_cast<unsigned>(st.goal.value));
    } else {
        snprintf(goal, sizeof(goal), "%s %u", goalName(st.goal.type), static_cast<unsigned>(st.goal.value));
    }
    const char* goalTexts[] = { goal };
    row(mBody, kGoalY, "Goal", !mMenuOpen && (mField == kGoalType || mField == kValue), goalTexts, 1, Theme::kText);

    Theme::setHidden(mMenuBox, !mMenuOpen);
    if (mMenuOpen) {
        // The menu replaces the box and the Rest row; a line above it says
        // what is being saved (or shows the toast).
        setChevron(mUp, true, true);
        setChevron(mDown, true, true);
        if (mToastText) {
            Theme::label(mBody, Font::Regular14, mToastText, 20, Theme::topFor(Font::Regular14, kSummaryY), 200,
                         LV_TEXT_ALIGN_CENTER, Theme::kBad);
        } else {
            char wname[score::kNameLen + 1];
            snprintf(wname, sizeof(wname), "%.*s", static_cast<int>(score::kNameLen), d.name);
            snprintf(buf, sizeof(buf), "%s%s%u step%s", wname, Theme::kMidDot, static_cast<unsigned>(d.stepCount),
                     d.stepCount == 1 ? "" : "s");
            const char* texts[] = { buf, wname };
            Theme::fitLabel(mBody, texts, 2, kRowFonts, 1, kRowLeft, kRowRight - kRowLeft, kSummaryY,
                            LV_TEXT_ALIGN_CENTER, Theme::kDim);
        }
        return;
    }

    renderBox(st);

    char rest[12];
    if (st.rest_s == 0) {
        snprintf(rest, sizeof(rest), "none");
    } else {
        snprintf(rest, sizeof(rest), "%u s", static_cast<unsigned>(st.rest_s));
    }
    const char* restTexts[] = { rest };
    row(mBody, kRestY, "Rest", mField == kRest, restTexts, 1, mField == kRest ? Theme::kText : Theme::kDim);
}

void StepBuilderScreen::renderBox(const score::Step& st)
{
    lv_obj_t* box = Theme::container(mBody, 120 - kBoxW / 2, kBoxY - kBoxH / 2, kBoxW, kBoxH);
    lv_obj_set_style_border_width(box, kBoxBorder, LV_PART_MAIN);
    lv_obj_set_style_border_color(box, Theme::rgb(Theme::kAccent), LV_PART_MAIN);
    lv_obj_set_style_border_opa(box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(box, kBoxRadius, LV_PART_MAIN);

    const int32_t x = 120 - kBoxInnerW / 2;
    char value[12];
    const char* unit = "";
    bool up   = true;
    bool down = true;
    switch (mField) {
        case kSpot: {
            const char* name    = spotName(st.spot);
            const char* texts[] = { name, Theme::spotShort(name) };
            Theme::fitLabel(mBody, texts, 2, kSpotFonts, 3, x, kBoxInnerW, kValueY, LV_TEXT_ALIGN_CENTER,
                            Theme::kText);
            unit = "SELECT: court";
        } break;
        case kGoalType:
            Theme::label(mBody, Font::Medium25, goalName(st.goal.type), x, Theme::topFor(Font::Medium25, kValueY),
                         kBoxInnerW);
            unit = "goal";
            break;
        case kValue: {
            const Values& l = valuesFor(st.goal.type);
            snprintf(value, sizeof(value), "%u", static_cast<unsigned>(st.goal.value));
            Theme::label(mBody, Font::SemiBold40, value, x, Theme::topFor(Font::SemiBold40, kValueY), kBoxInnerW);
            unit = isTime(st.goal.type) ? "s"
                 : st.goal.type == static_cast<uint8_t>(score::GoalType::Makes) ? "makes" : "shots";
            up   = stepValue(l, st.goal.value, 1) != st.goal.value;
            down = stepValue(l, st.goal.value, -1) != st.goal.value;
        } break;
        case kRest:
        default:
            if (st.rest_s == 0) {
                Theme::label(mBody, Font::Medium25, "none", x, Theme::topFor(Font::Medium25, kValueY), kBoxInnerW);
                unit = "rest";
            } else {
                snprintf(value, sizeof(value), "%u", static_cast<unsigned>(st.rest_s));
                Theme::label(mBody, Font::SemiBold40, value, x, Theme::topFor(Font::SemiBold40, kValueY), kBoxInnerW);
                unit = "s rest";
            }
            up   = stepValue(kRests, st.rest_s, 1) != st.rest_s;
            down = stepValue(kRests, st.rest_s, -1) != st.rest_s;
            break;
    }
    Theme::label(mBody, Font::Regular14, unit, x, Theme::topFor(Font::Regular14, kUnitY), kBoxInnerW,
                 LV_TEXT_ALIGN_CENTER, Theme::kDim);
    // An arrow greys out at the end of a list (Spot and Goal type cycle).
    setChevron(mUp, false, up);
    setChevron(mDown, false, down);
}

// --- Fields ------------------------------------------------------------------

void StepBuilderScreen::adjust(int8_t dir)
{
    score::Step& st = step();
    switch (mField) {
        case kSpot:
            st.spot = static_cast<uint8_t>((st.spot + score::kSpotCount + dir) % score::kSpotCount);
            break;
        case kGoalType: {
            const uint8_t type = st.goal.type < kGoalTypes ? st.goal.type : 0;
            const uint8_t next = static_cast<uint8_t>((type + kGoalTypes + dir) % kGoalTypes);
            if (isTime(type) != isTime(next)) {
                // Shots <-> seconds: the value this step last had in the new
                // kind, else the nearest entry of its list.
                mStash[isTime(type) ? 1 : 0] = st.goal.value;
                const uint16_t stashed = mStash[isTime(next) ? 1 : 0];
                st.goal.value = stashed ? stashed : nearest(valuesFor(next), st.goal.value);
            }
            st.goal.type = next;
        } break;
        case kValue:
            st.goal.value = stepValue(valuesFor(st.goal.type), st.goal.value, dir);
            break;
        case kRest:
            st.rest_s = static_cast<uint8_t>(stepValue(kRests, st.rest_s, dir));
            break;
        default:
            break;
    }
    render();
}

void StepBuilderScreen::select()
{
    switch (mField) {
        case kSpot:
            // The full court; it writes the spot and comes back here.
            mModel.pickerMode = Model::PickerMode::StepSpot;
            mModel.pickerSpot = step().spot < score::kSpotCount ? step().spot : 0;
            ScreenManager::instance().goTo(ScreenId::SpotPicker);
            break;
        case kGoalType:
        case kValue:
            ++mField;
            render();
            break;
        case kRest:
        default:
            openMenu();
            break;
    }
}

void StepBuilderScreen::back()
{
    if (mField > kSpot) {
        --mField;
        render();
    } else if (mModel.draftStep > 0) {
        goToStep(static_cast<uint8_t>(mModel.draftStep - 1), kRest);
    } else {
        // Drop the draft; a stored workout being edited stays as it was.
        mModel.draft() = score::Workout {};
        ScreenManager::instance().goTo(ScreenId::Workouts);
    }
}

void StepBuilderScreen::goToStep(uint8_t index, uint8_t field)
{
    if (mConfirming) {
        setConfirm(false);
    }
    mModel.draftStep = index;
    mField           = field;
    mMenuOpen        = false;
    mStash[0] = mStash[1] = 0;
    render();
}

// --- Mini-menu ---------------------------------------------------------------

void StepBuilderScreen::fillItems()
{
    const uint8_t n = mModel.draft().stepCount;
    mItemCount = 0;
    auto add = [this](Item id, const ListMenu::Item& it) {
        mIds[mItemCount]   = id;
        mItems[mItemCount] = it;
        ++mItemCount;
    };
    // Add step / Save workout always lead, so the menu opens on the same
    // rows on every step. Next step is the only way forward to a later step
    // (BACK goes back).
    add(kAddStep, { "Add step", nullptr, n < score::kMaxSteps ? 0u : Theme::kDim });
    add(kSave, { "Save workout", nullptr, 0 });
    if (mModel.draftStep + 1 < n) {
        add(kNextStep, { "Next step", nullptr, 0 });
    }
    add(kDeleteStep, { "Delete step", nullptr, n > 1 ? Theme::kBad : Theme::kDim });
    if (!mModel.draftIsNew) {   // a new draft has nothing stored to delete
        if (runningWorkout()) {
            add(kDeleteWorkout, kDeleteWorkoutDimItem);   // the service would refuse it
        } else {
            add(kDeleteWorkout, mConfirming ? kConfirmItem : kDeleteWorkoutItem);
        }
    }
}

void StepBuilderScreen::openMenu()
{
    mMenuOpen = true;
    fillItems();
    mMenu->setItems(mItems, mItemCount, 0);
    render();
}

void StepBuilderScreen::closeMenu()
{
    goToStep(mModel.draftStep, kRest);
}

void StepBuilderScreen::menuSelect()
{
    const Item id = mIds[mMenu->selected()];
    if (id != kDeleteWorkout && mConfirming) {
        setConfirm(false);
    }
    switch (id) {
        case kNextStep:   goToStep(static_cast<uint8_t>(mModel.draftStep + 1), kSpot); break;
        case kAddStep:    addStep(); break;
        case kSave:       save(); break;
        case kDeleteStep: deleteStep(); break;
        case kDeleteWorkout:
            if (runningWorkout()) {
                break;   // greyed: the session in progress runs this workout
            }
            if (!mConfirming) {
                setConfirm(true);
                break;
            }
            setConfirm(false);
            // The service answers with the refreshed list, which the
            // Workouts screen picks up (onWorkouts()).
            setBusy(true);
            mModel.deleteWorkout(mModel.draft().id);
            mModel.draft() = score::Workout {};
            ScreenManager::instance().goTo(ScreenId::Workouts);
            break;
        default:
            break;
    }
}

bool StepBuilderScreen::runningWorkout() const
{
    return mModel.sessionRunning() && mModel.draft().id != 0 && mModel.snapshot().workoutId == mModel.draft().id;
}

void StepBuilderScreen::addStep()
{
    score::Workout& d = mModel.draft();
    if (d.stepCount >= score::kMaxSteps) {
        return;
    }
    // A copy of this step right after it; later steps move up one.
    const uint8_t at = static_cast<uint8_t>(mModel.draftStep + 1);
    for (uint8_t i = d.stepCount; i > at; --i) {
        d.steps[i] = d.steps[i - 1];
    }
    d.steps[at] = d.steps[mModel.draftStep];
    ++d.stepCount;
    goToStep(at, kSpot);
}

void StepBuilderScreen::deleteStep()
{
    score::Workout& d = mModel.draft();
    if (d.stepCount <= 1) {
        return;
    }
    for (uint8_t i = mModel.draftStep; i + 1 < d.stepCount; ++i) {
        d.steps[i] = d.steps[i + 1];
    }
    --d.stepCount;
    d.steps[d.stepCount] = score::Step {};
    goToStep(mModel.draftStep < d.stepCount ? mModel.draftStep : static_cast<uint8_t>(d.stepCount - 1), kSpot);
}

void StepBuilderScreen::save()
{
    score::Workout& d = mModel.draft();
    bool ok = d.stepCount >= 1 && d.stepCount <= score::kMaxSteps;
    for (uint8_t i = 0; ok && i < d.stepCount; ++i) {
        const score::Step& st = d.steps[i];
        ok = st.goal.value > 0 && st.spot < score::kSpotCount && st.goal.type < kGoalTypes;
    }
    if (!ok) {
        toast("Check the steps");
        return;
    }
    if (mModel.draftIsNew && mModel.userWorkoutCount() >= score::kMaxUserWorkouts) {
        toast("Store full (10)");
        return;
    }
    // Persisted as is: zero the pads and the unused steps.
    for (uint8_t i = 0; i < score::kMaxSteps; ++i) {
        if (i < d.stepCount) {
            d.steps[i].goal.pad = 0;
        } else {
            d.steps[i] = score::Step {};
        }
    }
    setBusy(true);
    mModel.saveWorkout(d);   // WORKOUT_SAVED -> onWorkoutSaved()
}

void StepBuilderScreen::setConfirm(bool on)
{
    if (mConfirmTimer) {
        lv_timer_delete(mConfirmTimer);
        mConfirmTimer = nullptr;
    }
    mConfirming = on;
    fillItems();
    mMenu->refresh();
    if (on) {
        mConfirmTimer = lv_timer_create(&StepBuilderScreen::confirmTimerCb, kConfirmMs, this);
    }
}

void StepBuilderScreen::confirmTimerCb(lv_timer_t* t)
{
    static_cast<StepBuilderScreen*>(lv_timer_get_user_data(t))->setConfirm(false);   // deletes the timer
}

void StepBuilderScreen::setBusy(bool on)
{
    if (mBusyTimer) {
        lv_timer_delete(mBusyTimer);
        mBusyTimer = nullptr;
    }
    mBusy = on;
    if (on) {
        mBusyTimer = lv_timer_create(&StepBuilderScreen::busyTimerCb, kBusyMs, this);
    }
}

void StepBuilderScreen::busyTimerCb(lv_timer_t* t)
{
    // No WORKOUT_SAVED in time (e.g. the message was lost): give the keys
    // back rather than leave the screen dead.
    auto* self = static_cast<StepBuilderScreen*>(lv_timer_get_user_data(t));
    self->setBusy(false);   // deletes the timer
    self->toast("No reply");
}

void StepBuilderScreen::toast(const char* text)
{
    if (mToastTimer) {
        lv_timer_delete(mToastTimer);
    }
    mToastText  = text;
    mToastTimer = lv_timer_create(&StepBuilderScreen::toastTimerCb, kToastMs, this);
    render();
}

void StepBuilderScreen::toastTimerCb(lv_timer_t* t)
{
    auto* self = static_cast<StepBuilderScreen*>(lv_timer_get_user_data(t));
    lv_timer_delete(self->mToastTimer);
    self->mToastTimer = nullptr;
    self->mToastText  = nullptr;
    self->render();
}

// --- Keys and model events -----------------------------------------------------

void StepBuilderScreen::onKey(uint8_t code)
{
    const KeyInput::Action a = mKeys.feed(code);
    if (!KeyInput::isActivate(a) || mBusy) {
        return;
    }
    if (mMenuOpen) {
        switch (a.btn) {
            case KeyInput::Btn::Up:
            case KeyInput::Btn::Down:
                if (a.btn == KeyInput::Btn::Up) {
                    mMenu->prev();
                } else {
                    mMenu->next();
                }
                if (mConfirming) {
                    setConfirm(false);
                }
                break;
            case KeyInput::Btn::Select: menuSelect(); break;
            case KeyInput::Btn::Back:   closeMenu(); break;
            case KeyInput::Btn::None:   break;
        }
        return;
    }
    switch (a.btn) {
        case KeyInput::Btn::Up:     adjust(1); break;
        case KeyInput::Btn::Down:   adjust(-1); break;
        case KeyInput::Btn::Select: select(); break;
        case KeyInput::Btn::Back:   back(); break;
        case KeyInput::Btn::None:   break;
    }
}

void StepBuilderScreen::onWorkoutSaved(uint8_t id)
{
    if (!mBusy) {
        return;
    }
    setBusy(false);
    if (id == 0) {
        // Refused: the store is full (an add), the workout is gone, or its
        // steps were out of range.
        toast("Not saved");
        return;
    }
    // The Model marked the list held stale on WORKOUT_SAVED (the service
    // streams the fresh one right after it), so the preview waits for it.
    mModel.selectedWorkoutId = id;
    ScreenManager::instance().goTo(ScreenId::WorkoutPreview);
}

void StepBuilderScreen::onSuspend()
{
    mKeys.cancel();
}
