#include "state/optionsState.h"

#include <memory>

#include "core/game.h"
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
        gameContext->refreshActionGrid();
    }
}

void optionsState::onExit(game* gameContext) {}

void optionsState::update(game* gameContext, float deltaTime) {}

void optionsState::goBack(game* gameContext)
{
    if (!gameContext) return;
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

void optionsState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
    {
        goBack(gameContext);
    }
}

void optionsState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        goBack(gameContext);
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
        }
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
    else if (contentCategory == ContentOptionsCategory::GAMEPLAY || contentCategory == ContentOptionsCategory::MISC)
    {
        gameContext->settings.gameplay.autoSaveOnMapChange = true;
        gameContext->settings.gameplay.autoSaveOnSceneExit = true;
        gameContext->settings.gameplay.maxAutoSaves = 3;
    }
    else if (contentCategory == ContentOptionsCategory::SEX_AND_FETISHES || contentCategory == ContentOptionsCategory::BODIES)
    {
        gameContext->settings.content.pregnancyEnabled = true;
        gameContext->settings.content.lactationEnabled = true;
        gameContext->settings.content.fluidMultiplier = 1.0f;
        gameContext->settings.content.transformationSpeedMultiplier = 1.0f;
    }
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
    gameContext->refreshActionGrid();
}
