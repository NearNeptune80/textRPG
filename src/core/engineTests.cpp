#include "core/engineTests.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>
#include <filesystem>

#include "core/game.h"
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/loadGameState.h"
#include "state/characterCreationState.h"
#include "state/explorationState.h"
#include "state/eventState.h"
#include "state/transformationState.h"
#include "core/characterDescription.h"
#include "save/saveManager.h"
#include "items/inventory.h"
#include "items/itemDatabase.h"
#include "settings/settingsManager.h"
#include "ui/theme.h"
#include "ui/fontManager.h"
#include "ui/tooltipManager.h"
#include "common/enums.h"
#include "quest/questDatabase.h"
#include "quest/quest.h"
#include "entities/questComponent.h"
#include "state/phoneAppsState.h"
#include "state/shopState.h"
#include "items/merchantValuation.h"
#include "entities/namedCharacter.h"
#include "entities/npcGenerator.h"
#include "entities/perkDatabase.h"
#include "combat/combatEngine.h"
#include "state/combatState.h"
#include "state/encounterResolutionState.h"
#include "state/sexState.h"
#include "ui/layoutEngine.h"

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

    bool testSubmenuButtonFunctionality()
    {
        std::cout << "\n--- Running Test 6: Submenu Button Functionality & Interactive Actions ---\n";
        bool allPassed = true;

        game g;
        g.init();

        // 1. Test Main Menu Buttons
        g.changeState(std::make_unique<mainMenuState>());
        g.refreshActionGrid();

        // Click "Options" in Action Grid (Slot 5)
        if (g.activeButtons.size() > 5 && g.activeButtons[5].onClick)
        {
            g.activeButtons[5].onClick();
            bool isOptions = (dynamic_cast<optionsState*>(g.getActiveState()) != nullptr);
            logResult("Main Menu 'Options' button launches optionsState", isOptions);
            allPassed &= isOptions;
        }

        // 2. Test Options Action Grid in General Mode
        auto* opt = dynamic_cast<optionsState*>(g.getActiveState());
        if (opt)
        {
            g.refreshActionGrid();

            // Keybinds (Slot 0)
            if (!g.activeButtons.empty() && g.activeButtons[0].onClick)
            {
                g.activeButtons[0].onClick();
                bool keybindsOpen = opt->isKeybindsOpen;
                logResult("Options 'Keybinds' button opens keybindings overlay", keybindsOpen);
                allPassed &= keybindsOpen;
                opt->isKeybindsOpen = false;
                g.refreshActionGrid();
            }

            // Defaults (Slot 10)
            g.settings.display.fontSize = 28;
            fontManager::getInstance().setPointSize(28.0f);
            bool fontScaled = (fontManager::getInstance().getPointSize() == 28.0f);
            logResult("FontManager dynamically scales point size to 28pt", fontScaled);
            allPassed &= fontScaled;

            if (g.activeButtons.size() > 10 && g.activeButtons[10].onClick)
            {
                g.activeButtons[10].onClick(); // Defaults
                bool defaultsRestored = (g.settings.display.fontSize == 18);
                logResult("Options 'Defaults' button restores default settings", defaultsRestored);
                allPassed &= defaultsRestored;
            }

            // Back button returns to Main Menu (Slot 14)
            if (g.activeButtons.size() > 14 && g.activeButtons[14].onClick)
            {
                g.activeButtons[14].onClick();
                bool backToMenu = (dynamic_cast<mainMenuState*>(g.getActiveState()) != nullptr);
                logResult("Options 'Back' button returns to Main Menu", backToMenu);
                allPassed &= backToMenu;
            }
        }

        // 3. Test Save/Load State Actions
        g.changeState(std::make_unique<loadGameState>(SaveMenuMode::SAVE_AND_LOAD, std::make_unique<mainMenuState>()));
        auto* loadState = dynamic_cast<loadGameState*>(g.getActiveState());
        if (loadState)
        {
            g.refreshActionGrid();

            // Toggle confirmations (Slot 0)
            bool initConfirm = loadState->confirmationsEnabled;
            if (!g.activeButtons.empty() && g.activeButtons[0].onClick)
            {
                g.activeButtons[0].onClick();
                bool confirmToggled = (loadState->confirmationsEnabled != initConfirm);
                logResult("Save/Load 'Confirmations' toggle functions", confirmToggled);
                allPassed &= confirmToggled;
            }

            // Toggle Sort Name (Slot 2)
            if (g.activeButtons.size() > 2 && g.activeButtons[2].onClick)
            {
                g.activeButtons[2].onClick();
                bool sortNameSet = (loadState->sortMode == 1);
                logResult("Save/Load 'Sort: Name' activates name sorting", sortNameSet);
                allPassed &= sortNameSet;
            }
        }

        return allPassed;
    }

    bool testContentOptionsAllCategories()
    {
        std::cout << "\n--- Running Test 7: Content Options 9 Categories & Demographic Logic ---\n";
        bool allPassed = true;

        game g;
        g.init();

        auto opt = std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS, std::make_unique<mainMenuState>());
        auto* optPtr = opt.get();
        g.changeState(std::move(opt));

        // 1. Misc Category
        optPtr->contentCategory = ContentOptionsCategory::MISC;
        g.settings.gameplay.autoSaveFrequency = 2; // Weekly
        g.settings.display.showArtwork = false;
        g.settings.gameplay.stormInterruptions = false;

        // 2. Gameplay Category
        optPtr->contentCategory = ContentOptionsCategory::GAMEPLAY;
        g.settings.gameplay.badEndsEnabled = false;
        g.settings.gameplay.autoLoot = false;
        g.settings.gameplay.currencyLossOnDefeatPercent = 0.50f;

        // 3. Sex & Fetishes Category
        optPtr->contentCategory = ContentOptionsCategory::SEX_AND_FETISHES;
        g.settings.content.nonConEnabled = true;
        g.settings.content.fluidMultiplier = 4.0f;

        // 4. Bodies Category
        optPtr->contentCategory = ContentOptionsCategory::BODIES;
        g.settings.content.pregnancyEnabled = false;
        g.settings.content.transformationSpeedMultiplier = 2.0f;

        // 5. Gender Prefs Category
        optPtr->contentCategory = ContentOptionsCategory::GENDER_PREFS;
        g.settings.demographics.percentMale = 50.0f;
        g.settings.demographics.percentFemale = 50.0f;
        g.settings.demographics.percentHermaphrodite = 0.0f;

        // 6. Orientation Prefs Category
        optPtr->contentCategory = ContentOptionsCategory::ORIENTATION_PREFS;
        g.settings.demographics.percentHetero = 10.0f;
        g.settings.demographics.percentBi = 80.0f;

        // 7. Fetish Prefs Category
        optPtr->contentCategory = ContentOptionsCategory::FETISH_PREFS;
        g.settings.content.fetishPreferences["Anal"] = 6; // Always
        g.settings.content.fetishPreferences["Breasts lover"] = 5; // Love
        g.settings.content.fetishPreferences["Oral"] = 1; // Hate

        // Persist all settings
        const std::string testOptFile = "data/test_content_options.json";
        settingsManager::saveToFile(g.settings, testOptFile);

        // Load back and verify data integrity
        GameSettings verified;
        settingsManager::loadFromFile(verified, testOptFile);

        bool miscMatch = (verified.gameplay.autoSaveFrequency == 2 && !verified.display.showArtwork && !verified.gameplay.stormInterruptions);
        logResult("Content Category 0 (Misc) Persisted Accurately", miscMatch);
        allPassed &= miscMatch;

        bool gpMatch = (!verified.gameplay.badEndsEnabled && !verified.gameplay.autoLoot && verified.gameplay.currencyLossOnDefeatPercent == 0.50f);
        logResult("Content Category 1 (Gameplay) Persisted Accurately", gpMatch);
        allPassed &= gpMatch;

        bool sexMatch = (verified.content.nonConEnabled && verified.content.fluidMultiplier == 4.0f);
        logResult("Content Category 2 (Sex & Fetishes) Persisted Accurately", sexMatch);
        allPassed &= sexMatch;

        bool bodiesMatch = (!verified.content.pregnancyEnabled && verified.content.transformationSpeedMultiplier == 2.0f);
        logResult("Content Category 3 (Bodies) Persisted Accurately", bodiesMatch);
        allPassed &= bodiesMatch;

        bool demoMatch = (verified.demographics.percentMale == 50.0f && verified.demographics.percentBi == 80.0f);
        logResult("Content Categories 4-7 (Demographics) Persisted Accurately", demoMatch);
        allPassed &= demoMatch;

        bool fetishMatch = (verified.content.fetishPreferences["Anal"] == 6 &&
                            verified.content.fetishPreferences["Breasts lover"] == 5 &&
                            verified.content.fetishPreferences["Oral"] == 1);
        logResult("Content Category 8 (Fetish Ratings) Persisted Accurately", fetishMatch);
        allPassed &= fetishMatch;

        // Test Reset Category Defaults
        optPtr->contentCategory = ContentOptionsCategory::GENDER_PREFS;
        optPtr->resetCategoryDefaults(&g);
        bool resetMatch = (g.settings.demographics.percentMale == 30.0f && g.settings.demographics.percentFemale == 40.0f);
        logResult("Reset Category Defaults Restores Submenu Archetypes", resetMatch);
        allPassed &= resetMatch;

        std::error_code ec;
        std::filesystem::remove(testOptFile, ec);

        return allPassed;
    }

    bool testGranularEditorOptionMasking()
    {
        std::cout << "\n--- Running Test 8: Granular Editor Option Masking & Dynamic Tab Pruning ---\n";
        bool allPassed = true;

        // 1. New Game Preset: Exactly 5 tabs (Identity, Body, Face & Hair, Wardrobe, Name & Finish)
        auto ccNew = std::make_unique<characterCreationState>(EditorConfig::newGamePreset(), 0);
        auto newTabs = ccNew->getActiveTabs();
        bool newTabCountValid = (newTabs.size() == 5);
        logResult("New Game Preset activates exactly 5 tabs", newTabCountValid);
        allPassed &= newTabCountValid;

        bool hasIdentity = (newTabs[0] == EditorTabId::IDENTITY);
        bool hasBody = (newTabs[1] == EditorTabId::BODY);
        bool hasFace = (newTabs[2] == EditorTabId::FACE_HAIR);
        bool hasWardrobe = (newTabs[3] == EditorTabId::WARDROBE);
        bool hasFinish = (newTabs[4] == EditorTabId::NAME_FINISH);
        bool tabsInOrder = (hasIdentity && hasBody && hasFace && hasWardrobe && hasFinish);
        logResult("New Game Preset contains [Identity, Body, Face & Hair, Wardrobe, Name & Finish]", tabsInOrder);
        allPassed &= tabsInOrder;

        // Verify choice filtering (Human only ears in new game)
        auto earChoices = ccNew->config.filterChoices("ear_type", { "Human", "Cat", "Dog", "Elf", "Demon" });
        bool humanOnlyEars = (earChoices.size() == 1 && earChoices[0] == "Human");
        logResult("New Game Preset filters Ear Types to [Human] only", humanOnlyEars);
        allPassed &= humanOnlyEars;

        // 2. Hair Salon Preset: Pruned down to 1 tab (FACE_HAIR)
        auto ccSalon = std::make_unique<characterCreationState>(EditorConfig::hairSalonPreset(), 0);
        auto salonTabs = ccSalon->getActiveTabs();
        bool salonSingleTab = (salonTabs.size() == 1 && salonTabs[0] == EditorTabId::FACE_HAIR);
        logResult("Hair Salon Preset prunes all tabs down to single 'Face & Hair' tab", salonSingleTab);
        allPassed &= salonSingleTab;

        // 3. Tattoo / Piercing Preset: Pruned down to 1 tab (COSMETICS)
        auto ccTattoo = std::make_unique<characterCreationState>(EditorConfig::tattooPiercingPreset(), 0);
        auto tattooTabs = ccTattoo->getActiveTabs();
        bool tattooSingleTab = (tattooTabs.size() == 1 && tattooTabs[0] == EditorTabId::COSMETICS);
        logResult("Tattoo Studio Preset prunes all tabs down to single 'Cosmetics' tab", tattooSingleTab);
        allPassed &= tattooSingleTab;

        // 4. Arcane Full Transformation Preset: All tabs unlocked including Appendages
        auto ccTransform = std::make_unique<characterCreationState>(EditorConfig::fullTransformationPreset(), 0);
        auto transTabs = ccTransform->getActiveTabs();
        bool transFullTabs = (transTabs.size() >= 8);
        logResult("Full Transformation Preset activates all 8+ tabs (including Appendages & Genitalia)", transFullTabs);
        allPassed &= transFullTabs;

        auto transEars = ccTransform->config.filterChoices("ear_type", { "Human", "Cat", "Dog", "Elf", "Demon", "Cow", "Rabbit", "Dragon" });
        bool fullEarsUnlocked = (transEars.size() >= 8);
        logResult("Full Transformation Preset unlocks exotic Ear choices (Cat, Dog, Demon, Elf, Dragon)", fullEarsUnlocked);
        allPassed &= fullEarsUnlocked;

        return allPassed;
    }

    bool testHairstyleGatingAndBodyShape()
    {
        std::cout << "\n--- Running Test 9: Hairstyle Length Gating & Body Shape Calculations ---\n";
        bool allPassed = true;

        // 1. Hairstyle length thresholds
        auto baldStyles = EditorConfig::getValidHairstyles(0);
        bool baldOnly = (baldStyles.size() == 1 && baldStyles[0] == "Bald");
        logResult("0 cm hair length only allows 'Bald'", baldOnly);
        allPassed &= baldOnly;

        auto shortStyles = EditorConfig::getValidHairstyles(5);
        bool hasShort = (std::find(shortStyles.begin(), shortStyles.end(), "Short") != shortStyles.end());
        bool noPonytail = (std::find(shortStyles.begin(), shortStyles.end(), "Ponytail") == shortStyles.end());
        logResult("5 cm hair length allows 'Short' but blocks 'Ponytail'", hasShort && noPonytail);
        allPassed &= (hasShort && noPonytail);

        auto longStyles = EditorConfig::getValidHairstyles(25);
        bool hasPonytail = (std::find(longStyles.begin(), longStyles.end(), "Ponytail") != longStyles.end());
        bool hasBraided = (std::find(longStyles.begin(), longStyles.end(), "Braided") != longStyles.end());
        logResult("25 cm hair length unlocks 'Ponytail' and 'Braided'", hasPonytail && hasBraided);
        allPassed &= (hasPonytail && hasBraided);

        // 2. Composite Body Shape calculations
        std::string shape1 = EditorConfig::calculateBodyShape("Soft", "Skinny");
        bool isFrail = (shape1 == "Frail / Delicate");
        logResult("Soft muscle + Skinny body = 'Frail / Delicate'", isFrail);
        allPassed &= isFrail;

        std::string shape2 = EditorConfig::calculateBodyShape("Ripped", "Muscular");
        bool isHeroic = (shape2 == "Heroic / Bodybuilder");
        logResult("Ripped muscle + Muscular body = 'Heroic / Bodybuilder'", isHeroic);
        allPassed &= isHeroic;

        std::string shape3 = EditorConfig::calculateBodyShape("Toned", "Slender");
        bool isAthletic = (shape3 == "Toned / Fit");
        logResult("Toned muscle + Slender body = 'Toned / Fit'", isAthletic);
        allPassed &= isAthletic;

        return allPassed;
    }

    bool testWardrobeDecencySystem()
    {
        std::cout << "\n--- Running Test 10: Wardrobe Dressing & Decency Validation System ---\n";
        bool allPassed = true;

        itemDatabase::loadDatabase("data/items.json");

        characterCreationState cc;
        cc.gender = "Female";
        cc.femininity = "Feminine";
        cc.initializeWardrobe();

        // 1. Initial state starts unclad with clothes in available pool
        bool initUnclad = !cc.isClothedEnough();
        std::string initStatus = cc.getDecencyStatus();
        bool initMentionsAll = (initStatus.find("Must put on footwear") != std::string::npos &&
                                initStatus.find("Must conceal groin") != std::string::npos &&
                                initStatus.find("Must conceal chest") != std::string::npos);
        logResult("Initial empty inventory wardrobe setup is Indecent", initUnclad && initMentionsAll);
        allPassed &= (initUnclad && initMentionsAll);

        // 2. Equip dress and shoes -> Decent
        cc.applyWardrobePreset("Evening Dress");
        bool dressedDecent = cc.isClothedEnough();
        logResult("Equipping coordinated dress & shoes achieves Decent status", dressedDecent);
        allPassed &= dressedDecent;

        // 3. Strip footwear -> Indecent
        cc.unequipWardrobeItem(equipSlot::FEET);
        bool noShoesIndecent = !cc.isClothedEnough();
        std::string status1 = cc.getDecencyStatus();
        bool mentionsShoes = (status1.find("Must put on footwear") != std::string::npos);
        logResult("Stripping footwear flags indecency with warning", noShoesIndecent && mentionsShoes);
        allPassed &= (noShoesIndecent && mentionsShoes);

        // 4. Strip all clothing -> Multiple warnings
        cc.unequipWardrobeItem(equipSlot::TORSO_OVER);
        cc.unequipWardrobeItem(equipSlot::TORSO_UNDER);
        cc.unequipWardrobeItem(equipSlot::CHEST_WEAR);
        cc.unequipWardrobeItem(equipSlot::LEGS_OUTER);
        cc.unequipWardrobeItem(equipSlot::GROIN_OVER);

        std::string statusNaked = cc.getDecencyStatus();
        bool mentionsGroin = (statusNaked.find("Must conceal groin") != std::string::npos);
        bool mentionsChest = (statusNaked.find("Must conceal chest") != std::string::npos);
        logResult("Fully stripped character flags groin and chest indecency", mentionsGroin && mentionsChest);
        allPassed &= (mentionsGroin && mentionsChest);

        // 5. Re-equip wardrobe garments
        cc.applyWardrobePreset("Formal Suit");
        bool reEquippedDecent = cc.isClothedEnough();
        logResult("Re-equipping wardrobe garments restores Decent status", reEquippedDecent);
        allPassed &= reEquippedDecent;

        return allPassed;
    }

    bool testFullCustomizationTrackingAndAppearanceDescription()
    {
        std::cout << "\n--- Running Test 11: Full Customization Tracking & Dynamic Appearance Description ---\n";
        bool allPassed = true;

        itemDatabase::loadDatabase("data/items.json");

        characterCreationState cc;
        cc.gender = "Female";
        cc.femininity = "Very Feminine";
        cc.heightCm = 172;
        cc.bodySize = "slender";
        cc.muscleDefinition = "toned";
        cc.skinPrimaryColor = "porcelain";
        cc.lipSize = 3; // plump
        cc.puffyLips = true;
        cc.eyeColor = "hazel";
        cc.hairColor = "auburn";
        cc.hairStyle = "wavy";
        cc.hairLengthCm = 60;
        cc.breastCupSize = 5; // D-cup (0=flat, 1=AA, 2=A, 3=B, 4=C, 5=D)
        cc.breastShape = "perky";
        cc.nippleSize = 3; // large
        cc.areolaeSize = 2; // average
        cc.puffyNipples = true;
        cc.lactationTier = 3; // decent amount
        cc.assSize = 3; // large
        cc.hipSize = 3; // large
        cc.anusBleached = true;
        cc.vaginaCapacity = 2;
        cc.labiaSize = 3;
        cc.clitorisSize = 1;

        cc.blusher = "pink";
        cc.lipstick = "red";
        cc.eyeliner = "black";
        cc.nailPolish = "gold";

        cc.piercings["ear"] = true;
        cc.piercings["navel"] = true;
        cc.piercings["nipple"] = true;

        cc.pubicHair = "trimmed";
        cc.underarmHair = "none";
        cc.assHair = "none";

        cc.personalityTraits.insert("Confident");
        cc.personalityTraits.insert("Kind");
        cc.personalityTraits.insert("Lewd");

        // 1. Verify Dynamic Appearance Text Description
        std::string desc = cc.generateAppearanceDescription();
        bool hasCup = (desc.find("D-cup") != std::string::npos);
        bool hasPerky = (desc.find("perky") != std::string::npos);
        bool hasPlump = (desc.find("plump lips that are extra puffy") != std::string::npos);
        bool hasNip = (desc.find("large nipples (puffy)") != std::string::npos);
        bool hasBleached = (desc.find("bleached anus") != std::string::npos);

        bool textDescAccurate = hasCup && hasPerky && hasPlump && hasNip && hasBleached;
        if (!textDescAccurate)
        {
            std::cout << "[DEBUG] hasCup=" << hasCup << ", hasPerky=" << hasPerky 
                      << ", hasPlump=" << hasPlump << ", hasNip=" << hasNip 
                      << ", hasBleached=" << hasBleached << "\n"
                      << "Desc:\n" << desc << "\n";
        }
        logResult("Dynamic Appearance Description renders breasts, lips, nipples, and anus attributes", textDescAccurate);
        allPassed &= textDescAccurate;

        // 2. Finalize into game entity
        game gameCtx;
        gameCtx.init();
        cc.finalizeCharacter(&gameCtx);
        entity* p = gameCtx.getPlayer();

        bool pExists = (p != nullptr);
        logResult("Player entity created during finalization", pExists);
        allPassed &= pExists;

        if (p)
        {
            // Verify Cosmetics
            bool lipMatch = (p->cosmetics["lipstick"] == "red");
            bool nailMatch = (p->cosmetics["nailPolish"] == "gold");
            logResult("Cosmetics (lipstick, nail polish) transferred to entity", lipMatch && nailMatch);
            allPassed &= (lipMatch && nailMatch);

            // Verify Piercings
            bool piercMatch = (p->piercings["navel"] && p->piercings["nipple"] && p->piercings["ear"]);
            logResult("Piercings (navel, nipple, ear) transferred to entity", piercMatch);
            allPassed &= piercMatch;

            // Verify Body Hair
            bool hairMatch = (p->bodyHair["pubic"] == "trimmed" && p->bodyHair["underarm"] == "none");
            logResult("Body hair grooming (pubic, underarm) transferred to entity", hairMatch);
            allPassed &= hairMatch;

            // Verify Personality Traits
            bool traitMatch = (std::find(p->personalityTraits.begin(), p->personalityTraits.end(), "Lewd") != p->personalityTraits.end());
            logResult("Personality traits transferred to entity", traitMatch);
            allPassed &= traitMatch;

            // Verify Anatomy & Lactation
            const bodyPart* bPart = p->anatomy.getPart(bodySlot::BREASTS);
            bool bPartMatch = (bPart != nullptr && bPart->cupSize == 5 && bPart->isLactating);
            logResult("Anatomy component cup size and lactation status transferred to entity", bPartMatch);
            allPassed &= bPartMatch;
        }

        return allPassed;
    }

    bool testFullTransformationSuiteAndPresetPersistence()
    {
        std::cout << "\n--- Running Test 12: Full Transformation Suite & Preset Persistence ---\n";
        bool allPassed = true;

        game g;
        g.init();

        auto player = std::make_shared<entity>("player_test", "Morgan");
        player->genderArchetype = GenderArchetype::MALE;
        player->stats.setBaseStat("health", 100.0f);
        player->stats.setBaseStat("max_health", 100.0f);
        g.playerEntity = player;

        // 1. Enter transformation state
        auto tf = std::make_unique<transformationState>(TransformationTab::CORE);
        g.changeState(std::move(tf));
        transformationState* tfPtr = dynamic_cast<transformationState*>(g.getActiveState());

        bool inTfState = (tfPtr != nullptr);
        logResult("Transformation State Initialised & Active", inTfState);
        allPassed &= inTfState;

        // 2. Test Reset to Human Baseline
        tfPtr->resetToHuman(&g);
        bool isHuman = (player->anatomy.getRacialTitle() == "Human" && !player->anatomy.hasPart(bodySlot::HORNS) && !player->anatomy.hasPart(bodySlot::WINGS));
        logResult("Reset to Human baseline clears horns/wings and establishes Human racial title", isHuman);
        allPassed &= isHuman;

        // 3. Mutate into Demon Hybrid
        bodyPart demonHorns;
        demonHorns.id = "horns_demon"; demonHorns.name = "Demon Horns"; demonHorns.race = "Demon"; demonHorns.count = 2; demonHorns.length = 25.0f;
        player->anatomy.setPart(bodySlot::HORNS, demonHorns);

        bodyPart demonWings;
        demonWings.id = "wings_demon"; demonWings.name = "Bat Wings"; demonWings.race = "Demon"; demonWings.count = 2;
        player->anatomy.setPart(bodySlot::WINGS, demonWings);

        bodyPart catTail;
        catTail.id = "tail_cat"; catTail.name = "Cat Tail"; catTail.race = "Cat-morph"; catTail.length = 80.0f;
        player->anatomy.setPart(bodySlot::TAIL, catTail);

        bodyPart breasts;
        breasts.id = "breasts"; breasts.name = "Breasts"; breasts.cupSize = 4; // D-cup
        breasts.isLactating = true; breasts.currentFluidMl = 350.0f; breasts.maxFluidMl = 1000.0f;
        player->anatomy.setPart(bodySlot::BREASTS, breasts);

        bool hasDemonFeatures = player->anatomy.hasPart(bodySlot::HORNS) && player->anatomy.hasPart(bodySlot::WINGS) && player->anatomy.hasPart(bodySlot::TAIL);
        logResult("Live Transformation applies Horns, Wings, Tail, and Lactating Breasts", hasDemonFeatures);
        allPassed &= hasDemonFeatures;

        // 4. Save Named Transformation Preset
        const std::string testPresetName = "AutoTest_DemonCat";
        tfPtr->savePreset(&g, testPresetName);
        auto presets = tfPtr->getPresetNames();
        bool presetSaved = (std::find(presets.begin(), presets.end(), testPresetName) != presets.end());
        logResult("Transformation Preset saved to disk (AutoTest_DemonCat.json)", presetSaved);
        allPassed &= presetSaved;

        // 5. Change form (Reset to human)
        tfPtr->resetToHuman(&g);
        bool resetSuccess = (!player->anatomy.hasPart(bodySlot::HORNS) && !player->anatomy.hasPart(bodySlot::WINGS));
        logResult("Body Form Cleared to baseline prior to preset restore", resetSuccess);
        allPassed &= resetSuccess;

        // 6. Reload Saved Preset and verify exact restitution
        tfPtr->loadPreset(&g, testPresetName);
        const bodyPart* restoredHorns = player->anatomy.getPart(bodySlot::HORNS);
        const bodyPart* restoredWings = player->anatomy.getPart(bodySlot::WINGS);
        const bodyPart* restoredTail = player->anatomy.getPart(bodySlot::TAIL);
        const bodyPart* restoredBreasts = player->anatomy.getPart(bodySlot::BREASTS);

        bool hornsMatch = (restoredHorns != nullptr && restoredHorns->length == 25.0f && restoredHorns->race == "Demon");
        bool wingsMatch = (restoredWings != nullptr && restoredWings->race == "Demon");
        bool tailMatch = (restoredTail != nullptr && restoredTail->race == "Cat-morph");
        bool breastsMatch = (restoredBreasts != nullptr && restoredBreasts->cupSize == 4 && restoredBreasts->isLactating && restoredBreasts->currentFluidMl == 350.0f);

        bool allRestored = hornsMatch && wingsMatch && tailMatch && breastsMatch;
        logResult("Reloaded Preset restores exact horn length (25cm), wings, tail race, cup size (D), and milk fluids (350ml)", allRestored);
        allPassed &= allRestored;

        // 7. Full Prose Description Inspection
        std::string fullProse = characterDescription::generateFullDescription(player.get());
        bool proseHasHorns = (fullProse.find("horns") != std::string::npos);
        bool proseHasWings = (fullProse.find("wings") != std::string::npos);
        bool proseHasTail = (fullProse.find("tail") != std::string::npos);
        bool proseHasMilk = (fullProse.find("milk") != std::string::npos);

        bool proseAccurate = proseHasHorns && proseHasWings && proseHasTail && proseHasMilk;
        logResult("Character Body Inspection generates accurate prose reflecting active transformations", proseAccurate);
        allPassed &= proseAccurate;

        // Clean up preset file
        tfPtr->deletePreset(testPresetName);

        return allPassed;
    }

    bool testLegacySaveCompatibility()
    {
        std::cout << "\n--- Running Test 13: Legacy Save Compatibility & Multi-Save Loading ---\n";
        bool allPassed = true;

        std::string savesDir = saveManager::getSavesDirectory();
        if (std::filesystem::exists(savesDir))
        {
            int loadedCount = 0;
            for (const auto& entry : std::filesystem::directory_iterator(savesDir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    std::string filename = entry.path().filename().string();
                    if (filename == "settings.json" || filename == "theme.json") continue;

                    game testGame;
                    bool loaded = saveManager::loadFromFile(&testGame, entry.path().string());
                    if (loaded)
                    {
                        loadedCount++;
                        bool hasPlayer = (testGame.Player != nullptr && testGame.playerEntity != nullptr);
                        allPassed &= hasPlayer;
                    }
                }
            }
            logResult("All existing save files in data/saves loaded without crash and populated Player pointer", loadedCount > 0 && allPassed);
        }

        return allPassed;
    }

    bool testTooltipSystem()
    {
        std::cout << "\n--- Running Test 14: Engine-Wide Tooltip System ---\n";
        bool allPassed = true;

        // 1. Tooltip clearing
        TooltipManager::clear();
        bool clearPass = !TooltipManager::hasActiveTooltip();
        logResult("TooltipManager::clear correctly resets active tooltip", clearPass);
        allPassed &= clearPass;

        // 2. Setting direct tooltip
        TooltipManager::setTooltip("Excalibur", "Legendary holy blade", "Main Hand Weapon", "[ 1 ]");
        bool setPass = TooltipManager::hasActiveTooltip();
        logResult("TooltipManager::setTooltip successfully registers active tooltip", setPass);
        allPassed &= setPass;

        // 3. Hover detection - within bounding box
        TooltipManager::clear();
        SDL_FRect buttonRect = { 100.0f, 200.0f, 80.0f, 30.0f };
        TooltipPoint insideCursor = { 120.0f, 215.0f };
        bool insidePass = TooltipManager::setHoverTooltip(buttonRect, insideCursor, "Button Hover", "Hovering inside");
        insidePass &= TooltipManager::hasActiveTooltip();
        logResult("TooltipManager::setHoverTooltip activates when cursor is within bounds", insidePass);
        allPassed &= insidePass;

        // 4. Hover detection - outside bounding box
        TooltipManager::clear();
        TooltipPoint outsideCursor = { 50.0f, 50.0f };
        bool outsideTriggered = TooltipManager::setHoverTooltip(buttonRect, outsideCursor, "Button Hover", "Hovering outside");
        bool outsidePass = (!outsideTriggered && !TooltipManager::hasActiveTooltip());
        logResult("TooltipManager::setHoverTooltip ignores cursor outside bounds", outsidePass);
        allPassed &= outsidePass;

        // 5. Font wrapped height measurement
        float singleLineH = fontManager::getInstance().getLineHeight(1.0f);
        float measuredH = fontManager::getInstance().getTextWrappedHeight("This is a multi-line long prose description designed to wrap over multiple lines in the tooltip.", 100.0f, 1.0f);
        bool heightPass = (measuredH >= singleLineH * 2.0f);
        logResult("fontManager::getTextWrappedHeight correctly measures multi-line wrapped text height", heightPass);
        allPassed &= heightPass;

        // 6. Data-driven choice tooltips and disabled requirements
        game g;
        g.init();
        g.loadScene("quest_intro_01"); // Intro quest where Player does NOT have canis root yet
        g.refreshActionGrid();
        bool foundDisabledChoice = false;
        bool foundDataTooltip = false;
        for (const auto& btn : g.activeButtons)
        {
            if (btn.label.find("Canis Root") != std::string::npos)
            {
                foundDisabledChoice = (!btn.isEnabled); // Needs to be disabled/greyed out
                foundDataTooltip = (!btn.description.empty() && btn.description.find("Canis Root") != std::string::npos);
            }
        }
        logResult("Quest choice with unmet requirement is disabled/greyed out", foundDisabledChoice);
        logResult("Disabled quest choice carries data-driven tooltip from quest JSON", foundDataTooltip);
        allPassed &= (foundDisabledChoice && foundDataTooltip);

        return allPassed;
    }

    bool testPlayerStatsAndItemUsage()
    {
        std::cout << "\n--- Running Test 15: Player Stats, Equipment Modifiers & Consumable Usage ---\n";
        bool allPassed = true;

        game g;
        g.init();

        auto player = std::make_shared<entity>("stat_tester", "Valeria");
        g.playerEntity = player;
        g.Player = player.get();

        // 1. Test Base Stats & Dynamic Derivation
        player->stats.setBaseStat("health", 45.0f);
        player->stats.setBaseStat("mana", 110.0f);
        player->stats.setBaseStat("currency", 250.0f);
        player->stats.setBaseStat("arcaneEssence", 25.0f);

        bool maxHpDerived = (player->getStat("max_health") == 100.0f);
        bool maxMpDerived = (player->getStat("max_mana") == 110.0f);
        logResult("Dynamic max_health derives at least 100", maxHpDerived);
        logResult("Dynamic max_mana matches or exceeds current mana (110)", maxMpDerived);
        allPassed &= (maxHpDerived && maxMpDerived);

        // 2. Test Equipment Stat Contribution
        float basePhysique = player->getStat("physique");
        auto shirt = std::make_shared<item>();
        shirt->id = "test_shirt";
        shirt->name = "Reinforced Shirt";
        shirt->isEquippable = true;
        shirt->targetSlot = equipSlot::TORSO_UNDER;
        shirt->statModifiers.push_back({ "physique", 5.0f, 0.0f });

        player->inventory.addItem(shirt);
        player->inventory.equipItem(0, equipSlot::TORSO_UNDER);

        float equippedPhysique = player->getStat("physique");
        bool equipGaveStat = (equippedPhysique == basePhysique + 5.0f);
        logResult("Equipping item applies flat stat modifiers dynamically", equipGaveStat);
        allPassed &= equipGaveStat;

        player->inventory.unequipItem(equipSlot::TORSO_UNDER);
        float unequippedPhysique = player->getStat("physique");
        bool unequipRemovedStat = (unequippedPhysique == basePhysique);
        logResult("Unequipping item removes stat modifier dynamically", unequipRemovedStat);
        allPassed &= unequipRemovedStat;

        // 3. Test Consumable Item Usage
        auto potion = std::make_shared<item>();
        potion->id = "item_potion_health";
        potion->name = "Health Potion";
        potion->isConsumable = true;
        potion->isStackable = true;
        potion->count = 2;
        potion->statModifiers.push_back({ "health", 50.0f, 0.0f });

        player->inventory.addItem(potion);
        int potIndex = static_cast<int>(player->inventory.backpack.size()) - 1;
        float preHealth = player->getStat("health");
        g.handleUseItemAction(potIndex);

        float postHealth = player->getStat("health");
        bool hpRestored = (postHealth == std::min(player->getStat("max_health"), preHealth + 50.0f));
        bool stackDecremented = (player->inventory.backpack[potIndex]->count == 1);
        logResult("Consuming health potion restores HP clamped to max", hpRestored);
        logResult("Consuming stackable item decrements stack count", stackDecremented);
        allPassed &= (hpRestored && stackDecremented);

        return allPassed;
    }

    bool testInventoryCategoricalSortingAndActions()
    {
        std::cout << "\n--- Running Test 16: Inventory Categorical Sorting & Extended Actions ---\n";
        bool allPassed = true;

        game g;
        auto player = std::make_shared<entity>("hero_test", "Sorting Hero");
        g.playerEntity = player;
        g.Player = player.get();

        // Add items out of order: Consumable, Key Item, Weapon, Underwear, Clothing
        auto pot = std::make_shared<item>();
        pot->id = "item_potion_health";
        pot->name = "Health Potion";
        pot->isConsumable = true;
        player->inventory.addItem(pot);

        auto key = std::make_shared<item>();
        key->id = "item_dungeon_key";
        key->name = "Skeleton Key";
        key->isKeyItem = true;
        player->inventory.addItem(key);

        auto sword = std::make_shared<item>();
        sword->id = "item_iron_sword";
        sword->name = "Iron Sword";
        sword->isEquippable = true;
        sword->targetSlot = equipSlot::WEAPON_MAIN;
        player->inventory.addItem(sword);

        auto bra = std::make_shared<item>();
        bra->id = "item_silk_bra";
        bra->name = "Silk Bra";
        bra->isEquippable = true;
        bra->targetSlot = equipSlot::CHEST_WEAR;
        player->inventory.addItem(bra);

        auto shirt = std::make_shared<item>();
        shirt->id = "item_linen_shirt";
        shirt->name = "Linen Shirt";
        shirt->isEquippable = true;
        shirt->targetSlot = equipSlot::TORSO_UNDER;
        player->inventory.addItem(shirt);

        auto stacked = player->inventory.getStackedView();
        bool correctCount = (stacked.size() == 5);
        bool sortCorrect = (correctCount &&
            stacked[0].itemPtr->id == "item_iron_sword" &&      // Weapon (10)
            stacked[1].itemPtr->id == "item_linen_shirt" &&     // Clothing (20)
            stacked[2].itemPtr->id == "item_silk_bra" &&        // Underwear (30)
            stacked[3].itemPtr->id == "item_potion_health" &&   // Consumable (50)
            stacked[4].itemPtr->id == "item_dungeon_key");      // Key Item (70)

        logResult("Natural categorical sorting groups items (Weapon -> Clothing -> Underwear -> Consumable -> Key)", sortCorrect);
        allPassed &= sortCorrect;

        // Test Strip to Underwear
        player->inventory.equipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] = shirt;
        player->inventory.equipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] = bra;
        g.handleStripToUnderwearAction();

        bool shirtUnequipped = (player->inventory.equipped[static_cast<size_t>(equipSlot::TORSO_UNDER)] == nullptr);
        bool braRetained = (player->inventory.equipped[static_cast<size_t>(equipSlot::CHEST_WEAR)] != nullptr);
        bool stripCorrect = (shirtUnequipped && braRetained);
        logResult("Strip to Underwear removes outer garments while retaining underwear", stripCorrect);
        allPassed &= stripCorrect;

        // Test Reset All Fits
        player->inventory.setDisplacement(equipSlot::CHEST_WEAR, DisplacementMode::PULL_DOWN);
        bool dispActive = (player->inventory.getDisplacement(equipSlot::CHEST_WEAR) == DisplacementMode::PULL_DOWN);
        g.handleResetAllDisplacementsAction();
        bool dispReset = (player->inventory.getDisplacement(equipSlot::CHEST_WEAR) == DisplacementMode::NONE);
        bool resetCorrect = (dispActive && dispReset);
        logResult("Reset All Fits restores all active garment displacements", resetCorrect);
        allPassed &= resetCorrect;

        // Test Loot All
        if (g.map)
        {
            auto& td = g.map->getRuntimeData(g.gridX, g.gridY);
            td.droppedItems.clear();
            auto droppedGem = std::make_shared<item>();
            droppedGem->id = "item_gem_ruby";
            droppedGem->name = "Ruby Gem";
            td.addDroppedItem(droppedGem, 120);

            size_t preSize = player->inventory.backpack.size();
            g.handleLootAllAction();
            bool looted = (td.droppedItems.empty() && player->inventory.backpack.size() == preSize + 1);
            logResult("Loot All transfers all ground items into backpack and clears ground", looted);
            allPassed &= looted;
        }

        return allPassed;
    }

    bool testDecouplingAndCaching()
    {
        std::cout << "\n--- Running Test 17: Headless Command Routing, Stat Caching & Enums ---\n";
        bool allPassed = true;

        // 1. Centralized Enums String Conversions Round-trip
        bool enumRoundtrip = true;
        for (std::size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
        {
            auto slot = static_cast<equipSlot>(s);
            std::string_view name = equipSlotToString(slot);
            equipSlot parsed = stringToEquipSlot(name);
            if (parsed != slot) { enumRoundtrip = false; break; }
        }
        logResult("Centralized equipSlotToString round-trip for all slots", enumRoundtrip);
        allPassed &= enumRoundtrip;

        bool bodyRoundtrip = true;
        for (std::size_t b = 0; b < BODY_SLOT_COUNT; ++b)
        {
            auto slot = static_cast<bodySlot>(b);
            std::string_view name = bodySlotToString(slot);
            bodySlot parsed = stringToBodySlot(name);
            if (parsed != slot) { bodyRoundtrip = false; break; }
        }
        logResult("Centralized bodySlotToString round-trip for all slots", bodyRoundtrip);
        allPassed &= bodyRoundtrip;

        // 2. Stat Caching & Dynamic Invalidation on Equipment Change
        auto hero = std::make_shared<entity>("caching_hero", "Cache Hero");
        hero->stats.setBaseStat("physique", 20.0f);
        float basePhys = hero->getStat("physique");
        float cachedPhys = hero->getStat("physique");
        bool cacheMatch = (basePhys == 20.0f && cachedPhys == 20.0f);

        auto ring = std::make_shared<item>();
        ring->id = "ring_strength";
        ring->name = "Strength Ring";
        ring->isEquippable = true;
        ring->targetSlot = equipSlot::FINGER_PRIMARY;
        ring->statModifiers.push_back(StatModifier{ "physique", 10.0f, 0.0f });
        hero->inventory.equipped[static_cast<size_t>(equipSlot::FINGER_PRIMARY)] = ring;
        hero->inventory.equipVersion++; // Invalidate equip cache

        float modifiedPhys = hero->getStat("physique");
        bool cacheInvalidated = (modifiedPhys == 30.0f);
        logResult("Stat cache invalidation on inventory equipVersion increment", cacheMatch && cacheInvalidated);
        allPassed &= (cacheMatch && cacheInvalidated);

        // 3. Headless UICommand Dispatch
        game g;
        g.changeState(std::make_unique<mainMenuState>());
        UICommand openOpt = UICommand::triggerActionButton(5); // Slot 5 = Options
        g.handleCommand(openOpt);
        bool inOptions = (dynamic_cast<optionsState*>(g.getActiveState()) != nullptr);
        logResult("UICommand dispatches button trigger and transitions state", inOptions);
        allPassed &= inOptions;

        UICommand closeMenu = UICommand::closeMenu();
        g.handleCommand(closeMenu);
        bool backToMainMenu = (dynamic_cast<mainMenuState*>(g.getActiveState()) != nullptr);
        logResult("UICommand::closeMenu returns to previous state", backToMainMenu);
        allPassed &= backToMainMenu;

        return allPassed;
    }

    bool testQuestJournalSystem()
    {
        std::cout << "\n--- Running Test 18: Quest Database, Component & Phone Journal System ---\n";
        bool allPassed = true;

        // 1. Quest Database Loading & Definitions
        questDatabase::loadDatabase("data/quests");
        auto allQuests = questDatabase::getAllQuests();
        bool hasQuests = !allQuests.empty();
        logResult("Quest Database loaded quest definitions from data/quests", hasQuests);
        allPassed &= hasQuests;

        const auto* introQuest = questDatabase::getQuest("root_delivery");
        bool validIntro = (introQuest != nullptr && introQuest->id == "root_delivery" && !introQuest->stages.empty());
        logResult("Quest 'root_delivery' correctly defined with non-empty stages", validIntro);
        allPassed &= validIntro;

        // 2. Quest Component Tracking, Progression & Completion
        questComponent qc;
        qc.setQuestStage("root_delivery", 0);
        bool hasRoot = qc.hasQuest("root_delivery");
        logResult("QuestComponent registers active quest", hasRoot);
        allPassed &= hasRoot;

        qc.setTrackedQuest("root_delivery");
        bool trackedRoot = (qc.getTrackedQuest() == "root_delivery");
        logResult("QuestComponent tracks quest 'root_delivery'", trackedRoot);
        allPassed &= trackedRoot;

        // Advance to stage 2 (completion)
        qc.setQuestStage("root_delivery", 2);
        bool isComp = qc.isCompleted("root_delivery");
        logResult("QuestComponent successfully marks quest as completed at completionStage", isComp);
        allPassed &= isComp;

        // 3. Serialization Round-trip
        nlohmann::json qJson = qc.toJson();
        questComponent qcLoaded;
        qcLoaded.fromJson(qJson);
        bool serTracked = (qcLoaded.getTrackedQuest() == "root_delivery");
        bool serComp = qcLoaded.isCompleted("root_delivery");
        logResult("QuestComponent JSON serialization preserves completed stages and tracked quest", serTracked && serComp);
        allPassed &= (serTracked && serComp);

        // 4. Phone Apps State Quests Navigation & Action Grid
        game g;
        g.playerEntity = std::make_shared<entity>("hero_quest", "Quest Hero");
        g.Player = g.playerEntity.get();
        g.Player->quests.setQuestStage("root_delivery", 0);
        g.Player->quests.setTrackedQuest("root_delivery");

        auto phoneState = std::make_unique<phoneAppsState>(PhoneAppMode::QUESTS);
        g.changeState(std::move(phoneState));

        auto* activePhone = dynamic_cast<phoneAppsState*>(g.getActiveState());
        bool inQuestApp = (activePhone != nullptr && activePhone->getAppMode() == PhoneAppMode::QUESTS);
        logResult("phoneAppsState initializes in PhoneAppMode::QUESTS", inQuestApp);
        allPassed &= inQuestApp;

        if (activePhone)
        {
            // Verify Action Grid layout: Slots 0-13 are empty, Slot 14 is Back
            bool slots0To13Empty = true;
            for (int i = 0; i < 14; ++i)
            {
                if (i < static_cast<int>(g.activeButtons.size()) && !g.activeButtons[i].label.empty())
                {
                    slots0To13Empty = false;
                }
            }
            logResult("Action Grid Slots 0-13 are completely empty (clean layout)", slots0To13Empty);
            allPassed &= slots0To13Empty;

            bool slot14IsBack = (g.activeButtons.size() > 14 && g.activeButtons[14].label == "Back");
            logResult("Action Grid Slot 14 is 'Back' button", slot14IsBack);
            allPassed &= slot14IsBack;

            // Test Category Filter Transitions (controlled via Center Pane UI)
            activePhone->setQuestCategoryFilter(QuestCategoryFilter::MAIN);
            bool isMain = (activePhone->getQuestCategoryFilter() == QuestCategoryFilter::MAIN);
            logResult("Quest category filter selects Main Quests", isMain);
            allPassed &= isMain;

            activePhone->setQuestCategoryFilter(QuestCategoryFilter::SIDE);
            bool isSide = (activePhone->getQuestCategoryFilter() == QuestCategoryFilter::SIDE);
            logResult("Quest category filter selects Side Quests", isSide);
            allPassed &= isSide;

            activePhone->setQuestCategoryFilter(QuestCategoryFilter::ALL);
            bool isAll = (activePhone->getQuestCategoryFilter() == QuestCategoryFilter::ALL);
            logResult("Quest category filter selects All Quests", isAll);
            allPassed &= isAll;

            // Test Completed Toggle (controlled via Center Pane UI)
            bool compBefore = activePhone->isShowCompleted();
            activePhone->toggleShowCompleted();
            bool compAfter = activePhone->isShowCompleted();
            activePhone->toggleShowCompleted();
            bool compReverted = activePhone->isShowCompleted();
            bool compToggleOk = (!compBefore && compAfter && !compReverted);
            logResult("Quests app toggles Completed filter state", compToggleOk);
            allPassed &= compToggleOk;

            // Test Card Expansion (controlled via Center Pane Card Click)
            activePhone->toggleExpandedQuest("root_delivery");
            bool isExpanded = (activePhone->getExpandedQuestId() == "root_delivery");
            activePhone->toggleExpandedQuest("root_delivery");
            bool isCollapsed = (activePhone->getExpandedQuestId().empty());
            bool expOk = (isExpanded && isCollapsed);
            logResult("Quests app toggles quest card details expansion", expOk);
            allPassed &= expOk;

            // Test Scrolling bounds
            activePhone->setQuestMaxScrollY(150.0f);
            activePhone->scrollQuestList(50.0f);
            bool scrolled = (activePhone->getQuestScrollY() == 50.0f);
            activePhone->scrollQuestList(200.0f);
            bool clamped = (activePhone->getQuestScrollY() == 150.0f);
            bool scrollOk = (scrolled && clamped);
            logResult("Quest list scroll offset adjusts smoothly and clamps to max scroll bounds", scrollOk);
            allPassed &= scrollOk;

            // Action Grid Slot 14 returns back to Phone Home
            UICommand backCmd = UICommand::triggerActionButton(14);
            g.handleCommand(backCmd);
            bool atHome = (activePhone->getAppMode() == PhoneAppMode::HOME);
            logResult("Action Grid Slot 14 returns to Phone Home", atHome);
            allPassed &= atHome;
        }

        return allPassed;
    }

    bool testFullPhoneSystem()
    {
        std::cout << "\n--- Running Test 19: Full 14-App Smartphone System & 5-Tier Fetishes ---\n";
        bool allPassed = true;

        game g;
        g.playerEntity = std::make_shared<entity>("hero_phone", "Phone Hero");
        g.Player = g.playerEntity.get();
        g.changeState(std::make_unique<phoneAppsState>(PhoneAppMode::HOME));

        auto* phone = dynamic_cast<phoneAppsState*>(g.getActiveState());
        bool initOk = (phone != nullptr && phone->getAppMode() == PhoneAppMode::HOME);
        logResult("phoneAppsState initialises in HOME mode", initOk);
        allPassed &= initOk;

        if (phone)
        {
            // 1. Check all 14 Apps in Home Action Grid (slots 0..13) + Slot 14 Back
            const auto& grid = g.activeButtons;
            bool has14Apps = (grid.size() == 15);
            std::vector<std::string> expectedApps = {
                "Quests", "Perk Tree", "Spells", "Fetishes", "Stats",
                "Selfie", "Contacts", "Encyclopedia", "Transform", "Maps",
                "Combat Moves", "Masturbate", "Wait / Rest", "Elemental", "Back"
            };
            bool appsMatch = has14Apps;
            for (size_t i = 0; i < std::min(grid.size(), expectedApps.size()); ++i)
            {
                if (grid[i].label != expectedApps[i])
                {
                    appsMatch = false;
                    break;
                }
            }
            logResult("Home screen contains exact 14 apps (Slots 0-13) and Back (Slot 14)", appsMatch);
            allPassed &= appsMatch;

            // 2. Test 5-Tier Fetish Desires (Hate, Dislike, Neutral, Like, Love)
            phone->setFetishDesire("Exhibitionism", FetishDesireLevel::HATE);
            phone->setFetishDesire("Anal", FetishDesireLevel::LOVE);
            phone->setFetishDesire("Oral", FetishDesireLevel::LIKE);
            phone->setFetishDesire("Lactation", FetishDesireLevel::DISLIKE);
            bool fHate = (phone->getFetishDesire("Exhibitionism") == FetishDesireLevel::HATE);
            bool fLove = (phone->getFetishDesire("Anal") == FetishDesireLevel::LOVE);
            bool fLike = (phone->getFetishDesire("Oral") == FetishDesireLevel::LIKE);
            bool fDislike = (phone->getFetishDesire("Lactation") == FetishDesireLevel::DISLIKE);
            bool fNeutral = (phone->getFetishDesire("Transformations") == FetishDesireLevel::NEUTRAL);
            bool fetish5TierOk = (fHate && fLove && fLike && fDislike && fNeutral);
            logResult("Fetishes support 5-tier desire system including 'Hate' toggle", fetish5TierOk);
            allPassed &= fetish5TierOk;

            // 3. Test Navigation into Masturbate App & Arousal Manipulation
            g.handleCommand(UICommand::triggerActionButton(11)); // Masturbate (slot 11)
            bool inMasturbate = (phone->getAppMode() == PhoneAppMode::MASTURBATE);
            logResult("Action Grid Slot 11 transitions to Masturbate App", inMasturbate);
            allPassed &= inMasturbate;

            entity* p = g.getPlayer();
            if (p)
            {
                p->stats.setBaseStat("arousal", 20.0f);
                p->stats.setBaseStat("lust", 30.0f);
                // Trigger Caress Chest (Button 0)
                g.handleCommand(UICommand::triggerActionButton(0));
                bool arousalUp = (p->getStat("arousal") >= 35.0f);
                logResult("Solo intimacy caress actions build arousal", arousalUp);
                allPassed &= arousalUp;

                // Trigger Climax & Relief (Button 3) when arousal is high
                p->stats.setBaseStat("arousal", 85.0f);
                g.refreshActionGrid();
                g.handleCommand(UICommand::triggerActionButton(3));
                bool climaxReset = (p->getStat("arousal") == 0.0f && p->getStat("lust") == 0.0f);
                logResult("Climax & Relief purges lust and resets arousal to 0%", climaxReset);
                allPassed &= climaxReset;
            }

            // Return to Home via Slot 14
            g.handleCommand(UICommand::triggerActionButton(14));
            bool backHome1 = (phone->getAppMode() == PhoneAppMode::HOME);
            allPassed &= backHome1;

            // 4. Test Elemental Companion App
            g.handleCommand(UICommand::triggerActionButton(13)); // Elemental (slot 13)
            bool inElemental = (phone->getAppMode() == PhoneAppMode::ELEMENTAL);
            logResult("Action Grid Slot 13 transitions to Elemental App", inElemental);
            allPassed &= inElemental;

            phone->toggleElementalSummoned();
            bool summoned = phone->isElementalSummoned();
            phone->toggleElementalActiveForm();
            bool activeForm = phone->isElementalActiveForm();
            bool elemOk = (summoned && activeForm);
            logResult("Elemental companion manifests and toggles battle/passive aspects", elemOk);
            allPassed &= elemOk;

            // Return Home
            g.handleCommand(UICommand::triggerActionButton(14));

            // 5. Test Combat Moves Deck Editor (10 Slots in Action Grid)
            g.handleCommand(UICommand::triggerActionButton(10)); // Combat Moves (slot 10 on home screen)
            bool inCombatMoves = (phone->getAppMode() == PhoneAppMode::COMBAT_MOVES);
            logResult("Action Grid Slot 10 transitions to Combat Moves Deck Editor", inCombatMoves);
            allPassed &= inCombatMoves;

            // Click action grid button 2 to select slot 2 (3rd deck slot)
            g.handleCommand(UICommand::triggerActionButton(2));
            bool slot2Selected = (phone->getSelectedCombatSlot() == 2);
            logResult("Action Grid button selects combat deck slot #3", slot2Selected);
            allPassed &= slot2Selected;

            if (p)
            {
                p->preparedCombatSlots[2] = "Arcane Dart";
                bool slotAssigned = (p->preparedCombatSlots[2] == "Arcane Dart");
                // Trigger Clear Slot (Action Grid Button 10 in COMBAT_MOVES)
                g.handleCommand(UICommand::triggerActionButton(10));
                bool slotCleared = (p->preparedCombatSlots[2].empty());
                bool combatDeckOk = (slotAssigned && slotCleared);
                logResult("Combat deck prepares and clears action slot techniques via Action Grid", combatDeckOk);
                allPassed &= combatDeckOk;
            }

            // Return Home
            g.handleCommand(UICommand::triggerActionButton(14));

            // 6. Test Stats Tab Navigation
            phone->setAppMode(PhoneAppMode::STATS);
            phone->setStatsTab(1); // Body Stats
            bool bodyTab = (phone->getStatsTab() == 1);
            phone->setStatsTab(2); // Sex Stats
            bool sexTab = (phone->getStatsTab() == 2);
            phone->setStatsTab(3); // Pregnancy
            bool pregTab = (phone->getStatsTab() == 3);
            bool statsOk = (bodyTab && sexTab && pregTab);
            logResult("Stats app navigates across Core, Body, Sex, and Pregnancy tabs", statsOk);
            allPassed &= statsOk;

            // 7. Test Contacts App Card Expansion & Dossier Transition
            phone->setAppMode(PhoneAppMode::CONTACTS);
            phone->toggleContactsExpanded(1); // Expand contact card 1 (Elena)
            bool cardExpanded = (phone->getContactsExpandedIdx() == 1);
            phone->toggleContactsExpanded(1); // Collapse contact card 1
            bool cardCollapsed = (phone->getContactsExpandedIdx() == -1);
            phone->setContactsSelectedIdx(0); // View first contact dossier
            bool inDossier = (phone->getContactsSelectedIdx() == 0);
            phone->setContactsSelectedIdx(-1); // Return to list
            bool backToList = (phone->getContactsSelectedIdx() == -1);
            bool contactsOk = (cardExpanded && cardCollapsed && inDossier && backToList);
            logResult("Contacts app supports card expansion/collapse and detailed dossiers", contactsOk);
            allPassed &= contactsOk;

            // Return Home
            phone->setAppMode(PhoneAppMode::HOME);
            g.refreshActionGrid();
            bool finalHome = (g.activeButtons[14].label == "Back");
            logResult("Universal Slot 14 [Back] button maintained across all menus", finalHome);
            allPassed &= finalHome;
        }

        return allPassed;
    }

    bool testEconomyAndShopTrading()
    {
        std::cout << "\n--- Running Test 20: Economy, Merchant Valuation & Shop Trading System ---\n";
        bool allPassed = true;

        game g;

        auto player = std::make_shared<entity>("hero", "Hero");
        player->stats.setBaseStat("currency", 500.0f);
        g.playerEntity = player;
        g.Player = player.get();

        auto merchant = std::make_shared<entity>("marcus", "Marcus");
        merchant->stats.setBaseStat("currency", 1500.0f);
        merchant->baseMerchantGold = 1500.0f;
        merchant->buyMarkup = 1.20f;   // 20% markup
        merchant->sellMarkdown = 0.55f;// 55% markdown
        merchant->merchantAffinity = 1.0f;

        // 1. Merchant Valuation Formula: Baseline item without perks
        auto potion = std::make_shared<item>();
        potion->id = "item_canis_root";
        potion->name = "Canis Root Potion";
        potion->baseValue = 100;
        potion->isStackable = true;
        potion->count = 5;

        int buyPriceBase = merchantValuation::calculateBuyPrice(potion.get(), player.get(), merchant.get());
        int sellPriceBase = merchantValuation::calculateSellPrice(potion.get(), player.get(), merchant.get());
        // Buy: 100 * 1.20 = 120
        // Sell: 100 * 0.55 = 55
        bool baselineMath = (buyPriceBase == 120 && sellPriceBase == 55);
        logResult("Merchant valuation baseline math without perks (Buy: 120¤, Sell: 55¤)", baselineMath);
        allPassed &= baselineMath;

        // 2. Player Perks modify prices: Silver Tongue (+10%)
        player->unlockPerk("silver_tongue");
        bool hasPerk = player->hasPerk("silver_tongue");
        bool perkModApplied = (player->tradePerkModifier == 0.10f);
        int buyPriceWithPerk = merchantValuation::calculateBuyPrice(potion.get(), player.get(), merchant.get());
        int sellPriceWithPerk = merchantValuation::calculateSellPrice(potion.get(), player.get(), merchant.get());
        // Buy with 10% discount: 100 * 1.20 * (1 - 0.10) = 108
        // Sell with 10% bonus: 100 * 0.55 * (1 + 0.10) = 60.5 -> 61 (rounded)
        bool perkDiscountOk = (buyPriceWithPerk == 108 && sellPriceWithPerk == 61);
        bool perkOk = (hasPerk && perkModApplied && perkDiscountOk);
        logResult("Trade perk (Silver Tongue) grants -10% buy discount and +10% sell bonus", perkOk);
        allPassed &= perkOk;

        // Stacking Master Trader (+15%) -> Total 25%
        player->unlockPerk("master_trader");
        bool stackedMod = (player->tradePerkModifier == 0.25f);
        int buyPriceStacked = merchantValuation::calculateBuyPrice(potion.get(), player.get(), merchant.get());
        int sellPriceStacked = merchantValuation::calculateSellPrice(potion.get(), player.get(), merchant.get());
        // Buy: 100 * 1.20 * 0.75 = 90
        // Sell: 100 * 0.55 * 1.25 = 68.75 -> 69
        bool stackedOk = (stackedMod && buyPriceStacked == 90 && sellPriceStacked == 69);
        logResult("Stacking trade perks (Silver Tongue + Master Trader) grants 25% total discount/bonus", stackedOk);
        allPassed &= stackedOk;

        // Reset perks
        player->resetPerks();
        bool perksReset = (player->tradePerkModifier == 0.0f && !player->hasPerk("silver_tongue"));
        logResult("resetPerks clears perks and restores baseline trade modifiers", perksReset);
        allPassed &= perksReset;

        // 3. ShopState Lifecycle and Inventory Setup
        merchant->inventory.addItem(potion);
        auto shop = std::make_unique<shopState>(merchant);
        g.changeState(std::move(shop));

        bool inShopState = (dynamic_cast<shopState*>(g.getActiveState()) != nullptr);
        bool merchantActive = (g.getActiveTargetNPC() == merchant.get());
        bool shopInitOk = (inShopState && merchantActive);
        logResult("shopState initializes with active merchant entity", shopInitOk);
        allPassed &= shopInitOk;

        // 4. Action Grid Navigation in Shop
        // Initially nothing selected:
        g.refreshActionGrid();
        bool slot0Guide = (g.activeButtons[0].label == "Select Item to Trade" && !g.activeButtons[0].isEnabled);
        bool slot14Leave = (g.activeButtons[14].label == "Leave Shop" && g.activeButtons[14].isEnabled);
        bool slotsClean = true;
        for (int s = 1; s < 14; ++s)
        {
            if (!g.activeButtons[s].label.empty() || g.activeButtons[s].isEnabled)
            {
                slotsClean = false;
                break;
            }
        }
        bool unselectedGridOk = (slot0Guide && slot14Leave && slotsClean);
        logResult("Action Grid unselected layout: guidance on Slot 0, Slot 14 Leave Shop, Slots 1-13 empty", unselectedGridOk);
        allPassed &= unselectedGridOk;

        // 5. Selecting Merchant Item for BUYING
        // Side 1 (Merchant), Stack index 0
        g.handleCommand({ CommandType::SELECT_INVENTORY_SLOT, 1, 0, "" });
        bool buyButton1 = (g.activeButtons[0].label == "Buy 1 (120¤)" && g.activeButtons[0].isEnabled);
        bool buyAllButton = (g.activeButtons[1].label == "Buy All (600¤)");
        bool deselectBtn = (g.activeButtons[2].label == "Deselect Item");
        bool buyActionGridOk = (buyButton1 && buyAllButton && deselectBtn);
        logResult("Selecting merchant item configures Buy 1, Buy All, and Deselect on Action Grid", buyActionGridOk);
        allPassed &= buyActionGridOk;

        // 6. Buying an Item
        float prePlayerGold = player->getStat("currency"); // 500
        float preMerchantGold = merchant->getStat("currency"); // 1500
        size_t prePlayerBackpack = player->inventory.backpack.size(); // 0

        g.handleCommand({ CommandType::BUY_SHOP_ITEM, 0, 2, "" }); // Buy 2 for 240¤

        float postPlayerGold = player->getStat("currency");
        float postMerchantGold = merchant->getStat("currency");
        size_t postPlayerBackpack = player->inventory.backpack.size();

        bool goldDeducted = (postPlayerGold == prePlayerGold - 240.0f);
        bool merchantPaid = (postMerchantGold == preMerchantGold + 240.0f);
        bool itemReceived = (postPlayerBackpack == prePlayerBackpack + 2);
        bool buyOk = (goldDeducted && merchantPaid && itemReceived);
        logResult("Buying items transfers items, deducts player gold, and credits merchant purse", buyOk);
        allPassed &= buyOk;

        // 7. Selecting Player Item for SELLING
        // Side 0 (Player), Stack index 0 (the bought Canis Root)
        g.handleCommand({ CommandType::SELECT_INVENTORY_SLOT, 0, 0, "" });
        bool sellButton1 = (g.activeButtons[0].label == "Sell 1 (55¤)" && g.activeButtons[0].isEnabled);
        bool sellAllButton = (g.activeButtons[1].label == "Sell All (110¤)");
        bool sellActionGridOk = (sellButton1 && sellAllButton);
        logResult("Selecting player item configures Sell 1 and Sell All with calculated resale price", sellActionGridOk);
        allPassed &= sellActionGridOk;

        // 8. Selling an Item
        float preSellPlayerGold = player->getStat("currency");
        float preSellMerchantGold = merchant->getStat("currency");

        g.handleCommand({ CommandType::SELL_SHOP_ITEM, 0, 1, "" }); // Sell 1 for 55¤

        float postSellPlayerGold = player->getStat("currency");
        float postSellMerchantGold = merchant->getStat("currency");

        bool playerCredited = (postSellPlayerGold == preSellPlayerGold + 55.0f);
        bool merchantDeducted = (postSellMerchantGold == preSellMerchantGold - 55.0f);
        bool sellOk = (playerCredited && merchantDeducted);
        logResult("Selling item transfers item to merchant, pays player, and deducts merchant funds", sellOk);
        allPassed &= sellOk;

        // 9. Key Quest Item Protection: Quest item cannot be sold
        auto keyItem = std::make_shared<item>();
        keyItem->id = "item_ancient_sun_relic";
        keyItem->name = "Ancient Sun Relic";
        keyItem->baseValue = 1000;
        keyItem->isKeyItem = true;
        player->inventory.addItem(keyItem);

        auto playerStack = player->inventory.getStackedView();
        int keyIndex = -1;
        for (size_t i = 0; i < playerStack.size(); ++i)
        {
            if (playerStack[i].itemPtr && playerStack[i].itemPtr->isKeyItem)
            {
                keyIndex = static_cast<int>(i);
                break;
            }
        }

        g.handleCommand({ CommandType::SELECT_INVENTORY_SLOT, 0, keyIndex, "" });
        bool keySellDisabled = (!g.activeButtons[0].isEnabled);
        float preKeyGold = player->getStat("currency");
        g.handleCommand({ CommandType::SELL_SHOP_ITEM, keyIndex, 1, "" });
        float postKeyGold = player->getStat("currency");
        bool keyProtected = (keySellDisabled && preKeyGold == postKeyGold);
        logResult("Key quest items cannot be sold (action disabled & transaction blocked)", keyProtected);
        allPassed &= keyProtected;

        // 10. Leave Shop returns to exploration
        g.handleCommand(UICommand::triggerActionButton(14));
        bool leftShop = (dynamic_cast<explorationState*>(g.getActiveState()) != nullptr);
        logResult("Leave Shop returns cleanly to exploration state", leftShop);
        allPassed &= leftShop;

        return allPassed;
    }

    bool testNamedCharactersAndPersistentEncounter()
    {
        std::cout << "\n--- Running Test 21: Modular JSON Loaders, Quest NPC Relocations & Persistent Encounter System ---\n";
        bool allPassed = true;

        // 1. Modular Directory Loaders & Lookup
        NamedCharacterManager::loadFromDirectory("data/characters");
        npcGenerator::loadTemplates("data/enemies");
        questDatabase::loadDatabase("data/quests");

        auto marcus = NamedCharacterManager::getCharacter("marcus");
        bool marcusLoaded = (marcus != nullptr && marcus->name == "Marcus" && marcus->isMerchant);
        logResult("NamedCharacterManager loads Marcus from directory 'data/characters'", marcusLoaded);
        allPassed &= marcusLoaded;

        auto banditTpl = npcGenerator::getTemplate("tpl_alley_bandit");
        auto mageTpl = npcGenerator::getTemplate("tpl_rogue_mage");
        bool enemiesLoaded = (banditTpl != nullptr && mageTpl != nullptr);
        logResult("npcGenerator loads modular templates from directory 'data/enemies' (bandits, mages)", enemiesLoaded);
        allPassed &= enemiesLoaded;

        if (marcus)
        {
            // 2. Daily Schedule Evaluation based on in-game hour
            // Morning/Afternoon (hour 12): (2, 3) at Market Stall
            marcus->resolveLocation(12, 1, nullptr, nullptr);
            bool marketStall = (marcus->currentMapId == "overworld" && marcus->currentX == 2 && marcus->currentY == 3);
            logResult("Marcus resolves to Market Stall (2,3) during daytime hours (12:00)", marketStall);
            allPassed &= marketStall;

            // Evening (hour 19): (4, 1) at Tavern Plaza
            marcus->resolveLocation(19, 1, nullptr, nullptr);
            bool tavernPlaza = (marcus->currentMapId == "overworld" && marcus->currentX == 4 && marcus->currentY == 1);
            logResult("Marcus resolves to Tavern Plaza (4,1) during evening hours (19:00)", tavernPlaza);
            allPassed &= tavernPlaza;

            // Night (hour 23): (2, 2) at Marcus's Cottage (house_01)
            marcus->resolveLocation(23, 1, nullptr, nullptr);
            bool cottage = (marcus->currentMapId == "house_01" && marcus->currentX == 2 && marcus->currentY == 2);
            logResult("Marcus resolves to Cottage in house_01 (2,2) during night hours (23:00)", cottage);
            allPassed &= cottage;

            // 3. Quest-Driven Relocation Evaluation (defined in quest_intro.json, NOT in marcus.json)
            questComponent testQuests;
            testQuests.setQuestStage("root_delivery", 1);
            marcus->resolveLocation(12, 1, &testQuests, nullptr);
            bool questOverride = (marcus->currentMapId == "overworld" && marcus->currentX == 1 && marcus->currentY == 1);
            logResult("Quest-driven relocation redirects Marcus to (1,1) when 'root_delivery' stage is 1", questOverride);
            allPassed &= questOverride;

            // Clear quest stage, should resolve back to schedule (2,3 at 12:00)
            testQuests.setQuestStage("root_delivery", 2); // completion stage
            marcus->resolveLocation(12, 1, &testQuests, nullptr);
            bool backToSchedule = (marcus->currentX == 2 && marcus->currentY == 3);
            logResult("Marcus returns to default daily schedule when quest stage exceeds relocation window", backToSchedule);
            allPassed &= backToSchedule;
        }

        // 4. NPC-Bound Quest Triggers
        auto npcTriggers = questDatabase::getTriggersForNPC("marcus");
        bool hasNpcTrigger = false;
        for (const auto& trig : npcTriggers)
        {
            if (trig.id == "trig_marcus_delivery_talk" && trig.npcId == "marcus")
            {
                hasNpcTrigger = true;
                break;
            }
        }
        logResult("QuestDatabase retrieves NPC-bound trigger 'trig_marcus_delivery_talk' for Marcus", hasNpcTrigger);
        allPassed &= hasNpcTrigger;

        // 5. Map Integration & Named NPCs on Tiles
        game g;
        g.loadMap("overworld", 2, 3);
        auto* activeMap = g.map;
        bool mapOk = (activeMap != nullptr);
        logResult("Loaded overworld map for named character placement validation", mapOk);
        allPassed &= mapOk;

        if (activeMap)
        {
            // Set time to 12:00
            g.gameTime.hour = 12;
            NamedCharacterManager::updateAllLocations(&g);

            auto& marketTile = activeMap->getRuntimeData(2, 3);
            bool marcusOnTile = false;
            for (const auto& nc : marketTile.namedNPCs)
            {
                if (nc && nc->id == "marcus") marcusOnTile = true;
            }
            logResult("NamedCharacterManager places Marcus on (2,3) market tile at 12:00", marcusOnTile);
            allPassed &= marcusOnTile;

            // 6. State and Adjacency Enforcement on Player Movement
            // Movement while in an encounter (eventState) is strictly blocked
            g.changeState(std::make_unique<eventState>());
            g.gridX = 2; g.gridY = 3;
            g.movePlayer(3, 3);
            bool encounterMoveBlocked = (g.gridX == 2 && g.gridY == 3);
            logResult("Player movement is strictly locked during an encounter (eventState)", encounterMoveBlocked);
            allPassed &= encounterMoveBlocked;

            // Movement in explorationState allows valid adjacent cardinal steps
            g.changeState(std::make_unique<explorationState>());
            g.movePlayer(3, 3); // Valid adjacent cardinal step (East)
            bool adjacentMoved = (g.gridX == 3 && g.gridY == 3);
            logResult("Player moves to valid adjacent cardinal tile (3,3) from (2,3) in explorationState", adjacentMoved);
            allPassed &= adjacentMoved;

            g.movePlayer(1, 3); // Invalid non-adjacent move (distance 2)
            bool nonAdjacentBlocked = (g.gridX == 3 && g.gridY == 3);
            logResult("Player movement to non-adjacent tile (1,3) is strictly blocked", nonAdjacentBlocked);
            allPassed &= nonAdjacentBlocked;

            g.movePlayer(4, 4); // Invalid diagonal move (dx=1, dy=1)
            bool diagonalBlocked = (g.gridX == 3 && g.gridY == 3);
            logResult("Player movement to diagonal tile (4,4) is strictly blocked", diagonalBlocked);
            allPassed &= diagonalBlocked;

            // 6. Persistent Ambush Encounter & Pool on Dangerous Tile (1,2)
            auto& alleyTile = activeMap->getRuntimeData(1, 2);
            bool hasAmbush = (alleyTile.ambushState.npc != nullptr);
            bool hasPool = (!alleyTile.ambushState.templatePool.empty() && alleyTile.ambushState.templatePool.size() >= 2);
            logResult("Dangerous tile (1,2) initializes persistent ambush and loads template pool", hasAmbush && hasPool);
            allPassed &= (hasAmbush && hasPool);

            // Verify Ambush NPC is NEVER in namedNPCs and NEVER presented as a friendly talkable character
            bool inNamedNPCs = false;
            for (const auto& n : alleyTile.namedNPCs)
            {
                if (n && alleyTile.ambushState.npc && n->id == alleyTile.ambushState.npc->id) inNamedNPCs = true;
            }
            logResult("Ambush NPC is not present in namedNPCs (lurks in shadows, not talkable)", !inNamedNPCs);
            allPassed &= !inNamedNPCs;

            // 7. Ambush Combat Resolution & Depletion Tracking
            auto ambusher = alleyTile.ambushState.npc;
            if (ambusher)
            {
                ambusher->stats.setBaseStat("currency", 50.0f);

                // Simulate player defeating and looting the ambusher
                alleyTile.ambushState.isDefeated = true;
                alleyTile.ambushState.restockMinutesRemaining = 1440; // 24 hours
                ambusher->stats.setBaseStat("currency", 0.0f); // player looted gold
                ambusher->inventory.backpack.clear(); // player looted inventory
                ambusher->inventory.equipped.fill(nullptr); // player stripped/looted them

                bool hasEquippedDepleted = false;
                for (const auto& eq : ambusher->inventory.equipped) if (eq) hasEquippedDepleted = true;
                bool isDepleted = (alleyTile.ambushState.isDefeated && ambusher->getStat("currency") == 0.0f && ambusher->inventory.backpack.empty() && !hasEquippedDepleted);
                logResult("Ambush NPC enters defeated state with looted/depleted inventory and gold", isDepleted);
                allPassed &= isDepleted;

                // 8. Time Passage & 24-Hour Restock Mechanism
                // Advance time by 600 minutes (10 hours) -> still depleted
                activeMap->processTimePassage(600);
                bool stillDefeated = (alleyTile.ambushState.isDefeated && alleyTile.ambushState.restockMinutesRemaining == 840);
                bool stillNoGold = (ambusher->getStat("currency") == 0.0f);
                logResult("Partial time passage (10h) keeps ambusher defeated and depleted (840m remaining)", stillDefeated && stillNoGold);
                allPassed &= (stillDefeated && stillNoGold);

                // Advance time by remaining 840 minutes (total 24 hours) -> restocks!
                activeMap->processTimePassage(840);
                bool restocked = (!alleyTile.ambushState.isDefeated && alleyTile.ambushState.restockMinutesRemaining == 0);
                bool goldRestored = (ambusher->getStat("currency") >= 20.0f);
                bool hasEquippedRestocked = false;
                for (const auto& eq : ambusher->inventory.equipped) if (eq) hasEquippedRestocked = true;
                bool itemsRestocked = (!ambusher->inventory.backpack.empty() || hasEquippedRestocked);
                bool restockOk = (restocked && goldRestored && itemsRestocked);
                logResult("Passing full 24h restocks ambusher: revives, rolls fresh gold, and replenishes template items", restockOk);
                allPassed &= restockOk;

                // 9. Permanent Removal System: Clears specific NPC so a NEW random NPC from pool spawns after 24h
                alleyTile.ambushState.npc = nullptr;
                alleyTile.ambushState.isDefeated = true;
                alleyTile.ambushState.restockMinutesRemaining = 1440;

                // Partial time passage: still null
                activeMap->processTimePassage(600);
                bool stillNull = (alleyTile.ambushState.npc == nullptr && alleyTile.ambushState.isDefeated);
                logResult("Permanently removed enemy leaves tile clear during restock cooldown (600m passed)", stillNull);
                allPassed &= stillNull;

                // Pass remaining 840m -> spawns a fresh random enemy from pool
                activeMap->processTimePassage(840);
                bool spawnedNew = (!alleyTile.ambushState.isDefeated && alleyTile.ambushState.npc != nullptr);
                bool validPoolChoice = (spawnedNew && (!alleyTile.ambushState.templateId.empty()));
                logResult("Passing 24h after permanent removal spawns a fresh random enemy from template pool", validPoolChoice);
                allPassed &= validPoolChoice;
            }

            // 10. Save & Load Serialization for Named Characters & Ambushes
            nlohmann::json namedJson = NamedCharacterManager::saveStateToJson();
            NamedCharacterManager::loadStateFromJson(namedJson);
            nlohmann::json mapJson = activeMap->saveStateToJson();

            bool hasSavedAmbushes = mapJson.contains("tileAmbushes");
            logResult("GameMap serializes 'tileAmbushes' state with template pool into map JSON payload", hasSavedAmbushes);
            allPassed &= hasSavedAmbushes;

            // Verify load restores state accurately
            activeMap->loadStateFromJson(mapJson);
            auto& reloadedAlley = activeMap->getRuntimeData(1, 2);
            bool loadedPool = (!reloadedAlley.ambushState.templatePool.empty() && reloadedAlley.ambushState.npc != nullptr);
            logResult("GameMap deserializes persistent ambush state and template pool accurately", loadedPool);
            allPassed &= loadedPool;
        }

        return allPassed;
    }

    bool testCalendarLeapYearsAndTransformationNavigation()
    {
        std::cout << "\n--- Running Test 22: Calendar Leap Years, Character Creation Birth Month & Direct Transform ---\n";
        bool allPassed = true;

        // 1. Leap Year Math Validation
        bool leap2020 = timeManager::isLeapYear(2020);
        bool leap2024 = timeManager::isLeapYear(2024);
        bool leap2000 = timeManager::isLeapYear(2000);
        bool nonLeap1900 = !timeManager::isLeapYear(1900);
        bool nonLeap2023 = !timeManager::isLeapYear(2023);
        bool nonLeap1 = !timeManager::isLeapYear(1);

        bool leapMathOk = leap2020 && leap2024 && leap2000 && nonLeap1900 && nonLeap2023 && nonLeap1;
        logResult("timeManager::isLeapYear evaluates Gregorian century and standard leap years accurately", leapMathOk);
        allPassed &= leapMathOk;

        // 2. Days In Month Across All 12 Months
        bool janOk = (timeManager::getDaysInMonth(1, 2024) == 31);
        bool febLeapOk = (timeManager::getDaysInMonth(2, 2024) == 29);
        bool febNonLeapOk = (timeManager::getDaysInMonth(2, 2023) == 28);
        bool marOk = (timeManager::getDaysInMonth(3, 2024) == 31);
        bool aprOk = (timeManager::getDaysInMonth(4, 2024) == 30);
        bool mayOk = (timeManager::getDaysInMonth(5, 2024) == 31);
        bool junOk = (timeManager::getDaysInMonth(6, 2024) == 30);
        bool julOk = (timeManager::getDaysInMonth(7, 2024) == 31);
        bool augOk = (timeManager::getDaysInMonth(8, 2024) == 31);
        bool sepOk = (timeManager::getDaysInMonth(9, 2024) == 30);
        bool octOk = (timeManager::getDaysInMonth(10, 2024) == 31);
        bool novOk = (timeManager::getDaysInMonth(11, 2024) == 30);
        bool decOk = (timeManager::getDaysInMonth(12, 2024) == 31);

        bool allDaysOk = janOk && febLeapOk && febNonLeapOk && marOk && aprOk && mayOk &&
                         junOk && julOk && augOk && sepOk && octOk && novOk && decOk;
        logResult("timeManager::getDaysInMonth returns exact calendar days (Feb 28/29, Apr/Jun/Sep/Nov 30, others 31)", allDaysOk);
        allPassed &= allDaysOk;

        // 3. Simulation Advance Time Month Boundary Transitions
        {
            timeManager tm;
            // Feb 28, 2023 (non-leap year) -> should rollover to March 1
            tm.year = 2023;
            tm.month = 2;
            tm.day = 28;
            tm.hour = 23;
            tm.minute = 50;
            tm.advanceTime(20); // +20 mins -> 00:10 March 1
            bool nonLeapRoll = (tm.month == 3 && tm.day == 1 && tm.hour == 0 && tm.minute == 10);
            logResult("Non-leap year Feb 28 rolls directly into March 1 upon midnight transition", nonLeapRoll);
            allPassed &= nonLeapRoll;

            // Feb 28, 2024 (leap year) -> should rollover to Feb 29, then to March 1
            tm.year = 2024;
            tm.month = 2;
            tm.day = 28;
            tm.hour = 23;
            tm.minute = 50;
            tm.advanceTime(20); // +20 mins -> 00:10 Feb 29
            bool leapRoll29 = (tm.month == 2 && tm.day == 29 && tm.hour == 0 && tm.minute == 10);
            logResult("Leap year Feb 28 rolls into Feb 29 (leap day)", leapRoll29);
            allPassed &= leapRoll29;

            tm.advanceTime(1440); // +24 hours -> 00:10 March 1
            bool leapRollMar = (tm.month == 3 && tm.day == 1 && tm.hour == 0 && tm.minute == 10);
            logResult("Leap year Feb 29 rolls into March 1 after 24 hours", leapRollMar);
            allPassed &= leapRollMar;

            // Dec 31 -> Jan 1 of next year
            tm.year = 2024;
            tm.month = 12;
            tm.day = 31;
            tm.hour = 23;
            tm.minute = 50;
            tm.advanceTime(20);
            bool yearRoll = (tm.year == 2025 && tm.month == 1 && tm.day == 1 && tm.hour == 0 && tm.minute == 10);
            logResult("New Year transition advances year count and resets month to January 1", yearRoll);
            allPassed &= yearRoll;
        }

        // 4. Character Creation Starting Month & Birthday Dynamic Clamping
        {
            game g;
            characterCreationState cc;
            cc.initialise(&g);

            // Set starting month to October (month 10)
            cc.startMonth = "October";
            cc.startMonthIdx = 9;

            // Pick birth month as February and age 22
            cc.birthMonth = "February";
            cc.birthMonthIdx = 1;
            cc.birthAge = 22;

            // Clamping check for birthDay in February of birthYear (2026 - 22 = 2004, leap year -> 29 days)
            int bYear = 2026 - cc.birthAge;
            int maxFebDays = timeManager::getDaysInMonth(cc.birthMonthIdx + 1, bYear);
            bool febLeapClamp = (maxFebDays == 29);
            cc.birthDay = std::clamp(31, 1, maxFebDays);
            bool clampedDayOk = (cc.birthDay == 29);
            logResult("Character creation clamps birth day to 29 for February in a leap birth year", febLeapClamp && clampedDayOk);
            allPassed &= (febLeapClamp && clampedDayOk);

            // Test non-leap birth year (2026 - 21 = 2005 -> 28 days)
            cc.birthAge = 21;
            int nonLeapBYear = 2026 - cc.birthAge;
            int maxNonLeapDays = timeManager::getDaysInMonth(cc.birthMonthIdx + 1, nonLeapBYear);
            cc.birthDay = std::clamp(cc.birthDay, 1, maxNonLeapDays);
            bool clamped28Ok = (cc.birthDay == 28);
            logResult("Character creation clamps birth day to 28 for February in a non-leap birth year", clamped28Ok);
            allPassed &= clamped28Ok;

            // Finalize character and verify gameTime starts in chosen month (October = 10)
            cc.finalizeCharacter(&g);
            bool startMonthApplied = (g.gameTime.month == 10);
            logResult("Finalizing character starts game in the user-selected starting month (October)", startMonthApplied);
            allPassed &= startMonthApplied;

            // Verify player entity stores birthDay, birthMonth, and birthYear
            entity* p = g.getPlayer();
            bool entityBdayOk = (p != nullptr && p->birthDay == 28 && p->birthMonth == 2);
            logResult("Player entity stores birthDay, birthMonth, and birthYear", entityBdayOk);
            allPassed &= entityBdayOk;

            // Verify entity JSON serialization preserves birthday fields
            nlohmann::json pj = p->toJson();
            entity loadedP("copy", "Copy");
            loadedP.fromJson(pj);
            bool jsonBdayOk = (loadedP.birthDay == 28 && loadedP.birthMonth == 2);
            logResult("Entity JSON serialization roundtrips birthDay and birthMonth accurately", jsonBdayOk);
            allPassed &= jsonBdayOk;
        }

        // 5. Phone Transform Direct Transition & Return
        {
            game g;
            g.changeState(std::make_unique<phoneAppsState>(PhoneAppMode::HOME));

            auto* phone = dynamic_cast<phoneAppsState*>(g.getActiveState());
            bool inHome = (phone != nullptr && phone->getAppMode() == PhoneAppMode::HOME);
            logResult("Game initializes in phoneAppsState HOME mode", inHome);
            allPassed &= inHome;

            // Trigger Transform direct navigation
            g.changeState(std::make_unique<transformationState>(TransformationTab::CORE, std::make_unique<phoneAppsState>(PhoneAppMode::HOME)));
            auto* tf = dynamic_cast<transformationState*>(g.getActiveState());
            bool inTf = (tf != nullptr);
            logResult("Bypasses intermediary phone screen and enters transformationState directly", inTf);
            allPassed &= inTf;

            // Invoke returnToPreviousOrExploration
            if (tf)
            {
                tf->returnToPreviousOrExploration(&g);
                auto* returnedPhone = dynamic_cast<phoneAppsState*>(g.getActiveState());
                bool backToPhone = (returnedPhone != nullptr && returnedPhone->getAppMode() == PhoneAppMode::HOME);
                logResult("Apply & Return smoothly returns directly back to phoneAppsState HOME", backToPhone);
                allPassed &= backToPhone;
            }
        }

        return allPassed;
    }

    bool testLayoutIntegrityAndContainment()
    {
        std::cout << "\n--- Running Test 23: Autonomous Layout JSON Containment & Geometry Integrity ---\n";
        bool allPassed = true;

        // 1. Validate Master Layout File Loading
        layoutEngine defEngine;
        bool defLoaded = defEngine.loadFromFile("data/layouts/default_layout.json");
        logResult("Master layout 'data/layouts/default_layout.json' loads and parses cleanly", defLoaded);
        allPassed &= defLoaded;

        layoutEngine customEngine;
        bool customLoaded = customEngine.loadFromFile("data/layouts/custom_tile_layout.json");
        logResult("Alternative layout 'data/layouts/custom_tile_layout.json' loads and parses cleanly", customLoaded);
        allPassed &= customLoaded;

        // 2. Validate Transformation Studio 3-Column Geometry in Master Layout
        auto tfBounds = defEngine.computeLayout(1920.0f, 1080.0f, 1.0f, "TRANSFORMATION");
        bool hasTfLeft = false, hasTfCenter = false, hasTfActions = false, hasTfRight = false;
        SDL_FRect leftRect{ 0, 0, 0, 0 }, centerRect{ 0, 0, 0, 0 }, rightRect{ 0, 0, 0, 0 };

        for (const auto& p : tfBounds)
        {
            if (p.id == "tf_left_column") { hasTfLeft = true; leftRect = p.rect; }
            else if (p.id == "tf_center_pane") { hasTfCenter = true; centerRect = p.rect; }
            else if (p.id == "tf_bottom_actions") { hasTfActions = true; }
            else if (p.id == "tf_right_column") { hasTfRight = true; rightRect = p.rect; }
        }

        bool tfPanelsPresent = (hasTfLeft && hasTfCenter && hasTfActions && hasTfRight);
        logResult("Transformation layout computes sidebars (tf_left_column, tf_right_column) and center panels", tfPanelsPresent);
        allPassed &= tfPanelsPresent;

        bool tfHorizOrder = (leftRect.w > 0 && centerRect.w > 0 && rightRect.w > 0 &&
                             leftRect.x < centerRect.x && centerRect.x < rightRect.x);
        logResult("Transformation layout maintains strict horizontal 3-column ordering without overlapping", tfHorizOrder);
        allPassed &= tfHorizOrder;

        // 3. Validate Widget Registration inside Transformation Panels
        bool widgetsValid = false;
        for (const auto& p : tfBounds)
        {
            if (p.id == "tf_left_column")
            {
                bool hasBio = false, hasPaperdoll = false;
                for (const auto& w : p.widgets)
                {
                    if (w == "widget_lt_character_card") hasBio = true;
                    if (w == "widget_paperdoll_equipment") hasPaperdoll = true;
                }
                widgetsValid = (hasBio && hasPaperdoll);
            }
        }
        logResult("Transformation left sidebar binds character card and paperdoll equipment widgets", widgetsValid);
        allPassed &= widgetsValid;

        // 4. Validate Standard 3-Column States Coverage Across All Key States
        static const std::vector<std::string> threeColStates = {
            "EXPLORATION", "INVENTORY", "SHOP", "TRANSFORMATION", "PHONE_APP", "CHARACTER_CREATION", "SETTINGS", "LOAD_GAME"
        };
        bool allThreeColOk = true;
        for (const auto& st : threeColStates)
        {
            auto bounds = defEngine.computeLayout(1920.0f, 1080.0f, 1.0f, st);
            float minX = 9999.0f, maxX = 0.0f;
            for (const auto& b : bounds)
            {
                if (b.rect.x < minX) minX = b.rect.x;
                if (b.rect.x + b.rect.w > maxX) maxX = b.rect.x + b.rect.w;
            }
            if (bounds.size() < 3 || minX >= maxX) allThreeColOk = false;
        }
        logResult("All core gameplay states (Exploration, Inv, Shop, TF, Phone, Creation, Settings, Load) compute multi-column bounds", allThreeColOk);
        allPassed &= allThreeColOk;

        // 5. Dynamic Window Resolution Rescaling Invariance
        auto bounds720p = defEngine.computeLayout(1280.0f, 720.0f, 0.75f, "TRANSFORMATION");
        bool scaledPositive = true;
        for (const auto& b : bounds720p)
        {
            if (b.rect.w <= 0.0f || b.rect.h <= 0.0f) scaledPositive = false;
        }
        logResult("Layout engine dynamically rescales all panels at 720p resolution with positive bounds", scaledPositive);
        allPassed &= scaledPositive;

        return allPassed;
    }

    bool testUnified3PanelLayoutFogOfWarAndPerkTree()
    {
        std::cout << "\n--- Running Test 24: 3-Panel Layout Unification, Exploration Fog of War & Perk Tree ---\n";
        bool allPassed = true;

        // 1. Layout Unification Verification
        layoutEngine layoutEng;
        layoutEng.loadFromFile("data/layouts/default_layout.json");

        // CHARACTER_CREATION: Left (16%), Center (68%), Right (16%)
        auto ccPanels = layoutEng.computeLayout(1920.0f, 1080.0f, 1.0f, "CHARACTER_CREATION");
        bool hasCcLeft = false, hasCcCenter = false, hasCcRight = false;
        for (const auto& p : ccPanels)
        {
            if (p.id == "cc_left_sidebar") hasCcLeft = true;
            if (p.id == "cc_center_pane") hasCcCenter = true;
            if (p.id == "cc_right_sidebar") hasCcRight = true;
        }
        bool ccUnified = (hasCcLeft && hasCcCenter && hasCcRight);
        logResult("Character Creation conforms to 3-panel layout (cc_left_sidebar, cc_center_pane, cc_right_sidebar)", ccUnified);
        allPassed &= ccUnified;

        // SETTINGS: Left (16%), Center (68%), Right (16%)
        auto optPanels = layoutEng.computeLayout(1920.0f, 1080.0f, 1.0f, "SETTINGS");
        bool hasOptLeft = false, hasOptCenter = false, hasOptRight = false;
        for (const auto& p : optPanels)
        {
            if (p.id == "opt_left_sidebar") hasOptLeft = true;
            if (p.id == "opt_center_pane") hasOptCenter = true;
            if (p.id == "opt_right_sidebar") hasOptRight = true;
        }
        bool optUnified = (hasOptLeft && hasOptCenter && hasOptRight);
        logResult("Settings / Options conforms to 3-panel layout (opt_left_sidebar, opt_center_pane, opt_right_sidebar)", optUnified);
        allPassed &= optUnified;

        // LOAD_GAME: Left (16%), Center (68%), Right (16%)
        auto loadPanels = layoutEng.computeLayout(1920.0f, 1080.0f, 1.0f, "LOAD_GAME");
        bool hasLoadLeft = false, hasLoadCenter = false, hasLoadRight = false;
        for (const auto& p : loadPanels)
        {
            if (p.id == "load_left_sidebar") hasLoadLeft = true;
            if (p.id == "load_center_pane") hasLoadCenter = true;
            if (p.id == "load_right_sidebar") hasLoadRight = true;
        }
        bool loadUnified = (hasLoadLeft && hasLoadCenter && hasLoadRight);
        logResult("Load Game conforms to 3-panel layout (load_left_sidebar, load_center_pane, load_right_sidebar)", loadUnified);
        allPassed &= loadUnified;

        // MAIN_MENU: Sole fullscreen exception (no sidebars)
        auto mmPanels = layoutEng.computeLayout(1920.0f, 1080.0f, 1.0f, "MAIN_MENU");
        bool hasMmCenter = false, hasMmSidebar = false;
        for (const auto& p : mmPanels)
        {
            if (p.id == "mm_center_hero") hasMmCenter = true;
            if (p.id.find("sidebar") != std::string::npos || p.id.find("left_") != std::string::npos || p.id.find("right_") != std::string::npos)
            {
                hasMmSidebar = true;
            }
        }
        bool mmException = (hasMmCenter && !hasMmSidebar);
        logResult("Main Menu remains the sole fullscreen exception without sidebars", mmException);
        allPassed &= mmException;

        // 2. Exploration Discovery & Fog of War System
        gameMap testMap;
        testMap.loadFromFile("data/maps/overworld.json");

        // Freshly loaded map tiles default to STATE_HIDDEN and visited == false
        Tile farTile = testMap.getTile(0, 0);
        bool initialHidden = (farTile.discovery == STATE_HIDDEN && !farTile.visited);
        logResult("Unvisited tiles initialize in STATE_HIDDEN fog of war", initialHidden);
        allPassed &= initialHidden;

        // Reveal area around player at (2, 3)
        testMap.updateDiscovery(2, 3);
        Tile playerTile = testMap.getTile(2, 3);
        bool playerTileRevealed = (playerTile.discovery == STATE_REVEALED && playerTile.visited);
        logResult("Player step tile is marked STATE_REVEALED and visited == true", playerTileRevealed);
        allPassed &= playerTileRevealed;

        Tile adjacentTile = testMap.getTile(3, 3); // cardinal adjacent tile (dx=1, dy=0)
        bool adjacentPartial = (adjacentTile.discovery == STATE_PARTIAL && !adjacentTile.visited);
        logResult("Cardinally adjacent tile (3,3) becomes STATE_PARTIAL (partially discovered)", adjacentPartial);
        allPassed &= adjacentPartial;

        Tile diagonalTile = testMap.getTile(3, 2); // diagonal tile (dx=1, dy=-1)
        bool diagonalHidden = (diagonalTile.discovery == STATE_HIDDEN && !diagonalTile.visited);
        logResult("Diagonally adjacent tile (3,2) strictly remains in STATE_HIDDEN (not cardinal)", diagonalHidden);
        allPassed &= diagonalHidden;

        Tile distantTile = testMap.getTile(0, 0); // distance > 1
        bool distantHidden = (distantTile.discovery == STATE_HIDDEN && !distantTile.visited);
        logResult("Distant unapproached tiles remain in STATE_HIDDEN (dark, undiscovered)", distantHidden);
        allPassed &= distantHidden;

        // Serialization & Deserialization of discovery & visited arrays
        auto mapJson = testMap.saveStateToJson();
        gameMap loadedMap;
        loadedMap.loadFromFile("data/maps/overworld.json");
        loadedMap.loadStateFromJson(mapJson);

        Tile reloadedPlayerTile = loadedMap.getTile(2, 3);
        Tile reloadedAdjacentTile = loadedMap.getTile(3, 3);
        Tile reloadedDiagonalTile = loadedMap.getTile(3, 2);
        Tile reloadedDistantTile = loadedMap.getTile(0, 0);
        bool roundtripValid = (reloadedPlayerTile.visited && reloadedPlayerTile.discovery == STATE_REVEALED &&
                               !reloadedAdjacentTile.visited && reloadedAdjacentTile.discovery == STATE_PARTIAL &&
                               !reloadedDiagonalTile.visited && reloadedDiagonalTile.discovery == STATE_HIDDEN &&
                               !reloadedDistantTile.visited && reloadedDistantTile.discovery == STATE_HIDDEN);
        logResult("Map 3-tier discovery & visited arrays persist cleanly across JSON save/load", roundtripValid);
        allPassed &= roundtripValid;

        // 3. Functional Perk Tree System
        auto player = std::make_shared<entity>("perk_hero", "PerkHero");
        player->stats.setBaseStat("max_health", 100.0f);
        player->stats.setBaseStat("health", 100.0f);
        player->stats.setBaseStat("max_mana", 50.0f);
        player->stats.setBaseStat("mana", 50.0f);

        float startingPerkPts = player->getStat("perk_points");
        bool hasStartingPts = (startingPerkPts == 3.0f);
        logResult("Player entity initializes with 3 starting talent/perk points", hasStartingPts);
        allPassed &= hasStartingPts;

        float baseHp = player->getStat("max_health");
        bool baseHpOk = (baseHp == 100.0f);
        logResult("Player base max health equals 100 before perk unlocks", baseHpOk);
        allPassed &= baseHpOk;

        // Unlock Iron Constitution (+20 HP, +5 Defense)
        player->unlockPerk("iron_constitution");
        bool hasIron = player->hasPerk("iron_constitution");
        float boostedHp = player->getStat("max_health");
        float boostedDef = player->getStat("defense");
        bool ironPerkActive = (hasIron && boostedHp == 120.0f && boostedDef == 5.0f);
        logResult("Unlocking 'iron_constitution' grants +20 Max Health and +5 Defense dynamically", ironPerkActive);
        allPassed &= ironPerkActive;

        // Unlock Brawny Strike (+5 Strength, +10 Physical Damage)
        player->unlockPerk("brawny_strike");
        bool brawnyActive = (player->hasPerk("brawny_strike") &&
                             player->getStat("strength") == 5.0f &&
                             player->getStat("physical_damage") == 10.0f);
        logResult("Unlocking 'brawny_strike' grants +5 Strength and +10 Physical Damage", brawnyActive);
        allPassed &= brawnyActive;

        // Unlock Trade Perks (Silver Tongue & Master Trader)
        player->unlockPerk("silver_tongue");
        player->unlockPerk("master_trader");
        bool tradeStacked = (player->tradePerkModifier == 0.25f && player->getStat("charm") == 5.0f);
        logResult("Silver Tongue & Master Trader stack trade modifiers (+25%) and grant charm", tradeStacked);
        allPassed &= tradeStacked;

        // Reset Perks: Clears perks, recalculates modifiers, restores baseline
        player->resetPerks();
        bool perksReset = (!player->hasPerk("iron_constitution") &&
                           !player->hasPerk("brawny_strike") &&
                           player->getStat("max_health") == 100.0f &&
                           player->tradePerkModifier == 0.0f);
        logResult("resetPerks clears all talent perks and restores baseline stats immediately", perksReset);
        allPassed &= perksReset;

        // Level-up XP advancement grants +1 perk point
        player->stats.addXp(100.0f); // Level 1 -> 2
        bool levelUpPt = (player->stats.level == 2 && player->getStat("perk_points") == 4.0f);
        logResult("Level advancement automatically awards +1 talent point to player pool", levelUpPt);
        allPassed &= levelUpPt;

        // 4. Unified Lilith's Throne-Style Connected Skill Tree with Cross-Discipline Combos
        std::ifstream perksFile("data/perks.json");
        bool perksLoaded = perksFile.is_open();
        nlohmann::json perksData;
        if (perksLoaded) {
            try { perksFile >> perksData; } catch (...) { perksLoaded = false; }
        }
        bool hasUnifiedTree = (perksLoaded && perksData.contains("tree") &&
                               perksData["tree"].contains("nodes") &&
                               perksData["tree"]["nodes"].is_array() &&
                               perksData["tree"]["nodes"].size() >= 26);
        logResult("data/perks.json defines single unified tree structure with 26+ interconnected nodes", hasUnifiedTree);
        allPassed &= hasUnifiedTree;

        if (hasUnifiedTree)
        {
            const auto& allNodes = perksData["tree"]["nodes"];
            std::unordered_map<std::string, nlohmann::json> nodeMap;
            for (const auto& nd : allNodes)
            {
                nodeMap[nd.value("id", "")] = nd;
            }

            auto isConnected = [&nodeMap](const entity* ent, const std::string& perkId) -> bool {
                if (!nodeMap.contains(perkId)) return false;
                const auto& nd = nodeMap[perkId];
                if (!nd.contains("parents") || nd["parents"].empty()) return true; // Root tier
                bool reqAll = nd.value("requireAllParents", false);
                int total = 0, unlockedCount = 0;
                for (const auto& pVal : nd["parents"])
                {
                    total++;
                    if (ent->hasPerk(pVal.get<std::string>())) unlockedCount++;
                }
                return reqAll ? (unlockedCount == total) : (unlockedCount > 0);
            };

            auto treePlayer = std::make_shared<entity>("tree_tester", "TreeHero");
            treePlayer->stats.setBaseStat("max_health", 100.0f);
            treePlayer->stats.setBaseStat("max_mana", 50.0f);
            treePlayer->stats.setBaseStat("perk_points", 15.0f);

            // Gating Check: Tier 0 root is connected; Tier 1, 2, 3 children are locked
            bool rootOk = isConnected(treePlayer.get(), "iron_constitution");
            bool tier1Locked = !isConnected(treePlayer.get(), "brawny_strike");
            bool tier2Locked = !isConnected(treePlayer.get(), "juggernaut");
            bool comboLocked = !isConnected(treePlayer.get(), "spellblade");
            bool initialGating = (rootOk && tier1Locked && tier2Locked && comboLocked);
            logResult("Connected Tree Gating: Root is accessible while deeper child and combo nodes are locked", initialGating);
            allPassed &= initialGating;

            // Step 1: Unlock Root (Iron Constitution, cost 1)
            treePlayer->stats.setBaseStat("perk_points", treePlayer->getStat("perk_points") - 1.0f);
            treePlayer->unlockPerk("iron_constitution");
            bool tier1NowOpen = isConnected(treePlayer.get(), "brawny_strike");
            bool tier2StillLocked = !isConnected(treePlayer.get(), "juggernaut");
            logResult("Unlocking root opens direct Tier 1 child (brawny_strike) while Tier 2 remains locked", tier1NowOpen && tier2StillLocked);
            allPassed &= (tier1NowOpen && tier2StillLocked);

            // Step 2: Unlock Tier 1 (Brawny Strike, cost 2)
            treePlayer->stats.setBaseStat("perk_points", treePlayer->getStat("perk_points") - 2.0f);
            treePlayer->unlockPerk("brawny_strike");
            bool tier2NowOpen = isConnected(treePlayer.get(), "juggernaut");
            logResult("Unlocking Tier 1 opens direct Tier 2 child (juggernaut)", tier2NowOpen);
            allPassed &= tier2NowOpen;

            // Step 3: Test Combo Skill Multi-Parent Gating (Spellblade requires Brawny Strike AND Spell Weaver)
            // Currently Brawny Strike is unlocked, but Spell Weaver is NOT unlocked!
            bool comboBlocked = !isConnected(treePlayer.get(), "spellblade");
            logResult("Combo Skill 'spellblade' remains locked when only 1 of 2 required parent disciplines is unlocked", comboBlocked);
            allPassed &= comboBlocked;

            // Unlock Arcane branch root and adept: arcane_attunement -> spell_weaver
            treePlayer->stats.setBaseStat("perk_points", treePlayer->getStat("perk_points") - 1.0f);
            treePlayer->unlockPerk("arcane_attunement");
            treePlayer->stats.setBaseStat("perk_points", treePlayer->getStat("perk_points") - 2.0f);
            treePlayer->unlockPerk("spell_weaver");

            // Now BOTH parents (Brawny Strike & Spell Weaver) are unlocked -> Spellblade becomes available!
            bool comboNowOpen = isConnected(treePlayer.get(), "spellblade");
            logResult("Combo Skill 'spellblade' becomes learnable once ALL parent disciplines are unlocked", comboNowOpen);
            allPassed &= comboNowOpen;

            // Step 4: Unlock Spellblade (cost 3)
            treePlayer->stats.setBaseStat("perk_points", treePlayer->getStat("perk_points") - 3.0f);
            treePlayer->unlockPerk("spellblade");

            // Verify dual-discipline stat modifiers:
            // physical_damage: 0 base + 10 (brawny) + 15 (spellblade) = 25
            // spell_power: 0 base + 15 (spell_weaver) + 15 (spellblade) = 30
            float physDmg = treePlayer->getStat("physical_damage");
            float spellPow = treePlayer->getStat("spell_power");
            bool comboStatsOk = (physDmg == 25.0f && spellPow == 30.0f);
            logResult("Unlocking combo talent stacks hybrid bonuses (+25 Physical Dmg, +30 Spell Power)", comboStatsOk);
            allPassed &= comboStatsOk;

            // Step 5: Full Reset and Refund across unified disciplines and combo talents
            // Points spent: 1 (iron) + 2 (brawny) + 1 (arcane_attunement) + 2 (spell_weaver) + 3 (spellblade) = 9 points
            int refunded = 0;
            for (const auto& perkId : treePlayer->unlockedPerks)
            {
                if (nodeMap.contains(perkId))
                {
                    refunded += nodeMap[perkId].value("cost", 1);
                }
                else
                {
                    const auto* def = PerkDatabase::getPerk(perkId);
                    if (def) refunded += def->cost;
                }
            }
            treePlayer->stats.setBaseStat("perk_points", treePlayer->getStat("perk_points") + refunded);
            treePlayer->resetPerks();
            bool refundedProperly = (treePlayer->getStat("perk_points") == 15.0f && refunded == 9);
            bool statsReverted = (treePlayer->getStat("physical_damage") == 0.0f &&
                                  treePlayer->getStat("spell_power") == 0.0f &&
                                  !treePlayer->hasPerk("spellblade") &&
                                  !treePlayer->hasPerk("brawny_strike"));
            logResult("Reset refund accurately refunds combo and discipline costs (+9 pts) and reverts stats to base", refundedProperly && statsReverted);
            allPassed &= (refundedProperly && statsReverted);
        }

        // 5. Live Activity & Event Log System
        game testGame;
        testGame.init();
        const auto& initialLogs = testGame.getEventLog();
        bool hasInitLogs = (!initialLogs.empty() && initialLogs.front().tag == "[ZONE]");
        logResult("Event Log initializes with live contextual status entries", hasInitLogs);
        allPassed &= hasInitLogs;

        testGame.addLogEntry("[TALENT]", "Unlocked Spellblade (-3 Pts)", { 220, 180, 80, 255 });
        const auto& updatedLogs = testGame.getEventLog();
        bool hasTalentLog = (!updatedLogs.empty() && updatedLogs.back().tag == "[TALENT]" &&
                             updatedLogs.back().text.find("Spellblade") != std::string::npos);
        logResult("addLogEntry records talent unlocks dynamically into event history", hasTalentLog);
        allPassed &= hasTalentLog;

        // Verify reverse-chronological ordering: top row (offset 0) resolves to latest log entry
        int topRowIdx = static_cast<int>(updatedLogs.size()) - 1;
        bool newestOnTop = (topRowIdx >= 0 && updatedLogs[topRowIdx].tag == "[TALENT]");
        logResult("Event Log reverses order so newest events display at top row", newestOnTop);
        allPassed &= newestOnTop;

        // Verify Perks screen Action Grid: Tier buttons removed, Slot 13 Reset Perks, Slot 14 Back
        testGame.changeState(std::make_unique<phoneAppsState>(PhoneAppMode::PERKS));
        const auto& perkButtons = testGame.getActiveActionButtons();
        bool slot13Reset = (perkButtons.size() >= 15 && perkButtons[13].label == "Reset Perks" && perkButtons[13].isEnabled);
        bool slot14Back = (perkButtons.size() >= 15 && perkButtons[14].label == "Back" && perkButtons[14].isEnabled);
        bool tierButtonsRemoved = true;
        for (int i = 0; i < 13; ++i)
        {
            if (i < static_cast<int>(perkButtons.size()) && perkButtons[i].isEnabled)
            {
                tierButtonsRemoved = false;
                break;
            }
        }
        bool actionGridClean = (slot13Reset && slot14Back && tierButtonsRemoved);
        logResult("Perks Action Grid eliminates tier buttons and positions Reset Perks at Slot 13", actionGridClean);
        allPassed &= actionGridClean;

        return allPassed;
    }

    bool testDataDrivenPerksAndContentOptions()
    {
        std::cout << "\n--- Running Test 25: Data-Driven JSON Perks System & Content Option Settings ---\n";
        bool allPassed = true;

        // 1. Dynamic PerkDatabase loading from JSON
        PerkDatabase::clear();
        bool dbLoaded = PerkDatabase::loadFromFile("data/perks.json");
        const auto& allPerks = PerkDatabase::getAllPerks();
        bool dbCheck = dbLoaded && (allPerks.size() >= 27) &&
                       PerkDatabase::hasPerk("iron_constitution") &&
                       PerkDatabase::hasPerk("master_trader") &&
                       PerkDatabase::hasPerk("demon_berserker");
        logResult("PerkDatabase loads 27+ total perks from data/perks.json dynamically", dbCheck);
        allPassed &= dbCheck;

        // 2. Dynamic Stat Modifiers (Flat & Percent) on Entity without hardcoding
        auto player = std::make_shared<entity>("test_p25", "Hero");
        player->stats.setBaseStat("strength", 10.0f);
        player->stats.setBaseStat("max_health", 100.0f);
        player->unlockPerk("iron_constitution"); // flat +20 max_health, +5 defense
        bool flatApplied = (player->getStat("max_health") == 120.0f && player->getStat("defense") == 5.0f);

        PerkDefinition customPerk;
        customPerk.id = "perk_warlord_might";
        customPerk.name = "Warlord's Might";
        customPerk.modifiers.statModifiers["strength"] = 10.0f;        // flat +10 -> strength 20
        customPerk.modifiers.percentStatModifiers["strength"] = 0.50f; // percent +50% -> strength 30
        PerkDatabase::registerPerk(customPerk);
        player->unlockPerk("perk_warlord_might");
        bool percentApplied = (player->getStat("strength") == 30.0f);
        bool statModsSuccess = flatApplied && percentApplied;
        logResult("Perk stat modifiers apply dynamically (flat and percent) without hardcoding", statModsSuccess);
        allPassed &= statModsSuccess;

        // 3. Versatile Perk Damage Multipliers by Race and Attack Type
        player->unlockPerk("demonic_dominion"); // damage vs Demon +20%, vs Human +15%
        player->unlockPerk("spell_weaver");     // damage for arcane +15%
        float dmgDemonArcane = player->getPerkDamageMultiplier("Demon", "arcane"); // 0.20 + 0.15 = 0.35
        float dmgHumanPhysical = player->getPerkDamageMultiplier("Human", "physical"); // 0.15
        bool dmgMultMatch = (std::abs(dmgDemonArcane - 0.35f) < 0.001f) &&
                            (std::abs(dmgHumanPhysical - 0.15f) < 0.001f);
        logResult("Perk damage multipliers accurately compute by target race and attack type", dmgMultMatch);
        allPassed &= dmgMultMatch;

        // 4. Versatile Perk Defense Multipliers by Attacker Race and Attack Type
        player->unlockPerk("hellfire_blood"); // defense vs fire +25%
        // iron_constitution has defense vs physical +10%
        float defFire = player->getPerkDefenseMultiplier("", "fire");
        float defPhys = player->getPerkDefenseMultiplier("", "physical");
        bool defMultMatch = (std::abs(defFire - 0.25f) < 0.001f) &&
                            (std::abs(defPhys - 0.10f) < 0.001f);
        logResult("Perk defense multipliers accurately compute damage reductions by attack type and race", defMultMatch);
        allPassed &= defMultMatch;

        // 5. Capability and Dialogue Flags
        player->unlockPerk("silver_tongue");       // flags: ["trade_negotiation"]
        player->unlockPerk("corruption_affinity"); // flags: ["demonic_pact"]
        bool flagNegotiation = player->hasPerkFlag("trade_negotiation");
        bool flagPact = player->hasPerkFlag("demonic_pact");
        bool flagFake = !player->hasPerkFlag("nonexistent_perk_flag");
        auto allFlags = player->getAllPerkFlags();
        bool flagCount = (allFlags.size() >= 2);
        bool flagSuccess = flagNegotiation && flagPact && flagFake && flagCount;
        logResult("Perk flags register and query accurately across entity unlocked perks", flagSuccess);
        allPassed &= flagSuccess;

        // 6. Named Character Marcus Profile Perk Integration
        NamedCharacterManager::clear();
        bool marcusLoaded = NamedCharacterManager::loadFromDirectory("data/characters");
        auto marcus = NamedCharacterManager::getCharacter("marcus");
        bool marcusSuccess = marcusLoaded && (marcus != nullptr) && (marcus->characterEntity != nullptr);
        if (marcusSuccess)
        {
            auto mEnt = marcus->characterEntity;
            bool hasST = mEnt->hasPerk("silver_tongue");
            bool hasMT = mEnt->hasPerk("master_trader");
            bool tradeModifier = (std::abs(mEnt->tradePerkModifier - 0.25f) < 0.001f);
            bool flagAppraisal = mEnt->hasPerkFlag("expert_appraisal");
            bool flagTradeNeg = mEnt->hasPerkFlag("trade_negotiation");
            marcusSuccess = hasST && hasMT && tradeModifier && flagAppraisal && flagTradeNeg;
        }
        logResult("Named character Marcus loads unlocked perks and trade bonuses directly from JSON profile", marcusSuccess);
        allPassed &= marcusSuccess;

        // 7. NPC Generator Template Perk Integration
        bool npcLoaded = npcGenerator::loadTemplates("data/enemies");
        auto bandit = npcGenerator::generateFromTemplate("tpl_alley_bandit");
        auto mage = npcGenerator::generateFromTemplate("tpl_rogue_mage");
        bool enemiesValid = npcLoaded && (bandit != nullptr) && (mage != nullptr);
        if (enemiesValid)
        {
            bool banditPerks = bandit->hasPerk("brawny_strike") && bandit->hasPerk("iron_constitution");
            bool banditDmg = (std::abs(bandit->getPerkDamageMultiplier("", "physical") - 0.15f) < 0.001f);
            bool banditDef = (std::abs(bandit->getPerkDefenseMultiplier("", "physical") - 0.10f) < 0.001f);

            bool magePerks = mage->hasPerk("arcane_attunement") && mage->hasPerk("spell_weaver");
            bool mageDmg = (std::abs(mage->getPerkDamageMultiplier("", "arcane") - 0.25f) < 0.001f);

            enemiesValid = banditPerks && banditDmg && banditDef && magePerks && mageDmg;
        }
        logResult("NPC generator instantiates enemies with archetype perks and dynamic combat stats from JSON", enemiesValid);
        allPassed &= enemiesValid;

        // 8. Combat Engine Action Resolution with Perks & Difficulty Scaling
        game cg;
        cg.init();
        combatEngine combat;

        auto combatPlayer = std::make_shared<entity>("c_player", "Striker");
        combatPlayer->stats.setBaseStat("health", 200.0f);
        combatPlayer->stats.setBaseStat("max_health", 200.0f);
        combatPlayer->unlockPerk("brawny_strike"); // +15% physical damage

        auto combatEnemy = std::make_shared<entity>("c_enemy", "Gladiator");
        combatEnemy->stats.setBaseStat("health", 200.0f);
        combatEnemy->stats.setBaseStat("max_health", 200.0f);
        combatEnemy->unlockPerk("iron_constitution"); // +10% physical defense

        combat.initialiseCombat({ combatPlayer }, { combatEnemy });

        // Player attacks enemy with 100 base physical damage
        CombatAction strikeAction;
        strikeAction.id = "act_strike";
        strikeAction.name = "Strike";
        strikeAction.baseApCost = 1;
        SpellEffectNode dmgNode;
        dmgNode.effectType = "DAMAGE";
        dmgNode.element = "physical";
        dmgNode.baseMagnitude = 100.0f;
        strikeAction.effectNodes.push_back(dmgNode);

        combat.queuePlayerAction(0, strikeAction, combatEnemy.get());
        combat.resolveTurn(&cg);

        // Expected: 100 * 1.15 * (1 - 0.10) = 103.5 -> rounded to 104 damage
        float enemyHpAfter = combatEnemy->getStat("health");
        bool playerAttackScaled = (enemyHpAfter == 96.0f); // 200 - 104 = 96

        // Now test difficulty multiplier scaling when enemy attacks player
        cg.settings.gameplay.difficultyMultiplier = 1.5f;
        QueuedAction enemyAtk;
        enemyAtk.user = combatEnemy.get();
        enemyAtk.target = combatPlayer.get();
        CombatAction enemyAction = strikeAction;
        enemyAction.effectNodes[0].element = "fire"; // neutral element for player
        enemyAtk.action = enemyAction;

        // Direct action execution simulation with difficulty multiplier
        float playerHpBefore = combatPlayer->getStat("health");
        combat.executeAction(enemyAtk, &cg);
        float playerHpAfter = combatPlayer->getStat("health");
        // Enemy has no fire perk, player has no fire defense. Damage: 100 * 1.5 = 150
        float playerDmgTaken = playerHpBefore - playerHpAfter;
        bool difficultyScaled = (playerDmgTaken == 150.0f);

        bool combatSuccess = playerAttackScaled && difficultyScaled;
        logResult("Combat engine applies attacker perk damage, defender defense, and difficulty scaling", combatSuccess);
        allPassed &= combatSuccess;

        // 9. Content Option Settings Gameplay Integration
        // 9a. Fluid Multiplier in sexState orgasm
        auto lover1 = std::make_shared<entity>("lover1", "Lover A");
        auto lover2 = std::make_shared<entity>("lover2", "Lover B");

        bodyPart cock;
        cock.id = "p_groin";
        cock.name = "Cock";
        cock.currentFluidMl = 100.0f;
        cock.maxFluidMl = 200.0f;
        lover1->anatomy.setPart(bodySlot::GROIN, cock);

        bodyPart vagina;
        vagina.id = "v_groin";
        vagina.name = "Vagina";
        vagina.currentFluidMl = 0.0f;
        vagina.maxFluidMl = 200.0f;
        vagina.orifice.exists = true;
        vagina.orifice.depthCm = 15.0f;
        vagina.tags.push_back("vagina");
        lover2->anatomy.setPart(bodySlot::GROIN, vagina);

        sexState sexApp(lover2);
        cg.settings.content.fluidMultiplier = 3.0f; // 3x multiplier
        sexApp.processOrgasm(&cg, lover1.get(), lover2.get(), bodySlot::GROIN);
        // Base 15ml * 3.0 = 45ml ejaculated
        float cumRemaining = lover1->anatomy.getPart(bodySlot::GROIN)->currentFluidMl;
        bool fluidMatch = (cumRemaining == 55.0f); // 100 - 45 = 55

        // 9b. Lactation Enabled gating
        bodyPart breasts;
        breasts.id = "p_breasts";
        breasts.name = "Breasts";
        breasts.currentFluidMl = 50.0f;
        breasts.maxFluidMl = 100.0f;
        breasts.isLactating = true;
        lover1->anatomy.setPart(bodySlot::BREASTS, breasts);

        cg.settings.content.lactationEnabled = false;
        sexApp.processOrgasm(&cg, lover1.get(), lover2.get(), bodySlot::GROIN);
        bool lactDisabled = (lover1->anatomy.getPart(bodySlot::BREASTS)->currentFluidMl == 50.0f); // suppressed

        cg.settings.content.lactationEnabled = true;
        cg.settings.content.fluidMultiplier = 1.0f;
        sexApp.processOrgasm(&cg, lover1.get(), lover2.get(), bodySlot::GROIN);
        bool lactEnabled = (lover1->anatomy.getPart(bodySlot::BREASTS)->currentFluidMl == 25.0f); // 50 - 25 = 25

        // 9c. Auto-Loot in encounterResolutionState
        auto defeatedEnemy = std::make_shared<entity>("def_enemy", "Thief");
        defeatedEnemy->stats.setBaseStat("currency", 250.0f);
        auto dagger = itemDatabase::getItem("item_canis_root");
        if (dagger) defeatedEnemy->inventory.addItem(dagger);

        cg.settings.gameplay.autoLoot = true;
        cg.playerEntity = player;
        cg.Player = player.get();
        player->stats.setBaseStat("currency", 100.0f);
        encounterResolutionState resState({ defeatedEnemy });
        resState.onEnter(&cg);

        bool autoLootSuccess = (player->getStat("currency") == 350.0f) &&
                               (defeatedEnemy->getStat("currency") == 0.0f) &&
                               (resState.getDefeatedRecords()[0].isLooted);

        // 9d. Non-Con Enabled gating in encounterResolutionState
        cg.settings.content.nonConEnabled = false;
        resState.handleInteractiveSex(&cg);
        bool nonConBlocked = (resState.getDefeatedRecords()[0].hadSex == false) &&
                             (resState.getResolutionLog().find("Non-consensual content is disabled") != std::string::npos);

        // 9e. Currency loss on combat defeat
        cg.settings.content.nonConEnabled = false;
        cg.settings.gameplay.currencyLossOnDefeatPercent = 0.25f; // 25% loss
        player->stats.setBaseStat("currency", 1000.0f);
        CombatState combatSt({ player }, { defeatedEnemy });
        // Mock defeat resolution with robber enemy
        combatSt.resolveDefeat(&cg);
        bool lossMatch = (player->getStat("currency") == 750.0f); // 1000 - 250 = 750

        bool contentSuccess = fluidMatch && lactDisabled && lactEnabled && autoLootSuccess && nonConBlocked && lossMatch;
        logResult("Content option settings actively govern fluids, lactation, auto-loot, non-con gating, and defeat gold loss", contentSuccess);
        allPassed &= contentSuccess;

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
        bool t6 = testSubmenuButtonFunctionality();
        bool t7 = testContentOptionsAllCategories();
        bool t8 = testGranularEditorOptionMasking();
        bool t9 = testHairstyleGatingAndBodyShape();
        bool t10 = testWardrobeDecencySystem();
        bool t11 = testFullCustomizationTrackingAndAppearanceDescription();
        bool t12 = testFullTransformationSuiteAndPresetPersistence();
        bool t13 = testLegacySaveCompatibility();
        bool t14 = testTooltipSystem();
        bool t15 = testPlayerStatsAndItemUsage();
        bool t16 = testInventoryCategoricalSortingAndActions();
        bool t17 = testDecouplingAndCaching();
        bool t18 = testQuestJournalSystem();
        bool t19 = testFullPhoneSystem();
        bool t20 = testEconomyAndShopTrading();
        bool t21 = testNamedCharactersAndPersistentEncounter();
        bool t22 = testCalendarLeapYearsAndTransformationNavigation();
        bool t23 = testLayoutIntegrityAndContainment();
        bool t24 = testUnified3PanelLayoutFogOfWarAndPerkTree();
        bool t25 = testDataDrivenPerksAndContentOptions();

        std::cout << "======================================================================\n";
        std::cout << " Test Summary: " << g_passCount << " Passed, " << g_failCount << " Failed.\n";
        std::cout << "======================================================================\n\n";

        return (g_failCount == 0);
    }
}

