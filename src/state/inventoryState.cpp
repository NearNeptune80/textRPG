#include "state/inventoryState.h"

#include <algorithm>
#include <memory>

#include "core/game.h"
#include "state/characterCreationState.h"
#include "state/explorationState.h"

inventoryState::inventoryState(std::unique_ptr<iGameState> returnState)
    : m_returnState(std::move(returnState))
{
}

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

void inventoryState::goBack(game* gameContext)
{
    if (!gameContext) return;
    if (m_returnState)
    {
        if (auto* cc = dynamic_cast<characterCreationState*>(m_returnState.get()))
        {
            auto tabs = cc->getActiveTabs();
            for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
            {
                if (tabs[i] == EditorTabId::NAME_FINISH)
                {
                    cc->step = i;
                    break;
                }
            }
        }
        gameContext->changeState(std::move(m_returnState));
    }
    else
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
}

void inventoryState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        goBack(gameContext);
    }
    else if (cmd.type == CommandType::OPEN_INVENTORY)
    {
        goBack(gameContext);
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