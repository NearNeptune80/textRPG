#include "state/combatState.h"

#include <algorithm>
#include <format>
#include <memory>

#include "common/randomEngine.h"
#include "core/eventBus.h"
#include "core/game.h"
#include "entities/entity.h"
#include "events/gameEvents.h"
#include "map/encounterResolver.h"
#include "state/encounterResolutionState.h"
#include "state/eventState.h"
#include "state/explorationState.h"

static bool isSexuallyCompatible(const entity* npc, const entity* player)
{
    if (!npc || !player) return false;
    if (npc->orientation == SexualOrientation::ASEXUAL) return false;

    GenderArchetype playerArch = player->anatomy.getGenderArchetype();
    BodyPresentation playerPres = player->anatomy.getVisualPresentation();

    if (npc->orientation == SexualOrientation::BISEXUAL)
    {
        return playerArch != GenderArchetype::ASEXUAL_NULL;
    }

    bool playerIsFeminine = (playerArch == GenderArchetype::FEMALE || playerArch == GenderArchetype::GYNOMORPH || playerPres == BodyPresentation::FEMININE);
    bool playerIsMasculine = (playerArch == GenderArchetype::MALE || playerArch == GenderArchetype::ANDROMORPH || playerPres == BodyPresentation::MASCULINE);

    if (npc->genderArchetype == GenderArchetype::MALE || npc->genderArchetype == GenderArchetype::ANDROMORPH)
    {
        if (npc->orientation == SexualOrientation::HETEROSEXUAL) return playerIsFeminine;
        if (npc->orientation == SexualOrientation::HOMOSEXUAL) return playerIsMasculine;
    }
    else if (npc->genderArchetype == GenderArchetype::FEMALE || npc->genderArchetype == GenderArchetype::GYNOMORPH)
    {
        if (npc->orientation == SexualOrientation::HETEROSEXUAL) return playerIsMasculine;
        if (npc->orientation == SexualOrientation::HOMOSEXUAL) return playerIsFeminine;
    }

    return true;
}

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

void CombatState::handleInput(game* gameContext, const SDL_Event& event) {}

void CombatState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::EXECUTE_COMBAT_ACTION)
    {
        if (cmd.stringPayload == "WIN")
        {
            std::vector<std::shared_ptr<entity>> defeatedEnemies;
            for (auto& enemyP : m_engine.getEnemyParty())
            {
                if (enemyP.character)
                {
                    enemyP.character->stats.setBaseStat("health", 0.0f);
                    defeatedEnemies.push_back(enemyP.character);
                }
            }
            if (defeatedEnemies.empty())
            {
                defeatedEnemies.push_back(std::make_shared<entity>("npc_bandit", "Rogue Bandit"));
            }

            if (gameContext->map)
            {
                TileRuntimeData& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                tileData.persistentNPC = nullptr;
            }

            m_engine.appendLog("[Debug] Simulated Combat Victory!");
            int outcomeVal = static_cast<int>(CombatOutcome::VICTORY);
            eventBus::getInstance().publishEvent({ gameEvent::combatEnded, outcomeVal, "VICTORY", nullptr });
            gameContext->changeState(std::make_unique<encounterResolutionState>(defeatedEnemies));
            return;
        }
        else if (cmd.stringPayload == "DEFEAT")
        {
            for (auto& playerP : m_engine.getPlayerParty())
            {
                if (playerP.character)
                {
                    playerP.character->stats.setBaseStat("health", 0.0f);
                }
            }
            m_engine.appendLog("[Debug] Simulated Combat Defeat!");
            resolveDefeat(gameContext);
            return;
        }
        else if (cmd.stringPayload == "ESCAPE")
        {
            m_engine.appendLog("[Debug] Simulated Combat Escape!");
            int outcomeVal = static_cast<int>(CombatOutcome::ESCAPE);
            eventBus::getInstance().publishEvent({ gameEvent::combatEnded, outcomeVal, "ESCAPE", nullptr });
            gameContext->changeState(std::make_unique<explorationState>());
            return;
        }
        else if (cmd.stringPayload == "SURRENDER")
        {
            m_engine.appendLog("[Debug] Simulated Combat Surrender!");
            handleSurrender(gameContext);
            return;
        }
        else if (cmd.stringPayload == "STRIKE")
        {
            if (!m_engine.getPlayerParty().empty() && !m_engine.getEnemyParty().empty())
            {
                entity* target = m_engine.getEnemyParty()[0].character.get();
                CombatAction strike;
                strike.id = "action_basic_strike";
                strike.name = "Strike";
                strike.baseApCost = 1;

                SpellEffectNode dmgNode;
                dmgNode.effectType = "DAMAGE";
                dmgNode.element = "Physical";
                dmgNode.baseMagnitude = m_engine.getPlayerParty()[0].character ? m_engine.getPlayerParty()[0].character->getStat("physique") : 10.0f;
                strike.effectNodes.push_back(dmgNode);

                m_engine.queuePlayerAction(0, strike, target);
                m_engine.resolveTurn(gameContext);
            }
            return;
        }
    }
    else if (cmd.type == CommandType::END_TURN)
    {
        handleEndTurn(gameContext);
    }
    else if (cmd.type == CommandType::RUN_ATTEMPT)
    {
        handleRunAttempt(gameContext);
    }
    else if (cmd.type == CommandType::SURRENDER)
    {
        handleSurrender(gameContext);
    }
}

