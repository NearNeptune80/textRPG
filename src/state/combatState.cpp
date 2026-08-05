#include "state/combatState.h"

#include <algorithm>
#include <format>
#include <random> // Added for std::random_device, std::mt19937, std::uniform_real_distribution

#include "core/game.h"
#include "entities/entity.h"
#include "core/eventBus.h"
#include "events/gameEvents.h"

CombatState::CombatState(const std::vector<std::shared_ptr<entity>>& playerParty,
                         const std::vector<std::shared_ptr<entity>>& enemyParty)
{
    m_engine.initialiseCombat(playerParty, enemyParty);
}

void CombatState::initialise(game* gameContext)
{
}

void CombatState::onEnter(game* gameContext)
{
}

void CombatState::onExit(game* gameContext)
{
}

void CombatState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        // Handle input click logic for action grid & target selection
    }
}

void CombatState::update(game* gameContext, float deltaTime)
{
    if (!gameContext) return;

    if (m_engine.isCombatOver())
    {
        eventData data;

        if (m_engine.isPlayerVictory())
        {
            data.numericValue = static_cast<float>(CombatOutcome::VICTORY);
            eventBus::getInstance().publish(gameEvent::combatEnded, data);
        }
        else
        {
            data.numericValue = static_cast<float>(CombatOutcome::DEFEAT);

            auto& playerParty = m_engine.getPlayerParty();
            if (!playerParty.empty() && playerParty[0].character)
            {
                float currentMoney = playerParty[0].character->getStat("currency");
                float penalty = currentMoney * 0.15f;
                playerParty[0].character->stats.modifyBaseStat("currency", -penalty);
            }

            eventBus::getInstance().publish(gameEvent::combatEnded, data);
        }

        gameContext->popState();
    }
}

void CombatState::render(game* gameContext)
{
    if (!gameContext) return;

    renderPartyCards(gameContext);
    renderCombatLog(gameContext);
    renderActionGrid(gameContext);
}

void CombatState::renderPartyCards(game* gameContext)
{
}

void CombatState::renderCombatLog(game* gameContext)
{
}

void CombatState::renderActionGrid(game* gameContext)
{
}

void CombatState::handleGridClick(game* gameContext, int slotIndex)
{
    if (slotIndex < 10)
    {
        entity* target = nullptr;

        if (m_targetIsEnemy && m_selectedTargetIndex < m_engine.getEnemyParty().size())
        {
            target = m_engine.getEnemyParty()[m_selectedTargetIndex].character.get();
        }
        else if (!m_targetIsEnemy && m_selectedTargetIndex < m_engine.getPlayerParty().size())
        {
            target = m_engine.getPlayerParty()[m_selectedTargetIndex].character.get();
        }

        if (!target) return;

        CombatAction selectedAction;
        m_engine.queuePlayerAction(0, selectedAction, target, m_showingSecondaryTab);
    }
    else if (slotIndex == 10)
    {
        handleEndTurn(gameContext);
    }
    else if (slotIndex == 11)
    {
        handleRunAttempt(gameContext);
    }
    else if (slotIndex == 12)
    {
        handleSurrender(gameContext);
    }
}

void CombatState::handleEndTurn(game* gameContext)
{
    m_engine.resolveTurn(gameContext);
}

void CombatState::handleRunAttempt(game* gameContext)
{
    if (!gameContext) return;

    float playerAgility = 10.0f;
    float enemyAgility = 10.0f;

    auto& players = m_engine.getPlayerParty();
    auto& enemies = m_engine.getEnemyParty();

    if (!players.empty() && players[0].character)
    {
        playerAgility = players[0].character->getStat("agility");
    }

    if (!enemies.empty() && enemies[0].character)
    {
        enemyAgility = enemies[0].character->getStat("agility");
    }

    float fleeChance = 0.50f + ((playerAgility - enemyAgility) * 0.05f);
    fleeChance = std::clamp(fleeChance, 0.10f, 0.90f);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    if (dist(gen) <= fleeChance)
    {
        eventData data;
        data.numericValue = static_cast<float>(CombatOutcome::ESCAPE);
        eventBus::getInstance().publish(gameEvent::combatEnded, data);

        gameContext->popState();
    }
    else
    {
        m_engine.appendLog("Failed to escape!");
        handleEndTurn(gameContext);
    }
}

void CombatState::handleSurrender(game* gameContext)
{
    if (!gameContext) return;

    eventData data;
    data.numericValue = static_cast<float>(CombatOutcome::SURRENDER);

    auto& playerParty = m_engine.getPlayerParty();
    if (!playerParty.empty() && playerParty[0].character)
    {
        float currentMoney = playerParty[0].character->getStat("currency");
        float penalty = currentMoney * 0.20f;
        playerParty[0].character->stats.modifyBaseStat("currency", -penalty);
    }

    eventBus::getInstance().publish(gameEvent::combatEnded, data);
    gameContext->popState();
}

std::vector<CombatAction> CombatState::getAvailableSecondaryActions()
{
    return {};
}