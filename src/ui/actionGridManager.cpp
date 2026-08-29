#include "ui/actionGridManager.h"

#include <format>
#include <memory>
#include <unordered_set>

#include "core/game.h"
#include "map/encounterResolver.h"
#include "save/saveManager.h"
#include "settings/settingsManager.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/eventState.h"
#include "state/explorationState.h"
#include "state/inventoryState.h"
#include "state/loadGameState.h"
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/sexState.h"
#include "state/shopState.h"
#include "state/transformationState.h"
#include "ui/theme.h"

void ActionGridManager::refresh(game* gameContext)
{
    if (!gameContext) return;

    iGameState* currentState = gameContext->getActiveState();
    if (!currentState) return;

    // Clear previous buttons to prevent duplication across frames/refreshes
    gameContext->activeButtons.clear();

    // Main Menu State Actions
    if (auto menu = dynamic_cast<mainMenuState*>(currentState))
    {
        // Slot 0 (1): New Game
        actionButton newGameBtn;
        newGameBtn.label = "New Game";
        newGameBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::START_NEW_GAME, 0, 0, "" });
        };
        gameContext->activeButtons.push_back(newGameBtn);

        // Slot 1 (2): Save/Load
        actionButton saveLoadBtn;
        saveLoadBtn.label = "Save/Load";
        saveLoadBtn.onClick = [gameContext]() {
            gameContext->changeState(std::make_unique<loadGameState>(SaveMenuMode::LOAD_ONLY));
        };
        gameContext->activeButtons.push_back(saveLoadBtn);

        // Slot 2 (3): blank
        actionButton blank3;
        blank3.label = "";
        blank3.isEnabled = false;
        gameContext->activeButtons.push_back(blank3);

        // Slot 3 (4): blank
        actionButton blank4;
        blank4.label = "";
        blank4.isEnabled = false;
        gameContext->activeButtons.push_back(blank4);

        // Slot 4 (5): Quit
        actionButton quitBtn;
        quitBtn.label = "Quit";
        quitBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::QUIT_GAME, 0, 0, "" });
        };
        gameContext->activeButtons.push_back(quitBtn);

        // Slot 5 (SHIFT + 1): Options
        actionButton optBtn;
        optBtn.label = "Options";
        optBtn.onClick = [gameContext]() {
            gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS));
        };
        gameContext->activeButtons.push_back(optBtn);

        // Slot 6 (SHIFT + 2): Content Options
        actionButton contentOptBtn;
        contentOptBtn.label = "Content Options";
        contentOptBtn.onClick = [gameContext]() {
            gameContext->changeState(std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS));
        };
        gameContext->activeButtons.push_back(contentOptBtn);

        while (gameContext->activeButtons.size() < 14)
        {
            actionButton blank;
            blank.label = "";
            blank.isEnabled = false;
            gameContext->activeButtons.push_back(blank);
        }

        // Slot 14 (CTRL + 5): Resume (if game in progress)
        actionButton resumeBtn;
        resumeBtn.label = "Resume";
        resumeBtn.isEnabled = (gameContext->getPlayer() != nullptr);
        resumeBtn.onClick = [gameContext]() {
            if (gameContext->getPlayer())
            {
                gameContext->changeState(std::make_unique<explorationState>());
            }
        };
        gameContext->activeButtons.push_back(resumeBtn);
        return;
    }

    // Load / Save Game State Actions
    if (auto loadState = dynamic_cast<loadGameState*>(currentState))
    {
        // Slot 0 (1): Confirmations: ON
        actionButton confBtn;
        confBtn.label = "Confirmations: ON";
        confBtn.onClick = [gameContext]() {};
        gameContext->activeButtons.push_back(confBtn);

        // Slot 1 (2): Sort: Date
        actionButton sortDateBtn;
        sortDateBtn.label = "Sort: Date";
        sortDateBtn.onClick = [gameContext]() {};
        gameContext->activeButtons.push_back(sortDateBtn);

        // Slot 2 (3): Sort: Name
        actionButton sortNameBtn;
        sortNameBtn.label = "Sort: Name";
        sortNameBtn.onClick = [gameContext]() {};
        gameContext->activeButtons.push_back(sortNameBtn);

        while (gameContext->activeButtons.size() < 14)
        {
            actionButton blank;
            blank.label = "";
            blank.isEnabled = false;
            gameContext->activeButtons.push_back(blank);
        }

        actionButton backBtn;
        backBtn.label = "Back";
        backBtn.onClick = [gameContext, loadState]() {
            loadState->goBack(gameContext);
        };
        gameContext->activeButtons.push_back(backBtn);
        return;
    }

    // Options / Content Options State Actions
    if (auto opt = dynamic_cast<optionsState*>(currentState))
    {
        if (opt->screenMode == OptionsScreenMode::GENERAL_OPTIONS)
        {
            // Slot 0 (1): Keybinds
            actionButton kbBtn;
            kbBtn.label = "Keybinds";
            kbBtn.onClick = [gameContext]() {};
            gameContext->activeButtons.push_back(kbBtn);

            // Slot 1 (2): Theme toggle
            std::string curTheme = gameContext->settings.display.activeTheme;
            std::string themeLabel = "Dark Fantasy";
            if (curTheme == "theme_cyber_neon" || curTheme == "cyber_neon") themeLabel = "Cyber Neon";
            else if (curTheme == "theme_parchment" || curTheme == "parchment") themeLabel = "Arcane Parchment";
            else if (curTheme == "default" || curTheme == "theme.json" || curTheme.empty()) themeLabel = "Lilith Midnight";

            actionButton themeBtn;
            themeBtn.label = "Theme: " + themeLabel;
            themeBtn.onClick = [gameContext]() {
                std::string cur = gameContext->settings.display.activeTheme;
                if (cur == "theme_dark_fantasy" || cur == "dark_fantasy") gameContext->settings.display.activeTheme = "theme_cyber_neon";
                else if (cur == "theme_cyber_neon" || cur == "cyber_neon") gameContext->settings.display.activeTheme = "theme_parchment";
                else if (cur == "theme_parchment" || cur == "parchment") gameContext->settings.display.activeTheme = "default";
                else gameContext->settings.display.activeTheme = "theme_dark_fantasy";

                Theme::applyTheme(gameContext->settings.display.activeTheme);
                settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(themeBtn);

            // Slot 2 (3): Font-size -
            actionButton fontMinusBtn;
            fontMinusBtn.label = "Font-size -";
            fontMinusBtn.onClick = [opt, gameContext]() {
                if (opt->fontSize > 12) opt->fontSize -= 2;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(fontMinusBtn);

            // Slot 3 (4): Font-size +
            actionButton fontPlusBtn;
            fontPlusBtn.label = "Font-size +";
            fontPlusBtn.onClick = [opt, gameContext]() {
                if (opt->fontSize < 36) opt->fontSize += 2;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(fontPlusBtn);

            // Slot 4 (5): Fade-in: OFF/ON
            actionButton fadeBtn;
            fadeBtn.label = opt->fadeInEnabled ? "Fade-in: ON" : "Fade-in: OFF";
            fadeBtn.onClick = [opt, gameContext]() {
                opt->fadeInEnabled = !opt->fadeInEnabled;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(fadeBtn);

            // Slot 5 (SHIFT + 1): Gender pronouns
            actionButton pronounBtn;
            pronounBtn.label = "Gender pronouns";
            pronounBtn.onClick = [opt, gameContext]() {
                opt->genderPronounMode = (opt->genderPronounMode == "Normal") ? "Custom" : "Normal";
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(pronounBtn);

            // Slot 6 (SHIFT + 2): Unit preferences
            actionButton unitBtn;
            unitBtn.label = "Unit preferences";
            unitBtn.onClick = [opt, gameContext]() {
                opt->unitPreference = (opt->unitPreference == "Metric") ? "Imperial" : "Metric";
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(unitBtn);

            // Slot 7 (SHIFT + 3): Difficulty: Human
            static const char* diffNames[] = { "Human", "Morph", "Demon", "Lilin", "Lilith" };
            actionButton diffBtn;
            diffBtn.label = std::string("Difficulty: ") + diffNames[opt->difficultyLevel];
            diffBtn.onClick = [opt, gameContext]() {
                opt->difficultyLevel = (opt->difficultyLevel + 1) % 5;
                if (opt->difficultyLevel == 0) gameContext->settings.gameplay.difficultyMultiplier = 1.0f;
                else if (opt->difficultyLevel == 1) gameContext->settings.gameplay.difficultyMultiplier = 1.25f;
                else if (opt->difficultyLevel == 2) gameContext->settings.gameplay.difficultyMultiplier = 2.0f;
                else if (opt->difficultyLevel == 3) gameContext->settings.gameplay.difficultyMultiplier = 2.5f;
                else gameContext->settings.gameplay.difficultyMultiplier = 4.0f;

                settingsManager::saveToFile(gameContext->settings, "data/settings.json");
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(diffBtn);

            while (gameContext->activeButtons.size() < 14)
            {
                actionButton blank;
                blank.label = "";
                blank.isEnabled = false;
                gameContext->activeButtons.push_back(blank);
            }

            actionButton backBtn;
            backBtn.label = "Back";
            backBtn.onClick = [gameContext, opt]() {
                opt->goBack(gameContext);
            };
            gameContext->activeButtons.push_back(backBtn);
            return;
        }
        else // CONTENT_OPTIONS
        {
            // Row 1: Tabs
            actionButton miscTab;
            miscTab.label = "Misc.";
            miscTab.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::MISC;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(miscTab);

            actionButton gameTab;
            gameTab.label = "Gameplay";
            gameTab.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::GAMEPLAY;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(gameTab);

            actionButton sexTab;
            sexTab.label = "Sex & Fetishes";
            sexTab.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::SEX_AND_FETISHES;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(sexTab);

            actionButton bodyTab;
            bodyTab.label = "Bodies";
            bodyTab.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::BODIES;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(bodyTab);

            actionButton resetBtn;
            resetBtn.label = "Reset";
            resetBtn.onClick = [opt, gameContext]() {
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(resetBtn);

            // Row 2: Sub-preferences
            actionButton genPrefBtn;
            genPrefBtn.label = "Gender preferences";
            genPrefBtn.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::GENDER_PREFS;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(genPrefBtn);

            actionButton oriPrefBtn;
            oriPrefBtn.label = "Orientation preferences";
            oriPrefBtn.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::ORIENTATION_PREFS;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(oriPrefBtn);

            actionButton agePrefBtn;
            agePrefBtn.label = "Age preferences";
            agePrefBtn.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::AGE_PREFS;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(agePrefBtn);

            actionButton furryPrefBtn;
            furryPrefBtn.label = "Furry preferences";
            furryPrefBtn.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::FURRY_PREFS;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(furryPrefBtn);

            actionButton fetPrefBtn;
            fetPrefBtn.label = "Fetish preferences";
            fetPrefBtn.onClick = [opt, gameContext]() {
                opt->contentCategory = ContentOptionsCategory::FETISH_PREFS;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(fetPrefBtn);

            while (gameContext->activeButtons.size() < 14)
            {
                actionButton blank;
                blank.label = "";
                blank.isEnabled = false;
                gameContext->activeButtons.push_back(blank);
            }

            actionButton backBtn;
            backBtn.label = "Back";
            backBtn.onClick = [gameContext, opt]() {
                opt->goBack(gameContext);
            };
            gameContext->activeButtons.push_back(backBtn);
            return;
        }
    }

    // Shop / Merchant State Actions
    if (auto shop = dynamic_cast<shopState*>(currentState))
    {
        const auto& catalog = shop->getCatalog();
        for (size_t i = 0; i < catalog.size(); ++i)
        {
            actionButton buyBtn;
            buyBtn.label = std::format("Buy {} ({}¤)", catalog[i].name, catalog[i].price);
            buyBtn.isEnabled = (catalog[i].stock > 0);
            buyBtn.onClick = [gameContext, i]() {
                gameContext->handleCommand({ CommandType::BUY_SHOP_ITEM, static_cast<int>(i), 0, "" });
            };
            gameContext->activeButtons.push_back(buyBtn);
        }

        actionButton closeBtn;
        closeBtn.label = "Leave Shop (ESC)";
        closeBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
        };
        gameContext->activeButtons.push_back(closeBtn);
        return;
    }

    // Transformation / Mutation State Actions
    if (auto tf = dynamic_cast<transformationState*>(currentState))
    {
        actionButton closeBtn;
        closeBtn.label = "Close (ESC)";
        closeBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::CLOSE_MENU, 0, 0, "" });
        };
        gameContext->activeButtons.push_back(closeBtn);
        return;
    }

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
        winBtn.label = "Victory (Simulate Win)";
        winBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "WIN" });
        };
        gameContext->activeButtons.push_back(winBtn);

        actionButton lossBtn;
        lossBtn.label = "Defeat (Simulate Loss)";
        lossBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "DEFEAT" });
        };
        gameContext->activeButtons.push_back(lossBtn);

        actionButton escBtn;
        escBtn.label = "Escape (Flee Combat)";
        escBtn.onClick = [gameContext]() {
            gameContext->handleCommand({ CommandType::EXECUTE_COMBAT_ACTION, 0, 0, "ESCAPE" });
        };
        gameContext->activeButtons.push_back(escBtn);

        actionButton strikeBtn;
        strikeBtn.label = "Strike (Attack)";
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
        return;
    }

    // 4. Scene Event Choices (if in eventState)
    if (dynamic_cast<eventState*>(currentState))
    {
        const auto& scene = gameContext->getCurrentScene();
        for (size_t i = 0; i < scene.choices.size(); ++i)
        {
            if (gameContext->checkConditions(scene.choices[i].requirements))
            {
                actionButton choiceBtn;
                choiceBtn.label = scene.choices[i].label;
                dialogueChoice choice = scene.choices[i];
                choiceBtn.onClick = [gameContext, choice]() {
                    gameContext->processChoice(choice);
                };
                gameContext->activeButtons.push_back(choiceBtn);
            }
        }
        return;
    }

    // 5. Exploration Movement & Interaction Shortcuts
    if (dynamic_cast<explorationState*>(currentState))
    {
        if (gameContext->isPhoneMenuOpen)
        {
            actionButton spellsBtn;
            spellsBtn.label = "Spells & Moves";
            spellsBtn.onClick = [gameContext]() {};
            gameContext->activeButtons.push_back(spellsBtn);

            actionButton tfBtn;
            tfBtn.label = "Transformations (M)";
            tfBtn.onClick = [gameContext]() {
                gameContext->changeState(std::make_unique<transformationState>());
            };
            gameContext->activeButtons.push_back(tfBtn);

            actionButton fetishBtn;
            fetishBtn.label = "Fetishes & Perks";
            fetishBtn.onClick = [gameContext]() {};
            gameContext->activeButtons.push_back(fetishBtn);

            actionButton encBtn;
            encBtn.label = "Encyclopedia";
            encBtn.onClick = [gameContext]() {};
            gameContext->activeButtons.push_back(encBtn);

            actionButton questBtn;
            questBtn.label = "Quest Journal";
            questBtn.onClick = [gameContext]() {};
            gameContext->activeButtons.push_back(questBtn);

            actionButton saveMenuBtn;
            saveMenuBtn.label = "Save / Load Menu";
            saveMenuBtn.onClick = [gameContext]() {
                gameContext->changeState(std::make_unique<loadGameState>(SaveMenuMode::SAVE_AND_LOAD, std::make_unique<explorationState>()));
            };
            gameContext->activeButtons.push_back(saveMenuBtn);

            actionButton saveBtn;
            saveBtn.label = "QuickSave (F5)";
            saveBtn.onClick = [gameContext]() {
                saveManager::saveNamedGame(gameContext, "QuickSave");
            };
            gameContext->activeButtons.push_back(saveBtn);

            actionButton loadBtn;
            loadBtn.label = "QuickLoad (F9)";
            loadBtn.onClick = [gameContext]() {
                saveManager::loadFromFile(gameContext, "QuickSave.json");
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(loadBtn);

            // Pad up to slot 14
            while (gameContext->activeButtons.size() < 14)
            {
                actionButton blank;
                blank.label = "";
                blank.isEnabled = false;
                gameContext->activeButtons.push_back(blank);
            }

            actionButton backBtn;
            backBtn.label = "< Back (Phone)";
            backBtn.onClick = [gameContext]() {
                gameContext->isPhoneMenuOpen = false;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(backBtn);
        }
        else
        {
            if (gameContext->map)
            {
                MapWarp warp;
                if (gameContext->map->checkWarp(gameContext->gridX, gameContext->gridY, warp))
                {
                    actionButton warpBtn;
                    warpBtn.label = std::format("Enter {}", warp.targetMap.empty() ? "Door" : warp.targetMap);
                    warpBtn.onClick = [gameContext, warp]() {
                        gameContext->loadMap(warp.targetMap, warp.targetX, warp.targetY);
                    };
                    gameContext->activeButtons.push_back(warpBtn);
                }

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

            actionButton invBtn;
            invBtn.label = "Inventory (I)";
            invBtn.onClick = [gameContext]() {
                gameContext->changeState(std::make_unique<inventoryState>());
            };
            gameContext->activeButtons.push_back(invBtn);

            actionButton shopBtn;
            shopBtn.label = "Visit Shop (K)";
            shopBtn.onClick = [gameContext]() {
                gameContext->changeState(std::make_unique<shopState>());
            };
            gameContext->activeButtons.push_back(shopBtn);

            actionButton tfBtn;
            tfBtn.label = "Mutations & TF (M)";
            tfBtn.onClick = [gameContext]() {
                gameContext->changeState(std::make_unique<transformationState>());
            };
            gameContext->activeButtons.push_back(tfBtn);

            actionButton combatBtn;
            combatBtn.label = "Test Combat (C)";
            combatBtn.onClick = [gameContext]() {
                std::vector<std::shared_ptr<entity>> pParty = { gameContext->playerEntity };
                std::vector<std::shared_ptr<entity>> eParty;
                auto& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                if (tileData.persistentNPC) eParty.push_back(tileData.persistentNPC);
                else if (gameContext->activeTargetNPC) eParty.push_back(gameContext->activeTargetNPC);
                else eParty.push_back(encounterResolver::createEncounterNPC(1, gameContext->settings));
                gameContext->changeState(std::make_unique<CombatState>(pParty, eParty));
            };
            gameContext->activeButtons.push_back(combatBtn);

            actionButton waitBtn;
            waitBtn.label = "Wait / Rest (1 hr)";
            waitBtn.onClick = [gameContext]() {
                gameContext->gameTime.advanceTime(60);
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(waitBtn);

            actionButton phoneBtn;
            phoneBtn.label = "Phone (Menu)";
            phoneBtn.onClick = [gameContext]() {
                gameContext->isPhoneMenuOpen = true;
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(phoneBtn);

            actionButton saveBtn;
            saveBtn.label = "QuickSave (F5)";
            saveBtn.onClick = [gameContext]() {
                saveManager::saveNamedGame(gameContext, "QuickSave");
            };
            gameContext->activeButtons.push_back(saveBtn);

            actionButton loadBtn;
            loadBtn.label = "QuickLoad (F9)";
            loadBtn.onClick = [gameContext]() {
                entity* p = gameContext->getPlayer();
                std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
                saveManager::loadFromFile(gameContext, charName + "_QuickSave.json");
                gameContext->refreshActionGrid();
            };
            gameContext->activeButtons.push_back(loadBtn);
        }
    }
}