#include "state/shopState.h"

#include <algorithm>
#include <format>
#include <memory>
#include <vector>

#include "core/game.h"
#include "items/itemDatabase.h"
#include "items/merchantValuation.h"
#include "state/explorationState.h"

shopState::shopState(std::shared_ptr<entity> merchant, std::unique_ptr<iGameState> returnState)
    : m_merchant(std::move(merchant))
    , m_returnState(std::move(returnState))
{
}

void shopState::initialise(game* gameContext) {}

void shopState::onEnter(game* gameContext)
{
    if (!m_merchant && gameContext)
    {
        if (gameContext->getActiveTargetNPCShared())
        {
            m_merchant = gameContext->getActiveTargetNPCShared();
        }
        else
        {
            m_merchant = std::make_shared<entity>("marcus", "Marcus");
            m_merchant->stats.setBaseStat("currency", 1500.0f);
            m_merchant->baseMerchantGold = 1500.0f;
            m_merchant->buyMarkup = 1.20f;   // 20% markup on wares sold to player
            m_merchant->sellMarkdown = 0.55f;// 55% markdown on items bought from player
            m_merchant->merchantAffinity = 1.0f;

            static const std::vector<std::pair<std::string, int>> defaultStock = {
                { "item_canis_root", 5 },
                { "item_linen_shirt", 2 },
                { "item_leather_trousers", 2 },
                { "item_leather_boots", 1 },
                { "item_cloth_gloves", 2 },
                { "item_leather_choker", 1 },
                { "item_silk_bra", 1 },
                { "item_silk_panties", 1 },
                { "item_ancient_tome", 1 }
            };

            for (const auto& [itemId, count] : defaultStock)
            {
                for (int i = 0; i < count; ++i)
                {
                    auto it = itemDatabase::getItem(itemId);
                    if (it) m_merchant->inventory.addItem(it);
                }
            }
        }
    }

    if (gameContext)
    {
        gameContext->activeTargetNPC = m_merchant;
        gameContext->selectedInventorySide = -1;
        gameContext->selectedInventoryIndex = -1;
        gameContext->selectedEquipmentSlot = equipSlot::NONE;
        gameContext->refreshActionGrid();
    }
}

void shopState::onExit(game* gameContext) {}

void shopState::update(game* gameContext, float deltaTime) {}

void shopState::goBack(game* gameContext)
{
    if (!gameContext) return;
    if (m_returnState)
    {
        gameContext->changeState(std::move(m_returnState));
    }
    else
    {
        gameContext->changeState(std::make_unique<explorationState>());
    }
}

void shopState::handleBuyAction(game* gameContext, int merchantStackIndex, int count)
{
    if (!gameContext || !m_merchant) return;
    entity* player = gameContext->getPlayer();
    if (!player) return;

    auto merchView = m_merchant->inventory.getStackedView();
    if (merchantStackIndex < 0 || static_cast<size_t>(merchantStackIndex) >= merchView.size()) return;

    const auto& slot = merchView[merchantStackIndex];
    if (!slot.itemPtr) return;

    int unitPrice = merchantValuation::calculateBuyPrice(slot.itemPtr.get(), player, m_merchant.get());
    int buyCount = std::clamp(count, 1, slot.totalCount);

    float playerGold = player->getStat("currency");
    int totalCost = unitPrice * buyCount;

    if (playerGold < totalCost)
    {
        int maxAffordable = static_cast<int>(playerGold / unitPrice);
        if (maxAffordable <= 0)
        {
            m_feedbackText = std::format("Cannot afford {}! Cost: {}¤ (You have {:.0f}¤).", slot.itemPtr->name, unitPrice, playerGold);
            gameContext->refreshActionGrid();
            return;
        }
        buyCount = maxAffordable;
        totalCost = unitPrice * buyCount;
    }

    // Execute transaction
    player->stats.modifyBaseStat("currency", -static_cast<float>(totalCost));
    m_merchant->stats.modifyBaseStat("currency", static_cast<float>(totalCost));

    std::string boughtItemId = slot.itemPtr->id;
    std::string boughtItemName = slot.itemPtr->name;

    m_merchant->inventory.removeItem(boughtItemId, buyCount);

    for (int i = 0; i < buyCount; ++i)
    {
        auto copy = std::make_shared<item>(*slot.itemPtr);
        copy->count = 1;
        player->inventory.addItem(copy);
    }

    m_feedbackText = std::format("Purchased {} x{} for {}¤.", boughtItemName, buyCount, totalCost);

    // Refresh stack view bounds
    auto updatedMerchView = m_merchant->inventory.getStackedView();
    if (merchantStackIndex >= static_cast<int>(updatedMerchView.size()))
    {
        gameContext->selectedInventoryIndex = -1;
    }

    gameContext->refreshActionGrid();
}

