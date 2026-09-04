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
#include "quest/questDatabase.h"
#include "items/merchantValuation.h"
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

        // Row 2: Wardrobe Dressing Room Launcher if on Wardrobe tab
        if (cc->getCurrentTabId() == EditorTabId::WARDROBE)
        {
            padButtonsTo(gameContext, 5);
            addBtn(gameContext, "Open Clothing (I)", [gameContext]() {
                gameContext->handleCommand(UICommand{ CommandType::OPEN_INVENTORY });
            }, true, false, "Open full dual inventory and dressing room to equip garments and inspect items.");
        }

        // Row 2: Final Action (Shift + 5 / Slot 9)
        if (cc->step == tabCount - 1)
        {
            padButtonsTo(gameContext, 9);
            std::string finishBtnLabel = cc->config.isNewGameCreation ? "Start Game" : "Apply Changes";
            addBtn(gameContext, finishBtnLabel, [cc, gameContext]() {
                cc->finalizeCharacter(gameContext);
            }, true, true);
        }

        // Row 3: Return to Menu (Ctrl + 5 / Slot 14)
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
        entity* player = gameContext->getPlayer();
        entity* merchant = shop->getMerchant().get();
        if (!merchant) merchant = gameContext->getActiveTargetNPC();

        if (gameContext->selectedInventoryIndex != -1)
        {
            auto slotInfo = gameContext->getInventorySlotItem(gameContext->selectedInventorySide, gameContext->selectedInventoryIndex);

            if (gameContext->selectedInventorySide == 0)
            {
                // Player's item selected -> Selling
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
                // Merchant's item selected -> Buying
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
            // Nothing selected: guidance button
            addBtn(gameContext, "Select Item to Trade", [gameContext]() {}, false, false, "Click any item in your inventory to sell, or in the shop inventory to buy.");
        }

        // Pad to slot 14 and place Leave Shop
        padButtonsTo(gameContext, 14);
        addBackBtn(gameContext, "Leave Shop", [gameContext]() {
            gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
        });
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

    // 8. Phone Apps & Submenus
    if (auto phoneApp = dynamic_cast<phoneAppsState*>(currentState))
    {
        auto setApp = [gameContext, phoneApp](PhoneAppMode mode) {
            phoneApp->setAppMode(mode);
            phoneApp->loadData(mode);
            gameContext->refreshActionGrid();
        };

        if (phoneApp->getAppMode() == PhoneAppMode::HOME)
        {
            // Row 1: Core Character Apps (Slots 0..4)
            addBtn(gameContext, "Quests", [=]() { setApp(PhoneAppMode::QUESTS); }, true, false, "View active quest objectives, stages, and rewards.");
            addBtn(gameContext, "Perk Tree", [=]() { setApp(PhoneAppMode::PERKS); }, true, false, "Browse and allocate talent perks and passive traits.");
            addBtn(gameContext, "Spells", [=]() { setApp(PhoneAppMode::SPELLS); }, true, false, "Review elemental spellbooks and cast out-of-combat spells.");
            addBtn(gameContext, "Fetishes", [=]() { setApp(PhoneAppMode::FETISHES); }, true, false, "Configure 5-tier desire ratings and sexual preferences.");
            addBtn(gameContext, "Stats", [=]() { setApp(PhoneAppMode::STATS); }, true, false, "Inspect core vitals, body measurements, and sex stats.");

            // Row 2: Secondary Utilities & Lore (Slots 5..9)
            addBtn(gameContext, "Selfie", [=]() { setApp(PhoneAppMode::SELFIE); }, true, false, "Take a selfie to inspect appearance, clothing, and modesty.");
            addBtn(gameContext, "Contacts", [=]() { setApp(PhoneAppMode::CONTACTS); }, true, false, "Address book of known characters and companions.");
            addBtn(gameContext, "Encyclopedia", [=]() { setApp(PhoneAppMode::ENCYCLOPEDIA); }, true, false, "Compendium of discovered species, weapons, and items.");
            addBtn(gameContext, "Transform", [=]() { setApp(PhoneAppMode::TRANSFORM); }, true, false, "Inspect mutations, bodily morphs, and alchemy.");
            addBtn(gameContext, "Maps", [=]() { setApp(PhoneAppMode::MAPS); }, true, false, "World and local regional maps with coordinates.");

            // Row 3: Combat, Intimacy & Rest (Slots 10..14)
            addBtn(gameContext, "Combat Moves", [=]() { setApp(PhoneAppMode::COMBAT_MOVES); }, true, false, "Prepare and customize active combat deck slots.");
            addBtn(gameContext, "Masturbate", [=]() { setApp(PhoneAppMode::MASTURBATE); }, true, false, "Take a secluded moment to relieve lust and build arousal.");
            addBtn(gameContext, "Wait / Rest", [=]() { setApp(PhoneAppMode::WAIT_REST); }, true, false, "Pass in-game time, sleep, and recover health/mana.");
            addBtn(gameContext, "Elemental", [=]() { setApp(PhoneAppMode::ELEMENTAL); }, true, false, "Manage and interact with your elemental companion.");
            addBtn(gameContext, "Back", [gameContext]() {
                gameContext->changeState(std::make_unique<explorationState>());
            }, true, false, "Close smartphone and return to exploration.");
            return;
        }

        // --- Submenu Modes ---
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
        else if (phoneApp->getAppMode() == PhoneAppMode::FETISHES)
        {
            addBtn(gameContext, "Options Menu", [gameContext]() {
                gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS, std::make_unique<phoneAppsState>(PhoneAppMode::FETISHES)));
            }, true, false, "Open complete content and demographic preference settings.");
        }
        else if (phoneApp->getAppMode() == PhoneAppMode::PERKS)
        {
            padButtonsTo(gameContext, 13);
            addBtn(gameContext, "Reset Perks", [gameContext, phoneApp]() {
                entity* p = gameContext->getPlayer();
                if (p)
                {
                    p->stats.setBaseStat("perk_points", p->getStat("perk_points") + 3.0f);
                    phoneApp->setFeedbackText("All spent perk points refunded.");
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
                if (move.size() > 10) move = move.substr(0, 9) + "..";
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

            // Slot 10 (Row 3, Col 1): Clear Slot
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

        // Slot 14: Universal Back button in all submenus
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
            hasGroundItems = !td.droppedItems.empty();
        }

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

            addBtn(gameContext, "Reset All Fits", [gameContext]() {
                gameContext->handleResetAllDisplacementsAction();
            }, hasDisplacement, false, "Restore all displaced garments to properly worn placement.");

            addBtn(gameContext, "Deselect Slot", [gameContext]() {
                gameContext->selectedEquipmentSlot = equipSlot::NONE;
                gameContext->refreshActionGrid();
            }, true, false, "Clear slot selection and return to general inventory actions.");
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
                    }

                    if (slotInfo.itemPtr->isConsumable)
                    {
                        std::string desc = "Consume " + slotInfo.itemPtr->name + ". " + (!slotInfo.itemPtr->tooltip.empty() ? slotInfo.itemPtr->tooltip : slotInfo.itemPtr->description);
                        addBtn(gameContext, "Use", [gameContext]() {
                            gameContext->handleUseItemAction(gameContext->selectedInventoryIndex);
                        }, true, false, desc);
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
                addBtn(gameContext, take1, [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 1); }, slotInfo.isValid, false, "Take 1 item into your backpack.");
                addBtn(gameContext, takeAll, [gameContext]() { gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 999); }, slotInfo.isValid, false, "Take entire stack into your backpack.");

                addBtn(gameContext, "Loot All", [gameContext]() {
                    gameContext->handleLootAllAction();
                }, hasGroundItems, false, "Pick up all items from the ground into your backpack.");

                if (slotInfo.isValid && slotInfo.itemPtr && slotInfo.itemPtr->isEquippable)
                {
                    auto validSlots = slotInfo.itemPtr->getValidEquipSlots();
                    for (equipSlot s : validSlots)
                    {
                        std::string slotName = gameContext->formatEquipSlotName(s);
                        std::string btnLabel = std::format("Equip ({})", slotName);
                        std::string desc = std::format("Pick up and equip {} to {}.", slotInfo.itemPtr->name, slotName);
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
            // Global Inventory Actions (nothing selected)
            addBtn(gameContext, "Loot All", [gameContext]() {
                gameContext->handleLootAllAction();
            }, hasGroundItems, false, hasGroundItems ? "Transfer all items from the ground into your backpack." : "No items on the ground to loot.");

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
        std::string backLabel = isSubmenu ? "Back to Creation" : "Close (I / ESC)";
        std::string backDesc = isSubmenu ? "Return to Character Creation." : "Return to exploration view.";
        addBackBtn(gameContext, backLabel, [gameContext, inv]() {
            if (inv) inv->goBack(gameContext);
            else gameContext->changeState(std::make_unique<explorationState>());
        }, backDesc);
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
        // Row 1: Context Actions on Tile (Warps, NPCs, Map & Quest Triggers, Ground Loot, Merchants)
        if (gameContext->map)
            {
                MapWarp warp;
                if (gameContext->map->checkWarp(gameContext->gridX, gameContext->gridY, warp))
                {
                    std::string warpLabel = "Enter Area";
                    if (warp.targetMap == "house_01") warpLabel = "Enter Cottage";
                    else if (warp.targetMap == "overworld") warpLabel = "Enter Town";
                    else if (!warp.targetMap.empty()) warpLabel = "Enter " + warp.targetMap;

                    addBtn(gameContext, warpLabel, [gameContext, warp]() {
                        gameContext->loadMap(warp.targetMap, warp.targetX, warp.targetY);
                    }, true, false, "Transition into " + (warp.targetMap.empty() ? "adjacent area" : warp.targetMap) + ".");
                }

                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                if (tileData.persistentNPC)
                {
                    addBtn(gameContext, std::format("Talk to {}", tileData.persistentNPC->name), [gameContext, npc = tileData.persistentNPC]() {
                        gameContext->triggerEncounter(npc);
                    }, true, false, "Initiate dialogue with " + tileData.persistentNPC->name + ".");

                    if (tileData.persistentNPC->name.find("Merchant") != std::string::npos || tileData.persistentNPC->name.find("Shop") != std::string::npos)
                    {
                        addBtn(gameContext, "Visit Shop", [gameContext]() {
                            gameContext->changeState(std::make_unique<shopState>());
                        }, true, false, "Browse merchant inventory and trade goods.");
                    }
                }

                // Map & Quest Triggers on this tile (deduplicated by id)
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
            }
        }
    }