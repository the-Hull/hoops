/**
 ******************************************************************************
 * @file    ResultsScreen.hpp
 * @brief   Session results (placeholder: header and "Coming soon").
 *
 * Reached after End & save, with Model::requestResults() already sent. BACK
 * returns Home. The four result pages (docs/mockup-layout.md §11-§14)
 * replace this in a later task.
 ******************************************************************************
 */

#ifndef RESULTS_SCREEN_HPP
#define RESULTS_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Hints.hpp"
#include "gui/widgets/KeyInput.hpp"

class ResultsScreen : public Screen
{
public:
    explicit ResultsScreen(Model& model);

    void onKey(uint8_t code) override;
    void onSuspend() override;

protected:
    void build() override;

private:

    KeyInput               mKeys;
    std::unique_ptr<Hints> mHints;
};

#endif // RESULTS_SCREEN_HPP
