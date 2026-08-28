#include "state/shopState.h"

#include <algorithm>
#include <memory>

#include "core/game.h"
#include "state/explorationState.h"

void shopState::initialise(game* gameContext) {}

void shopState::onEnter(game* gameContext)
{
    m_catalog = {
        { "item_health_elixir", "Greater Health Elixir", "Consumable", 45, 8 },
        { "item_succubus_milk", "Succubus Milk Extract", "Transformative", 120, 3 },
        { "item_runic_armor", "Runic Ward Armor", "Equipment", 350, 1 },
        { "item_mana_crystal", "Arcane Mana Crystal", "Consumable", 50, 5 },
        { "item_lust_amulet", "Amulet of Allure", "Accessory", 200, 2 }
    };
    m_selectedIndex = 0;

    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void shopState::onExit(game* gameContext) {}

void shopState::update(game* gameContext, float deltaTime) {}

void shopState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
}

void shopState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
    else if (cmd.type == CommandType::BUY_SHOP_ITEM)
    {
        int idx = cmd.intPayload1;
        if (idx >= 0 && idx < static_cast<int>(m_catalog.size()))
        {
            auto& it = m_catalog[idx];
            if (it.stock > 0 && gameContext->playerEntity)
            {
                // Process transaction
                it.stock--;
            }
        }
        gameContext->refreshActionGrid();
    }
}
