#include "ui/actionGridManager.h"

#include <format>
#include <memory>
#include <unordered_set>

#include "core/game.h"
#include "entities/entity.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/sexState.h"

void ActionGridManager::refresh(game* gameContext)
{
    if (!gameContext) return;

    iGameState* currentState = gameContext->getActiveState();
    if (!currentState) return;

    // Preserve existing choice buttons if in eventState and choices are already populated
    if (dynamic_cast<eventState*>(currentState) && !gameContext->activeButtons.empty())
    {
        return;
    }

    gameContext->activeButtons.clear();

    // 0. Dedicated Interactive Sex State Actions
    if (auto sex = dynamic_cast<sexState*>(currentState))
    {
        const auto& actions = sex->getAvailableActions();
        for (size_t i = 0; i < actions.size(); ++i)
        {
            actionButton btn;
            btn.label = actions[i].name;
            SexAction act = actions[i];
            btn.onClick = [gameContext, i]() {
                gameContext->handleCommand({ CommandType::EXECUTE_SEX_ACTION, static_cast<int>(i), 0, "" });
            };
            gameContext->activeButtons.push_back(btn);
        }

        if (sex->isPlayerDominant())
        {
            static const std::vector<SexStance> stances = {
                SexStance::MISSIONARY, SexStance::FROM_BEHIND, SexStance::KNEELING, SexStance::STANDING, SexStance::LAP_SITTING
            };
            for (SexStance st : stances)
            {
                if (st != sex->getStance())
                {
                    actionButton stanceBtn;
                    stanceBtn.label = std::format("Stance: {}", sexStanceToString(st));
                    stanceBtn.onClick = [gameContext, st]() {
                        gameContext->handleCommand({ CommandType::CHANGE_SEX_STANCE, static_cast<int>(st), 0, "" });
                    };
                    gameContext->activeButtons.push_back(stanceBtn);
                }
            }
        }

        actionButton endBtn;
        endBtn.label = "End Scene";
        endBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::END_SEX_SCENE, 0, 0, "" });
        };
        gameContext->activeButtons.push_back(endBtn);
        return;
    }

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

    // 4. Exploration Movement & Interaction Shortcuts
    if (dynamic_cast<explorationState*>(currentState))
    {
        if (gameContext->map)
        {
            std::unordered_set<std::string> seenScenes;

            // A. Map-specific triggers at current coordinates
            auto mapTrigs = gameContext->map->getTriggersAt(gameContext->gridX, gameContext->gridY);
            for (const auto& trig : mapTrigs)
            {
                if (seenScenes.find(trig.sceneId) == seenScenes.end() && gameContext->checkConditions(trig.conditions))
                {
                    seenScenes.insert(trig.sceneId);
                    actionButton trigBtn;
                    trigBtn.label = trig.label.empty() ? "Interact" : trig.label;
                    std::string sId = trig.sceneId;
                    trigBtn.onClick = [gameContext, sId]() {
                        gameContext->loadScene(sId);
                    };
                    gameContext->activeButtons.push_back(trigBtn);
                }
            }

            // B. Global quest triggers at current coordinates
            auto questTrigs = questDatabase::getTriggersForLocation(gameContext->map->getId(), gameContext->gridX, gameContext->gridY);
            for (const auto& trig : questTrigs)
            {
                if (seenScenes.find(trig.sceneId) == seenScenes.end() && gameContext->checkConditions(trig.conditions))
                {
                    seenScenes.insert(trig.sceneId);
                    actionButton trigBtn;
                    trigBtn.label = trig.label.empty() ? "Interact" : trig.label;
                    std::string sId = trig.sceneId;
                    trigBtn.onClick = [gameContext, sId]() {
                        gameContext->loadScene(sId);
                    };
                    gameContext->activeButtons.push_back(trigBtn);
                }
            }

            // C. Persistent NPC at current tile
            auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
            if (tileData.persistentNPC)
            {
                actionButton npcBtn;
                npcBtn.label = std::format("Talk to {}", tileData.persistentNPC->name);
                npcBtn.onClick = [gameContext, npc = tileData.persistentNPC]() {
                    gameContext->triggerEncounter(npc);
                };
                gameContext->activeButtons.push_back(npcBtn);
            }

            // D. Ground items at current tile
            if (!tileData.droppedItems.empty())
            {
                actionButton groundBtn;
                groundBtn.label = std::format("Examine Ground ({} items)", tileData.droppedItems.size());
                groundBtn.onClick = [gameContext]() {
                    gameContext->changeState(std::make_unique<inventoryState>());
                };
                gameContext->activeButtons.push_back(groundBtn);
            }
        }

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