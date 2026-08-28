#pragma once

#include <string>
#include <vector>

#include "items/item.h"
#include "state/iGameState.h"

struct ShopItem
{
    std::string id;
    std::string name;
    std::string type;
    int price = 0;
    int stock = 0;
};

/**
 * Headless state controller for Merchant Shopping & Trading.
 */
class shopState : public iGameState
{
public:
    shopState() = default;
    ~shopState() override = default;

    void initialise(game* gameContext) override;
    void handleInput(game* gameContext, const SDL_Event& event) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    const std::vector<ShopItem>& getCatalog() const { return m_catalog; }
    int getSelectedShopIndex() const { return m_selectedIndex; }

private:
    std::vector<ShopItem> m_catalog;
    int m_selectedIndex = 0;
};
