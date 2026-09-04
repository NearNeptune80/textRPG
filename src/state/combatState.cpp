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
        else if (cmd.stringPayload == "STRIKE" || cmd.stringPayload == "HEAVY_STRIKE" ||
                 cmd.stringPayload == "DEFEND" || cmd.stringPayload == "DISARM" ||
                 cmd.stringPayload == "SPELL_DART" || cmd.stringPayload == "SPELL_FIREBALL" ||
                 cmd.stringPayload == "SPELL_SHIELD" || cmd.stringPayload == "SPELL_CLEANSE" ||
                 cmd.stringPayload == "SPELL_BLINK" || cmd.stringPayload == "ITEM_POTION" ||
                 cmd.stringPayload == "ITEM_MANA")
        {
            if (!m_engine.getPlayerParty().empty() && !m_engine.getEnemyParty().empty())
            {
                entity* player = m_engine.getPlayerParty()[0].character.get();
                entity* target = m_engine.getEnemyParty()[0].character.get();
                if (!player || !target) return;

                CombatAction act;
                if (cmd.stringPayload == "STRIKE")
                {
                    act.id = "action_strike";
                    act.name = "Strike";
                    act.baseApCost = 1;
                    SpellEffectNode node;
                    node.effectType = "DAMAGE";
                    node.element = "Physical";
                    node.baseMagnitude = std::max(8.0f, player->getStat("physique") * 0.8f);
                    act.effectNodes.push_back(node);
                }
                else if (cmd.stringPayload == "HEAVY_STRIKE")
                {
                    act.id = "action_heavy_strike";
                    act.name = "Heavy Strike";
                    act.baseApCost = 2;
                    SpellEffectNode node;
                    node.effectType = "DAMAGE";
                    node.element = "Physical";
                    node.baseMagnitude = std::max(18.0f, player->getStat("physique") * 1.8f);
                    act.effectNodes.push_back(node);
                }
                else if (cmd.stringPayload == "DEFEND")
                {
                    act.id = "action_defend";
                    act.name = "Defensive Stance";
                    act.baseApCost = 1;
                    SpellEffectNode node;
                    node.effectType = "SHIELD";
                    node.element = "Physical";
                    node.baseMagnitude = 20.0f;
                    act.effectNodes.push_back(node);
                }
                else if (cmd.stringPayload == "DISARM")
                {
                    act.id = "action_disarm";
                    act.name = "Disarm";
                    act.baseApCost = 2;
                    SpellEffectNode node;
                    node.effectType = "DAMAGE";
                    node.element = "Physical";
                    node.baseMagnitude = 12.0f;
                    act.effectNodes.push_back(node);
                }
                else if (cmd.stringPayload == "SPELL_DART")
                {
                    if (player->getStat("mana") < 10.0f) { m_engine.appendLog("[Combat] Not enough mana for Arcane Dart!"); return; }
                    player->stats.modifyBaseStat("mana", -10.0f);
                    act.id = "spell_arcane_dart";
                    act.name = "Arcane Dart";
                    act.baseApCost = 1;
                    SpellEffectNode node;
                    node.effectType = "DAMAGE";
                    node.element = "Arcane";
                    node.baseMagnitude = 25.0f;
                    act.effectNodes.push_back(node);
                }
                else if (cmd.stringPayload == "SPELL_FIREBALL")
                {
                    if (player->getStat("mana") < 25.0f) { m_engine.appendLog("[Combat] Not enough mana for Fireball!"); return; }
                    player->stats.modifyBaseStat("mana", -25.0f);
                    act.id = "spell_fireball";
                    act.name = "Fireball";
                    act.baseApCost = 2;
                    SpellEffectNode node;
                    node.effectType = "DAMAGE";
                    node.element = "Fire";
                    node.baseMagnitude = 55.0f;
                    act.effectNodes.push_back(node);
                }
                else if (cmd.stringPayload == "SPELL_SHIELD")
                {
                    if (player->getStat("mana") < 15.0f) { m_engine.appendLog("[Combat] Not enough mana for Arcane Shield!"); return; }
                    player->stats.modifyBaseStat("mana", -15.0f);
                    act.id = "spell_arcane_shield";
                    act.name = "Arcane Shield";
                    act.baseApCost = 1;
                    SpellEffectNode node;
                    node.effectType = "SHIELD";
                    node.element = "Arcane";
                    node.baseMagnitude = 35.0f;
                    act.effectNodes.push_back(node);
                }
                else if (cmd.stringPayload == "SPELL_CLEANSE")
                {
                    if (player->getStat("mana") < 20.0f) { m_engine.appendLog("[Combat] Not enough mana for Cleanse!"); return; }
                    player->stats.modifyBaseStat("mana", -20.0f);
                    player->stats.modifyBaseStat("health", 40.0f);
                    m_engine.appendLog(std::format("[Spell] {} cast Cleanse and restored 40 HP!", player->name));
                    m_engine.resolveTurn(gameContext);
                    return;
                }
                else if (cmd.stringPayload == "SPELL_BLINK")
                {
                    if (player->getStat("mana") < 30.0f) { m_engine.appendLog("[Combat] Not enough mana for Blink!"); return; }
                    player->stats.modifyBaseStat("mana", -30.0f);
                    m_engine.appendLog(std::format("[Spell] {} cast Blink, vanishing into arcane mist!", player->name));
                    m_engine.resolveTurn(gameContext);
                    return;
                }
                else if (cmd.stringPayload == "ITEM_POTION")
                {
                    player->stats.modifyBaseStat("health", 50.0f);
                    m_engine.appendLog(std::format("[Item] {} consumed a Health Potion and recovered 50 HP!", player->name));
                    m_engine.resolveTurn(gameContext);
                    return;
                }
                else if (cmd.stringPayload == "ITEM_MANA")
                {
                    player->stats.modifyBaseStat("mana", 50.0f);
                    m_engine.appendLog(std::format("[Item] {} consumed a Mana Crystal and recovered 50 MP!", player->name));
                    m_engine.resolveTurn(gameContext);
                    return;
                }

                m_engine.queuePlayerAction(0, act, target);
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