#include "core/engineTests.h"

#include <iostream>
#include <memory>
#include <cassert>
#include <filesystem>

#include "core/game.h"
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/loadGameState.h"
#include "state/characterCreationState.h"
#include "state/explorationState.h"
#include "save/saveManager.h"
#include "items/inventory.h"
#include "items/itemDatabase.h"
#include "settings/settingsManager.h"
#include "ui/theme.h"

namespace EngineTests
{
    static int g_passCount = 0;
    static int g_failCount = 0;

    static void logResult(std::string_view testName, bool passed, std::string_view message = "")
    {
        if (passed)
        {
            std::cout << "  [PASS] " << testName;
            if (!message.empty()) std::cout << " (" << message << ")";
            std::cout << "\n";
            g_passCount++;
        }
        else
        {
            std::cout << "  [FAIL] " << testName;
            if (!message.empty()) std::cout << " - " << message;
            std::cout << "\n";
            g_failCount++;
        }
    }

    bool testStateNavigation()
    {
        std::cout << "\n--- Running Test 1: State Navigation & Action Grid ---\n";
        bool allPassed = true;

        game g;
        // 1. Initial State should be exploration or mainMenu
        g.changeState(std::make_unique<mainMenuState>());
        bool isMainMenu = (dynamic_cast<mainMenuState*>(g.getActiveState()) != nullptr);
        logResult("Initial State is MainMenu", isMainMenu);
        allPassed &= isMainMenu;

        // Check Action Grid for Main Menu
        g.refreshActionGrid();
        bool hasNewGame = (!g.activeButtons.empty() && g.activeButtons[0].label == "New Game");
        logResult("Main Menu has 'New Game' button at Slot 0", hasNewGame);
        allPassed &= hasNewGame;

        bool hasOptions = (g.activeButtons.size() > 5 && g.activeButtons[5].label == "Options");
        logResult("Main Menu has 'Options' button at Slot 5", hasOptions);
        allPassed &= hasOptions;

        // Navigate to Options with Return State
        g.changeState(std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS, std::make_unique<mainMenuState>()));
        bool isOptions = (dynamic_cast<optionsState*>(g.getActiveState()) != nullptr);
        logResult("Transition to OptionsState", isOptions);
        allPassed &= isOptions;

        // Go Back from Options
        auto* optState = dynamic_cast<optionsState*>(g.getActiveState());
        if (optState)
        {
            optState->goBack(&g);
            bool backToMainMenu = (dynamic_cast<mainMenuState*>(g.getActiveState()) != nullptr);
            logResult("OptionsState::goBack returns to MainMenuState", backToMainMenu);
            allPassed &= backToMainMenu;
        }

        // Navigate to Save/Load
        g.changeState(std::make_unique<loadGameState>(SaveMenuMode::LOAD_ONLY, std::make_unique<mainMenuState>()));
        bool isLoadState = (dynamic_cast<loadGameState*>(g.getActiveState()) != nullptr);
        logResult("Transition to LoadGameState", isLoadState);
        allPassed &= isLoadState;

        // Go Back from LoadGameState
        auto* loadState = dynamic_cast<loadGameState*>(g.getActiveState());
        if (loadState)
        {
            loadState->goBack(&g);
            bool backToMainMenu = (dynamic_cast<mainMenuState*>(g.getActiveState()) != nullptr);
            logResult("LoadGameState::goBack returns to MainMenuState", backToMainMenu);
            allPassed &= backToMainMenu;
        }

