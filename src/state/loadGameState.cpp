#include "state/loadGameState.h"

#include <memory>

#include "core/game.h"
#include "save/saveManager.h"
#include "state/explorationState.h"
#include "state/mainMenuState.h"

loadGameState::loadGameState(SaveMenuMode mode, std::unique_ptr<iGameState> returnState)
    : m_mode(mode), m_returnState(std::move(returnState))
{
}

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

void loadGameState::goBack(game* gameContext)
{
    if (!gameContext) return;
    if (m_returnState)
    {
        gameContext->changeState(std::move(m_returnState));
    }
    else
    {
        gameContext->changeState(std::make_unique<mainMenuState>());
    }
}

void loadGameState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_TEXT_INPUT && isEditingSaveName)
    {
        newSaveNameInput += event.text.text;
    }
    else if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (isEditingSaveName)
        {
            if (event.key.key == SDLK_BACKSPACE && !newSaveNameInput.empty())
            {
                newSaveNameInput.pop_back();
            }
            else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER)
            {
                isEditingSaveName = false;
            }
            else if (event.key.key == SDLK_ESCAPE)
            {
                isEditingSaveName = false;
            }
        }
        else
        {
            if (event.key.key == SDLK_ESCAPE)
            {
                goBack(gameContext);
            }
        }
    }
}

void loadGameState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        goBack(gameContext);
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
