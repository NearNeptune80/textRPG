#include "state/combatState.h"

#include <algorithm>
#include <format>
#include <memory>
#include "combat/weaponSkillDatabase.h"
#include "combat/combatActionDatabase.h"
#include "combat/spellDatabase.h"
#include "items/itemDatabase.h"
#include "quest/questDatabase.h"

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

void CombatState::initialise(game* gameContext)
{
    // Lifecycle hook: combat engine initialized in constructor, UI refreshed in onEnter()
}

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
            if (const auto* a = CombatActionDatabase::getAction("action_guard")) act = *a;
        }
        else if (cmd.stringPayload == "DEFENSE_DODGE")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_dodge")) act = *a;
        }
        else if (cmd.stringPayload == "DEFENSE_PARRY")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_parry")) act = *a;
        }
        else if (cmd.stringPayload == "SEDUCE_TEASE")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_seduce_tease")) act = *a;
        }
        else if (cmd.stringPayload == "SEDUCE_FLIRT")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_seduce_flirt")) act = *a;
        }
        else if (cmd.stringPayload == "SEDUCE_EXPOSE")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_seduce_expose")) act = *a;
        }
        else if (cmd.stringPayload == "COMPANION_ATTACK")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_companion_attack"))
            {
                act = *a;
                std::string cElem = "air";
                if (m_engine.getPlayerParty().size() > 1 && m_engine.getPlayerParty()[1].character)
                {
                    cElem = m_engine.getPlayerParty()[1].character->elementalType.empty() ? "air" : m_engine.getPlayerParty()[1].character->elementalType;
                }
                else if (gameContext && !gameContext->getCompanions().empty() && gameContext->getCompanions()[0])
                {
                    cElem = gameContext->getCompanions()[0]->elementalType.empty() ? "air" : gameContext->getCompanions()[0]->elementalType;
                }
                if (!act.effectNodes.empty()) act.effectNodes[0].element = cElem;
            }
        }
        else if (cmd.stringPayload == "STRIKE")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_strike"))
            {
                act = *a;
                if (!act.effectNodes.empty() && player)
                {
                    act.effectNodes[0].baseMagnitude = std::max(act.effectNodes[0].baseMagnitude, player->getStat("physique") * 0.8f);
                }
            }
        }
        else if (cmd.stringPayload == "HEAVY_STRIKE")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_heavy_strike"))
            {
                act = *a;
                if (!act.effectNodes.empty() && player)
                {
                    act.effectNodes[0].baseMagnitude = std::max(act.effectNodes[0].baseMagnitude, player->getStat("physique") * 1.8f);
                }
            }
        }
        else if (cmd.stringPayload == "DISARM")
        {
            if (const auto* a = CombatActionDatabase::getAction("action_disarm")) act = *a;
        }
        else if (cmd.stringPayload == "SPELL_DART")
        {
            if (const auto* sp = SpellDatabase::getSpell("arcane_dart")) act = sp->toCombatAction();
        }
        else if (cmd.stringPayload == "SPELL_FIREBALL")
        {
            if (const auto* sp = SpellDatabase::getSpell("fireball")) act = sp->toCombatAction();
        }
        else if (cmd.stringPayload == "SPELL_SHIELD")
        {
            if (const auto* sp = SpellDatabase::getSpell("arcane_shield")) act = sp->toCombatAction();
        }
        else if (cmd.stringPayload == "SPELL_CLEANSE")
        {
            if (const auto* sp = SpellDatabase::getSpell("cleanse")) act = sp->toCombatAction();
        }
        else if (cmd.stringPayload == "SPELL_BLINK")
        {
            if (const auto* sp = SpellDatabase::getSpell("teleport")) act = sp->toCombatAction();
        }
        else if (cmd.stringPayload == "ITEM_POTION")
        {
            if (const auto* a = CombatActionDatabase::getAction("item_potion")) act = *a;
        }
        else if (cmd.stringPayload == "ITEM_MANA")
        {
            if (const auto* a = CombatActionDatabase::getAction("item_mana")) act = *a;
        }
        else if (const auto* a = CombatActionDatabase::getAction(cmd.stringPayload))
        {
            act = *a;
        }
        else if (const auto* sp = SpellDatabase::getSpell(cmd.stringPayload))
        {
            act = sp->toCombatAction();
        }

        if (m_engine.queuePlayerAction(0, act, target))
        {
            if (cmd.stringPayload == "ITEM_POTION")
            {
                player->inventory.removeItem("item_health_potion", 1);
            }
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

        std::string npcArchetype = genderArchetypeToString(defeatingNPC->genderArchetype);
        std::string npcRace = defeatingNPC->anatomy.getDominantRace();

        questScene defeatScene = questDatabase::getScene("scene_combat_defeat_erotic");
        defeatScene.speakerName = defeatingNPC->name;

        auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
            size_t startPos = 0;
            while ((startPos = str.find(from, startPos)) != std::string::npos) {
                str.replace(startPos, from.length(), to);
                startPos += to.length();
            }
        };

        replaceAll(defeatScene.bodyText, "{speaker}", defeatingNPC->name);
        replaceAll(defeatScene.bodyText, "{archetype}", npcArchetype);
        replaceAll(defeatScene.bodyText, "{race}", npcRace);
        replaceAll(defeatScene.bodyText, "{currency}", std::to_string(currencyLost));

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

        questScene defeatScene = questDatabase::getScene("scene_combat_defeat");
        defeatScene.speakerName = defeatingNPC ? defeatingNPC->name : "System";

        auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
            size_t startPos = 0;
            while ((startPos = str.find(from, startPos)) != std::string::npos) {
                str.replace(startPos, from.length(), to);
                startPos += to.length();
            }
        };

        replaceAll(defeatScene.bodyText, "{speaker}", defeatScene.speakerName);
        replaceAll(defeatScene.bodyText, "{currency}", std::to_string(currencyLost));

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
                if (qa.action.id == "item_potion" || qa.action.id == "item_health_potion" || qa.action.id == "item_potion_health")
                {
                    auto pot = itemDatabase::getItem("item_health_potion");
                    if (!pot) pot = itemDatabase::getItem("item_potion_health");
                    if (pot)
                    {
                        p.character->inventory.addItem(pot);
                    }
                }
            }
        }
        m_engine.clearPlayerQueue(0);
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