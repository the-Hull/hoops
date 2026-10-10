/**
 ******************************************************************************
 * @file    RestScreen.hpp
 * @brief   Rest between workout steps, and the "workout done" summary.
 *
 * docs/mockup-layout.md §9. An amber ring drains with the countdown; below
 * the time, the next step and the result of the step just finished. Keys:
 * UP +15 s, SELECT skip, DOWN pause. The finished step comes from the
 * Model's last STEP_DONE (or, when the GUI missed it, from the snapshot:
 * during Rest its step counts are still the finished step's); the
 * countdown from the snapshots. The service ends the rest by itself; an
 * Active snapshot returns to the shooting screen.
 *
 * Done mode (the last STEP_DONE says workoutDone): the session totals and
 * SELECT save (SAVED -> Results), UP keep shooting (the shooting screen,
 * extra shots), DOWN pause. The session stays Active in the service, so
 * running snapshots are ignored here.
 ******************************************************************************
 */

#ifndef REST_SCREEN_HPP
#define REST_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"
#include "gui/widgets/Ring.hpp"

class RestScreen : public Screen
{
public:
    explicit RestScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;

    void onSnapshot(const score::Snapshot& s) override;
    void onStepDone(const CustomMessage::StepDone& d) override;
    void onSaved(uint32_t id) override;
    void onSuspend() override;

protected:
    void build() override;

private:
    /// What the rest is about: the finished step and the one that follows.
    struct Info {
        uint8_t  stepIndex;       ///< finished step, 0-based
        uint8_t  stepCount;
        uint16_t makes, attempts;
        uint16_t rest_s;
        uint8_t  nextSpot, nextGoalType;
        uint16_t nextGoalValue;
        bool     hasNext;
    };

    bool doneMode() const;
    Info resolve() const;

    /// (Re)build the static part (everything but the countdown and ring).
    void buildContent();
    void buildRest(const Info& in);
    void buildDone();
    void showCountdown(uint16_t remaining);

    void save();

    KeyInput               mKeys;
    std::unique_ptr<Ring>  mRing;
    std::unique_ptr<Hints> mHints;

    lv_obj_t* mContent   = nullptr;   ///< emptied and refilled by buildContent()
    lv_obj_t* mCountdown = nullptr;   ///< rest mode only

    bool     mDone   = false;
    bool     mEnding = false;   ///< END_SESSION sent (done mode)
    bool     mSaved  = false;   ///< SAVED arrived; Results on its way
    uint16_t mTotal  = 0;       ///< ring's full scale: the longest rest seen (grows with +15 s)
    bool     mIgnorePaused = false;   ///< entered by a resume: Paused snapshots are stale...
    bool     mLiveSeen     = false;   ///< ...until the first Active / Rest one
};

#endif // REST_SCREEN_HPP
