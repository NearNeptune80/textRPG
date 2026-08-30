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
#include "ui/fontManager.h"

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

        // 1. Initial outfit is decent
        bool initDecent = cc.isClothedEnough();
        logResult("Initial populated wardrobe setup is Decent", initDecent);
        allPassed &= initDecent;

        // 2. Strip footwear -> Indecent
        cc.unequipWardrobeItem(equipSlot::FEET);
        bool noShoesIndecent = !cc.isClothedEnough();
        std::string status1 = cc.getDecencyStatus();
        bool mentionsShoes = (status1.find("Must put on footwear") != std::string::npos);
        logResult("Stripping footwear flags indecency with warning", noShoesIndecent && mentionsShoes);
        allPassed &= (noShoesIndecent && mentionsShoes);

        // 3. Strip all clothing -> Multiple warnings
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

        // 4. Equip wardrobe pile items back
        while (!cc.availableWardrobe.empty())
        {
            size_t beforeSize = cc.availableWardrobe.size();
            for (size_t i = 0; i < cc.availableWardrobe.size(); ++i)
            {
                if (cc.availableWardrobe[i]->targetSlot == equipSlot::FEET ||
                    cc.availableWardrobe[i]->targetSlot == equipSlot::TORSO_OVER ||
                    cc.availableWardrobe[i]->targetSlot == equipSlot::LEGS_OUTER ||
                    cc.availableWardrobe[i]->targetSlot == equipSlot::GROIN_OVER ||
                    cc.availableWardrobe[i]->targetSlot == equipSlot::CHEST_WEAR)
                {
                    cc.equipWardrobeItem(i);
                    break;
                }
            }
            if (cc.availableWardrobe.size() == beforeSize) break;
        }

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

        std::cout << "======================================================================\n";
        std::cout << " Test Summary: " << g_passCount << " Passed, " << g_failCount << " Failed.\n";
        std::cout << "======================================================================\n\n";

        return (g_failCount == 0);
    }
}
