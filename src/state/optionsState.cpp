#include "state/optionsState.h"

#include <memory>

#include "core/game.h"
#include "settings/settingsManager.h"
#include "state/explorationState.h"
#include "state/mainMenuState.h"

optionsState::optionsState(OptionsScreenMode mode, std::unique_ptr<iGameState> returnState)
    : screenMode(mode), m_returnState(std::move(returnState))
{
}

void optionsState::initialise(game* gameContext) {}

void optionsState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        fontSize = gameContext->settings.display.fontSize;
        fadeInEnabled = gameContext->settings.display.fadeInEnabled;
        difficultyLevel = gameContext->settings.gameplay.difficultyLevel;
        genderPronounMode = gameContext->settings.gameplay.genderPronounMode;
        unitPreference = gameContext->settings.gameplay.unitPreference;
        gameContext->refreshActionGrid();
    }
}

void optionsState::onExit(game* gameContext)
{
    if (gameContext)
    {
        settingsManager::saveToFile(gameContext->settings, "data/settings.json");
    }
}

void optionsState::update(game* gameContext, float deltaTime) {}

void optionsState::goBack(game* gameContext)
{
    if (!gameContext) return;

    // Persist settings on leaving options
    settingsManager::saveToFile(gameContext->settings, "data/settings.json");

    if (m_returnState)
    {
        gameContext->changeState(std::move(m_returnState));
    }
    else if (gameContext->getPlayer() != nullptr)
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
    else
    {
        gameContext->changeState(std::make_unique<mainMenuState>());
    }
}

void optionsState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        if (isKeybindsOpen)
        {
            isKeybindsOpen = false;
            gameContext->refreshActionGrid();
        }
        else
        {
            goBack(gameContext);
        }
    }
    else if (cmd.type == CommandType::CYCLE_SETTING_OPTION)
    {
        if (cmd.stringPayload == "pregnancy")
        {
            gameContext->settings.content.pregnancyEnabled = !gameContext->settings.content.pregnancyEnabled;
        }
        else if (cmd.stringPayload == "lactation")
        {
            gameContext->settings.content.lactationEnabled = !gameContext->settings.content.lactationEnabled;
        }
        else if (cmd.stringPayload == "difficulty")
        {
            difficultyLevel = (difficultyLevel + 1) % 5;
            gameContext->settings.gameplay.difficultyLevel = difficultyLevel;
            static constexpr float diffMults[] = { 1.0f, 1.25f, 2.0f, 2.5f, 4.0f };
            gameContext->settings.gameplay.difficultyMultiplier = diffMults[difficultyLevel];
        }
        settingsManager::saveToFile(gameContext->settings, "data/settings.json");
        gameContext->refreshActionGrid();
    }
}

void optionsState::resetCategoryDefaults(game* gameContext)
{
    if (!gameContext) return;
    if (contentCategory == ContentOptionsCategory::GENDER_PREFS)
    {
        gameContext->settings.demographics.percentMale = 30.0f;
        gameContext->settings.demographics.percentFemale = 40.0f;
        gameContext->settings.demographics.percentHermaphrodite = 15.0f;
        gameContext->settings.demographics.percentGynomorph = 7.0f;
        gameContext->settings.demographics.percentAndromorph = 5.0f;
        gameContext->settings.demographics.percentNull = 3.0f;
    }
    else if (contentCategory == ContentOptionsCategory::ORIENTATION_PREFS)
    {
        gameContext->settings.demographics.percentHetero = 40.0f;
        gameContext->settings.demographics.percentBi = 30.0f;
        gameContext->settings.demographics.percentHomo = 20.0f;
        gameContext->settings.demographics.percentAsexual = 10.0f;
    }
    else if (contentCategory == ContentOptionsCategory::AGE_PREFS)
    {
        gameContext->settings.demographics.percentYoungAdult = 40.0f;
        gameContext->settings.demographics.percentAdult = 35.0f;
        gameContext->settings.demographics.percentMature = 20.0f;
        gameContext->settings.demographics.percentElder = 5.0f;
    }
    else if (contentCategory == ContentOptionsCategory::FURRY_PREFS)
    {
        gameContext->settings.demographics.percentHuman = 50.0f;
        gameContext->settings.demographics.percentPartial = 30.0f;
        gameContext->settings.demographics.percentAnthro = 15.0f;
        gameContext->settings.demographics.percentFeral = 5.0f;
    }
    else if (contentCategory == ContentOptionsCategory::FETISH_PREFS)
    {
        for (auto& [k, v] : gameContext->settings.content.fetishPreferences)
        {
            v = 3; // Neutral
        }
    }
    else if (contentCategory == ContentOptionsCategory::GAMEPLAY)
    {
        gameContext->settings.gameplay.enchantmentInstability = true;
        gameContext->settings.gameplay.badEndsEnabled = true;
        gameContext->settings.gameplay.levelDrainEnabled = true;
        gameContext->settings.gameplay.opportunisticAttackers = true;
        gameContext->settings.gameplay.autoLoot = true;
        gameContext->settings.gameplay.currencyLossOnDefeatPercent = 0.15f;
    }
    else if (contentCategory == ContentOptionsCategory::MISC)
    {
        gameContext->settings.gameplay.autoSaveFrequency = 0;
        gameContext->settings.display.showArtwork = true;
        gameContext->settings.display.showThumbnails = true;
        gameContext->settings.gameplay.sharedEncyclopedia = false;
        gameContext->settings.gameplay.stormInterruptions = true;
    }
    else if (contentCategory == ContentOptionsCategory::SEX_AND_FETISHES)
    {
        gameContext->settings.content.nonConEnabled = false;
        gameContext->settings.content.publicSexEnabled = true;
        gameContext->settings.content.extremeContentEnabled = false;
        gameContext->settings.content.fluidMultiplier = 1.0f;
    }
    else if (contentCategory == ContentOptionsCategory::BODIES)
    {
        gameContext->settings.content.pregnancyEnabled = true;
        gameContext->settings.content.lactationEnabled = true;
        gameContext->settings.content.transformationSpeedMultiplier = 1.0f;
    }

    settingsManager::saveToFile(gameContext->settings, "data/settings.json");
    gameContext->refreshActionGrid();
}

void optionsState::resetAllDefaults(game* gameContext)
{
    if (!gameContext) return;
    gameContext->settings = GameSettings{};
    difficultyLevel = 0;
    fontSize = 18;
    fadeInEnabled = false;
    genderPronounMode = "Normal";
    unitPreference = "Metric";
    settingsManager::saveToFile(gameContext->settings, "data/settings.json");
    gameContext->refreshActionGrid();
}
