#pragma once

#include "state/iGameState.h"

/**
 * Headless state controller for the Load Game Screen.
 * Groups saves by character from newest to oldest and supports global quicksaves and character autosaves.
 */
class loadGameState : public iGameState
{
public:
    loadGameState() = default;
    ~loadGameState() override = default;

    void initialise(game* gameContext) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;
};
