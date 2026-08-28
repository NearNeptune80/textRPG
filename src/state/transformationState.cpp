#include "state/transformationState.h"

#include <memory>

#include "core/game.h"
#include "state/explorationState.h"

void transformationState::initialise(game* gameContext) {}

void transformationState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void transformationState::onExit(game* gameContext) {}

void transformationState::update(game* gameContext, float deltaTime) {}

void transformationState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
}

void transformationState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
}
