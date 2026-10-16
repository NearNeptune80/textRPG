#include "ui/actionGridManager.h"

#include <format>
#include <fstream>
#include <memory>
#include <unordered_set>

#include "core/contentFilterManager.h"
#include "core/game.h"
#include "entities/namedCharacter.h"
#include "entities/npcGenerator.h"
#include "entities/perkDatabase.h"
#include "map/encounterResolver.h"
#include "save/saveManager.h"
#include "settings/settingsManager.h"
#include "state/characterCreationState.h"
#include "combat/weaponSkillDatabase.h"
#include "combat/combatActionDatabase.h"
#include "combat/spellDatabase.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/loadGameState.h"
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/phoneAppsState.h"
#include "state/sexState.h"
#include "state/shopState.h"
#include "state/transformationState.h"
#include "state/enchantingState.h"
#include "state/servantState.h"
#include "state/nurseryState.h"
#include "entities/offspringManager.h"
#include "entities/npcGenerator.h"
#include "items/fluidHarvesting.h"
#include "map/roomDatabase.h"
#include "core/servantManager.h"
#include "quest/questDatabase.h"
#include "items/merchantValuation.h"
#include "ui/theme.h"

namespace
{
    inline void padButtonsTo(game* g, size_t targetCount)
    {
        while (g->activeActionSlots.size() < targetCount)
        {
            ActionSlot btn;
            btn.label = "";
            btn.isEnabled = false;
            btn.isSelected = false;
            btn.onClick = nullptr;
            g->activeActionSlots.push_back(btn);
        }
    }

    inline void addBtn(game* g, std::string_view label, std::function<void()> onClick, bool enabled = true, bool selected = false, std::string_view description = "")
    {
        if (!label.empty() && !label.starts_with("["))
        {
            for (auto& existing : g->activeActionSlots)
            {
                if (existing.label == label)
                {
                    if (!existing.isEnabled && enabled)
                    {
                        existing.isEnabled = true;
                        existing.isSelected = selected;
                        existing.onClick = std::move(onClick);
                        existing.description = std::string(description);
                    }
                    return;
                }
            }
        }
        ActionSlot btn;
        btn.label = std::string(label);
        btn.description = std::string(description);
        btn.isEnabled = enabled;
        btn.isSelected = selected;
        btn.onClick = std::move(onClick);
        g->activeActionSlots.push_back(btn);
    }

    inline void addBackBtn(game* g, std::string_view label, std::function<void()> onClick, std::string_view description = "")
    {
        padButtonsTo(g, 14);
        ActionSlot btn;
        btn.label = std::string(label);
        btn.description = std::string(description);
        btn.isEnabled = true;
        btn.isSelected = false;
        btn.onClick = std::move(onClick);
        g->activeActionSlots.push_back(btn);
    }
}

