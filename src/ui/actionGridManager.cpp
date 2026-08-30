#include "ui/actionGridManager.h"

#include <format>
#include <memory>
#include <unordered_set>

#include "core/game.h"
#include "map/encounterResolver.h"
#include "save/saveManager.h"
#include "settings/settingsManager.h"
#include "state/characterCreationState.h"
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
#include "ui/theme.h"

namespace
{
    inline void padButtonsTo(game* g, size_t targetCount)
    {
        while (g->activeButtons.size() < targetCount)
        {
            actionButton btn;
            btn.label = "";
            btn.isEnabled = false;
            btn.isSelected = false;
            btn.onClick = nullptr;
            g->activeButtons.push_back(btn);
        }
    }

    inline void addBtn(game* g, std::string_view label, std::function<void()> onClick, bool enabled = true, bool selected = false)
    {
        actionButton btn;
        btn.label = std::string(label);
        btn.isEnabled = enabled;
        btn.isSelected = selected;
        btn.onClick = std::move(onClick);
        g->activeButtons.push_back(btn);
    }

    inline void addBackBtn(game* g, std::string_view label, std::function<void()> onClick)
    {
        padButtonsTo(g, 14);
        actionButton btn;
        btn.label = std::string(label);
        btn.isEnabled = true;
        btn.isSelected = false;
        btn.onClick = std::move(onClick);
        g->activeButtons.push_back(btn);
    }
}

