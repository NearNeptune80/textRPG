#include "state/combatState.h"

#include <algorithm>
#include <format>
#include <memory>
#include <random>

#include "eventState.h"
#include "core/eventBus.h"
#include "core/game.h"
#include "entities/entity.h"
#include "events/gameEvents.h"
#include "map/encounterResolver.h"
#include "state/explorationState.h"
#include "ui/uiRenderer.h"

CombatState::CombatState(const std::vector<std::shared_ptr<entity>>& playerParty,
                         const std::vector<std::shared_ptr<entity>>& enemyParty)
{
    m_engine.initialiseCombat(playerParty, enemyParty);
}

void CombatState::initialise(game* gameContext) {}

void CombatState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void CombatState::onExit(game* gameContext) {}

void CombatState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        // Target selection & action grid click interactions
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
            data.numericValue = static_cast<int>(CombatOutcome::VICTORY);
            eventBus::getInstance().publishEvent({ gameEvent::combatEnded, data.numericValue, "VICTORY", nullptr });

            std::string currentMapId = gameContext->map ? gameContext->map->getId() : "overworld";
            entity* defeatedNPC = gameContext->activeTargetNPC;

            gameContext->currentScene = encounterResolver::buildVictoryScene(gameContext, defeatedNPC, currentMapId);

            TileRuntimeData& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
            tileData.persistentNPC = nullptr;

            // Transition to eventState and refresh grid immediately
            gameContext->changeState(std::make_unique<eventState>());
            return;
        }
        else
        {
            data.numericValue = static_cast<int>(CombatOutcome::DEFEAT);

            auto& playerParty = m_engine.getPlayerParty();
            int currencyLost = 0;
            if (!playerParty.empty() && playerParty[0].character)
            {
                float currentMoney = playerParty[0].character->getStat("currency");
                currencyLost = static_cast<int>(currentMoney * 0.15f);
                playerParty[0].character->stats.modifyBaseStat("currency", -static_cast<float>(currencyLost));
            }

            eventBus::getInstance().publishEvent({ gameEvent::combatEnded, data.numericValue, "DEFEAT", nullptr });

            // Construct defeat scene
            questScene defeatScene;
            defeatScene.id = "scene_combat_defeat";
            defeatScene.speakerName = "Defeated";
            defeatScene.bodyText = std::format("You were overwhelmed in combat and collapsed! You managed to escape later, but lost {}¤ in the process.", currencyLost);

            dialogueChoice continueChoice;
            continueChoice.label = "Continue";
            continueChoice.nextSceneId = "EXIT";
            defeatScene.choices.push_back(continueChoice);

            gameContext->currentScene = defeatScene;
            gameContext->activeTargetNPC = nullptr;
            gameContext->activeTargetMode = TargetMode::NONE;

            // Transition to eventState
            gameContext->changeState(std::make_unique<eventState>());
            return;
        }
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
    if (!gameContext) return;

    ViewportGuard vpGuard(gameContext->renderer, gameContext->layout.textMainRect);
    UI::DrawPanel(gameContext->renderer,
                  { 0.0f, 0.0f, gameContext->layout.textMainRect.w, gameContext->layout.textMainRect.h },
                  Theme::colors.bgPanel, Theme::colors.borderNormal);

    std::string fullLogText = "";
    for (const auto& line : m_engine.getCombatLog())
    {
        fullLogText += line + "\n";
    }

    SDL_FRect logTextRect = { 12.0f, 12.0f, gameContext->layout.textMainRect.w - 24.0f, gameContext->layout.textMainRect.h - 24.0f };
    gameContext->renderTextWrapped(fullLogText, logTextRect, "button_font", Theme::colors.textPrimary);
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
        int outcomeVal = static_cast<int>(CombatOutcome::ESCAPE);
        eventBus::getInstance().publishEvent({ gameEvent::combatEnded, outcomeVal, "ESCAPE", nullptr });

        gameContext->changeState(std::make_unique<explorationState>());
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

    int outcomeVal = static_cast<int>(CombatOutcome::SURRENDER);

    auto& playerParty = m_engine.getPlayerParty();
    if (!playerParty.empty() && playerParty[0].character)
    {
        float currentMoney = playerParty[0].character->getStat("currency");
        float penalty = currentMoney * 0.20f;
        playerParty[0].character->stats.modifyBaseStat("currency", -penalty);
    }

    eventBus::getInstance().publishEvent({ gameEvent::combatEnded, outcomeVal, "SURRENDER", nullptr });
    gameContext->changeState(std::make_unique<explorationState>());
}

std::vector<CombatAction> CombatState::getAvailableSecondaryActions()
{
    return {};
}