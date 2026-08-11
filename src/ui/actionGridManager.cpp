#include "ui/actionGridManager.h"

#include "core/game.h"
#include "entities/entity.h"
#include "state/combatState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"

void ActionGridManager::refresh(game* gameContext)
{
    if (!gameContext) return;

    gameContext->activeButtons.clear();

    iGameState* currentState = gameContext->getActiveState();
    if (!currentState) return;

    // 1. Inventory State Actions
    if (dynamic_cast<inventoryState*>(currentState))
    {
        actionButton closeBtn;
        closeBtn.label = "Close Inventory (I)";
        closeBtn.onClick = [gameContext]() {
            gameContext->changeState(std::make_unique<explorationState>());
        };
        gameContext->activeButtons.push_back(closeBtn);

        if (gameContext->selectedInventoryIndex != -1)
        {
            if (gameContext->selectedInventorySide == 0) // Backpack Item
            {
                actionButton equipBtn;
                equipBtn.label = "Equip";
                equipBtn.onClick = [gameContext]() {
                    gameContext->handleEquipAction(gameContext->selectedInventoryIndex);
                };
                gameContext->activeButtons.push_back(equipBtn);

                actionButton dropBtn;
                dropBtn.label = "Drop 1";
                dropBtn.onClick = [gameContext]() {
                    gameContext->handleDropAction(gameContext->selectedInventoryIndex, 1);
                };
                gameContext->activeButtons.push_back(dropBtn);
            }
            else if (gameContext->selectedInventorySide == 1) // Ground Item
            {
                actionButton pickBtn;
                pickBtn.label = "Pickup 1";
                pickBtn.onClick = [gameContext]() {
                    gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 1);
                };
                gameContext->activeButtons.push_back(pickBtn);
            }
        }
        else if (gameContext->selectedEquipmentSlot != equipSlot::NONE)
        {
            actionButton unequipBtn;
            unequipBtn.label = "Unequip";
            unequipBtn.onClick = [gameContext]() {
                gameContext->handleUnequipAction(gameContext->selectedEquipmentSlot);
            };
            gameContext->activeButtons.push_back(unequipBtn);
        }
        return;
    }

    // 2. Combat State Actions
    if (auto combat = dynamic_cast<CombatState*>(currentState))
    {
        actionButton endTurnBtn;
        endTurnBtn.label = "End Turn";
        endTurnBtn.onClick = [combat, gameContext]() {
            combat->handleEndTurn(gameContext);
        };
        gameContext->activeButtons.push_back(endTurnBtn);

        actionButton fleeBtn;
        fleeBtn.label = "Attempt Run";
        fleeBtn.onClick = [combat, gameContext]() {
            combat->handleRunAttempt(gameContext);
        };
        gameContext->activeButtons.push_back(fleeBtn);

        actionButton surrenderBtn;
        surrenderBtn.label = "Surrender";
        surrenderBtn.onClick = [combat, gameContext]() {
            combat->handleSurrender(gameContext);
        };
        gameContext->activeButtons.push_back(surrenderBtn);
        return;
    }

    // 3. Exploration Movement Shortcuts
    if (dynamic_cast<explorationState*>(currentState))
    {
        actionButton northBtn;
        northBtn.label = "Move North (W)";
        northBtn.onClick = [gameContext]() {
            gameContext->movePlayer(gameContext->gridX, gameContext->gridY - 1);
        };
        gameContext->activeButtons.push_back(northBtn);

        actionButton southBtn;
        southBtn.label = "Move South (S)";
        southBtn.onClick = [gameContext]() {
            gameContext->movePlayer(gameContext->gridX, gameContext->gridY + 1);
        };
        gameContext->activeButtons.push_back(southBtn);

        actionButton westBtn;
        westBtn.label = "Move West (A)";
        westBtn.onClick = [gameContext]() {
            gameContext->movePlayer(gameContext->gridX - 1, gameContext->gridY);
        };
        gameContext->activeButtons.push_back(westBtn);

        actionButton eastBtn;
        eastBtn.label = "Move East (D)";
        eastBtn.onClick = [gameContext]() {
            gameContext->movePlayer(gameContext->gridX + 1, gameContext->gridY);
        };
        gameContext->activeButtons.push_back(eastBtn);
    }
}