#pragma once

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "state/iGameState.h"

class game;

enum class PhoneAppMode
{
    HOME,
    QUESTS,
    ENCYCLOPEDIA,
    SPELLS,
    PERKS,
    CONTACTS,
    STATS,
    MAPS,
    TRANSFORM,
    WAIT_REST,
    MASTURBATE,
    FETISHES
};

inline std::string phoneAppModeToString(PhoneAppMode mode)
{
    switch (mode)
    {
        case PhoneAppMode::HOME:         return "Smartphone Home";
        case PhoneAppMode::QUESTS:       return "Quests";
        case PhoneAppMode::ENCYCLOPEDIA: return "Encyclopedia";
        case PhoneAppMode::SPELLS:       return "Spells & Moves";
        case PhoneAppMode::PERKS:        return "Perk Tree";
        case PhoneAppMode::CONTACTS:     return "Contacts";
        case PhoneAppMode::STATS:        return "Stats & Diagnostics";
        case PhoneAppMode::MAPS:         return "Regional Maps";
        case PhoneAppMode::TRANSFORM:    return "Transformations";
        case PhoneAppMode::WAIT_REST:    return "Wait & Rest";
        case PhoneAppMode::MASTURBATE:   return "Solo Intimacy";
        case PhoneAppMode::FETISHES:     return "Fetishes & Preferences";
        default:                         return "Phone";
    }
}

class phoneAppsState : public iGameState
{
public:
    explicit phoneAppsState(PhoneAppMode mode = PhoneAppMode::HOME);
    ~phoneAppsState() override = default;

    void initialise(game* gameContext) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    PhoneAppMode getAppMode() const { return m_mode; }
    void setAppMode(PhoneAppMode mode) { m_mode = mode; m_selectedItemIndex = 0; m_feedbackText.clear(); }

    int getSelectedCategory() const { return m_selectedCategory; }
    void setSelectedCategory(int idx) { m_selectedCategory = idx; m_selectedItemIndex = 0; }

    int getSelectedItemIndex() const { return m_selectedItemIndex; }
    void setSelectedItemIndex(int idx) { m_selectedItemIndex = idx; }

    const std::string& getFeedbackText() const { return m_feedbackText; }
    void setFeedbackText(const std::string& text) { m_feedbackText = text; }

    const nlohmann::json& getAppData() const { return m_appData; }

    void loadData(PhoneAppMode mode);

private:
    PhoneAppMode m_mode = PhoneAppMode::HOME;
    int m_selectedCategory = 0;
    int m_selectedItemIndex = 0;
    std::string m_feedbackText;
    nlohmann::json m_appData;
};
