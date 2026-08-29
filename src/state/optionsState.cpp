#include "state/optionsState.h"

#include <memory>

#include "core/game.h"
#include "state/explorationState.h"
#include "state/mainMenuState.h"

void optionsState::initialise(game* gameContext) {}

void optionsState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void optionsState::onExit(game* gameContext) {}

void optionsState::update(game* gameContext, float deltaTime) {}

void optionsState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
    {
        if (gameContext->getActiveMap())
        {
            gameContext->changeState(std::make_unique<explorationState>());
        }
        else
        {
            gameContext->changeState(std::make_unique<mainMenuState>());
        }
    }
}

void optionsState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        if (gameContext->getActiveMap())
        {
            gameContext->changeState(std::make_unique<explorationState>());
        }
        else
        {
            gameContext->changeState(std::make_unique<mainMenuState>());
        }
    }
    else if (cmd.type == CommandType::CYCLE_SETTING_OPTION)
    {
        if (cmd.stringPayload == "pregnancy")
        {
            gameContext->settings.content.pregnancyEnabled = !gameContext->settings.content.pregnancyEnabled;
        }
        else if (cmd.stringPayload == "lactation")
        {
            gameContext->settings.content.lactationEnabled = !gameContext->settings.content.lactationEnabled;
        }
        else if (cmd.stringPayload == "difficulty")
        {
            float& diff = gameContext->settings.gameplay.difficultyMultiplier;
            if (diff <= 0.8f) diff = 1.0f;
            else if (diff <= 1.1f) diff = 1.5f;
            else diff = 0.75f;
        }
        gameContext->refreshActionGrid();
    }
}
