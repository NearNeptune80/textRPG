#include "state/explorationState.h"

#include <iostream>
#include <memory>

#include "core/game.h"
#include "map/encounterResolver.h"
#include "save/saveManager.h"
#include "state/combatState.h"
#include "state/inventoryState.h"
#include "state/mainMenuState.h"

void explorationState::initialise(game* gameContext) {}

void explorationState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void explorationState::onExit(game* gameContext) {}

void explorationState::update(game* gameContext, float deltaTime) {}

void explorationState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::MOVE_PLAYER)
    {
        gameContext->movePlayer(cmd.intPayload1, cmd.intPayload2);
    }
    else if (cmd.type == CommandType::CLOSE_MENU)
    {
        gameContext->changeState(std::make_unique<mainMenuState>());
    }
}