        return allPassed;
    }

    bool testCharacterCreation()
    {
        std::cout << "\n--- Running Test 2: Character Creation Sequence ---\n";
        bool allPassed = true;

        game g;
        g.init();

        auto cc = std::make_unique<characterCreationState>();
        auto* ccPtr = cc.get();
        g.changeState(std::move(cc));

        bool inCC = (dynamic_cast<characterCreationState*>(g.getActiveState()) != nullptr);
        logResult("Entered Character Creation State", inCC);
        allPassed &= inCC;

        // Step 0: Gender
        ccPtr->gender = "Female";
        ccPtr->femininity = "Feminine";
        ccPtr->step = 1;
        g.refreshActionGrid();

        // Step 1: Appearance
        ccPtr->hairColor = "Silver";
        ccPtr->eyeColor = "Amethyst";
        ccPtr->step = 2;
        g.refreshActionGrid();

        // Step 2: Name
        ccPtr->feminineName = "Aria";
        ccPtr->surname = "Vesper";
        ccPtr->personalityTraits.insert("Confident");
        ccPtr->personalityTraits.insert("Lewd");
        ccPtr->step = 3;
        g.refreshActionGrid();

        // Step 3: Museum Tour & Step 4: Finalize
        ccPtr->finalizeCharacter(&g);

        entity* p = g.getPlayer();
        bool playerCreated = (p != nullptr);
        logResult("Player Entity Created", playerCreated);
        allPassed &= playerCreated;

        if (p)
        {
            bool nameMatch = (p->name == "Aria Vesper");
            logResult("Player Full Name Matches", nameMatch, p->name);
            allPassed &= nameMatch;

            bool hasStartingItems = (!p->inventory.backpack.empty());
            logResult("Player has Starting Items in Backpack", hasStartingItems);
            allPassed &= hasStartingItems;

            bool isExploration = (dynamic_cast<explorationState*>(g.getActiveState()) != nullptr);
            logResult("Successfully Transitioned to ExplorationState", isExploration);
            allPassed &= isExploration;
        }

        return allPassed;
    }

    bool testSaveLoadRoundtrip()
    {
        std::cout << "\n--- Running Test 3: Save & Load Serialization ---\n";
        bool allPassed = true;

        game g;
        g.playerEntity = std::make_shared<entity>("test_hero", "TestHero");
        g.Player = g.playerEntity.get();
        g.Player->stats.setBaseStat("health", 85.0f);
        g.Player->stats.setBaseStat("mana", 45.0f);
        g.Player->stats.setBaseStat("lust", 20.0f);

        const std::string testSaveName = "AutoTestSave";
        bool saveSuccess = saveManager::saveNamedGame(&g, testSaveName);
        logResult("Save Game File Written", saveSuccess, testSaveName);
        allPassed &= saveSuccess;

        // Modify player in memory
        g.getPlayer()->name = "CorruptedName";
        g.getPlayer()->stats.setBaseStat("health", 10.0f);

        // Load back from file
        bool loadSuccess = saveManager::loadFromFile(&g, "TestHero_" + testSaveName + ".json");
        logResult("Save Game File Loaded", loadSuccess);
        allPassed &= loadSuccess;

        if (loadSuccess && g.getPlayer())
        {
            bool nameRestored = (g.getPlayer()->name == "TestHero");
            logResult("Player Name Restored Accurately", nameRestored, g.getPlayer()->name);
            allPassed &= nameRestored;

            bool hpRestored = (g.getPlayer()->getStat("health") == 85.0f);
            logResult("Player Health Restored Accurately", hpRestored, std::to_string(g.getPlayer()->getStat("health")));
            allPassed &= hpRestored;
        }

        // Clean up test file
        saveManager::deleteSave("TestHero_" + testSaveName + ".json");

        return allPassed;
    }

    bool testClothingDisplacement()
    {
        std::cout << "\n--- Running Test 4: Clothing Displacement & Exposure ---\n";
        bool allPassed = true;

        game g;
        g.playerEntity = std::make_shared<entity>("exposed_hero", "ExposedHero");
        g.Player = g.playerEntity.get();
        entity* player = g.getPlayer();

        if (!player)
        {
            logResult("Player Entity Exists", false);
            return false;
        }

        inventoryComponent& inv = player->inventory;

        // 1. Initial State: slots without clothes should be exposed
        bool initialGroinExposed = inv.isSlotExposed(bodySlot::GROIN);
        logResult("Naked Body Groin is Exposed", initialGroinExposed);
        allPassed &= initialGroinExposed;

        // 2. Equip boxer shorts and trousers
        auto boxers = std::make_shared<item>();
        boxers->id = "boxer_shorts";
        boxers->name = "Boxer Shorts";
        boxers->isEquippable = true;
        boxers->targetSlot = equipSlot::GROIN_OVER;
        boxers->supportedDisplacements[DisplacementMode::PULL_ASIDE] = { bodySlot::GROIN };
        inv.addItem(boxers);
        inv.equipItem(0, equipSlot::GROIN_OVER);

        auto trousers = std::make_shared<item>();
        trousers->id = "black_trousers";
        trousers->name = "Black Trousers";
        trousers->isEquippable = true;
        trousers->targetSlot = equipSlot::LEGS_OUTER;
        trousers->supportedDisplacements[DisplacementMode::UNBUTTON] = { bodySlot::GROIN, bodySlot::HIPS };
        inv.addItem(trousers);
        inv.equipItem(0, equipSlot::LEGS_OUTER);

        bool coveredGroin = !inv.isSlotExposed(bodySlot::GROIN);
        logResult("Clothed Groin is Covered", coveredGroin);
        allPassed &= coveredGroin;

        // 3. Displace trousers (UNBUTTON) + boxers (PULL_ASIDE)
        inv.setDisplacement(equipSlot::LEGS_OUTER, DisplacementMode::UNBUTTON);
        inv.setDisplacement(equipSlot::GROIN_OVER, DisplacementMode::PULL_ASIDE);

        bool displacedGroinExposed = inv.isSlotExposed(bodySlot::GROIN);
        logResult("Displaced Clothing Exposes Groin", displacedGroinExposed);
        allPassed &= displacedGroinExposed;

        // 4. Reset displacement
        inv.resetAllDisplacements();
        bool resetGroinCovered = !inv.isSlotExposed(bodySlot::GROIN);
        logResult("Reset Displacement Re-Covers Groin", resetGroinCovered);
        allPassed &= resetGroinCovered;

        return allPassed;
    }

    bool testSettingsAndThemes()
    {
        std::cout << "\n--- Running Test 5: Settings & Theme Persistence ---\n";
        bool allPassed = true;

        GameSettings settings;
        settings.display.activeTheme = "theme_cyber_neon";
        settings.gameplay.difficultyMultiplier = 2.0f;
        settings.content.lactationEnabled = true;

        const std::string testSettingsFile = "data/test_settings.json";
        bool saveSuccess = settingsManager::saveToFile(settings, testSettingsFile);
        logResult("Settings Saved to JSON", saveSuccess);
        allPassed &= saveSuccess;

        GameSettings loadedSettings;
        bool loadSuccess = settingsManager::loadFromFile(loadedSettings, testSettingsFile);
        logResult("Settings Loaded from JSON", loadSuccess);
        allPassed &= loadSuccess;

        if (loadSuccess)
        {
            bool themeMatch = (loadedSettings.display.activeTheme == "theme_cyber_neon");
            logResult("Active Theme Persisted", themeMatch, loadedSettings.display.activeTheme);
            allPassed &= themeMatch;

            bool diffMatch = (loadedSettings.gameplay.difficultyMultiplier == 2.0f);
            logResult("Difficulty Multiplier Persisted", diffMatch);
            allPassed &= diffMatch;
        }

        std::error_code ec;
        std::filesystem::remove(testSettingsFile, ec);

        return allPassed;
    }

    bool runAllTests()
    {
        g_passCount = 0;
        g_failCount = 0;

        std::cout << "======================================================================\n";
        std::cout << "   [textRPG Engine Autonomous Regression Test Suite]\n";
        std::cout << "======================================================================\n";

        bool t1 = testStateNavigation();
        bool t2 = testCharacterCreation();
        bool t3 = testSaveLoadRoundtrip();
        bool t4 = testClothingDisplacement();
        bool t5 = testSettingsAndThemes();

        std::cout << "======================================================================\n";
        std::cout << " Test Summary: " << g_passCount << " Passed, " << g_failCount << " Failed.\n";
        std::cout << "======================================================================\n\n";

        return (g_failCount == 0);
    }
}
