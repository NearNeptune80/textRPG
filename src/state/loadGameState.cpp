#include "state/loadGameState.h"

#include <memory>

#include "core/game.h"
#include "save/saveManager.h"
#include "state/explorationState.h"
#include "state/mainMenuState.h"

void loadGameState::initialise(game* gameContext) {}

void loadGameState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void loadGameState::onExit(game* gameContext) {}

void loadGameState::update(game* gameContext, float deltaTime) {}

void loadGameState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
    {
        gameContext->changeState(std::make_unique<mainMenuState>());
    }
}

void loadGameState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        gameContext->changeState(std::make_unique<mainMenuState>());
    }
    else if (cmd.type == CommandType::LOAD_GAME_SLOT)
    {
        if (!cmd.stringPayload.empty())
        {
            if (saveManager::loadFromFile(gameContext, cmd.stringPayload))
            {
                gameContext->changeState(std::make_unique<explorationState>());
            }
        }
    }
}