void CombatState::update(game* gameContext, float deltaTime)
{
    if (!gameContext) return;

    if (m_engine.isCombatOver())
    {
        if (m_engine.isPlayerVictory())
        {
            eventData data;
            data.numericValue = static_cast<int>(CombatOutcome::VICTORY);
            eventBus::getInstance().publishEvent({ gameEvent::combatEnded, data.numericValue, "VICTORY", nullptr });

            std::vector<std::shared_ptr<entity>> defeatedEnemies;
            for (const auto& enemyP : m_engine.getEnemyParty())
            {
                if (enemyP.character)
                {
                    defeatedEnemies.push_back(enemyP.character);
                }
            }

            if (gameContext->map)
            {
                TileRuntimeData& tileData = gameContext->map->getRuntimeData(gameContext->gridX, gameContext->gridY);
                tileData.persistentNPC = nullptr;
            }

            gameContext->changeState(std::make_unique<encounterResolutionState>(defeatedEnemies));
            return;
        }
        else
        {
            resolveDefeat(gameContext);
            return;
        }
    }
}

void CombatState::resolveDefeat(game* gameContext)
{
    if (!gameContext) return;

    entity* defeatingNPC = nullptr;
    for (const auto& enemyP : m_engine.getEnemyParty())
    {
        if (enemyP.character)
        {
            defeatingNPC = enemyP.character.get();
            break;
        }
    }

    entity* player = gameContext->getPlayer();
    bool sexuallyCompatible = defeatingNPC && player && isSexuallyCompatible(defeatingNPC, player);
    float npcLust = defeatingNPC ? defeatingNPC->getStat("lust") : 0.0f;

    eventData data;
    data.numericValue = static_cast<int>(CombatOutcome::DEFEAT);

    if (defeatingNPC && sexuallyCompatible && npcLust >= 20.0f)
    {
        eventBus::getInstance().publishEvent({ gameEvent::combatEnded, data.numericValue, "DEFEAT_SEDUCTION", nullptr });

        questScene defeatScene;
        defeatScene.id = "scene_combat_seduction";
        defeatScene.speakerName = defeatingNPC->name;

        std::string npcArchetype = genderArchetypeToString(defeatingNPC->genderArchetype);
        std::string npcRace = defeatingNPC->anatomy.getDominantRace();

        defeatScene.bodyText = std::format("With your strength exhausted, you collapse to the ground. "
                                          "{} ({}, {}) stands over your helpless body with a hungry, aroused glare. "
                                          "Rather than finishing you off, they claim their erotic prize before leaving you thoroughly spent.",
                                          defeatingNPC->name, npcArchetype, npcRace);

        dialogueChoice continueChoice;
        continueChoice.label = "Recover and Continue";
        continueChoice.nextSceneId = "EXIT";
        defeatScene.choices.push_back(continueChoice);

        gameContext->currentScene = defeatScene;
        gameContext->activeTargetNPC = nullptr;
        gameContext->activeTargetMode = TargetMode::NONE;
        gameContext->changeState(std::make_unique<eventState>());
    }
    else
    {
        float lossPercent = gameContext->settings.gameplay.currencyLossOnDefeatPercent;
        float currentMoney = player ? player->getStat("currency") : 0.0f;
        int currencyLost = static_cast<int>(currentMoney * lossPercent);

        if (player)
        {
            player->stats.modifyBaseStat("currency", -static_cast<float>(currencyLost));
        }

        eventBus::getInstance().publishEvent({ gameEvent::combatEnded, data.numericValue, "DEFEAT_ROBBERY", nullptr });

        questScene defeatScene;
        defeatScene.id = "scene_combat_defeat";
        defeatScene.speakerName = defeatingNPC ? defeatingNPC->name : "System";
        defeatScene.bodyText = std::format("You were defeated in combat! Your opponent looted {}¤ from your purse before departing.", currencyLost);

        dialogueChoice continueChoice;
        continueChoice.label = "Continue";
        continueChoice.nextSceneId = "EXIT";
        defeatScene.choices.push_back(continueChoice);

        gameContext->currentScene = defeatScene;
        gameContext->activeTargetNPC = nullptr;
        gameContext->activeTargetMode = TargetMode::NONE;
        gameContext->changeState(std::make_unique<eventState>());
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

    if (dice::roll01() <= fleeChance)
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