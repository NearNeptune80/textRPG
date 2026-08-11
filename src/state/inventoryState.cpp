#include "state/inventoryState.h"

#include <algorithm>
#include <memory>

#include "core/game.h"
#include "state/explorationState.h"

void inventoryState::initialise(game* gameContext) {}

void inventoryState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->selectedInventoryIndex = -1;
        gameContext->selectedEquipmentSlot = equipSlot::NONE;
        gameContext->refreshActionGrid();
    }
}

void inventoryState::onExit(game* gameContext) {}

void inventoryState::update(game* gameContext, float deltaTime) {}

void inventoryState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_I)
    {
        gameContext->changeState(std::make_unique<explorationState>());
        return;
    }
}

void inventoryState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
    else if (cmd.type == CommandType::SELECT_INVENTORY_SLOT)
    {
        gameContext->selectedInventorySide = cmd.intPayload1;
        gameContext->selectedInventoryIndex = cmd.intPayload2;
        gameContext->selectedEquipmentSlot = equipSlot::NONE;
        gameContext->refreshActionGrid();
    }
    else if (cmd.type == CommandType::SELECT_EQUIPMENT_SLOT)
    {
        gameContext->selectedEquipmentSlot = static_cast<equipSlot>(cmd.intPayload1);
        gameContext->selectedInventoryIndex = -1;
        gameContext->refreshActionGrid();
    }
}