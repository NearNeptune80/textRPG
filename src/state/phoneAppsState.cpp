#include "state/phoneAppsState.h"

#include <fstream>
#include "core/game.h"
#include "state/explorationState.h"

phoneAppsState::phoneAppsState(PhoneAppMode mode)
    : m_mode(mode)
{
    loadData(m_mode);
}

void phoneAppsState::initialise(game* gameContext)
{
    loadData(m_mode);
}

void phoneAppsState::onEnter(game* gameContext)
{
    loadData(m_mode);
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void phoneAppsState::onExit(game* gameContext) {}

void phoneAppsState::update(game* gameContext, float deltaTime) {}

void phoneAppsState::handleInput(game* gameContext, const SDL_Event& event) {}

void phoneAppsState::handleCommand(game* gameContext, const UICommand& cmd) {}

void phoneAppsState::loadData(PhoneAppMode mode)
{
    std::string path = "";
    if (mode == PhoneAppMode::ENCYCLOPEDIA)
    {
        path = "data/encyclopedia.json";
    }
    else if (mode == PhoneAppMode::SPELLS)
    {
        path = "data/spells.json";
    }
    else if (mode == PhoneAppMode::PERKS)
    {
        path = "data/perks.json";
    }
    else if (mode == PhoneAppMode::CONTACTS)
    {
        path = "data/contacts.json";
    }
    else if (mode == PhoneAppMode::QUESTS)
    {
        path = "data/quests/quest_intro.json";
    }

    if (!path.empty())
    {
        std::ifstream f(path);
        if (f.is_open())
        {
            try
            {
                f >> m_appData;
            }
            catch (...)
            {
                m_appData = nlohmann::json::object();
            }
        }
    }
}
