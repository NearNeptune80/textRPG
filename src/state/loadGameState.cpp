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

void loadGameState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        if (isEditingSaveName)
        {
            isEditingSaveName = false;
        }
        else
        {
            goBack(gameContext);
        }
    }
    else if (cmd.type == CommandType::TEXT_INPUT)
    {
        if (isEditingSaveName)
        {
            newSaveNameInput += cmd.stringPayload;
        }
    }
    else if (cmd.type == CommandType::TEXT_BACKSPACE)
    {
        if (isEditingSaveName && !newSaveNameInput.empty())
        {
            newSaveNameInput.pop_back();
        }
    }
    else if (cmd.type == CommandType::CONFIRM_INPUT)
    {
        isEditingSaveName = false;
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
