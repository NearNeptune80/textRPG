#include "state/mainMenuState.h"

#include <iostream>
#include <memory>

#include "core/game.h"
#include "state/explorationState.h"
#include "state/optionsState.h"

void mainMenuState::initialise(game* gameContext) {}

void mainMenuState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void mainMenuState::onExit(game* gameContext) {}

void mainMenuState::update(game* gameContext, float deltaTime) {}

void mainMenuState::handleInput(game* gameContext, const SDL_Event& event) {}

void mainMenuState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::START_NEW_GAME || cmd.type == CommandType::CONTINUE_GAME)
    {
        if (!gameContext->getActiveMap())
        {
            gameContext->loadMap("data/maps/tavern.json", 3, 3);
        }
        gameContext->changeState(std::make_unique<explorationState>());
    }
    else if (cmd.type == CommandType::OPEN_SETTINGS)
    {
        gameContext->changeState(std::make_unique<optionsState>());
    }
    else if (cmd.type == CommandType::QUIT_GAME)
    {
        gameContext->isRunning = false;
    }
}
