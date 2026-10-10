/**
 ******************************************************************************
 * @file    StepBuilderScreen.hpp
 * @brief   The on-watch workout builder: one step at a time.
 *
 * docs/mockup-layout.md §5. Edits Model::draft() (seeded by the Workouts
 * list or the preview), step Model::draftStep, field by field: Spot, Goal
 * type, Value, Rest. The value box shows the active field; UP / DOWN change
 * it (Spot and Goal type cycle, Value and Rest step through their lists and
 * stop at the ends). SELECT moves on: on Spot it opens the full SpotPicker
 * (which comes back on Goal type), after Rest a mini-menu: Add step / Save
 * workout / Next step (on any step but the last) / Delete step / Delete
 * workout (asked twice; a stored workout only, greyed while a session runs
 * it). BACK goes a field back, to the previous step's Rest, or on the
 * first field of the first step drops the draft and returns to the list.
 *
 * Nothing reaches the service before Save: SAVE_WORKOUT is answered by
 * WORKOUT_SAVED (id 0 = refused, e.g. a full store; a "Not saved" toast)
 * and the refreshed list. On an id the preview of the saved workout opens;
 * the Model marks the list held stale on WORKOUT_SAVED, so the preview
 * waits for the fresh one rather than show the workout as it was. Keys are
 * ignored while a SAVE / DELETE is out; after kBusyMs without a reply they
 * come back with a "No reply" toast.
 ******************************************************************************
 */

#ifndef STEP_BUILDER_SCREEN_HPP
#define STEP_BUILDER_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/ListMenu.hpp"

class StepBuilderScreen : public Screen
{
public:
    explicit StepBuilderScreen(Model& model);
    ~StepBuilderScreen() override;

    void onKey(uint8_t code) override;

    void onWorkoutSaved(uint8_t id) override;
    void onSuspend() override;

    static constexpr uint32_t kConfirmMs = 3000;   ///< Delete workout confirmation window
    static constexpr uint32_t kToastMs   = 2000;
    static constexpr uint32_t kBusyMs    = 3000;   ///< SAVE / DELETE reply watchdog

protected:
    void build() override;

private:
    enum Field : uint8_t { kSpot, kGoalType, kValue, kRest, kFieldCount };
    enum Item : uint8_t { kNextStep, kAddStep, kSave, kDeleteStep, kDeleteWorkout, kItemCount };

    score::Step& step();

    /// Redraw the rows and the value box (or the menu) from the draft.
    void render();
    void renderBox(const score::Step& st);

    void adjust(int8_t dir);
    void select();
    void back();

    void openMenu();
    void closeMenu();
    void fillItems();
    void menuSelect();
    void addStep();
    void deleteStep();
    /// Leave the menu (if open) onto step @p index, field @p field.
    void goToStep(uint8_t index, uint8_t field);
    void save();
    void setConfirm(bool on);
    /// The draft is the workout of the session in progress (not deletable).
    bool runningWorkout() const;
    /// mBusy, with the kBusyMs watchdog that clears it.
    void setBusy(bool on);
    void toast(const char* text);

    static void confirmTimerCb(lv_timer_t* t);
    static void toastTimerCb(lv_timer_t* t);
    static void busyTimerCb(lv_timer_t* t);

    KeyInput                  mKeys;
    std::unique_ptr<ListMenu> mMenu;
    std::unique_ptr<Hints>    mHints;

    lv_obj_t* mBody    = nullptr;   ///< rows and box, emptied and refilled by render()
    lv_obj_t* mMenuBox = nullptr;   ///< holds the mini-menu; hidden with it
    lv_obj_t* mUp      = nullptr;   ///< value box arrows
    lv_obj_t* mDown    = nullptr;

    // lv_line keeps a pointer to its points: they live here.
    lv_point_precise_t mUpPts[3]   = {};
    lv_point_precise_t mDownPts[3] = {};

    ListMenu::Item mItems[kItemCount] = {};   ///< the menu's rows...
    Item           mIds[kItemCount]   = {};   ///< ...and what each one does
    uint8_t        mItemCount = 0;

    uint8_t  mField    = kSpot;
    bool     mMenuOpen = false;
    /// The last value per goal kind (0 shots, 1 time) of this step, so
    /// cycling the goal type back restores it; 0 = none yet.
    uint16_t mStash[2] = {};

    lv_timer_t* mConfirmTimer = nullptr;
    lv_timer_t* mToastTimer   = nullptr;
    lv_timer_t* mBusyTimer    = nullptr;
    const char* mToastText    = nullptr;   ///< shown in place of the menu's summary line
    bool        mConfirming   = false;
    bool        mBusy         = false;     ///< SAVE / DELETE sent: keys ignored
};

#endif // STEP_BUILDER_SCREEN_HPP
