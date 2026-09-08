#include "state/loadGameState.h"

#include <memory>

#include "core/game.h"
#include "save/saveManager.h"
#include "state/explorationState.h"
#include "state/mainMenuState.h"

loadGameState::loadGameState(SaveMenuMode mode, std::unique_ptr<iGameState> returnState)
    : m_mode(mode), m_returnState(std::move(returnState))
{
    refreshSaves();
}

void loadGameState::initialise(game* gameContext) {}

void loadGameState::onEnter(game* gameContext)
{
    refreshSaves();
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void loadGameState::refreshSaves()
{
    m_cachedGroups = saveManager::getSavesGroupedByCharacter();
    applySorting();
    m_needsRefresh = false;
    m_lastSortMode = sortMode;
}

void loadGameState::setSortMode(int mode)
{
    sortMode = mode;
    applySorting();
}

void loadGameState::applySorting()
{
    if (sortMode == 1)
    {
        // Alphabetical sort
        std::sort(m_cachedGroups.begin(), m_cachedGroups.end(), [](const CharacterSaveGroup& a, const CharacterSaveGroup& b) {
            return a.characterName < b.characterName;
        });
        for (auto& grp : m_cachedGroups)
        {
            std::sort(grp.saves.begin(), grp.saves.end(), [](const SaveMetaData& a, const SaveMetaData& b) {
                return a.saveName < b.saveName;
            });
        }
    }
    else
    {
        // Date sort
        for (auto& grp : m_cachedGroups)
        {
            std::sort(grp.saves.begin(), grp.saves.end(), [](const SaveMetaData& a, const SaveMetaData& b) {
                return a.timestamp > b.timestamp;
            });
        }
        std::sort(m_cachedGroups.begin(), m_cachedGroups.end(), [](const CharacterSaveGroup& a, const CharacterSaveGroup& b) {
            std::string tA = a.saves.empty() ? "" : a.saves.front().timestamp;
            std::string tB = b.saves.empty() ? "" : b.saves.front().timestamp;
            return tA > tB;
        });
    }
    m_lastSortMode = sortMode;
}

const std::vector<CharacterSaveGroup>& loadGameState::getCharacterGroups()
{
    if (m_needsRefresh)
    {
        refreshSaves();
    }
    else if (m_lastSortMode != sortMode)
    {
        applySorting();
    }
    return m_cachedGroups;
}

void loadGameState::onExit(game* gameContext) {}

void loadGameState::update(game* gameContext, float deltaTime) {}

void loadGameState::goBack(game* gameContext)
{
    if (!gameContext) return;
    if (m_returnState)
    {
        gameContext->changeState(std::move(m_returnState));
    }
    else
    {
        gameContext->changeState(std::make_unique<mainMenuState>());
    }
}

void loadGameState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        if (isEditingSaveName)
        {
            isEditingSaveName = false;
        }
        else
        {
            goBack(gameContext);
        }
    }
    else if (cmd.type == CommandType::TEXT_INPUT)
    {
        if (isEditingSaveName)
        {
            newSaveNameInput += cmd.stringPayload;
        }
    }
    else if (cmd.type == CommandType::TEXT_BACKSPACE)
    {
        if (isEditingSaveName && !newSaveNameInput.empty())
        {
            newSaveNameInput.pop_back();
        }
    }
    else if (cmd.type == CommandType::CONFIRM_INPUT)
    {
        isEditingSaveName = false;
    }
    else if (cmd.type == CommandType::LOAD_GAME_SLOT)
    {
        if (!cmd.stringPayload.empty())
        {
            if (saveManager::loadFromFile(gameContext, cmd.stringPayload))
            {
                gameContext->changeState(std::make_unique<explorationState>());
            }
        }
    }
}
