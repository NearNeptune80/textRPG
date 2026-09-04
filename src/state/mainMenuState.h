#pragma once

#include "state/iGameState.h"

/**
 * Headless state controller for the Game Main Menu.
 * Manages options for starting a new game, loading save files, settings, and quitting.
 */
class mainMenuState : public iGameState
{
public:
    mainMenuState() = default;
    ~mainMenuState() override = default;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;
};
