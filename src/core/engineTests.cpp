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

        std::cout << "======================================================================\n";
        std::cout << " Test Summary: " << g_passCount << " Passed, " << g_failCount << " Failed.\n";
        std::cout << "======================================================================\n\n";

        return (g_failCount == 0);
    }
}