void ActionGridManager::refresh(game* gameContext)
{
    if (!gameContext) return;

    iGameState* currentState = gameContext->getActiveState();
    if (!currentState) return;

    gameContext->activeButtons.clear();

    // 1. Main Menu
    if (auto menu = dynamic_cast<mainMenuState*>(currentState))
    {
        addBtn(gameContext, "New Game", [gameContext]() {
            gameContext->handleCommand({ CommandType::START_NEW_GAME, 0, 0, "" });
        });
        addBtn(gameContext, "Save/Load", [gameContext]() {
            gameContext->changeState(std::make_unique<loadGameState>(SaveMenuMode::LOAD_ONLY));
        });
        addBtn(gameContext, "", nullptr, false);
        addBtn(gameContext, "", nullptr, false);
        addBtn(gameContext, "Quit", [gameContext]() {
            gameContext->handleCommand({ CommandType::QUIT_GAME, 0, 0, "" });
        });
        addBtn(gameContext, "Options", [gameContext]() {
            gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS, std::make_unique<mainMenuState>()));
        });
        addBtn(gameContext, "Content Options", [gameContext]() {
            gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS, std::make_unique<mainMenuState>()));
        });
        padButtonsTo(gameContext, 14);
        addBtn(gameContext, "Resume", [gameContext]() {
            if (gameContext->getPlayer()) gameContext->changeState(std::make_unique<explorationState>());
        }, gameContext->getPlayer() != nullptr);
        return;
    }

    // 2. Character Creation / Customization Editor
    if (auto cc = dynamic_cast<characterCreationState*>(currentState))
    {
        auto activeTabs = cc->getActiveTabs();
        int tabCount = static_cast<int>(activeTabs.size());
        if (cc->step >= tabCount) cc->step = std::max(0, tabCount - 1);

        // Row 1: Dynamic Category Step Tabs (up to 5 tabs per row)
        if (tabCount > 1)
        {
            for (int i = 0; i < std::min(5, tabCount); ++i)
            {
                std::string btnLabel = std::format("{}. {}", i + 1, cc->getTabName(activeTabs[i]));
                addBtn(gameContext, btnLabel, [cc, gameContext, i]() {
                    cc->step = i;
                    gameContext->refreshActionGrid();
                }, true, cc->step == i);
            }
        }
        else if (tabCount == 1)
        {
            addBtn(gameContext, cc->getTabName(activeTabs[0]), [cc, gameContext]() {}, true, true);
        }

        // Row 2: Step Navigation & Finish Action
        padButtonsTo(gameContext, 5);
        if (cc->step > 0 && tabCount > 1)
        {
            addBtn(gameContext, "Previous Step", [cc, gameContext]() {
                cc->step--;
                gameContext->refreshActionGrid();
            });
        }
        else
        {
            padButtonsTo(gameContext, 6);
        }

        if (cc->step < tabCount - 1 && tabCount > 1)
        {
            addBtn(gameContext, "Next Step", [cc, gameContext]() {
                cc->step++;
                gameContext->refreshActionGrid();
            });
        }
        else
        {
            addBtn(gameContext, cc->config.isNewGameCreation ? "Start Game" : "Apply Changes", [cc, gameContext]() {
                cc->finalizeCharacter(gameContext);
            }, true, true);
        }

        padButtonsTo(gameContext, 9);
        addBtn(gameContext, cc->config.isNewGameCreation ? "Start Game" : "Apply Changes", [cc, gameContext]() {
            cc->finalizeCharacter(gameContext);
        });

        // Row 3: Utilities & Return
        padButtonsTo(gameContext, 10);
        addBtn(gameContext, "Randomize All", [cc, gameContext]() {
            cc->randomizeAll();
            gameContext->refreshActionGrid();
        });

        padButtonsTo(gameContext, 14);
        std::string backLabel = cc->config.isNewGameCreation ? "Back to Menu" : "Cancel";
        addBackBtn(gameContext, backLabel, [gameContext]() {
            gameContext->changeState(std::make_unique<mainMenuState>());
        });
        return;
    }

    // 3. Load / Save Game State
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

    // 4. Options State
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
            addBtn(gameContext, "Sex & Fetishes", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::SEX_AND_FETISHES; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::SEX_AND_FETISHES);
            addBtn(gameContext, "Bodies", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::BODIES; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::BODIES);
            addBtn(gameContext, "Reset Category", [opt, gameContext]() { opt->resetCategoryDefaults(gameContext); });

            addBtn(gameContext, "Gender preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::GENDER_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::GENDER_PREFS);
            addBtn(gameContext, "Orientation preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::ORIENTATION_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::ORIENTATION_PREFS);
            addBtn(gameContext, "Age preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::AGE_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::AGE_PREFS);
            addBtn(gameContext, "Furry preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::FURRY_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::FURRY_PREFS);
            addBtn(gameContext, "Fetish preferences", [opt, gameContext]() { opt->contentCategory = ContentOptionsCategory::FETISH_PREFS; gameContext->refreshActionGrid(); }, true, opt->contentCategory == ContentOptionsCategory::FETISH_PREFS);

            addBtn(gameContext, "Reset All", [opt, gameContext]() { opt->resetAllDefaults(gameContext); });
            addBackBtn(gameContext, "Back", [gameContext, opt]() { opt->goBack(gameContext); });
            return;
        }
    }

    // 5. Shop State
    if (auto shop = dynamic_cast<shopState*>(currentState))
    {
        const auto& catalog = shop->getCatalog();
        for (size_t i = 0; i < catalog.size(); ++i)
        {
            addBtn(gameContext, std::format("Buy {} ({}¤)", catalog[i].name, catalog[i].price), [gameContext, i]() {
                gameContext->handleCommand({ CommandType::BUY_SHOP_ITEM, static_cast<int>(i), 0, "" });
            }, catalog[i].stock > 0);
        }
        addBackBtn(gameContext, "Leave Shop (ESC)", [gameContext]() { gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" }); });
        return;
    }

    // 6. Transformation State
    if (dynamic_cast<transformationState*>(currentState))
    {
        addBackBtn(gameContext, "Close (ESC)", [gameContext]() { gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" }); });
        return;
    }

    // 7. Interactive Sex State
    if (auto sex = dynamic_cast<sexState*>(currentState))
    {
        const auto& actions = sex->getAvailableActions();
        for (size_t i = 0; i < actions.size(); ++i)
        {
            addBtn(gameContext, actions[i].name, [gameContext, i]() {
                gameContext->handleCommand({ CommandType::EXECUTE_SEX_ACTION, static_cast<int>(i), 0, "" });
            });
        }

        if (sex->isSolo())
        {
            static constexpr SexStance soloStances[] = { SexStance::SOLO_BED, SexStance::SOLO_CHAIR, SexStance::SOLO_STANDING };
            for (SexStance st : soloStances)
            {
                if (st != sex->getStance())
                {
                    addBtn(gameContext, std::format("Pos: {}", sexStanceToString(st)), [gameContext, sex, st]() {
                        sex->changeStance(st);
                        gameContext->refreshActionGrid();
                    });
                }
            }
        }
        else if (sex->isPlayerDominant())
        {
            static constexpr SexStance stances[] = { SexStance::MISSIONARY, SexStance::FROM_BEHIND, SexStance::KNEELING, SexStance::STANDING, SexStance::LAP_SITTING };
            for (SexStance st : stances)
            {
                if (st != sex->getStance())
                {
                    addBtn(gameContext, std::format("Stance: {}", sexStanceToString(st)), [gameContext, st]() {
                        gameContext->handleCommand({ CommandType::CHANGE_SEX_STANCE, static_cast<int>(st), 0, "" });
                    });
                }
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

    // 8. Phone Apps
    if (auto phoneApp = dynamic_cast<phoneAppsState*>(currentState))
    {
        auto setApp = [gameContext, phoneApp](PhoneAppMode mode) {
            phoneApp->setAppMode(mode);
            phoneApp->loadData(mode);
            gameContext->refreshActionGrid();
        };

        addBtn(gameContext, "Quests", [=]() { setApp(PhoneAppMode::QUESTS); }, true, phoneApp->getAppMode() == PhoneAppMode::QUESTS);
        addBtn(gameContext, "Perk Tree", [=]() { setApp(PhoneAppMode::PERKS); }, true, phoneApp->getAppMode() == PhoneAppMode::PERKS);
        addBtn(gameContext, "Spells", [=]() { setApp(PhoneAppMode::SPELLS); }, true, phoneApp->getAppMode() == PhoneAppMode::SPELLS);
        addBtn(gameContext, "Contacts", [=]() { setApp(PhoneAppMode::CONTACTS); }, true, phoneApp->getAppMode() == PhoneAppMode::CONTACTS);
        addBtn(gameContext, "Stats", [gameContext, phoneApp]() { phoneApp->setAppMode(PhoneAppMode::STATS); gameContext->refreshActionGrid(); }, true, phoneApp->getAppMode() == PhoneAppMode::STATS);

        addBtn(gameContext, "Encyclopedia", [=]() { setApp(PhoneAppMode::ENCYCLOPEDIA); }, true, phoneApp->getAppMode() == PhoneAppMode::ENCYCLOPEDIA);
        addBtn(gameContext, "Maps", [gameContext, phoneApp]() { phoneApp->setAppMode(PhoneAppMode::MAPS); gameContext->refreshActionGrid(); }, true, phoneApp->getAppMode() == PhoneAppMode::MAPS);
        addBtn(gameContext, "Next Entry", [gameContext, phoneApp]() { phoneApp->setSelectedItemIndex(phoneApp->getSelectedItemIndex() + 1); gameContext->refreshActionGrid(); });
        addBtn(gameContext, "Prev Entry", [gameContext, phoneApp]() {
            if (phoneApp->getSelectedItemIndex() > 0)
            {
                phoneApp->setSelectedItemIndex(phoneApp->getSelectedItemIndex() - 1);
                gameContext->refreshActionGrid();
            }
        });

        addBackBtn(gameContext, "Back (Phone)", [gameContext]() {
            gameContext->isPhoneMenuOpen = true;
            gameContext->changeState(std::make_unique<explorationState>());
        });
        return;
    }

    // 9. Encounter Resolution Hub
    if (auto resState = dynamic_cast<encounterResolutionState*>(currentState))
    {
        const auto& records = resState->getDefeatedRecords();
        size_t selectedIdx = resState->getSelectedIndex();

        if (records.size() > 1)
        {
            for (size_t i = 0; i < records.size(); ++i)
            {
                addBtn(gameContext, std::format("Target: {}", records[i].npc ? records[i].npc->name : "Enemy"), [gameContext, i]() {
                    gameContext->handleCommand({ CommandType::SELECT_RESOLUTION_TARGET, static_cast<int>(i), 0, "" });
                }, i != selectedIdx);
            }
        }

        if (selectedIdx < records.size())
        {
            const auto& rec = records[selectedIdx];
            addBtn(gameContext, rec.isLooted ? "Looted" : "Loot Items & Gold", [gameContext]() {
                gameContext->handleCommand({ CommandType::LOOT_ENEMY, 0, 0, "" });
            }, !rec.isLooted);

            addBtn(gameContext, rec.isStripped ? "Stripped" : "Strip Clothing", [gameContext]() {
                gameContext->handleCommand({ CommandType::STRIP_ENEMY, 0, 0, "" });
            }, !rec.isStripped);

            addBtn(gameContext, rec.hadSex ? "Erotic Interaction (Repeat)" : "Interactive Sex", [gameContext]() {
                gameContext->handleCommand({ CommandType::INTERACTIVE_SEX, 0, 0, "" });
            });

            addBtn(gameContext, rec.isSubjugated ? "Subjugated" : "Subjugate", [gameContext]() {
                gameContext->handleCommand({ CommandType::SUBJUGATE_ENEMY, 0, 0, "" });
            }, !rec.isSubjugated);

            addBtn(gameContext, rec.isReleased ? "Released" : "Release", [gameContext]() {
                gameContext->handleCommand({ CommandType::RELEASE_ENEMY, 0, 0, "" });
            }, !rec.isReleased);
        }

        addBackBtn(gameContext, "Leave Resolution Hub", [gameContext]() {
            gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
        });
        return;
    }

    // 10. Inventory State Actions
    if (dynamic_cast<inventoryState*>(currentState))
    {
        if (gameContext->selectedEquipmentSlot != equipSlot::NONE)
        {
            addBtn(gameContext, "Unequip", [gameContext]() { gameContext->handleUnequipAction(gameContext->selectedEquipmentSlot); });
            addBtn(gameContext, "Pull Aside", [gameContext]() {
                if (auto p = gameContext->getPlayer()) { p->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::PULL_ASIDE); gameContext->refreshActionGrid(); }
            });
            addBtn(gameContext, "Pull Up / Down", [gameContext]() {
                if (auto p = gameContext->getPlayer()) { p->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::LIFT_UP); gameContext->refreshActionGrid(); }
            });
            addBtn(gameContext, "Unbutton / Open", [gameContext]() {
                if (auto p = gameContext->getPlayer()) { p->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::UNBUTTON); gameContext->refreshActionGrid(); }
            });
            addBtn(gameContext, "Reset Fit", [gameContext]() {
                if (auto p = gameContext->getPlayer()) { p->inventory.setDisplacement(gameContext->selectedEquipmentSlot, DisplacementMode::NONE); gameContext->refreshActionGrid(); }
            });
        }
        else if (gameContext->selectedInventoryIndex != -1)
        {
            if (gameContext->selectedInventorySide == 0)
            {
                addBtn(gameContext, "Equip / Use", [gameContext]() { gameContext->handleEquipAction(gameContext->selectedInventoryIndex); });
                addBtn(gameContext, "Drop 1", [gameContext]() { gameContext->handleDropAction(gameContext->selectedInventoryIndex, 1); });
                addBtn(gameContext, "Drop All", [gameContext]() { gameContext->handleDropAction(gameContext->selectedInventoryIndex, 999); });
            }
            else if (gameContext->selectedInventorySide == 1)
            {
                addBtn(gameContext, "Pickup 1", [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 1); });
                addBtn(gameContext, "Pickup All", [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 999); });
            }
        }

        addBackBtn(gameContext, "Close (I / ESC)", [gameContext]() { gameContext->changeState(std::make_unique<explorationState>()); });
        return;
    }

    // 11. Combat State Actions
    if (auto combat = dynamic_cast<CombatState*>(currentState))
    {
        // Row 1: Physical Attacks
        addBtn(gameContext, "Strike (1 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "STRIKE" }); });
        addBtn(gameContext, "Heavy Strike (2 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "HEAVY_STRIKE" }); });
        addBtn(gameContext, "Defend (1 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "DEFEND" }); });
        addBtn(gameContext, "Disarm (2 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "DISARM" }); });
        addBtn(gameContext, "End Turn", [combat, gameContext]() { combat->handleEndTurn(gameContext); });

        // Row 2: Spells & Magic
        addBtn(gameContext, "Arcane Dart (10 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_DART" }); });
        addBtn(gameContext, "Fireball (25 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_FIREBALL" }); });
        addBtn(gameContext, "Shield (15 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_SHIELD" }); });
        addBtn(gameContext, "Cleanse (20 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_CLEANSE" }); });
        addBtn(gameContext, "Blink (30 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_BLINK" }); });

        // Row 3: Items & Utility
        addBtn(gameContext, "Potion (+50 HP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ITEM_POTION" }); });
        addBtn(gameContext, "Mana (+50 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ITEM_MANA" }); });
        addBtn(gameContext, "Surrender", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SURRENDER" }); });
        addBtn(gameContext, "Escape", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ESCAPE" }); });
        addBtn(gameContext, "Victory (Skip)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "WIN" }); });
        return;
    }

    // 12. Scene Event Choices
    if (dynamic_cast<eventState*>(currentState))
    {
        const auto& scene = gameContext->getCurrentScene();
        for (const auto& choice : scene.choices)
        {
            if (gameContext->checkConditions(choice.requirements))
            {
                addBtn(gameContext, choice.label, [gameContext, choice]() { gameContext->processChoice(choice); });
            }
        }
        return;
    }

    // 13. Exploration Movement & Interaction Shortcuts
    if (dynamic_cast<explorationState*>(currentState))
    {
        if (gameContext->isPhoneMenuOpen)
        {
            // Row 1: Core Phone Apps
            addBtn(gameContext, "Quests", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::QUESTS)); });
            addBtn(gameContext, "Perk Tree", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::PERKS)); });
            addBtn(gameContext, "Spells", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::SPELLS)); });
            addBtn(gameContext, "Fetishes", [gameContext]() { gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS)); });
            addBtn(gameContext, "Stats", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::STATS)); });

            // Row 2: Secondary Phone Apps
            addBtn(gameContext, "Selfie", [gameContext]() { gameContext->changeState(std::make_unique<characterCreationState>(3)); });
            addBtn(gameContext, "Contacts", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::CONTACTS)); });
            addBtn(gameContext, "Encyclopedia", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::ENCYCLOPEDIA)); });
            addBtn(gameContext, "Transform", []() {}, false);
            addBtn(gameContext, "Maps", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::MAPS)); });

            // Row 3: Actions & Navigation
            addBtn(gameContext, "Combat Moves", [gameContext]() { gameContext->changeState(std::make_unique<phoneAppsState>(PhoneAppMode::SPELLS)); });
            addBtn(gameContext, "Masturbate", [gameContext]() { gameContext->changeState(std::make_unique<sexState>()); });
            addBtn(gameContext, "Loiter", [gameContext]() { gameContext->gameTime.advanceTime(15); gameContext->refreshActionGrid(); });
            addBtn(gameContext, "Elemental", []() {}, false);
            addBackBtn(gameContext, "Back", [gameContext]() { gameContext->isPhoneMenuOpen = false; gameContext->refreshActionGrid(); });
        }
        else
        {
            if (gameContext->map)
            {
                MapWarp warp;
                if (gameContext->map->checkWarp(gameContext->gridX, gameContext->gridY, warp))
                {
                    addBtn(gameContext, std::format("Enter {}", warp.targetMap.empty() ? "Door" : warp.targetMap), [gameContext, warp]() {
                        gameContext->loadMap(warp.targetMap, warp.targetX, warp.targetY);
                    });
                }

                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                if (tileData.persistentNPC)
                {
                    addBtn(gameContext, std::format("Talk to {}", tileData.persistentNPC->name), [gameContext, npc = tileData.persistentNPC]() {
                        gameContext->triggerEncounter(npc);
                    });
                }

                if (!tileData.droppedItems.empty())
                {
                    addBtn(gameContext, std::format("Examine Ground ({} items)", tileData.droppedItems.size()), [gameContext]() {
                        gameContext->changeState(std::make_unique<inventoryState>());
                    });
                }
            }

            addBtn(gameContext, "Inventory (I)", [gameContext]() { gameContext->changeState(std::make_unique<inventoryState>()); });
            addBtn(gameContext, "Visit Shop (K)", [gameContext]() { gameContext->changeState(std::make_unique<shopState>()); });
            addBtn(gameContext, "Mutations & TF (M)", [gameContext]() { gameContext->changeState(std::make_unique<transformationState>()); });
            addBtn(gameContext, "Test Combat (C)", [gameContext]() {
                std::vector<std::shared_ptr<entity>> pParty = { gameContext->playerEntity };
                std::vector<std::shared_ptr<entity>> eParty;
                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                if (tileData.persistentNPC) eParty.push_back(tileData.persistentNPC);
                else if (gameContext->activeTargetNPC) eParty.push_back(gameContext->activeTargetNPC);
                else eParty.push_back(encounterResolver::createEncounterNPC(1, gameContext->settings));
                gameContext->changeState(std::make_unique<CombatState>(pParty, eParty));
            });
            addBtn(gameContext, "Wait / Rest (1 hr)", [gameContext]() { gameContext->gameTime.advanceTime(60); gameContext->refreshActionGrid(); });
            addBtn(gameContext, "Phone (Menu)", [gameContext]() { gameContext->isPhoneMenuOpen = true; gameContext->refreshActionGrid(); });
            addBtn(gameContext, "QuickSave (F5)", [gameContext]() { saveManager::saveNamedGame(gameContext, "QuickSave"); });
            addBtn(gameContext, "QuickLoad (F9)", [gameContext]() {
                entity* p = gameContext->getPlayer();
                std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
                saveManager::loadFromFile(gameContext, charName + "_QuickSave.json");
                gameContext->refreshActionGrid();
            });
        }
    }
}