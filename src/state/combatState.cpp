#include "state/combatState.h"

#include <algorithm>
#include <format>
#include <memory>
#include "combat/weaponSkillDatabase.h"

#include "common/randomEngine.h"
#include "core/eventBus.h"
#include "core/game.h"
#include "entities/entity.h"
#include "events/gameEvents.h"
#include "map/encounterResolver.h"
#include "items/item.h"
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
        if (!gameContext->activeTargetNPC && !m_engine.getEnemyParty().empty())
        {
            gameContext->activeTargetNPC = m_engine.getEnemyParty().front().character;
            gameContext->activeTargetMode = TargetMode::COMBAT_ENEMY;
        }
        gameContext->refreshActionGrid();
    }
}

void CombatState::onExit(game* gameContext)
{
    if (gameContext)
    {
        gameContext->activeTargetNPC = nullptr;
        gameContext->activeTargetMode = TargetMode::NONE;
    }
}

void CombatState::setCombatFocus(CombatFocus focus)
{
    if (m_currentFocus != focus)
    {
        if (!m_engine.getPlayerParty().empty())
        {
            m_engine.clearPlayerQueue(0);
        }
        m_currentFocus = focus;
    }
}

void CombatState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::SELECT_COMBAT_FOCUS)
    {
        std::string fStr = cmd.stringPayload;
        std::transform(fStr.begin(), fStr.end(), fStr.begin(), ::toupper);
        CombatFocus nextFocus = CombatFocus::ROOT;
        if (fStr == "WEAPON") nextFocus = CombatFocus::WEAPON;
        else if (fStr == "MAGIC") nextFocus = CombatFocus::MAGIC;
        else if (fStr == "DEFENSE") nextFocus = CombatFocus::DEFENSE;
        else if (fStr == "SEDUCTION") nextFocus = CombatFocus::SEDUCTION;
        else if (fStr == "COMPANION") nextFocus = CombatFocus::COMPANION;
        else if (fStr == "ITEM") nextFocus = CombatFocus::ITEM;

        setCombatFocus(nextFocus);
        gameContext->refreshActionGrid();
        return;
    }
    else if (cmd.type == CommandType::CLEAR_COMBAT_QUEUE ||
            (cmd.type == CommandType::EXECUTE_COMBAT_ACTION && cmd.stringPayload == "CLEAR_QUEUE"))
    {
        handleClearQueue(gameContext);
        return;
    }
    else if (cmd.type == CommandType::EXECUTE_COMBAT_TURN || cmd.type == CommandType::END_TURN ||
            (cmd.type == CommandType::EXECUTE_COMBAT_ACTION && cmd.stringPayload == "END_TURN"))
    {
        handleEndTurn(gameContext);
        return;
    }
    else if (cmd.type == CommandType::SURRENDER ||
            (cmd.type == CommandType::EXECUTE_COMBAT_ACTION && cmd.stringPayload == "SURRENDER"))
    {
        handleSurrender(gameContext);
        return;
    }
    else if (cmd.type == CommandType::RUN_ATTEMPT ||
            (cmd.type == CommandType::EXECUTE_COMBAT_ACTION && cmd.stringPayload == "ESCAPE"))
    {
        handleRunAttempt(gameContext);
        return;
    }
    else if (cmd.type == CommandType::EXECUTE_COMBAT_ACTION && cmd.stringPayload == "WIN")
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
    else if (cmd.type == CommandType::EXECUTE_COMBAT_ACTION && cmd.stringPayload == "DEFEAT")
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
    else if (cmd.type == CommandType::QUEUE_COMBAT_ACTION || cmd.type == CommandType::EXECUTE_COMBAT_ACTION)
    {
        auto& players = m_engine.getPlayerParty();
        if (players.empty() || !players[0].character) return;
        auto& p = players[0];
        entity* player = p.character.get();

        entity* target = gameContext->getActiveTargetNPC();
        if (!target || target->getStat("health") <= 0.0f || target->getStat("lust") >= 100.0f)
        {
            for (const auto& ep : m_engine.getEnemyParty())
            {
                if (ep.character && ep.character->getStat("health") > 0.0f && ep.character->getStat("lust") < 100.0f)
                {
                    target = ep.character.get();
                    gameContext->activeTargetNPC = ep.character;
                    break;
                }
            }
        }
        if (!target) return;

        CombatAction act;
        const WeaponSkill* wSkill = WeaponSkillDatabase::getSkill(cmd.stringPayload);
        if (wSkill)
        {
            std::string elem = "physical";
            auto eqWep = player->inventory.getEquippedItem(equipSlot::WEAPON_MAIN);
            if (eqWep)
            {
                for (const auto& inf : eqWep->infusionEffects)
                {
                    if (inf.focus == EnchantmentFocus::WEAPON_LETHALITY && !eqWep->baseRace.empty())
                    {
                        elem = eqWep->baseRace;
                    }
                }
            }
            act = wSkill->toCombatAction(elem);
        }
        else if (cmd.stringPayload == "DEFENSE_GUARD" || cmd.stringPayload == "DEFEND")
        {
            act.id = "action_guard";
            act.name = "Guard Stance";
            act.baseApCost = 1;
            act.staminaCost = 15.0f;
            act.isGuarding = true;
        }
        else if (cmd.stringPayload == "DEFENSE_DODGE")
        {
            act.id = "action_dodge";
            act.name = "Evasive Dodge";
            act.baseApCost = 1;
            act.staminaCost = 20.0f;
            act.isDodging = true;
        }
        else if (cmd.stringPayload == "DEFENSE_PARRY")
        {
            act.id = "action_parry";
            act.name = "Counter Parry";
            act.baseApCost = 1;
            act.staminaCost = 25.0f;
            act.isParrying = true;
        }
        else if (cmd.stringPayload == "SEDUCE_TEASE")
        {
            act.id = "action_seduce_tease";
            act.name = "Tease";
            act.baseApCost = 1;
            act.staminaCost = 10.0f;
            act.lustDamage = 15.0f;
        }
        else if (cmd.stringPayload == "SEDUCE_FLIRT")
        {
            act.id = "action_seduce_flirt";
            act.name = "Flirtatious Whisper";
            act.baseApCost = 1;
            act.staminaCost = 15.0f;
            act.lustDamage = 25.0f;
        }
        else if (cmd.stringPayload == "SEDUCE_EXPOSE")
        {
            act.id = "action_seduce_expose";
            act.name = "Sensual Exposure";
            act.baseApCost = 1;
            act.staminaCost = 20.0f;
            act.lustDamage = 35.0f;
        }
        else if (cmd.stringPayload == "COMPANION_ATTACK")
        {
            act.id = "action_companion_attack";
            act.name = "Familiar Strike";
            act.baseApCost = 1;
            act.staminaCost = 15.0f;
            SpellEffectNode node;
            node.effectType = "DAMAGE";
            std::string cElem = "air";
            if (m_engine.getPlayerParty().size() > 1 && m_engine.getPlayerParty()[1].character)
            {
                cElem = m_engine.getPlayerParty()[1].character->elementalType.empty() ? "air" : m_engine.getPlayerParty()[1].character->elementalType;
            }
            else if (gameContext && !gameContext->getCompanions().empty() && gameContext->getCompanions()[0])
            {
                cElem = gameContext->getCompanions()[0]->elementalType.empty() ? "air" : gameContext->getCompanions()[0]->elementalType;
            }
            node.element = cElem;
            node.baseMagnitude = 30.0f;
            act.effectNodes.push_back(node);
        }
        else if (cmd.stringPayload == "STRIKE")
        {
            act.id = "action_strike";
            act.name = "Strike";
            act.baseApCost = 1;
            act.staminaCost = 15.0f;
            SpellEffectNode node;
            node.effectType = "DAMAGE";
            node.element = "physical";
            node.baseMagnitude = std::max(8.0f, player->getStat("physique") * 0.8f);
            act.effectNodes.push_back(node);
        }
        else if (cmd.stringPayload == "HEAVY_STRIKE")
        {
            act.id = "action_heavy_strike";
            act.name = "Heavy Strike";
            act.baseApCost = 2;
            act.staminaCost = 35.0f;
            SpellEffectNode node;
            node.effectType = "DAMAGE";
            node.element = "physical";
            node.baseMagnitude = std::max(18.0f, player->getStat("physique") * 1.8f);
            act.effectNodes.push_back(node);
        }
        else if (cmd.stringPayload == "DISARM")
        {
            act.id = "action_disarm";
            act.name = "Disarm";
            act.baseApCost = 2;
            act.staminaCost = 30.0f;
            SpellEffectNode node;
            node.effectType = "DAMAGE";
            node.element = "physical";
            node.baseMagnitude = 14.0f;
            act.effectNodes.push_back(node);
        }
        else if (cmd.stringPayload == "SPELL_DART")
        {
            act.id = "spell_arcane_dart";
            act.name = "Arcane Dart";
            act.baseApCost = 1;
            act.staminaCost = 10.0f;
            act.manaCost = 10.0f;
            SpellEffectNode node;
            node.effectType = "DAMAGE";
            node.element = "arcane";
            node.baseMagnitude = 25.0f;
            act.effectNodes.push_back(node);
        }
        else if (cmd.stringPayload == "SPELL_FIREBALL")
        {
            act.id = "spell_fireball";
            act.name = "Fireball";
            act.baseApCost = 2;
            act.staminaCost = 15.0f;
            act.manaCost = 25.0f;
            SpellEffectNode node;
            node.effectType = "DAMAGE";
            node.element = "fire";
            node.baseMagnitude = 55.0f;
            act.effectNodes.push_back(node);
        }
        else if (cmd.stringPayload == "SPELL_SHIELD")
        {
            act.id = "spell_arcane_shield";
            act.name = "Arcane Shield";
            act.baseApCost = 1;
            act.staminaCost = 10.0f;
            act.manaCost = 15.0f;
            act.customExecute = [this](entity* user, entity* target, game* g) {
                if (!user) return;
                user->stats.modifyBaseStat("health", 35.0f);
                m_engine.appendLog(std::format("{} conjures an Arcane Shield, absorbing 35 HP damage!", user->name));
            };
        }
        else if (cmd.stringPayload == "SPELL_CLEANSE")
        {
            act.id = "spell_cleanse";
            act.name = "Cleanse";
            act.baseApCost = 1;
            act.staminaCost = 10.0f;
            act.manaCost = 20.0f;
            act.customExecute = [this](entity* user, entity* target, game* g) {
                if (!user) return;
                user->stats.modifyBaseStat("health", 40.0f);
                user->statusEffects.clear();
                m_engine.appendLog(std::format("[Spell] {} cast Cleanse, purged debuffs and restored 40 HP!", user->name));
            };
        }
        else if (cmd.stringPayload == "SPELL_BLINK")
        {
            act.id = "spell_blink";
            act.name = "Blink";
            act.baseApCost = 1;
            act.staminaCost = 10.0f;
            act.manaCost = 30.0f;
            act.customExecute = [this](entity* user, entity* target, game* g) {
                if (!user) return;
                user->stats.modifyBaseStat("agility", 15.0f);
                m_engine.appendLog(std::format("[Spell] {} cast Blink, evading enemy strikes!", user->name));
            };
        }
        else if (cmd.stringPayload == "ITEM_POTION")
        {
            act.id = "item_potion";
            act.name = "Health Potion";
            act.baseApCost = 1;
            act.customExecute = [this](entity* user, entity* target, game* g) {
                if (!user) return;
                user->stats.modifyBaseStat("health", 50.0f);
                m_engine.appendLog(std::format("[Item] {} consumed a Health Potion and recovered 50 HP!", user->name));
            };
        }
        else if (cmd.stringPayload == "ITEM_MANA")
        {
            act.id = "item_mana";
            act.name = "Mana Crystal";
            act.baseApCost = 1;
            act.customExecute = [this](entity* user, entity* target, game* g) {
                if (!user) return;
                user->stats.modifyBaseStat("mana", 50.0f);
                m_engine.appendLog(std::format("[Item] {} consumed a Mana Crystal and recovered 50 MP!", user->name));
            };
        }

        if (m_engine.queuePlayerAction(0, act, target))
        {
            if (cmd.stringPayload == "ITEM_POTION")
            {
                player->inventory.removeItem("item_health_potion", 1);
            }
            m_engine.appendLog(std::format("[Queued #{}] {} -> {} ({:.0f} Sta, {:.0f} MP, {:.0f} Sta left)",
                p.turnQueue.size(), act.name, target->name, act.staminaCost, act.manaCost, p.currentStamina));
            gameContext->refreshActionGrid();
        }
        return;
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

    if (defeatingNPC && sexuallyCompatible && npcLust >= 20.0f && gameContext->settings.content.nonConEnabled)
    {
        float lossPercent = gameContext->settings.gameplay.currencyLossOnDefeatPercent;
        float currentMoney = player ? player->getStat("currency") : 0.0f;
        int currencyLost = static_cast<int>(currentMoney * lossPercent);
        if (player && currencyLost > 0)
        {
            player->stats.modifyBaseStat("currency", -static_cast<float>(currencyLost));
        }

        eventBus::getInstance().publishEvent({ gameEvent::combatEnded, data.numericValue, "DEFEAT_SEDUCTION", nullptr });

        questScene defeatScene;
        defeatScene.id = "scene_combat_seduction";
        defeatScene.speakerName = defeatingNPC->name;

        std::string npcArchetype = genderArchetypeToString(defeatingNPC->genderArchetype);
        std::string npcRace = defeatingNPC->anatomy.getDominantRace();

        defeatScene.bodyText = std::format("With your strength exhausted, you collapse to the ground. "
                                          "{} ({}, {}) stands over your helpless body with a hungry, aroused glare. "
                                          "Rather than finishing you off, they claim their erotic prize (and take {}¤) before leaving you thoroughly spent.",
                                          defeatingNPC->name, npcArchetype, npcRace, currencyLost);

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
    if (!gameContext) return;
    m_engine.resolveTurn(gameContext);
    if (!m_engine.isCombatOver())
    {
        gameContext->refreshActionGrid();
    }
}

void CombatState::handleClearQueue(game* gameContext)
{
    if (!gameContext) return;
    auto& players = m_engine.getPlayerParty();
    if (!players.empty())
    {
        auto& p = players[0];
        if (p.character)
        {
            for (const auto& qa : p.turnQueue)
            {
                if (qa.action.manaCost > 0.0f)
                {
                    p.character->stats.modifyBaseStat("mana", qa.action.manaCost);
                }
                if (qa.action.id == "item_potion")
                {
                    auto pot = std::make_shared<item>();
                    pot->id = "item_health_potion";
                    pot->name = "Health Potion";
                    pot->description = "Restores 50 health";
                    pot->category = ItemCategory::CONSUMABLE;
                    pot->isConsumable = true;
                    pot->count = 1;
                    p.character->inventory.addItem(pot);
                }
            }
        }
        m_engine.clearPlayerQueue(0);
        m_engine.appendLog("[Combat] Action queue cleared. AP and resources refunded.");
        gameContext->refreshActionGrid();
    }
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