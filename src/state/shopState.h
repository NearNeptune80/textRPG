#pragma once

#include <memory>
#include <string>
#include <vector>

#include "entities/entity.h"
#include "items/item.h"
#include "state/iGameState.h"

/**
 * Headless state controller for Merchant Shopping & Trading.
 * Manages player-merchant barter economy, buy/sell transactions,
 * page navigation, and feedback messages.
 */
class shopState : public iGameState
{
public:
    explicit shopState(std::shared_ptr<entity> merchant = nullptr, std::unique_ptr<iGameState> returnState = nullptr);
    ~shopState() override = default;

    void initialise(game* gameContext) override;
    void handleCommand(game* gameContext, const UICommand& cmd) override;
    void update(game* gameContext, float deltaTime) override;

    void onEnter(game* gameContext) override;
    void onExit(game* gameContext) override;

    std::shared_ptr<entity> getMerchant() const { return m_merchant; }
    void setMerchant(std::shared_ptr<entity> merchant) { m_merchant = std::move(merchant); }

    int getPlayerPage() const { return m_playerPage; }
    void setPlayerPage(int p) { m_playerPage = p; }

    int getMerchantPage() const { return m_merchantPage; }
    void setMerchantPage(int p) { m_merchantPage = p; }

    const std::string& getFeedbackText() const { return m_feedbackText; }
    void setFeedbackText(std::string feedback) { m_feedbackText = std::move(feedback); }

    void handleBuyAction(game* gameContext, int merchantStackIndex, int count = 1);
    void handleSellAction(game* gameContext, int playerStackIndex, int count = 1);
    void goBack(game* gameContext);

private:
    std::shared_ptr<entity> m_merchant = nullptr;
    std::unique_ptr<iGameState> m_returnState = nullptr;

    int m_playerPage = 0;
    int m_merchantPage = 0;
    std::string m_feedbackText = "";
};
