#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include "state/iGameState.h"

enum class SaveMenuMode
{
    LOAD_ONLY,
    SAVE_AND_LOAD
};

/**
 * Headless state controller for the unified Save & Load Screen.
 * Groups saves by character with collapsible dropdowns, newest-to-oldest sorting,
 * save creation with overwrite checks, and save deletion.
 */
class loadGameState : public iGameState
{
public:
    explicit loadGameState(SaveMenuMode mode = SaveMenuMode::LOAD_ONLY, std::unique_ptr<iGameState> returnState = nullptr);
    ~loadGameState() override = default;

    void initialise(game* gameContext) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    SaveMenuMode getMode() const { return m_mode; }
    void setMode(SaveMenuMode mode) { m_mode = mode; }

    bool isCharacterCollapsed(const std::string& charName) const
    {
        return m_collapsedCharacters.contains(charName);
    }

    void toggleCharacterCollapsed(const std::string& charName)
    {
        if (m_collapsedCharacters.contains(charName))
        {
            m_collapsedCharacters.erase(charName);
        }
        else
        {
            m_collapsedCharacters.insert(charName);
        }
    }

    std::string newSaveNameInput = "Manual_Save";
    bool isEditingSaveName = false;
    std::string pendingOverwriteSaveName = "";
    std::string pendingDeleteFileName = "";

    void goBack(game* gameContext);

private:
    SaveMenuMode m_mode = SaveMenuMode::LOAD_ONLY;
    std::unordered_set<std::string> m_collapsedCharacters;
    std::unique_ptr<iGameState> m_returnState;
};
