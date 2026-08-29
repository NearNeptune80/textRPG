#include "state/mainMenuState.h"

#include <iostream>
#include <memory>

#include "core/game.h"
#include "save/saveManager.h"
#include "state/characterCreationState.h"
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

    if (cmd.type == CommandType::START_NEW_GAME)
    {
        gameContext->changeState(std::make_unique<characterCreationState>());
    }
    else if (cmd.type == CommandType::CONTINUE_GAME)
    {
        entity* p = gameContext->getPlayer();
        std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
        if (!saveManager::loadFromFile(gameContext, charName + "_QuickSave.json"))
        {
            if (!saveManager::loadFromFile(gameContext, "QuickSave.json"))
            {
                saveManager::loadFromFile(gameContext, "Hero_Initial.json");
            }
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
