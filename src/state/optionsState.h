#pragma once

#include "state/iGameState.h"

enum class OptionsCategory
{
    GAMEPLAY,
    CONTENT,
    DEMOGRAPHICS,
    DISPLAY
};

/**
 * Headless state controller for Game Options and Settings.
 */
class optionsState : public iGameState
{
public:
    optionsState() = default;
    ~optionsState() override = default;

    OptionsCategory currentCategory{ OptionsCategory::GAMEPLAY };

    void initialise(game* gameContext) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;
};
