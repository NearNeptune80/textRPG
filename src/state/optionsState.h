#pragma once

#include <memory>
#include <string>
#include "state/iGameState.h"

enum class OptionsScreenMode
{
    GENERAL_OPTIONS,
    CONTENT_OPTIONS
};

enum class ContentOptionsCategory
{
    MISC,
    GAMEPLAY,
    FETISH_PREFS,
    SEX_AND_FETISHES = FETISH_PREFS, // Backward-compatibility alias
    BODIES,
    GENDER_PREFS,
    ORIENTATION_PREFS,
    AGE_PREFS,
    FURRY_PREFS
};

/**
 * Headless state controller for Game Options and Content Options.
 */
class optionsState : public iGameState
{
public:
    explicit optionsState(OptionsScreenMode mode = OptionsScreenMode::GENERAL_OPTIONS, std::unique_ptr<iGameState> returnState = nullptr);
    ~optionsState() override = default;

    OptionsScreenMode screenMode{ OptionsScreenMode::GENERAL_OPTIONS };
    ContentOptionsCategory contentCategory{ ContentOptionsCategory::MISC };

    int fontSize = 18;
    int difficultyLevel = 2; // 0: Very Easy, 1: Easy, 2: Normal, 3: Hard, 4: Nightmare
    std::string genderPronounMode = "Normal";
    std::string unitPreference = "Metric";
    bool isKeybindsOpen = false;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    void goBack(game* gameContext);
    void resetCategoryDefaults(game* gameContext);
    void resetAllDefaults(game* gameContext);

private:
    std::unique_ptr<iGameState> m_returnState;
};
