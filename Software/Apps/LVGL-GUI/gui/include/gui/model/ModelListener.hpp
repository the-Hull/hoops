/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 *
 * The Model binds exactly one listener (the screen on display) and calls
 * these as service messages arrive; the payloads are the service's, copied
 * verbatim. onSuspend() lets a screen cancel a hold in progress, since no
 * button release reaches it while the GUI is off screen.
 ******************************************************************************
 */

#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <cstdint>

#include "Commands.hpp"
#include "score/SessionEngine.hpp"

class Model;

class ModelListener
{
public:
    virtual ~ModelListener() = default;

    virtual void onSnapshot(const score::Snapshot& s) { (void)s; }
    virtual void onStepDone(const CustomMessage::StepDone& d) { (void)d; }
    virtual void onUndoResult(bool ok, bool made) { (void)ok; (void)made; }
    virtual void onWorkouts() {}                          ///< user workout list complete
    virtual void onWorkoutSaved(uint8_t id) { (void)id; } ///< 0 = refused
    virtual void onResults() {}                           ///< a RESULTS reply arrived
    virtual void onHistory() {}
    virtual void onSaved(uint32_t id) { (void)id; }       ///< session saved under id
    virtual void onSettings() {}                          ///< a SETTINGS message arrived
    virtual void onSpotStats() {}                         ///< a SPOT_STATS reply arrived
    virtual void onIdleTimeout() {}
    virtual void onSuspend() {}
    virtual void onResume() {}

protected:
    Model* model = nullptr;   ///< set by Model::bind()
    friend class Model;
};

#endif // MODELLISTENER_HPP