void shopState::handleSellAction(game* gameContext, int playerStackIndex, int count)
{
    if (!gameContext || !m_merchant) return;
    entity* player = gameContext->getPlayer();
    if (!player) return;

    auto playerView = player->inventory.getStackedView();
    if (playerStackIndex < 0 || static_cast<size_t>(playerStackIndex) >= playerView.size()) return;

    const auto& slot = playerView[playerStackIndex];
    if (!slot.itemPtr) return;

    if (slot.itemPtr->isKeyItem)
    {
        m_feedbackText = std::format("{} is a key quest item and cannot be sold!", slot.itemPtr->name);
        gameContext->refreshActionGrid();
        return;
    }

    int unitPrice = merchantValuation::calculateSellPrice(slot.itemPtr.get(), player, m_merchant.get());
    int sellCount = std::clamp(count, 1, slot.totalCount);

    float merchGold = m_merchant->getStat("currency");
    int totalEarned = unitPrice * sellCount;

    if (merchGold < totalEarned)
    {
        int maxAffordable = static_cast<int>(merchGold / unitPrice);
        if (maxAffordable <= 0)
        {
            m_feedbackText = std::format("{} cannot afford to purchase this! (Merchant has {:.0f}¤)", m_merchant->name, merchGold);
            gameContext->refreshActionGrid();
            return;
        }
        sellCount = maxAffordable;
        totalEarned = unitPrice * sellCount;
    }

    // Execute transaction
    std::string soldItemId = slot.itemPtr->id;
    std::string soldItemName = slot.itemPtr->name;

    player->inventory.removeItem(soldItemId, sellCount);

    for (int i = 0; i < sellCount; ++i)
    {
        auto copy = std::make_shared<item>(*slot.itemPtr);
        copy->count = 1;
        m_merchant->inventory.addItem(copy);
    }

    player->stats.modifyBaseStat("currency", static_cast<float>(totalEarned));
    m_merchant->stats.modifyBaseStat("currency", -static_cast<float>(totalEarned));

    m_feedbackText = std::format("Sold {} x{} for {}¤.", soldItemName, sellCount, totalEarned);

    auto updatedPlayerView = player->inventory.getStackedView();
    if (playerStackIndex >= static_cast<int>(updatedPlayerView.size()))
    {
        gameContext->selectedInventoryIndex = -1;
    }

    gameContext->refreshActionGrid();
}

void shopState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::CLOSE_MENU)
    {
        goBack(gameContext);
    }
    else if (cmd.type == CommandType::SELECT_INVENTORY_SLOT)
    {
        gameContext->selectedInventorySide = cmd.intPayload1;
        gameContext->selectedInventoryIndex = cmd.intPayload2;
        gameContext->selectedEquipmentSlot = equipSlot::NONE;
        gameContext->refreshActionGrid();
    }
    else if (cmd.type == CommandType::BUY_SHOP_ITEM)
    {
        int count = cmd.intPayload2 > 0 ? cmd.intPayload2 : 1;
        handleBuyAction(gameContext, cmd.intPayload1, count);
    }
    else if (cmd.type == CommandType::SELL_SHOP_ITEM)
    {
        int count = cmd.intPayload2 > 0 ? cmd.intPayload2 : 1;
        handleSellAction(gameContext, cmd.intPayload1, count);
    }
}