void ActionGridManager::refresh(game* gameContext)
{
    if (!gameContext) return;

    iGameState* currentState = gameContext->getActiveState();
    if (!currentState) return;

    gameContext->activeActionSlots.clear();

    if (auto menu = dynamic_cast<mainMenuState*>(currentState))
    {
        bool inGame = (gameContext->getPlayer() != nullptr);

        addBtn(gameContext, "New Game", [gameContext]() {
            gameContext->handleCommand({ CommandType::START_NEW_GAME, 0, 0, "" });
        }, true, false, "Begin a new adventure with character creation.");

        addBtn(gameContext, "Save/Load", [gameContext, inGame]() {
            gameContext->changeState(std::make_unique<loadGameState>(inGame ? SaveMenuMode::SAVE_AND_LOAD : SaveMenuMode::LOAD_ONLY, std::make_unique<mainMenuState>()));
        }, true, false, inGame ? "Save current progress or load another save profile." : "Load a saved character profile.");

        addBtn(gameContext, "", nullptr, false);
        addBtn(gameContext, "", nullptr, false);

        addBtn(gameContext, "Quit", [gameContext]() {
            gameContext->handleCommand({ CommandType::QUIT_GAME, 0, 0, "" });
        }, true, false, "Exit the game application.");

        addBtn(gameContext, "Options", [gameContext]() {
            gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS, std::make_unique<mainMenuState>()));
        }, true, false, "Adjust display, resolution, themes, and keybindings.");

        addBtn(gameContext, "Content Options", [gameContext]() {
            gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS, std::make_unique<mainMenuState>()));
        }, true, false, "Configure gameplay content filters and demographic preferences.");

        padButtonsTo(gameContext, 14);
        addBtn(gameContext, "Continue", [gameContext, inGame]() {
            if (inGame)
            {
                gameContext->changeState(std::make_unique<explorationState>());
            }
            else
            {
                gameContext->handleCommand({ CommandType::CONTINUE_GAME, 0, 0, "" });
            }
        }, true, false, inGame ? "Return to active game exploration." : "Continue from your latest saved profile.");
        return;
    }

    if (auto cc = dynamic_cast<characterCreationState*>(currentState))
    {
        auto activeTabs = cc->getActiveTabs();
        int tabCount = static_cast<int>(activeTabs.size());
        if (cc->step >= tabCount) cc->step = std::max(0, tabCount - 1);

        if (tabCount > 1)
        {
            int startIdx = (cc->step >= 5 && tabCount > 5) ? 5 : 0;
            for (int i = startIdx; i < std::min(startIdx + 5, tabCount); ++i)
            {
                std::string btnLabel = std::format("{}. {}", i + 1, cc->getTabName(activeTabs[i]));
                bool isClothingTab = (activeTabs[i] == EditorTabId::WARDROBE);
                addBtn(gameContext, btnLabel, [cc, gameContext, i, isClothingTab]() {
                    if (isClothingTab)
                    {
                        gameContext->handleCommand(UICommand{ CommandType::OPEN_INVENTORY });
                    }
                    else
                    {
                        cc->step = i;
                        gameContext->refreshActionGrid();
                    }
                }, true, cc->step == i);
            }
        }
        else if (tabCount == 1)
        {
            addBtn(gameContext, cc->getTabName(activeTabs[0]), [cc, gameContext]() {}, true, true);
        }

        if (cc->getCurrentTabId() == EditorTabId::WARDROBE)
        {
            padButtonsTo(gameContext, 5);
            addBtn(gameContext, "Open Clothing (I)", [gameContext]() {
                gameContext->handleCommand(UICommand{ CommandType::OPEN_INVENTORY });
            }, true, false, "Open full dual inventory and dressing room to equip garments and inspect items.");
        }

        if (cc->step == tabCount - 1)
        {
            padButtonsTo(gameContext, 9);
            std::string finishBtnLabel = cc->config.isNewGameCreation ? "Start Game" : "Apply Changes";
            addBtn(gameContext, finishBtnLabel, [cc, gameContext]() {
                cc->finalizeCharacter(gameContext);
            }, true, true);
        }

        padButtonsTo(gameContext, 14);
        std::string backLabel = cc->config.isNewGameCreation ? "Back to Menu" : "Cancel";
        addBackBtn(gameContext, backLabel, [gameContext]() {
            gameContext->changeState(std::make_unique<mainMenuState>());
        });
        return;
    }

    if (auto loadState = dynamic_cast<loadGameState*>(currentState))
    {
        addBtn(gameContext, loadState->confirmationsEnabled ? "Confirmations: ON" : "Confirmations: OFF", [gameContext, loadState]() {
            loadState->confirmationsEnabled = !loadState->confirmationsEnabled;
            gameContext->refreshActionGrid();
        }, true, loadState->confirmationsEnabled);

        addBtn(gameContext, "Sort: Date", [gameContext, loadState]() {
            loadState->sortMode = 0;
            gameContext->refreshActionGrid();
        }, true, loadState->sortMode == 0);

        addBtn(gameContext, "Sort: Name", [gameContext, loadState]() {
            loadState->sortMode = 1;
            gameContext->refreshActionGrid();
        }, true, loadState->sortMode == 1);

        addBackBtn(gameContext, "Back", [gameContext, loadState]() { loadState->goBack(gameContext); });
        return;
    }

    if (auto opt = dynamic_cast<optionsState*>(currentState))
    {
        if (opt->isKeybindsOpen)
        {
            padButtonsTo(gameContext, 14);
            addBackBtn(gameContext, "Close (ESC)", [gameContext, opt]() {
                opt->isKeybindsOpen = false;
                gameContext->refreshActionGrid();
            });
            return;
        }

        if (opt->screenMode == OptionsScreenMode::GENERAL_OPTIONS)
        {
            addBtn(gameContext, "Keybinds", [gameContext, opt]() {
                opt->isKeybindsOpen = true;
                gameContext->refreshActionGrid();
            });

            padButtonsTo(gameContext, 10);
            addBtn(gameContext, "Defaults", [gameContext, opt]() {
                opt->resetAllDefaults(gameContext);
            });

            addBackBtn(gameContext, "Back", [gameContext, opt]() { opt->goBack(gameContext); });
            return;
        }
        else
        {
            addBtn(gameContext, "Misc.", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::MISC; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::MISC);
            addBtn(gameContext, "Gameplay", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::GAMEPLAY; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::GAMEPLAY);
            addBtn(gameContext, "Fetishes", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::FETISH_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::FETISH_PREFS);
            addBtn(gameContext, "Bodies", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::BODIES; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::BODIES);
            addBtn(gameContext, "Reset Category", [opt, gameContext]() { opt->resetCategoryDefaults(gameContext); });

            addBtn(gameContext, "Gender preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::GENDER_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::GENDER_PREFS);
            addBtn(gameContext, "Orientation preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::ORIENTATION_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS);
            addBtn(gameContext, "Age preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::AGE_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::AGE_PREFS);
            addBtn(gameContext, "Furry preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::FURRY_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::FURRY_PREFS);
            addBtn(gameContext, "", nullptr, false);

            addBtn(gameContext, "Reset All", [opt, gameContext]() { opt->resetAllDefaults(gameContext); });

            addBtn(gameContext, "Test Scene", [gameContext]() {
                gameContext->loadScene("debug_content_test_scene");
            }, true, false, "Load debug content test scene to test inline tags, warnings, and choices.");

            addBtn(gameContext, "Roll Test NPC", [gameContext]() {
                auto npc = npcGenerator::generateRandomNPC(&gameContext->settings);
                if (npc)
                {
                    std::string info = std::format("[TEST NPC] Rolled: {} | Age: {} | Orientation: {} | Arch: {}",
                        npc->name, npc->age, sexualOrientationToString(npc->orientation), genderArchetypeToString(npc->genderArchetype));
                    gameContext->addLogEntry("[TEST NPC]", info, { 220, 200, 100, 255 });

                    std::string fetList;
                    for (const auto& [fet, desire] : npc->fetishDesires)
                    {
                        if (desire > 0)
                        {
                            if (!fetList.empty()) fetList += ", ";
                            fetList += fet + " (" + (desire >= 4 ? "Love" : "Like") + ")";
                        }
                    }
                    if (fetList.empty()) fetList = "None";
                    gameContext->addLogEntry("[TEST NPC]", "Rolled Fetishes: " + fetList, { 180, 220, 180, 255 });

                    auto breasts = npc->anatomy.getPart(bodySlot::BREASTS);
                    if (breasts && !breasts->tags.empty())
                    {
                        bool isLactating = false;
                        for (const auto& t : breasts->tags) if (t == "lactating") isLactating = true;
                        if (isLactating) gameContext->addLogEntry("[TEST NPC]", "Anatomy: Lactating Breasts active (" + std::to_string(static_cast<int>(breasts->currentFluidMl)) + "ml)", { 120, 200, 255, 255 });
                    }
                }
            }, true, false, "Generate a random NPC with current Content Settings and log rolled attributes and fetishes.");

            addBackBtn(gameContext, "Back", [gameContext, opt]() { opt->goBack(gameContext); });
            return;
        }
    }

    if (auto shop = dynamic_cast<shopState*>(currentState))
    {
        entity* player = gameContext->getPlayer();
        entity* merchant = shop->getMerchant().get();
        if (!merchant) merchant = gameContext->getActiveTargetNPC();

        if (gameContext->selectedInventoryIndex != -1)
        {
            auto slotInfo = gameContext->getInventorySlotItem(gameContext->selectedInventorySide, gameContext->selectedInventoryIndex);

            if (gameContext->selectedInventorySide == 0)
            {
                if (slotInfo.isValid && slotInfo.itemPtr)
                {
                    int unitSell = merchantValuation::calculateSellPrice(slotInfo.itemPtr.get(), player, merchant);
                    bool isKey = slotInfo.itemPtr->isKeyItem;
                    float merchGold = merchant ? merchant->getStat("currency") : 999999.0f;
                    bool canSell1 = !isKey && (merchGold >= static_cast<float>(unitSell));

                    std::string sell1Label = std::format("Sell 1 ({}¤)", unitSell);
                    std::string sell1Desc = isKey
                        ? "Key quest items cannot be sold."
                        : (merchGold < static_cast<float>(unitSell)
                            ? std::format("Merchant cannot afford {}¤ (has {:.0f}¤).", unitSell, merchGold)
                            : std::format("Sell 1 {} to {} for {}¤.", slotInfo.itemPtr->name, merchant ? merchant->name : "merchant", unitSell));

                    addBtn(gameContext, sell1Label, [gameContext]() {
                        gameContext->handleCommand({ CommandType::SELL_SHOP_ITEM, gameContext->selectedInventoryIndex, 1, "" });
                    }, canSell1, false, sell1Desc);

                    if (slotInfo.count > 1)
                    {
                        int totalSell = unitSell * slotInfo.count;
                        bool canSellAll = !isKey && (merchGold >= static_cast<float>(unitSell));
                        std::string sellAllLabel = std::format("Sell All ({}¤)", totalSell);
                        std::string sellAllDesc = std::format("Sell all {} {} to {} for up to {}¤.", slotInfo.count, slotInfo.itemPtr->name, merchant ? merchant->name : "merchant", totalSell);

                        addBtn(gameContext, sellAllLabel, [gameContext, cnt = slotInfo.count]() {
                            gameContext->handleCommand({ CommandType::SELL_SHOP_ITEM, gameContext->selectedInventoryIndex, cnt, "" });
                        }, canSellAll, false, sellAllDesc);
                    }

                    addBtn(gameContext, "Deselect Item", [gameContext]() {
                        gameContext->selectedInventoryIndex = -1;
                        gameContext->refreshActionGrid();
                    }, true, false, "Deselect item and return to general shop actions.");
                }
            }
            else if (gameContext->selectedInventorySide == 1)
            {
                if (slotInfo.isValid && slotInfo.itemPtr)
                {
                    int unitBuy = merchantValuation::calculateBuyPrice(slotInfo.itemPtr.get(), player, merchant);
                    float playerGold = player ? player->getStat("currency") : 0.0f;
                    bool canBuy1 = (playerGold >= static_cast<float>(unitBuy));

                    std::string buy1Label = std::format("Buy 1 ({}¤)", unitBuy);
                    std::string buy1Desc = canBuy1
                        ? std::format("Purchase 1 {} from {} for {}¤.", slotInfo.itemPtr->name, merchant ? merchant->name : "merchant", unitBuy)
                        : std::format("Cannot afford! Requires {}¤ (You have {:.0f}¤).", unitBuy, playerGold);

                    addBtn(gameContext, buy1Label, [gameContext]() {
                        gameContext->handleCommand({ CommandType::BUY_SHOP_ITEM, gameContext->selectedInventoryIndex, 1, "" });
                    }, canBuy1, false, buy1Desc);

                    if (slotInfo.count > 1)
                    {
                        int totalBuy = unitBuy * slotInfo.count;
                        bool canBuyAll = (playerGold >= static_cast<float>(totalBuy));
                        std::string buyAllLabel = std::format("Buy All ({}¤)", totalBuy);
                        std::string buyAllDesc = std::format("Purchase entire stock of {} {} for {}¤.", slotInfo.count, slotInfo.itemPtr->name, totalBuy);

                        addBtn(gameContext, buyAllLabel, [gameContext, cnt = slotInfo.count]() {
                            gameContext->handleCommand({ CommandType::BUY_SHOP_ITEM, gameContext->selectedInventoryIndex, cnt, "" });
                        }, canBuy1, false, buyAllDesc);
                    }

                    addBtn(gameContext, "Deselect Item", [gameContext]() {
                        gameContext->selectedInventoryIndex = -1;
                        gameContext->refreshActionGrid();
                    }, true, false, "Deselect item and return to general shop actions.");
                }
            }
        }
        else
        {
            // By design: informational disabled button acting as guidance prompt in action grid
            addBtn(gameContext, "Select Item to Trade", [gameContext]() {}, false, false, "Click any item in your backpack to sell, or in the merchant catalog to buy.");
        }

        padButtonsTo(gameContext, 14);
        addBackBtn(gameContext, "Leave Shop", [gameContext]() {
            gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
        });
        return;
    }

    if (auto tf = dynamic_cast<transformationState*>(currentState))
    {
        addBtn(gameContext, "1. Core", [gameContext, tf]() { tf->setTab(TransformationTab::CORE, gameContext); }, true, tf->currentTab == TransformationTab::CORE);
        addBtn(gameContext, "2. Head & Face", [gameContext, tf]() { tf->setTab(TransformationTab::HEAD_FACE, gameContext); }, true, tf->currentTab == TransformationTab::HEAD_FACE);
        addBtn(gameContext, "3. Hair", [gameContext, tf]() { tf->setTab(TransformationTab::HAIR, gameContext); }, true, tf->currentTab == TransformationTab::HAIR);
        addBtn(gameContext, "4. Eyes", [gameContext, tf]() { tf->setTab(TransformationTab::EYES, gameContext); }, true, tf->currentTab == TransformationTab::EYES);
        addBtn(gameContext, "5. Breasts", [gameContext, tf]() { tf->setTab(TransformationTab::BREASTS, gameContext); }, true, tf->currentTab == TransformationTab::BREASTS);

        addBtn(gameContext, "6. Ass & Hips", [gameContext, tf]() { tf->setTab(TransformationTab::ASS_HIPS, gameContext); }, true, tf->currentTab == TransformationTab::ASS_HIPS);
        addBtn(gameContext, "7. Vagina", [gameContext, tf]() { tf->setTab(TransformationTab::VAGINA, gameContext); }, true, tf->currentTab == TransformationTab::VAGINA);
        addBtn(gameContext, "8. Penis", [gameContext, tf]() { tf->setTab(TransformationTab::PENIS, gameContext); }, true, tf->currentTab == TransformationTab::PENIS);
        addBtn(gameContext, "9. Crotch", [gameContext, tf]() { tf->setTab(TransformationTab::CROTCH_BOOBS, gameContext); }, true, tf->currentTab == TransformationTab::CROTCH_BOOBS);
        addBtn(gameContext, "10. Appendages", [gameContext, tf]() { tf->setTab(TransformationTab::APPENDAGES, gameContext); }, true, tf->currentTab == TransformationTab::APPENDAGES);

        addBtn(gameContext, "11. Presets", [gameContext, tf]() { tf->setTab(TransformationTab::INSPECT_PRESETS, gameContext); }, true, tf->currentTab == TransformationTab::INSPECT_PRESETS);
        addBackBtn(gameContext, "Apply & Return", [gameContext, tf]() {
            tf->returnToPreviousOrExploration(gameContext);
        });
        return;
    }

    if (auto ench = dynamic_cast<enchantingState*>(currentState))
    {
        bool canCraft = ench->canAffordCraft(gameContext);
        int cost = ench->getTotalCost(gameContext);
        std::string craftLabel = std::format("Craft ({})", cost);
        addBtn(gameContext, craftLabel, [gameContext, ench]() {
            ench->craft(gameContext);
        }, canCraft, false, std::format("Perform crafting and imbue all staged enchantments into the item (Costs {} Arcane Essence).", cost));

        addBtn(gameContext, "Clear Staged", [ench]() {
            ench->clearStagedEffects();
        }, !ench->stagedEffects.empty(), false, "Clear all currently staged infusion effects.");

        addBackBtn(gameContext, "Leave Altar", [gameContext, ench]() {
            ench->exitAltar(gameContext);
        });
        return;
    }

    if (auto sex = dynamic_cast<sexState*>(currentState))
    {
        if (gameContext->selectedEquipmentSlot != equipSlot::NONE)
        {
            int side = gameContext->selectedEquipmentSide;
            entity* charEnt = (side == 0) ? gameContext->getPlayer() : (sex->isSolo() ? nullptr : sex->getTargetEntity());
            std::shared_ptr<item> eqItem = charEnt ? charEnt->inventory.getEquippedItem(gameContext->selectedEquipmentSlot) : nullptr;

            if (eqItem)
            {
                equipSlot selSlot = gameContext->selectedEquipmentSlot;
                DisplacementMode currentDisp = charEnt->inventory.getDisplacement(selSlot);
                std::string targetDesc = (side == 0) ? "your" : std::format("{}'s", charEnt->name);

                std::string removeLabel = "Remove " + eqItem->name;
                addBtn(gameContext, removeLabel, [gameContext, sex, side, selSlot]() {
                    sex->handleClothingAction(gameContext, side, selSlot, DisplacementMode::NONE, true);
                }, true, false, std::format("Fully remove and strip {} {} from their body.", targetDesc, eqItem->name));

                auto allowedModes = eqItem->getAllowedDisplacements();

                for (DisplacementMode mode : allowedModes)
                {
                    if (mode == DisplacementMode::PULL_DOWN)
                    {
                        bool isDown = (currentDisp == DisplacementMode::PULL_DOWN);
                        addBtn(gameContext, isDown ? "Pulled Down [*]" : "Pull Down", [gameContext, sex, side, selSlot]() {
                            sex->handleClothingAction(gameContext, side, selSlot, DisplacementMode::PULL_DOWN, false);
                        }, !isDown, isDown, std::format("Pull {} {} down to expose the lower body.", targetDesc, eqItem->name));
                    }
                    else if (mode == DisplacementMode::PULL_ASIDE)
                    {
                        bool isAside = (currentDisp == DisplacementMode::PULL_ASIDE);
                        addBtn(gameContext, isAside ? "Shifted Aside [*]" : "Shift Aside", [gameContext, sex, side, selSlot]() {
                            sex->handleClothingAction(gameContext, side, selSlot, DisplacementMode::PULL_ASIDE, false);
                        }, !isAside, isAside, std::format("Slide {} {} aside to grant intimate access.", targetDesc, eqItem->name));
                    }
                    else if (mode == DisplacementMode::LIFT_UP)
                    {
                        bool isLifted = (currentDisp == DisplacementMode::LIFT_UP);
                        addBtn(gameContext, isLifted ? "Pulled Up [*]" : "Pull Up", [gameContext, sex, side, selSlot]() {
                            sex->handleClothingAction(gameContext, side, selSlot, DisplacementMode::LIFT_UP, false);
                        }, !isLifted, isLifted, std::format("Pull {} {} up to expose chest and torso.", targetDesc, eqItem->name));
                    }
                    else if (mode == DisplacementMode::UNBUTTON)
                    {
                        bool isUnbuttoned = (currentDisp == DisplacementMode::UNBUTTON);
                        addBtn(gameContext, isUnbuttoned ? "Unbuttoned [*]" : "Unbutton", [gameContext, sex, side, selSlot]() {
                            sex->handleClothingAction(gameContext, side, selSlot, DisplacementMode::UNBUTTON, false);
                        }, !isUnbuttoned, isUnbuttoned, std::format("Unfasten the buttons on {} {}.", targetDesc, eqItem->name));
                    }
                    else if (mode == DisplacementMode::OPEN)
                    {
                        bool isOpen = (currentDisp == DisplacementMode::OPEN);
                        addBtn(gameContext, isOpen ? "Opened [*]" : "Open / Part", [gameContext, sex, side, selSlot]() {
                            sex->handleClothingAction(gameContext, side, selSlot, DisplacementMode::OPEN, false);
                        }, !isOpen, isOpen, std::format("Open and spread {} {}.", targetDesc, eqItem->name));
                    }
                }

                if (currentDisp != DisplacementMode::NONE)
                {
                    addBtn(gameContext, "Restore Fit", [gameContext, sex, side, selSlot]() {
                        sex->handleClothingAction(gameContext, side, selSlot, DisplacementMode::NONE, false);
                    }, true, false, std::format("Restore {} {} to its properly worn fit.", targetDesc, eqItem->name));
                }

                addBackBtn(gameContext, "Deselect Item", [gameContext]() {
                    gameContext->selectedEquipmentSlot = equipSlot::NONE;
                    gameContext->refreshActionGrid();
                });

                return;
            }
            else
            {
                gameContext->selectedEquipmentSlot = equipSlot::NONE;
            }
        }

        std::string activeCat = gameContext->getActiveActionCategory();
        if (activeCat.empty() || activeCat == "All")
        {
            activeCat = "Generic";
            gameContext->selectActionCategory("Generic");
        }
        const auto& actions = sex->getAvailableActions();

        if (activeCat == "Position" || activeCat == "Stances" || activeCat == "Stance")
        {
            if (sex->isSolo())
            {
                static constexpr SexStance soloStances[] = { SexStance::SOLO_BED, SexStance::SOLO_CHAIR, SexStance::SOLO_STANDING };
                for (SexStance st : soloStances)
                {
                    bool isCurrent = (st == sex->getStance());
                    std::string label = (st == SexStance::SOLO_BED) ? "In Bed" : ((st == SexStance::SOLO_CHAIR) ? "Seated" : "Standing Solo");
                    if (isCurrent) label += " [*]";
                    addBtn(gameContext, label, [gameContext, sex, st]() {
                        sex->changeStance(st);
                        gameContext->refreshActionGrid();
                    }, !isCurrent, isCurrent, std::format("Assume the {} position.", sexStanceToString(st)));
                }
            }
            else
            {
                float pDom = sex->getPlayerDominance();
                struct StanceDef {
                    SexStance stance;
                    const char* domName;
                    const char* subName;
                    const char* switchName;
                };

                static constexpr StanceDef partnerStances[] = {
                    { SexStance::MISSIONARY,  "Take Missionary",    "Request Missionary", "Shift to Missionary" },
                    { SexStance::FROM_BEHIND, "Push to All Fours",  "Offer Backside",     "Shift to All Fours" },
                    { SexStance::KNEELING,    "Force to Knees",     "Drop to Knees",      "Move to Kneeling" },
                    { SexStance::STANDING,    "Stand & Press",      "Offer Standing",     "Stand Together" },
                    { SexStance::LAP_SITTING, "Pull into Lap",      "Offer Lap",          "Guide into Lap" },
                    { SexStance::COWGIRL,     "Demand Cowgirl",     "Beg to Ride",        "Switch to Cowgirl" },
                    { SexStance::LOTUS,       "Pull into Lotus",    "Offer Embrace",      "Wrap in Lotus" },
                    { SexStance::PRONE_BONE,  "Pin Face Down",      "Offer Prone",        "Lay Flat" }
                };

                for (const auto& sd : partnerStances)
                {
                    bool isCurrent = (sd.stance == sex->getStance());
                    std::string label;
                    if (pDom > 25.0f) label = sd.domName;
                    else if (pDom < -25.0f) label = sd.subName;
                    else label = sd.switchName;

                    if (isCurrent) label += " [*]";
                    SexStance targetStance = sd.stance;
                    addBtn(gameContext, label, [gameContext, targetStance]() {
                        gameContext->handleCommand({ CommandType::CHANGE_SEX_STANCE, static_cast<int>(targetStance), 0, "" });
                    }, !isCurrent, isCurrent, std::format("Move or request to transition to {}.", sexStanceToString(targetStance)));
                }
            }
        }
        else
        {
            auto playerOngoing = sex->getOngoingActionsForParticipant(0);
            for (const auto* ongoing : playerOngoing)
            {
                std::string stopLabel = std::format("Stop {}", ongoing->actionName);
                std::string actId = ongoing->actionId;
                addBtn(gameContext, stopLabel, [gameContext, sex, actId]() {
                    sex->stopActionById(actId, gameContext);
                }, true, true, std::format("Disengage and stop your ongoing action '{}'.", ongoing->actionName));
            }

            if (activeCat == "Partner" && sex->getParticipantCount() > 2)
            {
                const auto& parts = sex->getParticipants();
                for (size_t pIdx = 0; pIdx < parts.size(); ++pIdx)
                {
                    if (parts[pIdx].isPlayer) continue;
                    bool isTarget = (pIdx == sex->getTargetParticipantIndex());
                    addBtn(gameContext, std::format("Target: {}{}", parts[pIdx].getName(), isTarget ? " [*]" : ""), [gameContext, pIdx]() {
                        gameContext->handleCommand({ CommandType::SELECT_SEX_TARGET, static_cast<int>(pIdx), 0, "" });
                    }, !isTarget, isTarget, std::format("Focus interactions on {}.", parts[pIdx].getName()));
                }
            }

            if (activeCat == "Generic")
            {
                entity* playerEnt = gameContext->getPlayer();
                entity* targetEnt = sex->isSolo() ? nullptr : sex->getTargetEntity();

                bool playerHasClothes = playerEnt && !playerEnt->inventory.isFullyExposed();
                std::string playerUndressTip = playerHasClothes
                    ? "Strip away all your clothing to expose yourself completely."
                    : "You are already unclothed.\n[Inaccessible: You are already unclothed.]";
                addBtn(gameContext, "Undress All (Self)", [gameContext, sex]() {
                    sex->handleUndressAll(gameContext, true);
                }, playerHasClothes, false, playerUndressTip);

                if (!sex->isSolo() && targetEnt)
                {
                    const SexParticipant* targetP = sex->getTargetParticipant();
                    std::string pName = targetP ? targetP->getName() : "Partner";
                    bool targetHasClothes = !targetEnt->inventory.isFullyExposed();
                    std::string targetUndressTip = targetHasClothes
                        ? std::format("Strip away all of {}'s clothing, exposing them completely.", pName)
                        : std::format("{} is already unclothed.\n[Inaccessible: {} is already unclothed.]", pName, pName);
                    addBtn(gameContext, "Undress All (Partner)", [gameContext, sex]() {
                        sex->handleUndressAll(gameContext, false);
                    }, targetHasClothes, false, targetUndressTip);
                }

                if (!sex->isSolo() && (!sex->isConsensual() || sex->getPlayerDominance() < 0.0f))
                {
                    std::string struggleReason;
                    bool canStruggle = sex->canParticipantStruggle(0, &struggleReason);
                    std::string struggleTip = "Attempt to break free from your partner's hold and seize control.";
                    if (!canStruggle && !struggleReason.empty())
                    {
                        struggleTip += std::format("\n[Inaccessible: {}]", struggleReason);
                    }
                    addBtn(gameContext, "Struggle", [gameContext, sex]() {
                        sex->handleStruggle(gameContext);
                    }, canStruggle, false, struggleTip);
                }

                addBtn(gameContext, sex->isSolo() ? "Finish & Rest" : "End Encounter", [gameContext, sex]() {
                    gameContext->handleCommand({ CommandType::END_SEX_SCENE, 0, 0, "" });
                }, true, false, "Conclude the intimate encounter.");
            }

            const SexParticipant* playerP = sex->getPlayerParticipant();
            const SexParticipant* targetP = sex->isSolo() ? playerP : sex->getTargetParticipant();

            for (size_t i = 0; i < actions.size(); ++i)
            {
                if (actions[i].category != activeCat) continue;

                // Hide submissive actions when player is in dominant role
                if (playerP && playerP->getRole() == SexRole::DOMINANT)
                {
                    if (actions[i].type == SexActionType::PLEAD_SUGGEST ||
                        actions[i].type == SexActionType::ENDURE ||
                        actions[i].type == SexActionType::BEG_CLIMAX ||
                        actions[i].type == SexActionType::STRUGGLE ||
                        actions[i].id.find("sub_") == 0)
                    {
                        continue;
                    }
                }

                if (actions[i].id == "generic_cmd_strip" || actions[i].id == "generic_undress_mutual" ||
                    actions[i].id == "partner_strip" || actions[i].id == "self_strip" || actions[i].id == "sub_strip_offer")
                {
                    continue;
                }

                std::string desc = actions[i].description;
                std::string effects;
                if (sex->isSolo())
                {
                    effects = std::format("Arousal: +{:.0f}", actions[i].arousalGainSelf);
                }
                else
                {
                    effects = std::format("Self Arousal: +{:.0f} | Partner: +{:.0f} | Dom Shift: {:+.0f}",
                                          actions[i].arousalGainSelf, actions[i].arousalGainPartner, actions[i].dominanceShift);
                }
                if (actions[i].isContinuous)
                {
                    effects += " | [Continuous]";
                }

                std::string inaccReason;
                bool accessible = true;
                const SexParticipant* effectiveTarget = (actions[i].category == "Self" || actions[i].isSolo) ? playerP : targetP;
                if (playerP && effectiveTarget)
                {
                    accessible = sex->isActionAccessible(actions[i], *playerP, *effectiveTarget, &inaccReason);
                }

                std::string fullDesc = desc.empty() ? effects : std::format("{}\n({})", desc, effects);
                if (!accessible && !inaccReason.empty())
                {
                    fullDesc += std::format("\n[Inaccessible: {}]", inaccReason);
                }

                std::string actId = actions[i].id;
                addBtn(gameContext, actions[i].name, [gameContext, i, actId]() {
                    gameContext->handleCommand({ CommandType::EXECUTE_SEX_ACTION, static_cast<int>(i), 0, actId });
                }, accessible, false, fullDesc);
            }
        }

        addBackBtn(gameContext, sex->isSolo() ? "Finish & Return" : "End Scene", [gameContext, sex]() {
            if (sex->isSolo())
            {
                gameContext->isPhoneMenuOpen = true;
                gameContext->changeState(std::make_unique<explorationState>());
            }
            else
            {
                gameContext->handleCommand({ CommandType::END_SEX_SCENE, 0, 0, "" });
            }
        });
        return;
    }

    if (auto phoneApp = dynamic_cast<phoneAppsState*>(currentState))
    {
        auto setApp = [gameContext, phoneApp](PhoneAppMode mode) {
            phoneApp->setAppMode(mode);
            phoneApp->loadData(mode);
            gameContext->refreshActionGrid();
        };

        if (phoneApp->getAppMode() == PhoneAppMode::HOME)
        {
            addBtn(gameContext, "Quests", [=]() { setApp(PhoneAppMode::QUESTS); }, true, false, "View active quest objectives, stages, and rewards.");
            addBtn(gameContext, "Perk Tree", [=]() { setApp(PhoneAppMode::PERKS); }, true, false, "Browse and allocate talent perks and passive traits.");
            addBtn(gameContext, "Spells", [=]() { setApp(PhoneAppMode::SPELLS); }, true, false, "Review elemental spellbooks and cast out-of-combat spells.");
            addBtn(gameContext, "Fetishes", [=]() { setApp(PhoneAppMode::FETISHES); }, true, false, "Configure 5-tier desire ratings and sexual preferences.");
            addBtn(gameContext, "Stats", [=]() { setApp(PhoneAppMode::STATS); }, true, false, "Inspect core vitals, body measurements, and sex stats.");

            addBtn(gameContext, "Selfie", [=]() { setApp(PhoneAppMode::SELFIE); }, true, false, "Take a selfie to inspect appearance, clothing, and modesty.");
            addBtn(gameContext, "Contacts", [=]() { setApp(PhoneAppMode::CONTACTS); }, true, false, "Address book of known characters and companions.");
            addBtn(gameContext, "Encyclopedia", [=]() { setApp(PhoneAppMode::ENCYCLOPEDIA); }, true, false, "Compendium of discovered species, weapons, and items.");
            addBtn(gameContext, "Transform", [gameContext]() {
                gameContext->changeState(std::make_unique<transformationState>(TransformationTab::CORE, std::make_unique<phoneAppsState>(PhoneAppMode::HOME)));
            }, true, false, "Inspect mutations, bodily morphs, and alchemy.");
            addBtn(gameContext, "Maps", [=]() { setApp(PhoneAppMode::MAPS); }, true, false, "World and local regional maps with coordinates.");

            addBtn(gameContext, "Combat Moves", [=]() { setApp(PhoneAppMode::COMBAT_MOVES); }, true, false, "Prepare and customize active combat deck slots.");
            addBtn(gameContext, "Masturbate", [=]() { setApp(PhoneAppMode::MASTURBATE); }, true, false, "Take a secluded moment to relieve lust and build arousal.");
            addBtn(gameContext, "Wait / Rest", [=]() { setApp(PhoneAppMode::WAIT_REST); }, true, false, "Pass in-game time, sleep, and recover health/mana.");
            addBtn(gameContext, "Elemental", [=]() { setApp(PhoneAppMode::ELEMENTAL); }, true, false, "Manage and interact with your elemental companion.");
            addBtn(gameContext, "Back", [gameContext]() {
                gameContext->changeState(std::make_unique<explorationState>());
            }, true, false, "Close smartphone and return to exploration.");
            return;
        }

        if (phoneApp->getAppMode() == PhoneAppMode::WAIT_REST)
        {
            addBtn(gameContext, "Wait 15 Mins", [gameContext, phoneApp]() {
                gameContext->gameTime.advanceTime(15);
                entity* p = gameContext->getPlayer();
                if (p) {
                    p->stats.setBaseStat("health", std::min(p->getStat("max_health"), p->getStat("health") + 5.0f));
                    p->stats.setBaseStat("mana", std::min(p->getStat("max_mana"), p->getStat("mana") + 5.0f));
                }
                phoneApp->setFeedbackText("You rested for 15 minutes. Regained a little stamina.");
                gameContext->refreshActionGrid();
            }, true, false, "Pass 15 minutes of in-game time (+5 HP/MP).");

            addBtn(gameContext, "Wait 1 Hour", [gameContext, phoneApp]() {
                gameContext->gameTime.advanceTime(60);
                entity* p = gameContext->getPlayer();
                if (p) {
                    p->stats.setBaseStat("health", std::min(p->getStat("max_health"), p->getStat("health") + 20.0f));
                    p->stats.setBaseStat("mana", std::min(p->getStat("max_mana"), p->getStat("mana") + 20.0f));
                }
                phoneApp->setFeedbackText("You waited for 1 hour. Regained 20 Health and Mana.");
                gameContext->refreshActionGrid();
            }, true, false, "Pass 1 hour of in-game time (+20 HP/MP).");

            addBtn(gameContext, "Rest 4 Hours", [gameContext, phoneApp]() {
                gameContext->gameTime.advanceTime(240);
                entity* p = gameContext->getPlayer();
                if (p) {
                    p->stats.setBaseStat("health", std::min(p->getStat("max_health"), p->getStat("health") + 60.0f));
                    p->stats.setBaseStat("mana", std::min(p->getStat("max_mana"), p->getStat("mana") + 60.0f));
                }
                phoneApp->setFeedbackText("You rested deeply for 4 hours. Regained 60 Health and Mana.");
                gameContext->refreshActionGrid();
            }, true, false, "Pass 4 hours of in-game time (+60 HP/MP).");

            addBtn(gameContext, "Sleep 8 Hours", [gameContext, phoneApp]() {
                gameContext->gameTime.advanceTime(480);
                entity* p = gameContext->getPlayer();
                if (p) {
                    p->stats.setBaseStat("health", p->getStat("max_health"));
                    p->stats.setBaseStat("mana", p->getStat("max_mana"));
                }
                phoneApp->setFeedbackText("You slept peacefully for 8 hours and feel completely revitalized! (Full Health & Mana restored)");
                gameContext->refreshActionGrid();
            }, true, false, "Sleep 8 hours to restore full Health and Mana.");

            addBtn(gameContext, "Wait to Dawn/Dusk", [gameContext, phoneApp]() {
                int targetHour = (gameContext->gameTime.hour >= 6 && gameContext->gameTime.hour < 20) ? 20 : 6;
                int hoursToAdvance = (targetHour - gameContext->gameTime.hour + 24) % 24;
                if (hoursToAdvance == 0) hoursToAdvance = 24;
                gameContext->gameTime.advanceTime(hoursToAdvance * 60);
                phoneApp->setFeedbackText(std::format("You waited until {} ({}:00).", targetHour == 6 ? "Dawn" : "Dusk", targetHour));
                gameContext->refreshActionGrid();
            }, true, false, "Wait until the next diurnal lighting phase shift.");
        }
        else if (phoneApp->getAppMode() == PhoneAppMode::MASTURBATE)
        {
            entity* p = gameContext->getPlayer();
            float curArousal = p ? p->getStat("arousal") : 0.0f;

            addBtn(gameContext, "Caress Chest", [gameContext, phoneApp, p]() {
                if (p) {
                    float newA = std::min(100.0f, p->getStat("arousal") + 15.0f);
                    p->stats.setBaseStat("arousal", newA);
                }
                phoneApp->setFeedbackText("You gently brush your fingers over your chest, feeling your breath hitch as sensitivity rises.");
                gameContext->refreshActionGrid();
            }, true, false, "Caress sensitive zones to gently build arousal (+15%).");

            addBtn(gameContext, "Fondle Groin", [gameContext, phoneApp, p]() {
                if (p) {
                    float newA = std::min(100.0f, p->getStat("arousal") + 25.0f);
                    p->stats.setBaseStat("arousal", newA);
                }
                phoneApp->setFeedbackText("Your touch slips lower, eliciting a soft gasp as intense warmth radiates through your core.");
                gameContext->refreshActionGrid();
            }, true, false, "Directly stimulate intimate areas (+25% arousal).");

            addBtn(gameContext, "Tease Climax", [gameContext, phoneApp, p]() {
                if (p) {
                    p->stats.setBaseStat("arousal", std::max(85.0f, p->getStat("arousal") + 20.0f));
                }
                phoneApp->setFeedbackText("You rhythmically build tension right on the edge of ecstasy, your body trembling with delicious anticipation.");
                gameContext->refreshActionGrid();
            }, true, false, "Edge close to orgasm (Brings arousal to 85%+).");

            bool canClimax = (curArousal >= 60.0f);
            addBtn(gameContext, "Climax & Relief", [gameContext, phoneApp, p]() {
                if (p) {
                    p->stats.setBaseStat("arousal", 0.0f);
                    p->stats.setBaseStat("lust", 0.0f);
                    p->stats.setBaseStat("mana", std::min(p->getStat("max_mana"), p->getStat("mana") + 15.0f));
                }
                phoneApp->setFeedbackText("A breathless, overwhelming wave of climax crashes through you, washing away all tension and leaving you thoroughly satisfied and blissfully relaxed! (Lust & Arousal reset to 0%)");
                gameContext->refreshActionGrid();
            }, canClimax, false, canClimax ? "Surrender to orgasm and purge all accumulated lust!" : "Requires at least 60% arousal to climax.");
        }
        else if (phoneApp->getAppMode() == PhoneAppMode::TRANSFORM)
        {
            addBtn(gameContext, "Open Full Editor", [gameContext]() {
                gameContext->changeState(std::make_unique<transformationState>());
            }, true, false, "Launch full mutation and body transformation studio.");

            addBtn(gameContext, "Reset Baseline", [gameContext, phoneApp]() {
                entity* p = gameContext->getPlayer();
                if (p) {
                    p->anatomy.removePart(bodySlot::HORNS);
                    p->anatomy.removePart(bodySlot::WINGS);
                    p->anatomy.removePart(bodySlot::TAIL);
                }
                phoneApp->setFeedbackText("Cleared active exotic mutations. Restored Human baseline.");
                gameContext->refreshActionGrid();
            }, true, false, "Clear horns, wings, and tails to revert to baseline.");
        }
        else if (phoneApp->getAppMode() == PhoneAppMode::SPELLS)
        {
            auto allSpells = SpellDatabase::getAllSpells();
            std::vector<SpellDefinition> castableSpells;
            for (const auto& [sId, spDef] : allSpells)
            {
                if (spDef.canCastInOverworld)
                {
                    castableSpells.push_back(spDef);
                }
            }
            std::sort(castableSpells.begin(), castableSpells.end(), [](const SpellDefinition& a, const SpellDefinition& b) {
                if (a.requiredLevel != b.requiredLevel) return a.requiredLevel < b.requiredLevel;
                return a.id < b.id;
            });

            entity* player = gameContext->getPlayer();
            float curPlayerMp = player ? player->getStat("mana") : 0.0f;
            float curPlayerHp = player ? player->getStat("health") : 0.0f;
            float maxPlayerHp = player ? std::max(1.0f, player->getStat("max_health")) : 100.0f;
            int playerLvl = player ? player->stats.level : 1;

            for (const auto& sp : castableSpells)
            {
                if (!sp.unlocked && playerLvl < sp.requiredLevel) continue;

                bool canAfford = (curPlayerMp >= sp.mpCost);
                bool targetConditionMet = true;
                std::string conditionReason;

                if (sp.overworldEffect.type == "HEAL_AND_PURGE")
                {
                    bool hpMissing = (curPlayerHp < maxPlayerHp);
                    bool hasDebuffs = false;
                    if (player)
                    {
                        for (const auto& s : player->statusEffects)
                        {
                            if (s.isDebuff) { hasDebuffs = true; break; }
                        }
                    }
                    bool hasDirty = false;
                    if (player)
                    {
                        for (const auto& eq : player->inventory.equipped)
                        {
                            if (eq && eq->soiling.isDirty) { hasDirty = true; break; }
                        }
                        if (!hasDirty)
                        {
                            for (const auto& bk : player->inventory.backpack)
                            {
                                if (bk && bk->soiling.isDirty) { hasDirty = true; break; }
                            }
                        }
                    }

                    if (!hpMissing && !hasDebuffs && !hasDirty)
                    {
                        targetConditionMet = false;
                        conditionReason = "Health is full, no debuffs, and no garments are soiled";
                    }
                }

                bool canCast = canAfford && targetConditionMet;
                std::string label = std::format("Cast {}", sp.name);
                std::string tt = sp.description;

                if (!canAfford)
                {
                    tt += std::format("\n[Insufficient MP: Requires {:.0f} MP, have {:.0f}]", sp.mpCost, curPlayerMp);
                }
                else if (!targetConditionMet)
                {
                    tt += std::format("\n[Target condition not met: {}]", conditionReason);
                }
                else
                {
                    tt += std::format("\nCost: {:.0f} MP.", sp.mpCost);
                }

                addBtn(gameContext, label, [gameContext, spellId = sp.id, sName = sp.name, phoneApp]() {
                    gameContext->castOverworldSpell(spellId);
                    phoneApp->setFeedbackText(std::format("Channeled and cast {}!", sName));
                    gameContext->refreshActionGrid();
                }, canCast, false, tt);
            }
        }
        else if (phoneApp->getAppMode() == PhoneAppMode::PERKS)
        {
            padButtonsTo(gameContext, 13);
            addBtn(gameContext, "Reset Perks", [gameContext, phoneApp]() {
                entity* p = gameContext->getPlayer();
                if (p)
                {
                    int refunded = 0;
                    for (const auto& perkId : p->unlockedPerks)
                    {
                        const auto* pk = PerkDatabase::getPerk(perkId);
                        refunded += pk ? pk->cost : 1;
                    }
                    p->stats.setBaseStat("perk_points", p->getStat("perk_points") + refunded);
                    p->resetPerks();
                    gameContext->addLogEntry("[TALENT]", std::format("Reset all perks (+{} Pts refunded)", refunded), { 220, 180, 80, 255 });
                    gameContext->refreshActionGrid();
                }
            }, true, false, "Refund all allocated talent perks and points.");
        }
        else if (phoneApp->getAppMode() == PhoneAppMode::COMBAT_MOVES)
        {
            entity* p = gameContext->getPlayer();
            int curSel = phoneApp->getSelectedCombatSlot();
            for (int i = 0; i < 10; ++i)
            {
                std::string move = (p && !p->preparedCombatSlots[i].empty()) ? p->preparedCombatSlots[i] : "Empty";
                std::string btnLabel = std::format("{}: {}", i + 1, move);
                bool isSelected = (curSel == i);
                std::string tooltip = (p && !p->preparedCombatSlots[i].empty())
                    ? std::format("Deck Slot #{}: {}. Click to select this slot for assignment.", i + 1, p->preparedCombatSlots[i])
                    : std::format("Deck Slot #{}: [Empty]. Click to select this slot for assignment.", i + 1);

                addBtn(gameContext, btnLabel, [gameContext, phoneApp, i]() {
                    phoneApp->setSelectedCombatSlot(i);
                    gameContext->refreshActionGrid();
                }, true, isSelected, tooltip);
            }

            addBtn(gameContext, "Clear Slot", [gameContext, phoneApp]() {
                entity* p = gameContext->getPlayer();
                int s = phoneApp->getSelectedCombatSlot();
                if (p && s >= 0 && s < 10)
                {
                    p->preparedCombatSlots[s] = "";
                    phoneApp->setFeedbackText(std::format("Cleared combat action slot #{}.", s + 1));
                    gameContext->refreshActionGrid();
                }
            }, true, false, "Clear technique from currently selected combat slot.");
        }
        else if (phoneApp->getAppMode() == PhoneAppMode::ELEMENTAL)
        {
            bool isSummoned = phoneApp->isElementalSummoned();
            addBtn(gameContext, isSummoned ? "Dispel" : "Summon", [gameContext, phoneApp]() {
                phoneApp->toggleElementalSummoned();
                phoneApp->setFeedbackText(phoneApp->isElementalSummoned() ? "Elemental companion manifested." : "Elemental companion dispelled.");
                gameContext->refreshActionGrid();
            }, true, false, isSummoned ? "Dismiss elemental companion back to its plane." : "Manifest elemental companion via arcane link.");

            if (isSummoned)
            {
                bool isActive = phoneApp->isElementalActiveForm();
                addBtn(gameContext, isActive ? "Passive Form" : "Active Form", [gameContext, phoneApp]() {
                    phoneApp->toggleElementalActiveForm();
                    phoneApp->setFeedbackText(phoneApp->isElementalActiveForm() ? "Elemental assumed humanoid battle form." : "Elemental assumed compact creature form.");
                    gameContext->refreshActionGrid();
                }, true, false, "Toggle between combat humanoid form and passive wisp creature form.");
            }
        }

        padButtonsTo(gameContext, 14);
        addBtn(gameContext, "Back", [gameContext, phoneApp]() {
            if (phoneApp->getAppMode() == PhoneAppMode::CONTACTS && phoneApp->getContactsSelectedIdx() >= 0)
            {
                phoneApp->setContactsSelectedIdx(-1);
            }
            else
            {
                phoneApp->setAppMode(PhoneAppMode::HOME);
            }
            gameContext->refreshActionGrid();
        }, true, false, "Return to previous menu.");
        return;
    }

    if (auto resState = dynamic_cast<encounterResolutionState*>(currentState))
    {
        const auto& records = resState->getDefeatedRecords();
        size_t selectedIdx = resState->getSelectedIndex();

        if (selectedIdx < records.size())
        {
            const auto& rec = records[selectedIdx];
            bool canTransform = (rec.npc && rec.npc->canTransform());

            addBtn(gameContext, "Loot & Equipment", [gameContext]() {
                gameContext->handleCommand({ CommandType::OPEN_INVENTORY, 0, 0, "" });
            }, true, false, "Open dual-inventory to inspect, unequip, and take the adversary's carried items and equipment.");

            bool nonConNever = (gameContext->settings.content.getFetishRating("Non-Consent") == 0);
            bool isNonConBlocked = (!gameContext->settings.content.nonConEnabled || nonConNever) &&
                                   (gameContext->settings.content.contentFilterMode == ContentFilterMode::BLOCK_AND_SKIP);
            bool isNonConWarn = nonConNever && (gameContext->settings.content.contentFilterMode == ContentFilterMode::WARN_CONFIRM);

            std::string domLabel = isNonConBlocked ? "[Locked: Non-Con]" : (isNonConWarn ? "[!] Dom Scene" : "Dom Scene");
            addBtn(gameContext, domLabel, [gameContext]() {
                gameContext->handleCommand({ CommandType::INTERACTIVE_SEX_DOM, 0, 0, "" });
            }, !isNonConBlocked, false, isNonConBlocked ? "Non-consensual scenes are blocked by your Content Settings." : "Initiate an intimate encounter taking the dominant lead.");

            std::string subLabel = isNonConBlocked ? "[Locked: Non-Con]" : (isNonConWarn ? "[!] Sub Scene" : "Sub Scene");
            addBtn(gameContext, subLabel, [gameContext]() {
                gameContext->handleCommand({ CommandType::INTERACTIVE_SEX_SUB, 0, 0, "" });
            }, !isNonConBlocked, false, isNonConBlocked ? "Non-consensual scenes are blocked by your Content Settings." : "Initiate an intimate encounter in a submissive role.");

            addBtn(gameContext, "Transform", [gameContext]() {
                gameContext->handleCommand({ CommandType::TRANSFORM_ENEMY, 0, 0, "" });
            }, canTransform, false, canTransform
                ? "Enter transformation studio to customize this adversary's anatomy to your liking."
                : "This enemy does not possess metamorphic abilities to transform.");

            addBtn(gameContext, "Talk", [gameContext]() {
                gameContext->handleCommand({ CommandType::TALK_ENEMY, 0, 0, "" });
            }, true, false, "Speak with the defeated adversary to build rapport and befriend them.");

            if (records.size() > 1)
            {
                padButtonsTo(gameContext, 5);
                for (size_t i = 0; i < records.size() && i < 5; ++i)
                {
                    addBtn(gameContext, std::format("Target: {}", records[i].npc ? records[i].npc->name : "Enemy"), [gameContext, i]() {
                        gameContext->handleCommand({ CommandType::SELECT_RESOLUTION_TARGET, static_cast<int>(i), 0, "" });
                    }, i != selectedIdx);
                }
            }

            padButtonsTo(gameContext, 10);
            addBtn(gameContext, rec.isPermanentlyRemoved ? "Removed" : "Remove Character", [gameContext]() {
                gameContext->handleCommand({ CommandType::PERMANENTLY_REMOVE_ENEMY, 0, 0, "" });
            }, !rec.isPermanentlyRemoved, false, "Permanently remove this defeated enemy from the area, preventing respawns.");

            padButtonsTo(gameContext, 14);
            addBtn(gameContext, "Continue", [gameContext]() {
                gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
            }, true, false, "Leave the resolution hub and return to exploration.");
        }
        else
        {
            addBackBtn(gameContext, "Continue", [gameContext]() {
                gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
            });
        }
        return;
    }

    if (dynamic_cast<inventoryState*>(currentState))
    {
        auto p = gameContext->getPlayer();
        bool hasEquipped = false;
        bool hasDisplacement = false;
        if (p)
        {
            for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
            {
                equipSlot slot = static_cast<equipSlot>(s);
                if (p->inventory.isEquipped(slot))
                {
                    hasEquipped = true;
                    if (p->inventory.getDisplacement(slot) != DisplacementMode::NONE)
                    {
                        hasDisplacement = true;
                    }
                }
            }
        }

        bool hasGroundItems = false;
        characterCreationState* ccState = dynamic_cast<characterCreationState*>(gameContext->getActiveState());
        if (!ccState)
        {
            if (auto* invState = dynamic_cast<inventoryState*>(gameContext->getActiveState()))
            {
                ccState = dynamic_cast<characterCreationState*>(invState->getReturnState());
            }
        }
        if (ccState)
        {
            hasGroundItems = !ccState->availableWardrobe.empty();
        }
        else if (gameContext->map)
        {
            const auto& td = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
            hasGroundItems = !td.droppedItems.empty() || td.droppedCurrency > 0.0f;
        }

        if (gameContext->selectedEquipmentSlot != equipSlot::NONE)
        {
            if (gameContext->selectedEquipmentSide == 1 && gameContext->getActiveTargetNPC())
            {
                entity* targetNpc = gameContext->getActiveTargetNPC();
                auto eqItem = targetNpc->inventory.getEquippedItem(gameContext->selectedEquipmentSlot);
                bool canTake = (eqItem != nullptr);
                std::string takeDesc = canTake ? ("Transfer " + eqItem->name + " directly into your backpack.") : "No item equipped in this slot.";
                addBtn(gameContext, "Take Equipment", [gameContext]() {
                    gameContext->handleTakeNPCEquipmentAction(gameContext->selectedEquipmentSlot);
                }, canTake, false, takeDesc);

                bool canUnequip = (eqItem != nullptr);
                std::string unequipDesc = canUnequip ? ("Unequip " + eqItem->name + " into " + targetNpc->name + "'s backpack.") : "No item equipped in this slot.";
                addBtn(gameContext, "Unequip to NPC Bag", [gameContext]() {
                    gameContext->handleUnequipNPCAction(gameContext->selectedEquipmentSlot);
                }, canUnequip, false, unequipDesc);

                if (gameContext->selectedInventorySide == 0 && gameContext->selectedInventoryIndex != -1)
                {
                    auto selSlot = gameContext->getInventorySlotItem(0, gameContext->selectedInventoryIndex);
                    if (selSlot.isValid && selSlot.itemPtr && selSlot.itemPtr->isEquippable)
                    {
                        auto vSlots = selSlot.itemPtr->getValidEquipSlots();
                        if (std::find(vSlots.begin(), vSlots.end(), gameContext->selectedEquipmentSlot) != vSlots.end())
                        {
                            std::string eqLabel = std::format("Equip {} here", selSlot.itemPtr->name);
                            bool isEnslave = gameContext->isEnslavingItem(selSlot.itemPtr.get());
                            std::string desc = isEnslave
                                ? std::format("Equip enslaving {} on {} to bind them in servitude!", selSlot.itemPtr->name, targetNpc->name)
                                : std::format("Equip {} onto {}.", selSlot.itemPtr->name, targetNpc->name);

                            addBtn(gameContext, eqLabel, [gameContext]() {
                                gameContext->handleEquipOnNPCAction(gameContext->selectedInventoryIndex, gameContext->selectedEquipmentSlot);
                            }, true, false, desc);
                        }
                    }
                }
            }
            else
            {
                auto eqItem = p ? p->inventory.getEquippedItem(gameContext->selectedEquipmentSlot) : nullptr;
                DisplacementMode currentDisp = p ? p->inventory.getDisplacement(gameContext->selectedEquipmentSlot) : DisplacementMode::NONE;

                bool canUnequip = (eqItem != nullptr);
                std::string unequipDesc = canUnequip ? ("Unequip " + eqItem->name + " back into inventory.") : "No item equipped in this socket to unequip.";
                addBtn(gameContext, "Unequip", [gameContext]() { gameContext->handleUnequipAction(gameContext->selectedEquipmentSlot); }, canUnequip, false, unequipDesc);

                bool canPullAside = (eqItem != nullptr && eqItem->supportedDisplacements.contains(DisplacementMode::PULL_ASIDE));
                addBtn(gameContext, "Pull Aside", [gameContext]() {
                    if (auto pl = gameContext->getPlayer()) { pl->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::PULL_ASIDE); gameContext->refreshActionGrid(); }
                }, canPullAside, false, canPullAside ? "Pull garment aside to expose underlying body." : "Garment does not support pulling aside.");

                bool canLift = (eqItem != nullptr && (eqItem->supportedDisplacements.contains(DisplacementMode::LIFT_UP) || eqItem->supportedDisplacements.contains(DisplacementMode::PULL_DOWN)));
                addBtn(gameContext, "Pull Up / Down", [gameContext]() {
                    if (auto pl = gameContext->getPlayer()) { pl->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::LIFT_UP); gameContext->refreshActionGrid(); }
                }, canLift, false, canLift ? "Lift or lower garment." : "Garment does not support pulling up or down.");

                bool canUnbutton = (eqItem != nullptr && eqItem->supportedDisplacements.contains(DisplacementMode::UNBUTTON));
                addBtn(gameContext, "Unbutton / Open", [gameContext]() {
                    if (auto pl = gameContext->getPlayer()) { pl->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::UNBUTTON); gameContext->refreshActionGrid(); }
                }, canUnbutton, false, canUnbutton ? "Unfasten buttons or open closures." : "Garment does not have buttons or openings.");

                bool canReset = (eqItem != nullptr && currentDisp != DisplacementMode::NONE);
                addBtn(gameContext, "Reset Fit", [gameContext]() {
                    if (auto pl = gameContext->getPlayer()) { pl->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::NONE); gameContext->refreshActionGrid(); }
                }, canReset, false, canReset ? "Restore garment to properly worn placement." : "Garment is already worn normally.");

                addBtn(gameContext, "Reset All Fits", [gameContext]() {
                    gameContext->handleResetAllDisplacementsAction();
                }, hasDisplacement, false, "Restore all displaced garments to properly worn placement.");
            }
        }
        else if (gameContext->selectedInventoryIndex != -1)
        {
            bool hasNpc = (gameContext->getActiveTargetNPC() != nullptr);
            auto slotInfo = gameContext->getInventorySlotItem(gameContext->selectedInventorySide, gameContext->selectedInventoryIndex);

            if (gameContext->selectedInventorySide == 0)
            {
                if (slotInfo.isValid && slotInfo.itemPtr)
                {
                    if (slotInfo.itemPtr->isEquippable)
                    {
                        auto validSlots = slotInfo.itemPtr->getValidEquipSlots();
                        for (equipSlot s : validSlots)
                        {
                            std::string slotName = gameContext->formatEquipSlotName(s);
                            std::string btnLabel = std::format("Equip ({})", slotName);
                            std::string desc = std::format("Equip {} to {}.", slotInfo.itemPtr->name, slotName);
                            if (!slotInfo.itemPtr->tooltip.empty()) desc += " " + slotInfo.itemPtr->tooltip;
                            else if (!slotInfo.itemPtr->description.empty()) desc += " " + slotInfo.itemPtr->description;

                            addBtn(gameContext, btnLabel, [gameContext, s]() {
                                gameContext->handleEquipAction(gameContext->selectedInventoryIndex, s);
                            }, true, false, desc);
                        }

                        if (hasNpc)
                        {
                            entity* targetNpc = gameContext->getActiveTargetNPC();
                            bool isEnslave = gameContext->isEnslavingItem(slotInfo.itemPtr.get());
                            for (equipSlot s : validSlots)
                            {
                                std::string slotName = gameContext->formatEquipSlotName(s);
                                std::string btnLabel = std::format("Equip on {} ({})", targetNpc ? targetNpc->name : "NPC", slotName);
                                std::string desc = isEnslave
                                    ? std::format("Equip enslaving {} on {} to bind them in servitude!", slotInfo.itemPtr->name, targetNpc ? targetNpc->name : "enemy")
                                    : std::format("Equip {} onto {}.", slotInfo.itemPtr->name, targetNpc ? targetNpc->name : "enemy");

                                addBtn(gameContext, btnLabel, [gameContext, s]() {
                                    gameContext->handleEquipOnNPCAction(gameContext->selectedInventoryIndex, s);
                                }, true, false, desc);
                            }
                        }
                    }

                    if (slotInfo.itemPtr->isConsumable)
                    {
                        std::string desc = "Consume " + slotInfo.itemPtr->name + ". " + (!slotInfo.itemPtr->tooltip.empty() ? slotInfo.itemPtr->tooltip : slotInfo.itemPtr->description);
                        addBtn(gameContext, "Use", [gameContext]() {
                            gameContext->handleUseItemAction(gameContext->selectedInventoryIndex);
                        }, true, false, desc);
                    }

                    if ((slotInfo.itemPtr->isConsumable || slotInfo.itemPtr->isEquippable) && !slotInfo.itemPtr->isKeyItem)
                    {
                        addBtn(gameContext, "Enchant", [gameContext, targetItem = slotInfo.itemPtr]() {
                            int actualBackpackIdx = -1;
                            if (gameContext && gameContext->Player)
                            {
                                int idx = gameContext->selectedInventoryIndex;
                                auto stackedView = gameContext->Player->inventory.getStackedView();
                                if (idx >= 0 && static_cast<size_t>(idx) < stackedView.size())
                                {
                                    actualBackpackIdx = stackedView[idx].firstBackpackIndex;
                                }
                                if (targetItem)
                                {
                                    const auto& bp = gameContext->Player->inventory.backpack;
                                    if (actualBackpackIdx < 0 || static_cast<size_t>(actualBackpackIdx) >= bp.size() || bp[actualBackpackIdx] != targetItem)
                                    {
                                        for (size_t i = 0; i < bp.size(); ++i)
                                        {
                                            if (bp[i] == targetItem)
                                            {
                                                actualBackpackIdx = static_cast<int>(i);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            gameContext->changeState(std::make_unique<enchantingState>(actualBackpackIdx, std::make_unique<inventoryState>(), targetItem));
                        }, true, false, "Take this item to the infusion altar to imbue it with arcane essences.");
                    }
                }

                bool isKey = (slotInfo.isValid && slotInfo.itemPtr && slotInfo.itemPtr->isKeyItem);
                bool canDrop = slotInfo.isValid && !isKey;
                std::string give1 = hasNpc ? "Give 1" : "Drop 1";
                std::string giveAll = hasNpc ? "Give All" : "Drop All";
                std::string dropDesc = isKey ? "Key quest items cannot be dropped." : (hasNpc ? "Transfer 1 item to NPC." : "Drop 1 item onto the floor.");
                std::string dropAllDesc = isKey ? "Key quest items cannot be dropped." : (hasNpc ? "Transfer entire stack to NPC." : "Drop entire stack onto the floor.");

                addBtn(gameContext, give1, [gameContext]() { gameContext->handleDropAction(gameContext->selectedInventoryIndex, 1); }, canDrop, false, dropDesc);
                addBtn(gameContext, giveAll, [gameContext]() { gameContext->handleDropAction(gameContext->selectedInventoryIndex, 999); }, canDrop, false, dropAllDesc);

                addBtn(gameContext, "Deselect Item", [gameContext]() {
                    gameContext->selectedInventoryIndex = -1;
                    gameContext->refreshActionGrid();
                }, true, false, "Clear item selection and return to general actions.");
            }
            else if (gameContext->selectedInventorySide == 1)
            {
                std::string take1 = hasNpc ? "Take 1" : "Pickup 1";
                std::string takeAll = hasNpc ? "Take All" : "Pickup All";
                addBtn(gameContext, take1, [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 1); }, slotInfo.isValid, false, hasNpc ? "Take 1 item into your backpack." : "Pick up 1 item from the ground.");
                addBtn(gameContext, takeAll, [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 999); }, slotInfo.isValid, false, hasNpc ? "Take entire stack into your backpack." : "Pick up entire stack from the ground.");

                if (!hasNpc)
                {
                    addBtn(gameContext, "Loot All", [gameContext]() {
                        gameContext->handleLootAllAction();
                    }, hasGroundItems, false, "Pick up all items from the ground into your backpack.");
                }

                if (slotInfo.isValid && slotInfo.itemPtr && slotInfo.itemPtr->isEquippable)
                {
                    auto validSlots = slotInfo.itemPtr->getValidEquipSlots();
                    for (equipSlot s : validSlots)
                    {
                        std::string slotName = gameContext->formatEquipSlotName(s);
                        std::string btnLabel = std::format("Equip ({})", slotName);
                        std::string desc = std::format("{} and equip {} to {}.", hasNpc ? "Take" : "Pick up", slotInfo.itemPtr->name, slotName);
                        if (!slotInfo.itemPtr->tooltip.empty()) desc += " " + slotInfo.itemPtr->tooltip;
                        else if (!slotInfo.itemPtr->description.empty()) desc += " " + slotInfo.itemPtr->description;

                        addBtn(gameContext, btnLabel, [gameContext, s]() {
                            gameContext->handleEquipGroundAction(gameContext->selectedInventoryIndex, s);
                        }, true, false, desc);
                    }
                }

                addBtn(gameContext, "Deselect Item", [gameContext]() {
                    gameContext->selectedInventoryIndex = -1;
                    gameContext->refreshActionGrid();
                }, true, false, "Clear item selection.");
            }
        }
        else
        {
            entity* targetNpc = gameContext->getActiveTargetNPC();
            if (targetNpc)
            {
                float npcGold = targetNpc->getStat("currency");
                bool hasNpcGold = (npcGold > 0.0f);
                std::string goldLabel = std::format("Take Gold ({:.0f}¤)", npcGold);
                addBtn(gameContext, goldLabel, [gameContext]() {
                    gameContext->handleTakeNPCGoldAction();
                }, hasNpcGold, false, hasNpcGold ? "Transfer all gold from the adversary to your purse." : "Adversary carries no gold.");

                size_t npcBagCount = 0;
                for (const auto& it : targetNpc->inventory.backpack) if (it) npcBagCount++;
                bool hasNpcBag = (npcBagCount > 0);
                addBtn(gameContext, "Take All Bag Items", [gameContext]() {
                    gameContext->handleTakeAllNPCItemsAction();
                }, hasNpcBag, false, hasNpcBag ? "Loot all backpack items from the adversary." : "Adversary backpack is empty.");

                int npcEqCount = 0;
                for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
                {
                    if (targetNpc->inventory.isEquipped(static_cast<equipSlot>(s))) npcEqCount++;
                }
                bool hasNpcEq = (npcEqCount > 0);
                addBtn(gameContext, "Strip All Equipment", [gameContext]() {
                    gameContext->handleStripAllNPCEquipmentAction();
                }, hasNpcEq, false, hasNpcEq ? "Unequip and take all worn equipment and garments from the adversary." : "Adversary wears no equipment.");
            }
            else
            {
                float groundGold = gameContext->getTileCurrency();
                bool hasGroundGold = (groundGold > 0.0f);
                if (hasGroundGold)
                {
                    std::string goldLabel = std::format("Take Gold ({:.0f}¤)", groundGold);
                    addBtn(gameContext, goldLabel, [gameContext, groundGold]() {
                        gameContext->transferCurrencyFromSide1(groundGold);
                    }, true, false, "Pick up all gold from the ground into your purse.");
                }

                addBtn(gameContext, "Loot All", [gameContext]() {
                    gameContext->handleLootAllAction();
                }, hasGroundItems, false, hasGroundItems ? "Transfer all items and gold from the ground into your backpack." : "No items on the ground to loot.");
            }

            addBtn(gameContext, "Unequip All", [gameContext]() {
                gameContext->handleUnequipAllAction();
            }, hasEquipped, false, hasEquipped ? "Unequip all worn garments, weapons, and accessories into your backpack." : "No items currently equipped to unequip.");

            addBtn(gameContext, "Strip to Underwear", [gameContext]() {
                gameContext->handleStripToUnderwearAction();
            }, hasEquipped, false, hasEquipped ? "Unequip outer and middle clothing, keeping only underwear and intimate items equipped." : "No clothing currently equipped.");

            addBtn(gameContext, "Reset All Fits", [gameContext]() {
                gameContext->handleResetAllDisplacementsAction();
            }, hasDisplacement, false, hasDisplacement ? "Restore all pulled aside/down or unbuttoned garments to properly worn fit." : "No garments are currently displaced.");
        }

        auto inv = dynamic_cast<inventoryState*>(currentState);
        bool isSubmenu = (inv && inv->getReturnState() != nullptr);
        std::string backLabel = "Close (I / ESC)";
        std::string backDesc = "Return to exploration view.";
        if (isSubmenu)
        {
            if (dynamic_cast<encounterResolutionState*>(inv->getReturnState()))
            {
                backLabel = "Back to Resolution Hub";
                backDesc = "Return to post-combat resolution hub.";
            }
            else if (dynamic_cast<characterCreationState*>(inv->getReturnState()))
            {
                backLabel = "Back to Creation";
                backDesc = "Return to Character Creation.";
            }
            else
            {
                backLabel = "Back";
                backDesc = "Return to previous screen.";
            }
        }
        addBackBtn(gameContext, backLabel, [gameContext, inv]() {
            if (inv) inv->goBack(gameContext);
            else gameContext->changeState(std::make_unique<explorationState>());
        }, backDesc);
        return;
    }

    if (auto combat = dynamic_cast<CombatState*>(currentState))
    {
        auto& players = combat->getEngine().getPlayerParty();
        float curStamina = (!players.empty()) ? players[0].currentStamina : 0.0f;
        bool hasQueuedActions = (!players.empty() && !players[0].turnQueue.empty());

        auto p = gameContext->getPlayer();
        float curMp = p ? p->getStat("mana") : 0.0f;
        int potionCount = 0;
        if (p)
        {
            for (const auto& it : p->inventory.backpack)
            {
                if (it && it->id.find("potion") != std::string::npos) potionCount += it->count;
            }
        }

        if (combat->getCombatFocus() == CombatFocus::ROOT)
        {
            addBtn(gameContext, "Weapon Skills", [gameContext]() {
                gameContext->handleCommand({ CommandType::SELECT_COMBAT_FOCUS, 0, 0, "WEAPON" });
            }, true, false, "Select Weapon Focus: Strike with equipped weapon skills and martial techniques.");

            addBtn(gameContext, "Spells & Magic", [gameContext]() {
                gameContext->handleCommand({ CommandType::SELECT_COMBAT_FOCUS, 0, 0, "MAGIC" });
            }, true, false, "Select Magic Focus: Cast arcane, elemental, and healing spells.");

            addBtn(gameContext, "Defensive Maneuvers", [gameContext]() {
                gameContext->handleCommand({ CommandType::SELECT_COMBAT_FOCUS, 0, 0, "DEFENSE" });
            }, true, false, "Select Defense Focus: Guard, dodge, or parry to mitigate and counter damage.");

            addBtn(gameContext, "Seduction & Lust", [gameContext]() {
                gameContext->handleCommand({ CommandType::SELECT_COMBAT_FOCUS, 0, 0, "SEDUCTION" });
            }, true, false, "Select Seduction Focus: Tease, flirt, and compel erotic surrender.");

            bool hasCompanion = (players.size() > 1 || !gameContext->getCompanions().empty());
            addBtn(gameContext, "Companion Orders", [gameContext]() {
                gameContext->handleCommand({ CommandType::SELECT_COMBAT_FOCUS, 0, 0, "COMPANION" });
            }, hasCompanion, false, hasCompanion ? "Select Companion Focus: Command party elementals and familiars." : "No active companions in party.");

            addBtn(gameContext, "Items & Potions", [gameContext]() {
                gameContext->handleCommand({ CommandType::SELECT_COMBAT_FOCUS, 0, 0, "ITEM" });
            }, true, false, "Select Item Focus: Consume restorative elixirs, potions, and crystals.");

            addBtn(gameContext, "Surrender", [gameContext]() {
                gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SURRENDER" });
            }, true, false, "Yield to enemy combatants and enter submission.");

            addBtn(gameContext, "Escape", [gameContext]() {
                gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ESCAPE" });
            }, true, false, "Attempt to flee from combat encounter.");

            padButtonsTo(gameContext, 10);
            addBtn(gameContext, "End Turn", [combat, gameContext]() {
                combat->handleEndTurn(gameContext);
            }, true, false, "Conclude planning phase and resolve all queued turn actions.");

            padButtonsTo(gameContext, 13);
            addBtn(gameContext, "Clear Queue", [combat, gameContext]() {
                combat->handleClearQueue(gameContext);
            }, hasQueuedActions, false, hasQueuedActions ? "Clear all queued actions and refund spent Stamina/Mana." : "No actions currently queued.");

            padButtonsTo(gameContext, 14);
            addBtn(gameContext, "Reset", [combat, gameContext]() {
                combat->handleClearQueue(gameContext);
            }, hasQueuedActions, false, "Reset all queued actions and refund spent Stamina/Mana.");

            return;
        }

        auto addCombatSubmenuNav = [&](size_t totalActions) {
            size_t totalPages = (totalActions == 0) ? 1 : ((totalActions - 1) / 10) + 1;
            for (size_t pIdx = 0; pIdx < totalPages; ++pIdx)
            {
                padButtonsTo(gameContext, (pIdx * 15) + 10);
                addBtn(gameContext, "End Turn", [combat, gameContext]() {
                    combat->handleEndTurn(gameContext);
                }, true, false, "Conclude planning phase and resolve all queued turn actions.");

                padButtonsTo(gameContext, (pIdx * 15) + 13);
                addBtn(gameContext, "Clear Queue", [combat, gameContext]() {
                    combat->handleClearQueue(gameContext);
                }, hasQueuedActions, false, "Clear all queued actions and refund spent stamina/mana.");

                padButtonsTo(gameContext, (pIdx * 15) + 14);
                addBtn(gameContext, "Back (Reset)", [combat, gameContext]() {
                    combat->setCombatFocus(CombatFocus::ROOT);
                    gameContext->refreshActionGrid();
                }, true, false, "Return to main focus menu (clears and refunds queued actions).");
            }
        };

        auto addPagedCombatAction = [&](const std::string& label, std::function<void()> onClick, bool enabled, const std::string& tip, size_t actionIndex) {
            size_t page = actionIndex / 10;
            size_t indexOnPage = actionIndex % 10;
            size_t targetSlot = (page * 15) + indexOnPage;
            padButtonsTo(gameContext, targetSlot);
            addBtn(gameContext, label, std::move(onClick), enabled, false, tip);
        };

        if (combat->getCombatFocus() == CombatFocus::WEAPON)
        {
            std::vector<std::string> skillIds;
            auto eqWep = p ? p->inventory.getEquippedItem(equipSlot::WEAPON_MAIN) : nullptr;
            if (eqWep && !eqWep->weaponSkills.empty())
            {
                skillIds = eqWep->weaponSkills;
            }
            else
            {
                skillIds = { "skill_slash", "skill_thrust" };
            }

            size_t actionIdx = 0;
            std::string pRace = p ? p->anatomy.getDominantRace() : "Human";
            int pLevel = p ? p->stats.level : 1;
            for (const auto& sId : skillIds)
            {
                const WeaponSkill* ws = WeaponSkillDatabase::getSkill(sId);
                if (ws)
                {
                    bool levelMatch = (pLevel >= ws->requiredLevel);
                    bool raceMatch = ws->requiredRace.empty() || (pRace.find(ws->requiredRace) != std::string::npos);
                    bool canAfford = (curStamina >= ws->staminaCost);
                    bool canUse = canAfford && raceMatch && levelMatch;
                    std::string label = std::format("{} ({:.0f} Sta)", ws->name, ws->staminaCost);
                    std::string tip = std::format("{}\nDamage: {:.0f}%\nType: {}\nCost: {:.0f} Stamina",
                                                  ws->description, ws->damageMult * 100.0f, ws->damageType, ws->staminaCost);
                    if (!raceMatch) tip += "\n[Requires " + ws->requiredRace + " lineage]";
                    if (!levelMatch) tip += std::format("\n[Locked: Requires Level {}]", ws->requiredLevel);
                    addPagedCombatAction(label, [gameContext, sId]() {
                        gameContext->handleCommand({ CommandType::QUEUE_COMBAT_ACTION, 0, 0, sId });
                    }, canUse, tip, actionIdx++);
                }
            }

            addCombatSubmenuNav(actionIdx);
            return;
        }

        if (combat->getCombatFocus() == CombatFocus::MAGIC)
        {
            static const std::vector<std::string> spellIds = {
                "arcane_dart", "fireball", "arcane_shield", "cleanse", "teleport"
            };

            size_t actionIdx = 0;
            std::string pRace = p ? p->anatomy.getDominantRace() : "Human";
            int pLevel = p ? p->stats.level : 1;
            for (const auto& sId : spellIds)
            {
                const auto* sp = SpellDatabase::getSpell(sId);
                if (sp)
                {
                    bool levelMatch = (sp->unlocked || pLevel >= sp->requiredLevel);
                    bool raceMatch = sp->requiredRace.empty() || (pRace.find(sp->requiredRace) != std::string::npos);
                    bool canAfford = (curMp >= sp->mpCost && curStamina >= sp->staminaCost);
                    bool canUse = canAfford && raceMatch && levelMatch;
                    std::string label = std::format("{} ({:.0f} MP)", sp->name, sp->mpCost);
                    std::string tip = std::format("{}\nCost: {:.0f} MP, {:.0f} Stamina", sp->description, sp->mpCost, sp->staminaCost);
                    if (!raceMatch) tip += "\n[Requires " + sp->requiredRace + " lineage]";
                    if (!levelMatch) tip += std::format("\n[Locked: Requires Level {} to cast]", sp->requiredLevel);
                    addPagedCombatAction(label, [gameContext, sId]() {
                        gameContext->handleCommand({ CommandType::QUEUE_COMBAT_ACTION, 0, 0, sId });
                    }, canUse, tip, actionIdx++);
                }
            }

            addCombatSubmenuNav(actionIdx);
            return;
        }

        if (combat->getCombatFocus() == CombatFocus::DEFENSE)
        {
            static const std::vector<std::string> defActionIds = {
                "action_guard", "action_dodge", "action_parry"
            };

            size_t actionIdx = 0;
            for (const auto& aId : defActionIds)
            {
                const auto* act = CombatActionDatabase::getAction(aId);
                if (act)
                {
                    bool canAfford = (curStamina >= act->staminaCost);
                    std::string label = std::format("{} ({:.0f} Sta)", act->name, act->staminaCost);
                    std::string tip = std::format("{}\nCost: {:.0f} Stamina", act->description, act->staminaCost);
                    addPagedCombatAction(label, [gameContext, aId]() {
                        gameContext->handleCommand({ CommandType::QUEUE_COMBAT_ACTION, 0, 0, aId });
                    }, canAfford, tip, actionIdx++);
                }
            }

            addCombatSubmenuNav(actionIdx);
            return;
        }

        if (combat->getCombatFocus() == CombatFocus::SEDUCTION)
        {
            static const std::vector<std::string> seduceActionIds = {
                "action_seduce_tease", "action_seduce_flirt", "action_seduce_expose"
            };

            size_t actionIdx = 0;
            for (const auto& aId : seduceActionIds)
            {
                const auto* act = CombatActionDatabase::getAction(aId);
                if (act)
                {
                    bool canAfford = (curStamina >= act->staminaCost);
                    std::string label = std::format("{} ({:.0f} Sta)", act->name, act->staminaCost);
                    std::string tip = std::format("{}\nCost: {:.0f} Stamina\nLust: +{:.0f}", act->description, act->staminaCost, act->lustDamage);
                    addPagedCombatAction(label, [gameContext, aId]() {
                        gameContext->handleCommand({ CommandType::QUEUE_COMBAT_ACTION, 0, 0, aId });
                    }, canAfford, tip, actionIdx++);
                }
            }

            addCombatSubmenuNav(actionIdx);
            return;
        }

        if (combat->getCombatFocus() == CombatFocus::COMPANION)
        {
            size_t actionIdx = 0;
            const auto* act = CombatActionDatabase::getAction("action_companion_attack");
            float staCost = act ? act->staminaCost : 15.0f;
            std::string name = act ? act->name : "Familiar Strike";
            std::string desc = act ? act->description : "Command your elemental companion to unleash an elemental burst.";
            bool canCompAtk = (curStamina >= staCost);
            std::string label = std::format("{} ({:.0f} Sta)", name, staCost);
            std::string tip = std::format("{}\nCost: {:.0f} Stamina", desc, staCost);
            addPagedCombatAction(label, [gameContext]() {
                gameContext->handleCommand({ CommandType::QUEUE_COMBAT_ACTION, 0, 0, "COMPANION_ATTACK" });
            }, canCompAtk, tip, actionIdx++);

            addCombatSubmenuNav(actionIdx);
            return;
        }

        if (combat->getCombatFocus() == CombatFocus::ITEM)
        {
            size_t actionIdx = 0;
            const auto* potAct = CombatActionDatabase::getAction("item_potion");
            const auto* manaAct = CombatActionDatabase::getAction("item_mana");
            float potCost = (potAct && potAct->staminaCost > 0.0f) ? potAct->staminaCost : 5.0f;
            float manaCost = (manaAct && manaAct->staminaCost > 0.0f) ? manaAct->staminaCost : 5.0f;
            std::string potName = potAct ? potAct->name : "Health Potion";
            std::string potDesc = potAct ? potAct->description : "Queue drinking a healing potion to restore 50 HP.";
            std::string manaName = manaAct ? manaAct->name : "Mana Crystal";
            std::string manaDesc = manaAct ? manaAct->description : "Queue consuming a mana crystal to restore 50 MP.";

            bool canPot = (potionCount > 0 && curStamina >= potCost);
            std::string potLabel = std::format("{} ({:.0f} Sta)", potName, potCost);
            std::string potTip = canPot ? std::format("{}\nCost: {:.0f} Stamina", potDesc, potCost) : "No healing potions in inventory or insufficient stamina.";
            addPagedCombatAction(potLabel, [gameContext]() {
                gameContext->handleCommand({ CommandType::QUEUE_COMBAT_ACTION, 0, 0, "ITEM_POTION" });
            }, canPot, potTip, actionIdx++);

            bool canMana = (curStamina >= manaCost);
            std::string manaLabel = std::format("{} ({:.0f} Sta)", manaName, manaCost);
            std::string manaTip = std::format("{}\nCost: {:.0f} Stamina", manaDesc, manaCost);
            addPagedCombatAction(manaLabel, [gameContext]() {
                gameContext->handleCommand({ CommandType::QUEUE_COMBAT_ACTION, 0, 0, "ITEM_MANA" });
            }, canMana, manaTip, actionIdx++);

            addCombatSubmenuNav(actionIdx);
            return;
        }
    }

    if (dynamic_cast<eventState*>(currentState))
    {
        const auto& scene = gameContext->getCurrentScene();
        for (const auto& choice : scene.choices)
        {
            bool meetsReqs = gameContext->checkConditions(choice.requirements);
            if (!meetsReqs && choice.hiddenUnlessMet)
            {
                continue;
            }
            std::string tooltipText = choice.tooltip;

            ContentTagStatus choiceStatus = ContentFilterManager::evaluateChoice(gameContext->settings.content, choice);
            auto disallowed = ContentFilterManager::getDisallowedTags(gameContext->settings.content, choice.contentTags);
            auto warnings = ContentFilterManager::getWarningTags(gameContext->settings.content, choice.contentTags);

            bool isBlocked = (choiceStatus == ContentTagStatus::BLOCKED || !disallowed.empty());
            bool isWarned = (choiceStatus == ContentTagStatus::WARN || !warnings.empty());

            if (isBlocked && gameContext->settings.content.contentFilterMode == ContentFilterMode::BLOCK_AND_SKIP)
            {
                std::string tagStr;
                for (size_t i = 0; i < disallowed.size(); ++i)
                {
                    if (i > 0) tagStr += ", ";
                    tagStr += ContentFilterManager::getTagDisplayName(disallowed[i]);
                }
                addBtn(gameContext, "[Locked: " + tagStr + "]", nullptr, false, false, "Disabled in Content Settings: " + tagStr);
                continue;
            }

            std::string btnLabel = choice.label;
            if (isWarned || (isBlocked && gameContext->settings.content.contentFilterMode == ContentFilterMode::WARN_CONFIRM))
            {
                btnLabel = "[!] " + btnLabel;
            }

            if (!meetsReqs)
            {
                if (tooltipText.empty())
                {
                    tooltipText = "Requirements not met for this option.";
                }
                else
                {
                    tooltipText = "[Locked] " + tooltipText;
                }
                addBtn(gameContext, btnLabel, nullptr, false, false, tooltipText);
            }
            else
            {
                if (tooltipText.empty())
                {
                    tooltipText = std::format("Select: {}", choice.label);
                }
                addBtn(gameContext, btnLabel, [gameContext, choice]() { gameContext->processChoice(choice); }, true, false, tooltipText);
            }
        }
        return;
    }

    if (dynamic_cast<explorationState*>(currentState))
    {
        if (gameContext->map)
            {
                MapWarp warp;
                if (gameContext->map->checkWarp(gameContext->gridX, gameContext->gridY, warp))
                {
                    std::string warpLabel = warp.label;
                    if (warpLabel.empty())
                    {
                        std::string targetName = gameMap::getMapNameById(warp.targetMap);
                        warpLabel = targetName.empty() ? ("Enter " + warp.targetMap) : ("Enter " + targetName);
                    }

                    addBtn(gameContext, warpLabel, [gameContext, warp]() {
                        gameContext->loadMap(warp.targetMap, warp.targetX, warp.targetY);
                    }, true, false, "Transition into " + (warp.targetMap.empty() ? "adjacent area" : warp.targetMap) + ".");
                }

                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                auto targetNPC = gameContext->getActiveTargetNPCShared();

                bool isRestocking = tileData.ambushState.isDefeated && tileData.ambushState.restockMinutesRemaining > 0;
                bool hasEncounterHazard = !tileData.ambushState.isPermanentlyRemoved && !isRestocking &&
                                          (tileData.getEffectiveDangerLevel() > 0 ||
                                           tileData.ambushState.npc != nullptr ||
                                           !tileData.ambushState.templateId.empty() ||
                                           !tileData.ambushState.templatePool.empty());

                if (hasEncounterHazard)
                {
                    addBtn(gameContext, "Explore", [gameContext]() {
                        gameContext->exploreTile();
                    }, true, false, "Scout and search the area, rerolling the tile encounter chance.");
                }

                if (targetNPC)
                {
                    bool isEnemy = (tileData.ambushState.npc && tileData.ambushState.npc == targetNPC) ||
                                   (targetNPC->stats.getBaseStat("corruption") > 50.0f) ||
                                   (targetNPC->name.find("Bandit") != std::string::npos ||
                                    targetNPC->name.find("Cutpurse") != std::string::npos ||
                                    targetNPC->name.find("Rogue") != std::string::npos);

                    if (!isEnemy)
                    {
                        addBtn(gameContext, std::format("Talk to {}", targetNPC->name), [gameContext, targetNPC]() {
                            gameContext->openNPCTalkMenu(targetNPC);
                        }, true, false, "Initiate dialogue conversation with " + targetNPC->name + ".");

                        auto namedChar = NamedCharacterManager::getCharacter(targetNPC->id);
                        bool isMerchant = (namedChar && namedChar->isMerchant) ||
                                          targetNPC->name.find("Merchant") != std::string::npos ||
                                          targetNPC->name.find("Shop") != std::string::npos;

                        if (isMerchant)
                        {
                            addBtn(gameContext, "Visit Shop", [gameContext, targetNPC]() {
                                gameContext->changeState(std::make_unique<shopState>(targetNPC));
                            }, true, false, "Browse " + targetNPC->name + "'s inventory and trade goods.");
                        }
                    }
                }

                std::unordered_set<std::string> addedTriggerIds;

                auto tileTriggers = gameContext->map->getTriggersAt(gameContext->gridX, gameContext->gridY);
                for (const auto& trig : tileTriggers)
                {
                    if (gameContext->checkConditions(trig.conditions) && !addedTriggerIds.contains(trig.id))
                    {
                        addedTriggerIds.insert(trig.id);
                        addBtn(gameContext, trig.label, [gameContext, scene = trig.sceneId]() {
                            gameContext->loadScene(scene);
                        }, true, false, trig.tooltip.empty() ? ("Trigger " + trig.label) : trig.tooltip);
                    }
                }

                auto questTriggers = questDatabase::getTriggersForLocation(gameContext->map->getId(), gameContext->gridX, gameContext->gridY);
                for (const auto& trig : questTriggers)
                {
                    if (gameContext->checkConditions(trig.conditions) && !addedTriggerIds.contains(trig.id))
                    {
                        addedTriggerIds.insert(trig.id);
                        addBtn(gameContext, trig.label, [gameContext, scene = trig.sceneId]() {
                            gameContext->loadScene(scene);
                        }, true, false, trig.tooltip.empty() ? ("Trigger " + trig.label) : trig.tooltip);
                    }
                }

                if (!tileData.droppedItems.empty())
                {
                    addBtn(gameContext, std::format("Examine Ground ({} items)", tileData.droppedItems.size()), [gameContext]() {
                        gameContext->changeState(std::make_unique<inventoryState>());
                    }, true, false, "Inspect and pick up items lying on the ground.");
                }

                if (servantManager::getServantCount() > 0)
                {
                    addBtn(gameContext, "Servants", [gameContext]() {
                        gameContext->changeState(std::make_unique<servantState>());
                    }, true, false, "Manage servant schedules, psychology, and work assignments.");
                }

                bool isNurseryStation = (tileData.customization.stationType == "nursery" ||
                                         std::find(tileData.customization.roomTags.begin(), tileData.customization.roomTags.end(), "nursery") != tileData.customization.roomTags.end() ||
                                         !offspringManager::getAllOffspring().empty());
                if (isNurseryStation)
                {
                    addBtn(gameContext, "Visit Nursery", [gameContext]() {
                        gameContext->changeState(std::make_unique<nurseryState>());
                    }, true, false, "Inspect developing offspring, rename children, or recruit adult offspring.");
                }

                bool isMilkingStation = (tileData.customization.stationType == "milking_parlor" ||
                                         std::find(tileData.customization.roomTags.begin(), tileData.customization.roomTags.end(), "milking") != tileData.customization.roomTags.end());
                if (isMilkingStation)
                {
                    float totalFluids = 0.0f;
                    if (gameContext->getPlayer())
                    {
                        for (const auto& optPart : gameContext->getPlayer()->anatomy.getAllParts())
                        {
                            if (optPart.has_value())
                            {
                                totalFluids += optPart->currentFluidMl;
                            }
                        }
                    }
                    bool canExtract = (totalFluids >= 5.0f);
                    std::string milkTip = "Connect to automated milking apparatus to extract and bottle internal biological fluids.";
                    if (!canExtract)
                    {
                        milkTip += "\n[Inaccessible: No extractable bodily fluids available in reserves.]";
                    }

                    addBtn(gameContext, "Extract Personal Fluids", [gameContext]() {
                        if (gameContext->getPlayer())
                        {
                            auto res = fluidHarvesting::harvestAllFluids(gameContext->getPlayer());
                            gameContext->refreshActionGrid();
                        }
                    }, canExtract, false, milkTip);
                }

                bool isServiceStation = (tileData.customization.stationType == "service_room" ||
                                         std::find(tileData.customization.roomTags.begin(), tileData.customization.roomTags.end(), "service_room") != tileData.customization.roomTags.end());
                if (isServiceStation)
                {
                    addBtn(gameContext, "Get Client", [gameContext]() {
                        auto client = npcGenerator::composeRandomClient();
                        if (client)
                        {
                            gameContext->activeTargetNPC = client;
                            float budget = client->getStat("currency");
                            if (budget <= 0.0f) budget = 100.0f;
                            auto sex = std::make_unique<sexState>(client, SexStance::MISSIONARY, 20.0f);
                            sex->setClientSession(true, 50.0f, budget);
                            gameContext->changeState(std::move(sex));
                        }
                    }, true, false, "Solicit a visiting commercial client for an intimate service session.");
                }

                bool isWashStation = (tileData.customization.stationType == "wash" ||
                                      tileData.customization.stationType == "water" ||
                                      tileData.customization.stationType == "bath" ||
                                      tileData.customization.stationType == "well" ||
                                      std::find(tileData.customization.roomTags.begin(), tileData.customization.roomTags.end(), "water") != tileData.customization.roomTags.end() ||
                                      std::find(tileData.customization.roomTags.begin(), tileData.customization.roomTags.end(), "bath") != tileData.customization.roomTags.end() ||
                                      std::find(tileData.customization.roomTags.begin(), tileData.customization.roomTags.end(), "wash") != tileData.customization.roomTags.end() ||
                                      gameContext->map->hasTileTag(gameContext->gridX, gameContext->gridY, "water") ||
                                      gameContext->map->hasTileTag(gameContext->gridX, gameContext->gridY, "wash") ||
                                      gameContext->map->hasTileTag(gameContext->gridX, gameContext->gridY, "bath") ||
                                      gameContext->map->hasTileTag(gameContext->gridX, gameContext->gridY, "well") ||
                                      gameContext->map->hasTileTag(gameContext->gridX, gameContext->gridY, "lake") ||
                                      gameContext->map->hasTileTag(gameContext->gridX, gameContext->gridY, "river") ||
                                      gameContext->map->hasTileTag(gameContext->gridX, gameContext->gridY, "fountain"));

                if (isWashStation)
                {
                    bool hasSoiledGarments = false;
                    if (gameContext->getPlayer())
                    {
                        for (const auto& eq : gameContext->getPlayer()->inventory.equipped)
                        {
                            if (eq && eq->soiling.isDirty) { hasSoiledGarments = true; break; }
                        }
                        if (!hasSoiledGarments)
                        {
                            for (const auto& bk : gameContext->getPlayer()->inventory.backpack)
                            {
                                if (bk && bk->soiling.isDirty) { hasSoiledGarments = true; break; }
                            }
                        }
                    }

                    std::string washTip = "Wash and scrub all equipped and carried garments, removing dirt, cum, and fluid stains.";
                    if (!hasSoiledGarments)
                    {
                        washTip += "\n[Inaccessible: All garments are completely clean.]";
                    }

                    addBtn(gameContext, "Wash Garments", [gameContext]() {
                        gameContext->washGarments();
                    }, hasSoiledGarments, false, washTip);
                }

                if (tileData.customization.isCustomizable)
                {
                    float playerGold = gameContext->getPlayer() ? gameContext->getPlayer()->getStat("currency") : 0.0f;
                    if (tileData.customization.currentRoomId.empty())
                    {
                        auto rootRooms = roomDatabase::getRootRooms();
                        for (const auto& rDef : rootRooms)
                        {
                            bool canAfford = (playerGold >= rDef.costGold);
                            std::string label = std::format("Build {}", rDef.name);
                            std::string tt = std::format("Construct {} for {:.0f}¤. {}", rDef.name, rDef.costGold, rDef.description);
                            if (!canAfford)
                            {
                                tt += std::format("\n[Inaccessible: Insufficient gold (Requires {:.0f}¤, have {:.0f}¤)]", rDef.costGold, playerGold);
                            }
                            addBtn(gameContext, label, [gameContext, rId = rDef.id]() {
                                gameContext->map->upgradeTileRoom(gameContext->gridX, gameContext->gridY, rId, gameContext);
                                gameContext->refreshActionGrid();
                            }, canAfford, false, tt);
                        }
                    }
                    else
                    {
                        const auto* curDef = roomDatabase::getRoom(tileData.customization.currentRoomId);
                        auto upgrades = roomDatabase::getAvailableUpgrades(tileData.customization.currentRoomId);
                        for (const auto& upDef : upgrades)
                        {
                            bool canAfford = (playerGold >= upDef.costGold);
                            std::string label = std::format("Upgrade: {}", upDef.name);
                            std::string tt = std::format("Upgrade to {} for {:.0f}¤ (Tier {}). {}", upDef.name, upDef.costGold, upDef.tier, upDef.description);
                            if (!canAfford)
                            {
                                tt += std::format("\n[Inaccessible: Insufficient gold (Requires {:.0f}¤, have {:.0f}¤)]", upDef.costGold, playerGold);
                            }
                            addBtn(gameContext, label, [gameContext, upId = upDef.id]() {
                                gameContext->map->upgradeTileRoom(gameContext->gridX, gameContext->gridY, upId, gameContext);
                                gameContext->refreshActionGrid();
                            }, canAfford, false, tt);
                        }

                        float refund = curDef ? (curDef->costGold * 0.5f) : 0.0f;
                        std::string demoLabel = std::format("Demolish (+{:.0f}¤)", refund);
                        addBtn(gameContext, demoLabel, [gameContext]() {
                            gameContext->map->demolishTileRoom(gameContext->gridX, gameContext->gridY, gameContext);
                            gameContext->refreshActionGrid();
                        }, true, false, "Demolish room and revert to plot foundation. 50% gold refunded; ground storage preserved.");
                    }
                }
            }
            return;
        }

        if (auto* sState = dynamic_cast<servantState*>(currentState))
        {
            auto servants = servantManager::getAllServants();
            if (servants.empty())
            {
                addBtn(gameContext, "Back", [gameContext]() {
                    gameContext->changeState(std::make_unique<explorationState>());
                }, true, false, "Return to area exploration.");
                return;
            }

            for (const auto& s : servants)
            {
                if (!s) continue;
                bool isSelected = (s->id == sState->selectedServantId);
                std::string sLabel = isSelected ? ("* " + s->name + " *") : s->name;
                std::string tt = std::format("Obed: {:.0f} | Morale: {:.0f} | Stam: {:.0f}/100", 
                                             s->servant.stats.obedience, s->servant.stats.morale, s->servant.stats.stamina);
                addBtn(gameContext, sLabel, [sState, sId = s->id, gameContext]() {
                    sState->selectServant(sId, gameContext);
                }, true, isSelected, tt);
            }

            auto curServant = servantManager::getServant(sState->selectedServantId);
            if (curServant)
            {
                std::string currentHourJob = curServant->servant.getJobForHour(sState->selectedHour);
                std::string hourBtnLabel = std::format("Hour {:02d}:00 [{}]", sState->selectedHour, currentHourJob);
                addBtn(gameContext, hourBtnLabel, [sState, gameContext]() {
                    sState->selectedHour = (sState->selectedHour + 1) % 24;
                    gameContext->refreshActionGrid();
                }, true, false, "Cycle to the next hour slot (00:00 to 23:00).");

                for (const auto& [jId, jDef] : servantManager::getAllJobs())
                {
                    bool isCurJob = (currentHourJob == jId);
                    std::string jBtnLabel = std::format("Set {}", jDef.name);
                    std::string jTooltip = std::format("Drain: {:.0f}/h | {}", jDef.staminaCostPerHour, jDef.description);
                    addBtn(gameContext, jBtnLabel, [sState, jId, gameContext]() {
                        sState->assignJobToSelected(sState->selectedHour, jId, gameContext);
                    }, true, isCurJob, jTooltip);
                }

                addBtn(gameContext, "Full Day Rest", [sState, gameContext]() {
                    sState->setFullDayJob("rest", gameContext);
                }, true, false, "Set all 24 hours to rest and recuperate stamina.");

                addBtn(gameContext, "Release Servant", [sState, gameContext]() {
                    sState->releaseServant(gameContext);
                }, true, false, "Grant this servant their freedom.");
            }

            addBtn(gameContext, "Back", [gameContext, sState]() {
                if (sState->previousState) gameContext->changeState(std::move(sState->previousState));
                else gameContext->changeState(std::make_unique<explorationState>());
            }, true, false, "Return to area exploration.");
            return;
        }

        if (auto* nState = dynamic_cast<nurseryState*>(currentState))
        {
            auto& records = offspringManager::getAllOffspring();
            if (records.empty())
            {
                addBtn(gameContext, "Back", [gameContext, nState]() {
                    if (nState->previousState) gameContext->changeState(std::move(nState->previousState));
                    else gameContext->changeState(std::make_unique<explorationState>());
                }, true, false, "Return to area exploration.");
                return;
            }

            for (const auto& r : records)
            {
                bool isSelected = (r.id == nState->selectedOffspringId);
                std::string label = isSelected ? ("* " + r.name + " *") : r.name;
                std::string tt = std::format("{} | Age: {}d | {}", r.isAdult() ? "Adult NPC" : "Child", r.daysAlive, r.rolledRace);
                addBtn(gameContext, label, [nState, rId = r.id, gameContext]() {
                    nState->selectOffspring(rId, gameContext);
                }, true, isSelected, tt);
            }

            auto* curRecord = offspringManager::getOffspring(nState->selectedOffspringId);
            if (curRecord)
            {
                if (curRecord->isAdult())
                {
                    addBtn(gameContext, "Recruit Companion", [nState, gameContext]() {
                        nState->promoteToCompanion(gameContext);
                    }, true, false, "Recruit this matured adult offspring into your active adventuring party.");

                    addBtn(gameContext, "Assign to Servants", [nState, gameContext]() {
                        nState->promoteToServant(gameContext);
                    }, true, false, "Assign this adult offspring to estate servitude and daily labor.");
                }

                addBtn(gameContext, "Rename Child", [nState, gameContext, curRecord]() {
                    std::string newName = curRecord->name + " Jr.";
                    nState->renameSelected(newName, gameContext);
                }, true, false, "Give this offspring a new name.");
            }

            addBtn(gameContext, "Back", [gameContext, nState]() {
                if (nState->previousState) gameContext->changeState(std::move(nState->previousState));
                else gameContext->changeState(std::make_unique<explorationState>());
            }, true, false, "Return to area exploration.");
            return;
        }
    }