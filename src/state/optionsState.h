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
    SEX_AND_FETISHES,
    BODIES,
    GENDER_PREFS,
    ORIENTATION_PREFS,
    AGE_PREFS,
    FURRY_PREFS,
    FETISH_PREFS
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
    bool fadeInEnabled = false;
    int difficultyLevel = 0; // 0: Human, 1: Morph, 2: Demon, 3: Lilin, 4: Lilith
    std::string genderPronounMode = "Normal";
    std::string unitPreference = "Metric";

    void initialise(game* gameContext) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    void goBack(game* gameContext);

private:
    std::unique_ptr<iGameState> m_returnState;
};
