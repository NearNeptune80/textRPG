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
            static constexpr float diffMults[] = { 0.75f, 0.85f, 1.0f, 1.5f, 2.5f };
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
            v = 3;
        }
        gameContext->settings.content.contentFilterMode = ContentFilterMode::DROPDOWN;
        gameContext->settings.content.nonConEnabled = false;
        gameContext->settings.content.nonConState = ContentToggleState::OFF;
        gameContext->settings.content.extremeContentEnabled = false;
        gameContext->settings.content.extremeContentState = ContentToggleState::OFF;
        gameContext->settings.content.publicSexEnabled = true;
        gameContext->settings.content.publicSexState = ContentToggleState::ON;
        gameContext->settings.content.fluidMultiplier = 1.0f;
        gameContext->settings.content.watersportsEnabled = false;
        gameContext->settings.content.watersportsState = ContentToggleState::OFF;
        gameContext->settings.content.spittingEnabled = false;
        gameContext->settings.content.spittingState = ContentToggleState::OFF;
        gameContext->settings.content.forcedTfEnabled = false;
        gameContext->settings.content.forcedTfState = ContentToggleState::OFF;
        gameContext->settings.content.tentaclesEnabled = false;
        gameContext->settings.content.tentaclesState = ContentToggleState::OFF;
        gameContext->settings.content.bdsmEnabled = false;
        gameContext->settings.content.bdsmState = ContentToggleState::OFF;
        gameContext->settings.content.incestEnabled = false;
        gameContext->settings.content.incestState = ContentToggleState::OFF;
        gameContext->settings.content.sizeDifferenceEnabled = false;
        gameContext->settings.content.sizeDifferenceState = ContentToggleState::OFF;
        gameContext->settings.content.prolapseEnabled = false;
        gameContext->settings.content.prolapseState = ContentToggleState::OFF;
        gameContext->settings.content.aphrodisiacsEnabled = false;
        gameContext->settings.content.aphrodisiacsState = ContentToggleState::OFF;
        gameContext->settings.content.hypnosisEnabled = false;
        gameContext->settings.content.hypnosisState = ContentToggleState::OFF;
        gameContext->settings.content.cumInflationEnabled = false;
        gameContext->settings.content.cumInflationState = ContentToggleState::OFF;
        gameContext->settings.content.feralEnabled = false;
        gameContext->settings.content.feralState = ContentToggleState::OFF;
    }
    else if (contentCategory == ContentOptionsCategory::BODIES)
    {
        gameContext->settings.content.pregnancyEnabled = true;
        gameContext->settings.content.lactationEnabled = true;
        gameContext->settings.content.transformationSpeedMultiplier = 1.0f;
        gameContext->settings.content.furryGenitalStyle = "Humanoid";
    }

    settingsManager::saveToFile(gameContext->settings, "data/settings.json");
    gameContext->refreshActionGrid();
}

void optionsState::resetAllDefaults(game* gameContext)
{
    if (!gameContext) return;
    gameContext->settings = GameSettings{};
    difficultyLevel = 2;
    fontSize = 18;
    genderPronounMode = "Normal";
    unitPreference = "Metric";
    settingsManager::saveToFile(gameContext->settings, "data/settings.json");
    gameContext->refreshActionGrid();
}
