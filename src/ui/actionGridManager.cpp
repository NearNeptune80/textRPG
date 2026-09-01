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

    inline void addBtn(game* g, std::string_view label, std::function<void()> onClick, bool enabled = true, bool selected = false, std::string_view description = "")
    {
        actionButton btn;
        btn.label = std::string(label);
        btn.description = std::string(description);
        btn.isEnabled = enabled;
        btn.isSelected = selected;
        btn.onClick = std::move(onClick);
        g->activeButtons.push_back(btn);
    }

    inline void addBackBtn(game* g, std::string_view label, std::function<void()> onClick, std::string_view description = "")
    {
        padButtonsTo(g, 14);
        actionButton btn;
        btn.label = std::string(label);
        btn.description = std::string(description);
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

        // Row 1: Dynamic Category Step Tabs (up to 5 tabs per row page)
        if (tabCount > 1)
        {
            int startIdx = (cc->step >= 5 && tabCount > 5) ? 5 : 0;
            for (int i = startIdx; i < std::min(startIdx + 5, tabCount); ++i)
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

        // Row 2: Step Navigation & Finish Action (Shift + 1..5)
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

        // Only place finish button in Slot 9 (Shift + 5) when on the final step
        if (cc->step == tabCount - 1)
        {
            padButtonsTo(gameContext, 9);
            std::string finishBtnLabel = cc->config.isNewGameCreation ? "Start Game" : "Apply Changes";
            addBtn(gameContext, finishBtnLabel, [cc, gameContext]() {
                cc->finalizeCharacter(gameContext);
            }, true, true);
        }

        // Row 3: Utilities, Skip Prologue & Return (Ctrl + 1..5)
        padButtonsTo(gameContext, 10);
        addBtn(gameContext, "Randomize All", [cc, gameContext]() {
            cc->randomizeAll();
            gameContext->refreshActionGrid();
        });

        if (cc->config.isNewGameCreation && cc->step == tabCount - 1)
        {
            padButtonsTo(gameContext, 11);
            addBtn(gameContext, "Skip Prologue", [cc, gameContext]() {
                cc->skipPrologue = true;
                cc->finalizeCharacter(gameContext);
            });
        }

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
    if (auto tf = dynamic_cast<transformationState*>(currentState))
    {
        // Row 1: Core Physical Categories (1..5)
        addBtn(gameContext, "1. Core", [gameContext, tf]() { tf->setTab(TransformationTab::CORE, gameContext); }, true, tf->currentTab == TransformationTab::CORE);
        addBtn(gameContext, "2. Eyes", [gameContext, tf]() { tf->setTab(TransformationTab::EYES, gameContext); }, true, tf->currentTab == TransformationTab::EYES);
        addBtn(gameContext, "3. Hair", [gameContext, tf]() { tf->setTab(TransformationTab::HAIR, gameContext); }, true, tf->currentTab == TransformationTab::HAIR);
        addBtn(gameContext, "4. Head & Face", [gameContext, tf]() { tf->setTab(TransformationTab::HEAD_FACE, gameContext); }, true, tf->currentTab == TransformationTab::HEAD_FACE);
        addBtn(gameContext, "5. Ass & Hips", [gameContext, tf]() { tf->setTab(TransformationTab::ASS_HIPS, gameContext); }, true, tf->currentTab == TransformationTab::ASS_HIPS);

        // Row 2: Secondary & Genital Categories (Shift + 1..5)
        addBtn(gameContext, "6. Breasts", [gameContext, tf]() { tf->setTab(TransformationTab::BREASTS, gameContext); }, true, tf->currentTab == TransformationTab::BREASTS);
        addBtn(gameContext, "7. Vagina", [gameContext, tf]() { tf->setTab(TransformationTab::VAGINA, gameContext); }, true, tf->currentTab == TransformationTab::VAGINA);
        addBtn(gameContext, "8. Penis", [gameContext, tf]() { tf->setTab(TransformationTab::PENIS, gameContext); }, true, tf->currentTab == TransformationTab::PENIS);
        addBtn(gameContext, "9. Crotch", [gameContext, tf]() { tf->setTab(TransformationTab::CROTCH_BOOBS, gameContext); }, true, tf->currentTab == TransformationTab::CROTCH_BOOBS);
        addBtn(gameContext, "10. Appendages", [gameContext, tf]() { tf->setTab(TransformationTab::APPENDAGES, gameContext); }, true, tf->currentTab == TransformationTab::APPENDAGES);

        // Row 3: Presets, Inspection & Global Tools (Ctrl + 1..5)
        addBtn(gameContext, "Inspect & Presets", [gameContext, tf]() { tf->setTab(TransformationTab::INSPECT_PRESETS, gameContext); }, true, tf->currentTab == TransformationTab::INSPECT_PRESETS);
        addBtn(gameContext, "Randomize Form", [gameContext, tf]() {
            tf->randomizeForm(gameContext);
            gameContext->refreshActionGrid();
        });
        addBtn(gameContext, "Reset Human", [gameContext, tf]() {
            tf->resetToHuman(gameContext);
            gameContext->refreshActionGrid();
        });
        addBtn(gameContext, "Save Preset", [gameContext, tf]() {
            if (auto p = gameContext->getPlayer()) tf->savePreset(gameContext, "Preset_" + p->anatomy.getRacialTitle());
            gameContext->refreshActionGrid();
        });
        addBackBtn(gameContext, "Apply & Return", [gameContext]() {
            gameContext->changeState(std::make_unique<explorationState>());
        });
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
        auto p = gameContext->getPlayer();
        if (gameContext->selectedEquipmentSlot != equipSlot::NONE)
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
        }
        else if (gameContext->selectedInventoryIndex != -1)
        {
            bool hasNpc = (gameContext->getActiveTargetNPC() != nullptr);
            auto slotInfo = gameContext->getInventorySlotItem(gameContext->selectedInventorySide, gameContext->selectedInventoryIndex);

            if (gameContext->selectedInventorySide == 0)
            {
                bool canEquipOrUse = false;
                std::string equipUseDesc = "Select an item to equip or consume.";
                std::string btnLabel = "Equip / Use";

                if (slotInfo.isValid && slotInfo.itemPtr)
                {
                    if (slotInfo.itemPtr->isEquippable)
                    {
                        btnLabel = "Equip";
                        canEquipOrUse = true;
                        equipUseDesc = "Equip " + slotInfo.itemPtr->name + " to " + gameContext->formatEquipSlotName(slotInfo.itemPtr->targetSlot) + ".";
                        if (!slotInfo.itemPtr->tooltip.empty()) equipUseDesc += " " + slotInfo.itemPtr->tooltip;
                        else if (!slotInfo.itemPtr->description.empty()) equipUseDesc += " " + slotInfo.itemPtr->description;
                    }
                    else if (slotInfo.itemPtr->isConsumable)
                    {
                        btnLabel = "Use";
                        canEquipOrUse = true;
                        equipUseDesc = "Consume " + slotInfo.itemPtr->name + ". " + (!slotInfo.itemPtr->tooltip.empty() ? slotInfo.itemPtr->tooltip : slotInfo.itemPtr->description);
                    }
                    else
                    {
                        equipUseDesc = "This item cannot be equipped or consumed.";
                    }
                }

                bool isConsumable = (slotInfo.isValid && slotInfo.itemPtr && slotInfo.itemPtr->isConsumable);
                addBtn(gameContext, btnLabel, [gameContext, isConsumable]() {
                    if (isConsumable) gameContext->handleUseItemAction(gameContext->selectedInventoryIndex);
                    else gameContext->handleEquipAction(gameContext->selectedInventoryIndex);
                }, canEquipOrUse, false, equipUseDesc);

                bool isKey = (slotInfo.isValid && slotInfo.itemPtr && slotInfo.itemPtr->isKeyItem);
                bool canDrop = slotInfo.isValid && !isKey;
                std::string give1 = hasNpc ? "Give 1" : "Drop 1";
                std::string giveAll = hasNpc ? "Give All" : "Drop All";
                std::string dropDesc = isKey ? "Key quest items cannot be dropped." : (hasNpc ? "Transfer 1 item to NPC." : "Drop 1 item onto the floor.");
                std::string dropAllDesc = isKey ? "Key quest items cannot be dropped." : (hasNpc ? "Transfer entire stack to NPC." : "Drop entire stack onto the floor.");

                addBtn(gameContext, give1, [gameContext]() { gameContext->handleDropAction(gameContext->selectedInventoryIndex, 1); }, canDrop, false, dropDesc);
                addBtn(gameContext, giveAll, [gameContext]() { gameContext->handleDropAction(gameContext->selectedInventoryIndex, 999); }, canDrop, false, dropAllDesc);
            }
            else if (gameContext->selectedInventorySide == 1)
            {
                std::string take1 = hasNpc ? "Take 1" : "Pickup 1";
                std::string takeAll = hasNpc ? "Take All" : "Pickup All";
                addBtn(gameContext, take1, [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 1); }, slotInfo.isValid, false, "Take 1 item into your backpack.");
                addBtn(gameContext, takeAll, [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 999); }, slotInfo.isValid, false, "Take entire stack into your backpack.");
            }
        }

        addBackBtn(gameContext, "Close (I / ESC)", [gameContext]() { gameContext->changeState(std::make_unique<explorationState>()); }, "Return to exploration view.");
        return;
    }

    // 11. Combat State Actions
    if (auto combat = dynamic_cast<CombatState*>(currentState))
    {
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

        // Row 1: Physical Attacks
        addBtn(gameContext, "Strike (1 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "STRIKE" }); }, true, false, "Basic physical strike dealing weapon damage.");
        addBtn(gameContext, "Heavy Strike (2 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "HEAVY_STRIKE" }); }, true, false, "Heavy blow dealing high physical damage.");
        addBtn(gameContext, "Defend (1 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "DEFEND" }); }, true, false, "Raise defense stance to reduce incoming damage.");
        addBtn(gameContext, "Disarm (2 AP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "DISARM" }); }, true, false, "Attempt to disarm opponent's equipped weapon.");
        addBtn(gameContext, "End Turn", [combat, gameContext]() { combat->handleEndTurn(gameContext); }, true, false, "Conclude turn and pass initiative to enemies.");

        // Row 2: Spells & Magic (greyed out if insufficient MP)
        bool canDart = (curMp >= 10.0f);
        addBtn(gameContext, "Arcane Dart (10 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_DART" }); }, canDart, false, canDart ? "Fires a sharp dart of concentrated arcane power." : "Insufficient Mana (Requires 10 MP).");

        bool canFireball = (curMp >= 25.0f);
        addBtn(gameContext, "Fireball (25 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_FIREBALL" }); }, canFireball, false, canFireball ? "Hurls an explosive sphere of demonic flame." : "Insufficient Mana (Requires 25 MP).");

        bool canShield = (curMp >= 15.0f);
        addBtn(gameContext, "Shield (15 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_SHIELD" }); }, canShield, false, canShield ? "Conjures a shimmering protective barrier." : "Insufficient Mana (Requires 15 MP).");

        bool canCleanse = (curMp >= 20.0f);
        addBtn(gameContext, "Cleanse (20 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_CLEANSE" }); }, canCleanse, false, canCleanse ? "Channels pure energy to purge negative debuffs." : "Insufficient Mana (Requires 20 MP).");

        bool canBlink = (curMp >= 30.0f);
        addBtn(gameContext, "Blink (30 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SPELL_BLINK" }); }, canBlink, false, canBlink ? "Teleport short distance to evade attacks." : "Insufficient Mana (Requires 30 MP).");

        // Row 3: Items & Utility
        bool hasPot = (potionCount > 0);
        addBtn(gameContext, "Potion (+50 HP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ITEM_POTION" }); }, hasPot, false, hasPot ? "Drink a healing potion to restore 50 Health." : "No healing potions in inventory.");
        addBtn(gameContext, "Mana (+50 MP)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ITEM_MANA" }); }, true, false, "Restore 50 Mana points.");
        addBtn(gameContext, "Surrender", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "SURRENDER" }); }, true, false, "Yield to enemy combatants and enter submission.");
        addBtn(gameContext, "Escape", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ESCAPE" }); }, true, false, "Flee from combat encounter.");
        addBtn(gameContext, "Victory (Skip)", [gameContext]() { gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "WIN" }); }, true, false, "Instantly defeat remaining enemies.");
        return;
    }

    // 12. Scene Event Choices (Data-Driven Tooltips & Greyed-Out when requirements unmet)
    if (dynamic_cast<eventState*>(currentState))
    {
        const auto& scene = gameContext->getCurrentScene();
        for (const auto& choice : scene.choices)
        {
            bool meetsReqs = gameContext->checkConditions(choice.requirements);
            std::string tooltipText = choice.tooltip;

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
                addBtn(gameContext, choice.label, nullptr, false, false, tooltipText);
            }
            else
            {
                if (tooltipText.empty())
                {
                    tooltipText = std::format("Select: {}", choice.label);
                }
                addBtn(gameContext, choice.label, [gameContext, choice]() { gameContext->processChoice(choice); }, true, false, tooltipText);
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
            addBtn(gameContext, "Transform", [gameContext]() { gameContext->changeState(std::make_unique<transformationState>()); });
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
                    }, true, false, "Transition into " + (warp.targetMap.empty() ? "adjacent area" : warp.targetMap) + ".");
                }

                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                if (tileData.persistentNPC)
                {
                    addBtn(gameContext, std::format("Talk to {}", tileData.persistentNPC->name), [gameContext, npc = tileData.persistentNPC]() {
                        gameContext->triggerEncounter(npc);
                    }, true, false, "Initiate dialogue with " + tileData.persistentNPC->name + ".");
                }

                if (!tileData.droppedItems.empty())
                {
                    addBtn(gameContext, std::format("Examine Ground ({} items)", tileData.droppedItems.size()), [gameContext]() {
                        gameContext->changeState(std::make_unique<inventoryState>());
                    }, true, false, "Inspect and pick up items lying on the ground.");
                }
            }

            addBtn(gameContext, "Inventory (I)", [gameContext]() { gameContext->changeState(std::make_unique<inventoryState>()); }, true, false, "Open dual 5x4 player inventory and ground storage.");
            addBtn(gameContext, "Visit Shop (K)", [gameContext]() { gameContext->changeState(std::make_unique<shopState>()); }, true, false, "Browse merchant catalog and purchase supplies.");
            addBtn(gameContext, "Mutations & TF (M)", [gameContext]() { gameContext->changeState(std::make_unique<transformationState>()); }, true, false, "Inspect anatomical changes and body transformations.");
            addBtn(gameContext, "Test Combat (C)", [gameContext]() {
                std::vector<std::shared_ptr<entity>> pParty = { gameContext->playerEntity };
                std::vector<std::shared_ptr<entity>> eParty;
                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                if (tileData.persistentNPC) eParty.push_back(tileData.persistentNPC);
                else if (gameContext->activeTargetNPC) eParty.push_back(gameContext->activeTargetNPC);
                else eParty.push_back(encounterResolver::createEncounterNPC(1, gameContext->settings));
                gameContext->changeState(std::make_unique<CombatState>(pParty, eParty));
            }, true, false, "Engage in tactical turn-based combat encounter.");
            addBtn(gameContext, "Wait / Rest (1 hr)", [gameContext]() { gameContext->gameTime.advanceTime(60); gameContext->refreshActionGrid(); }, true, false, "Pass 1 hour of in-game time.");
            addBtn(gameContext, "Phone (Menu)", [gameContext]() { gameContext->isPhoneMenuOpen = true; gameContext->refreshActionGrid(); }, true, false, "Access smartphone apps, contacts, and map.");
            addBtn(gameContext, "QuickSave (F5)", [gameContext]() { saveManager::saveNamedGame(gameContext, "QuickSave"); }, true, false, "Quick save your current progress.");
            addBtn(gameContext, "QuickLoad (F9)", [gameContext]() {
                entity* p = gameContext->getPlayer();
                std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
                saveManager::loadFromFile(gameContext, charName + "_QuickSave.json");
                gameContext->refreshActionGrid();
            }, true, false, "Quick load your last saved game state.");
        }
    }
}