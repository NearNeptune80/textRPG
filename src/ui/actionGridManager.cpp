#include "ui/actionGridManager.h"

#include <format>
#include <memory>

#include "core/game.h"
#include "entities/entity.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"

void ActionGridManager::refresh(game* gameContext)
{
    if (!gameContext) return;

    gameContext->activeButtons.clear();

    iGameState* currentState = gameContext->getActiveState();
    if (!currentState) return;

    // 1. Encounter Resolution Hub Actions
    if (auto resState = dynamic_cast<encounterResolutionState*>(currentState))
    {
        const auto& records = resState->getDefeatedRecords();
        size_t selectedIdx = resState->getSelectedIndex();

        if (records.size() > 1)
        {
            for (size_t i = 0; i < records.size(); ++i)
            {
                actionButton targetBtn;
                targetBtn.label = std::format("Target: {}", records[i].npc ? records[i].npc->name : "Enemy");
                targetBtn.isEnabled = (i != selectedIdx);
                targetBtn.onClick = [gameContext, i]() {
                    gameContext->handleCommand({ CommandType::SELECT_RESOLUTION_TARGET, static_cast<int>(i), 0, "" });
                };
                gameContext->activeButtons.push_back(targetBtn);
            }
        }

        if (selectedIdx < records.size())
        {
            const auto& rec = records[selectedIdx];

            actionButton lootBtn;
            lootBtn.label = rec.isLooted ? "Looted" : "Loot Items & Gold";
            lootBtn.isEnabled = !rec.isLooted;
            lootBtn.onClick = [gameContext]() {
                gameContext->handleCommand({ CommandType::LOOT_ENEMY, 0, 0, "" });
            };
            gameContext->activeButtons.push_back(lootBtn);

            actionButton stripBtn;
            stripBtn.label = rec.isStripped ? "Stripped" : "Strip Clothing";
            stripBtn.isEnabled = !rec.isStripped;
            stripBtn.onClick = [gameContext]() {
                gameContext->handleCommand({ CommandType::STRIP_ENEMY, 0, 0, "" });
            };
            gameContext->activeButtons.push_back(stripBtn);

            actionButton sexBtn;
            sexBtn.label = rec.hadSex ? "Erotic Interaction (Repeat)" : "Interactive Sex";
            sexBtn.isEnabled = true;
            sexBtn.onClick = [gameContext]() {
                gameContext->handleCommand({ CommandType::INTERACTIVE_SEX, 0, 0, "" });
            };
            gameContext->activeButtons.push_back(sexBtn);

            actionButton subjugateBtn;
            subjugateBtn.label = rec.isSubjugated ? "Subjugated" : "Subjugate";
            subjugateBtn.isEnabled = !rec.isSubjugated;
            subjugateBtn.onClick = [gameContext]() {
                gameContext->handleCommand({ CommandType::SUBJUGATE_ENEMY, 0, 0, "" });
            };
            gameContext->activeButtons.push_back(subjugateBtn);

            actionButton releaseBtn;
            releaseBtn.label = rec.isReleased ? "Released" : "Release";
            releaseBtn.isEnabled = !rec.isReleased;
            releaseBtn.onClick = [gameContext]() {
                gameContext->handleCommand({ CommandType::RELEASE_ENEMY, 0, 0, "" });
            };
            gameContext->activeButtons.push_back(releaseBtn);
        }

        actionButton leaveBtn;
        leaveBtn.label = "Leave Resolution Hub";
        leaveBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
        };
        gameContext->activeButtons.push_back(leaveBtn);
        return;
    }

    // 2. Inventory State Actions
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
            if (gameContext->selectedInventorySide == 0)
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
            else if (gameContext->selectedInventorySide == 1)
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

    // 3. Combat State Actions
    if (auto combat = dynamic_cast<CombatState*>(currentState))
    {
        // Debug Simulation Buttons
        actionButton winBtn;
        winBtn.label = "[Simulate Win]";
        winBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "WIN" });
        };
        gameContext->activeButtons.push_back(winBtn);

        actionButton lossBtn;
        lossBtn.label = "[Simulate Defeat]";
        lossBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "DEFEAT" });
        };
        gameContext->activeButtons.push_back(lossBtn);

        actionButton escBtn;
        escBtn.label = "[Simulate Escape]";
        escBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ESCAPE" });
        };
        gameContext->activeButtons.push_back(escBtn);

        actionButton surrBtn;
        surrBtn.label = "[Simulate Surrender]";
        surrBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SURRENDER" });
        };
        gameContext->activeButtons.push_back(surrBtn);

        // Standard Combat Actions
        actionButton strikeBtn;
        strikeBtn.label = "Strike";
        strikeBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "STRIKE" });
        };
        gameContext->activeButtons.push_back(strikeBtn);

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

    // 4. Exploration Movement Shortcuts
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