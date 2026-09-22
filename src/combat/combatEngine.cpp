#include "combat/combatEngine.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <cstddef>
#include <nlohmann/json.hpp>

#include "core/game.h"
#include "entities/entity.h"

using json = nlohmann::json;

combatEngine::combatEngine()
{
    loadElements("data/elements.json");
}

void combatEngine::loadElements(const std::string& path)
{
    m_elementalMatchups.clear();
    m_elementalReactions.clear();

    std::ifstream file(path);
    if (!file.is_open()) return;

    try
    {
        json j = json::parse(file);
        if (j.contains("matchups") && j["matchups"].is_object())
        {
            for (auto& [atkElem, matchupObj] : j["matchups"].items())
            {
                std::string lowerAtk = atkElem;
                std::transform(lowerAtk.begin(), lowerAtk.end(), lowerAtk.begin(), ::tolower);

                if (matchupObj.contains("advantages") && matchupObj["advantages"].is_object())
                {
                    for (auto& [defElem, mult] : matchupObj["advantages"].items())
                    {
                        std::string lowerDef = defElem;
                        std::transform(lowerDef.begin(), lowerDef.end(), lowerDef.begin(), ::tolower);
                        m_elementalMatchups[lowerAtk][lowerDef] = mult.get<float>();
                    }
                }
                if (matchupObj.contains("disadvantages") && matchupObj["disadvantages"].is_object())
                {
                    for (auto& [defElem, mult] : matchupObj["disadvantages"].items())
                    {
                        std::string lowerDef = defElem;
                        std::transform(lowerDef.begin(), lowerDef.end(), lowerDef.begin(), ::tolower);
                        m_elementalMatchups[lowerAtk][lowerDef] = mult.get<float>();
                    }
                }
            }
        }

        if (j.contains("reactions") && j["reactions"].is_object())
        {
            for (auto& [comboKey, rObj] : j["reactions"].items())
            {
                ElementalReactionDef def;
                def.name = rObj.value("name", "Elemental Reaction");
                def.damageMultiplier = rObj.value("damageMultiplier", 1.2f);
                def.effect = rObj.value("effect", "");
                def.description = rObj.value("description", "");
                m_elementalReactions[comboKey] = def;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[CombatEngine] Error parsing elements.json: " << e.what() << "\n";
    }
}

float combatEngine::getElementalMultiplier(const std::string& attackElement, const std::string& targetElement) const
{
    std::string lowerAtk = attackElement;
    std::transform(lowerAtk.begin(), lowerAtk.end(), lowerAtk.begin(), ::tolower);
    std::string lowerDef = targetElement;
    std::transform(lowerDef.begin(), lowerDef.end(), lowerDef.begin(), ::tolower);

    if (m_elementalMatchups.contains(lowerAtk))
    {
        const auto& defMap = m_elementalMatchups.at(lowerAtk);
        if (defMap.contains(lowerDef))
        {
            return defMap.at(lowerDef);
        }
    }
    return 1.0f;
}

int combatEngine::calculateParticipantMaxAp(const entity* ent) const
{
    if (!ent) return 3;

    float physique = ent->getStat("physique");
    float agility = ent->getStat("agility");
    int bonusAp = static_cast<int>((physique + agility) / 20.0f);

    return std::clamp(3 + bonusAp, 1, 6);
}

float combatEngine::calculateParticipantMaxStamina(const entity* ent) const
{
    if (!ent) return 100.0f;
    float physique = ent->getStat("physique");
    float toughness = ent->getStat("toughness");
    return 80.0f + (physique * 1.5f) + (toughness * 1.0f);
}

void combatEngine::initialiseCombat(const std::vector<std::shared_ptr<entity>>& playerParty,
                                    const std::vector<std::shared_ptr<entity>>& enemyParty)
{
    m_playerParty.clear();
    m_enemyParty.clear();
    m_combatLog.clear();
    m_currentRound = 0;
    m_lustSurrenderAchieved = false;

    size_t maxPartySize = 4;

    for (size_t i = 0; i < playerParty.size() && i < maxPartySize; ++i)
    {
        const auto& ent = playerParty[i];
        if (!ent) continue;
        CombatParticipant p;
        p.character = ent;
        p.isEnemy = false;
        p.maxAp = calculateParticipantMaxAp(ent.get());
        p.currentAp = p.maxAp;
        p.maxStamina = calculateParticipantMaxStamina(ent.get());
        p.currentStamina = p.maxStamina;
        m_playerParty.push_back(p);
    }

    for (size_t i = 0; i < enemyParty.size() && i < maxPartySize; ++i)
    {
        const auto& ent = enemyParty[i];
        if (!ent) continue;
        CombatParticipant p;
        p.character = ent;
        p.isEnemy = true;
        p.maxAp = calculateParticipantMaxAp(ent.get());
        p.currentAp = p.maxAp;
        p.maxStamina = calculateParticipantMaxStamina(ent.get());
        p.currentStamina = p.maxStamina;
        m_enemyParty.push_back(p);
    }

    appendLog(std::format("=== COMBAT INITIALISED ({} Player(s) vs {} Enemy(ies)) ===",
                          m_playerParty.size(), m_enemyParty.size()));
    startNewRound();
}

void combatEngine::startNewRound()
{
    m_currentRound++;
    appendLog(std::format("--- Round {} ---", m_currentRound));

    for (auto& p : m_playerParty)
    {
        p.maxAp = calculateParticipantMaxAp(p.character.get());
        p.currentAp = p.maxAp;
        p.turnQueue.clear();
        p.isGuarding = false;
        p.isDodging = false;
        p.isParrying = false;
        float regen = 35.0f + (p.character ? p.character->getStat("agility") * 0.5f : 0.0f);
        p.currentStamina = std::min(p.maxStamina, p.currentStamina + regen);
    }

    for (auto& p : m_enemyParty)
    {
        p.maxAp = calculateParticipantMaxAp(p.character.get());
        p.currentAp = p.maxAp;
        p.turnQueue.clear();
        p.isGuarding = false;
        p.isDodging = false;
        p.isParrying = false;
        float regen = 35.0f + (p.character ? p.character->getStat("agility") * 0.5f : 0.0f);
        p.currentStamina = std::min(p.maxStamina, p.currentStamina + regen);
    }
}

bool combatEngine::queuePlayerAction(size_t participantIndex, const CombatAction& action, entity* target, bool isFromSecondaryGrid)
{
    if (participantIndex >= m_playerParty.size()) return false;

    auto& p = m_playerParty[participantIndex];
    int apCost = action.baseApCost + (isFromSecondaryGrid ? 1 : 0);

    if (p.currentAp < apCost) return false;
    if (action.staminaCost > 0.0f && p.currentStamina < action.staminaCost) return false;
    if (action.manaCost > 0.0f && p.character && p.character->getStat("mana") < action.manaCost) return false;

    QueuedAction qa;
    qa.action = action;
    qa.user = p.character.get();
    qa.target = target;
    qa.actualApCost = apCost;

    p.turnQueue.push_back(qa);
    p.currentAp -= apCost;
    if (action.staminaCost > 0.0f) p.currentStamina -= action.staminaCost;
    if (action.manaCost > 0.0f && p.character) p.character->stats.modifyBaseStat("mana", -action.manaCost);

    if (action.isGuarding) p.isGuarding = true;
    if (action.isDodging) p.isDodging = true;
    if (action.isParrying) p.isParrying = true;

    return true;
}

void combatEngine::clearPlayerQueue(size_t participantIndex)
{
    if (participantIndex >= m_playerParty.size()) return;
    auto& p = m_playerParty[participantIndex];
    for (const auto& qa : p.turnQueue)
    {
        if (qa.action.staminaCost > 0.0f) p.currentStamina += qa.action.staminaCost;
        if (qa.action.manaCost > 0.0f && p.character) p.character->stats.modifyBaseStat("mana", qa.action.manaCost);
    }
    p.turnQueue.clear();
    p.currentAp = p.maxAp;
    p.isGuarding = false;
    p.isDodging = false;
    p.isParrying = false;
}

void combatEngine::generateNpcQueues()
{
    for (auto& enemyP : m_enemyParty)
    {
        if (!enemyP.character || enemyP.character->getStat("health") <= 0 || enemyP.character->getStat("lust") >= 100) continue;
        if (m_playerParty.empty()) break;

        entity* playerTarget = m_playerParty[0].character.get();

        while (enemyP.currentAp >= 1)
        {
            CombatAction strike;
            strike.id = "action_basic_strike";
            strike.name = "Strike";
            strike.baseApCost = 1;
            strike.staminaCost = 15.0f;

            SpellEffectNode dmgNode;
            dmgNode.effectType = "DAMAGE";
            dmgNode.element = "physical";
            dmgNode.baseMagnitude = enemyP.character->getStat("physique");
            strike.effectNodes.push_back(dmgNode);

            QueuedAction qa;
            qa.action = strike;
            qa.user = enemyP.character.get();
            qa.target = playerTarget;
            qa.actualApCost = 1;

            enemyP.turnQueue.push_back(qa);
            enemyP.currentAp -= 1;
        }
    }
}

void combatEngine::resolveTurn(game* g)
{
    generateNpcQueues();

    std::vector<QueuedAction> allActions;

    for (const auto& p : m_playerParty)
    {
        allActions.insert(allActions.end(), p.turnQueue.begin(), p.turnQueue.end());
    }
    for (const auto& p : m_enemyParty)
    {
        allActions.insert(allActions.end(), p.turnQueue.begin(), p.turnQueue.end());
    }

    std::stable_sort(allActions.begin(), allActions.end(), [](const QueuedAction& a, const QueuedAction& b) {
        float agiA = a.user ? a.user->getStat("agility") : 0.0f;
        float agiB = b.user ? b.user->getStat("agility") : 0.0f;
        return agiA > agiB;
    });

    for (const auto& qa : allActions)
    {
        if (qa.user && qa.user->getStat("health") > 0 && qa.user->getStat("lust") < 100)
        {
            executeAction(qa, g);
        }

        if (isCombatOver()) break;
    }

    if (!isCombatOver())
    {
        startNewRound();
    }
}

void combatEngine::executeAction(const QueuedAction& qa, game* g)
{
    if (!qa.user || qa.user->getStat("health") <= 0.0f || qa.user->getStat("lust") >= 100.0f) return;

    entity* target = qa.target;
    if (target && (target->getStat("health") <= 0.0f || target->getStat("lust") >= 100.0f))
    {
        bool isUserPlayer = false;
        for (const auto& pp : m_playerParty)
        {
            if (pp.character.get() == qa.user) { isUserPlayer = true; break; }
        }
        if (isUserPlayer)
        {
            for (const auto& ep : m_enemyParty)
            {
                if (ep.character && ep.character->getStat("health") > 0.0f && ep.character->getStat("lust") < 100.0f)
                {
                    target = ep.character.get();
                    break;
                }
            }
        }
        else
        {
            for (const auto& pp : m_playerParty)
            {
                if (pp.character && pp.character->getStat("health") > 0.0f && pp.character->getStat("lust") < 100.0f)
                {
                    target = pp.character.get();
                    break;
                }
            }
        }
    }

    if (qa.action.customExecute)
    {
        qa.action.customExecute(qa.user, target ? target : qa.target, g);
        return;
    }

    if (!target || (target->getStat("health") <= 0.0f && target->getStat("lust") >= 100.0f)) return;

    CombatParticipant* targetPart = nullptr;
    for (auto& ep : m_enemyParty) { if (ep.character.get() == target) targetPart = &ep; }
    for (auto& pp : m_playerParty) { if (pp.character.get() == target) targetPart = &pp; }

    // Seduction / Lust Action Resolution
    if (qa.action.lustDamage > 0.0f)
    {
        float lustGained = qa.action.lustDamage * (1.0f + (qa.user->getStat("willpower") * 0.02f));
        target->stats.modifyBaseStat("lust", lustGained);
        appendLog(std::format("{} executes {} on {}! (+{:.0f} Lust -> {:.0f}/100)",
                              qa.user->name, qa.action.name, target->name, lustGained, target->getStat("lust")));

        if (target->getStat("lust") >= 100.0f)
        {
            target->stats.setBaseStat("lust", 100.0f);
            appendLog(std::format("{} collapses in an overwhelming, trembling climax and surrenders!", target->name));
            if (std::all_of(m_enemyParty.begin(), m_enemyParty.end(), [](const CombatParticipant& ep) {
                return !ep.character || ep.character->getStat("health") <= 0.0f || ep.character->getStat("lust") >= 100.0f;
            }))
            {
                m_lustSurrenderAchieved = true;
            }
        }
    }

    for (const auto& node : qa.action.effectNodes)
    {
        if (node.effectType == "DAMAGE")
        {
            float rawDamage = node.baseMagnitude * qa.action.damageMultiplier;
            std::string attackType = node.element.empty() ? "physical" : node.element;
            std::transform(attackType.begin(), attackType.end(), attackType.begin(), ::tolower);

            // Target Defensive Element Check
            std::string targetDefElement = target->elementalType.empty() ? "physical" : target->elementalType;
            float elemMult = getElementalMultiplier(attackType, targetDefElement);
            float modifiedDamage = rawDamage * elemMult;

            // Attacker & Defender Perk Multipliers
            std::string targetRace = target->anatomy.getDominantRace();
            float perkDmgMult = qa.user->getPerkDamageMultiplier(targetRace, attackType);
            modifiedDamage *= (1.0f + perkDmgMult);

            std::string attackerRace = qa.user->anatomy.getDominantRace();
            float perkDefMult = target->getPerkDefenseMultiplier(attackerRace, attackType);
            modifiedDamage *= std::max(0.05f, (1.0f - perkDefMult));

            // Elemental Reactions Check
            if (targetPart && !targetPart->currentElementStatus.empty() && targetPart->currentElementStatus != attackType)
            {
                std::string comboKey = targetPart->currentElementStatus + "+" + attackType;
                std::string reverseComboKey = attackType + "+" + targetPart->currentElementStatus;

                if (m_elementalReactions.contains(comboKey))
                {
                    const auto& rx = m_elementalReactions.at(comboKey);
                    modifiedDamage *= rx.damageMultiplier;
                    appendLog(std::format("[REACTION: {}] {}", rx.name, rx.description));
                }
                else if (m_elementalReactions.contains(reverseComboKey))
                {
                    const auto& rx = m_elementalReactions.at(reverseComboKey);
                    modifiedDamage *= rx.damageMultiplier;
                    appendLog(std::format("[REACTION: {}] {}", rx.name, rx.description));
                }
            }

            if (targetPart) targetPart->currentElementStatus = attackType;

            // Gameplay Difficulty Multiplier: scale enemy attack power
            if (g && g->settings.gameplay.difficultyMultiplier > 0.0f)
            {
                bool userIsEnemy = false;
                for (const auto& ep : m_enemyParty)
                {
                    if (ep.character.get() == qa.user)
                    {
                        userIsEnemy = true;
                        break;
                    }
                }
                if (userIsEnemy)
                {
                    modifiedDamage *= g->settings.gameplay.difficultyMultiplier;
                }
            }

            // Guard Absorption
            int finalDamage = std::max(1, static_cast<int>(std::round(modifiedDamage)));
            if (targetPart && targetPart->isGuarding)
            {
                if (targetPart->currentStamina >= finalDamage)
                {
                    targetPart->currentStamina -= finalDamage;
                    appendLog(std::format("{} holds their guard and absorbs {} incoming damage with their stamina!", target->name, finalDamage));
                    finalDamage = 0;
                }
                else
                {
                    finalDamage -= static_cast<int>(targetPart->currentStamina);
                    targetPart->currentStamina = 0.0f;
                    targetPart->isGuarding = false;
                    appendLog(std::format("{}'s guard is SHATTERED! (Staggered!)", target->name));
                }
            }

            if (finalDamage > 0)
            {
                target->stats.modifyBaseStat("health", -static_cast<float>(finalDamage));
                std::string effectivenessText = "";
                if (elemMult > 1.1f) effectivenessText = " (Super Effective!)";
                else if (elemMult < 0.9f) effectivenessText = " (Resisted)";

                appendLog(std::format("{} uses {} on {} for {} {} damage!{}",
                    qa.user->name, qa.action.name, target->name,
                    finalDamage, attackType, effectivenessText));
            }

            if (target->getStat("health") <= 0.0f)
            {
                appendLog(std::format("{} has been defeated!", target->name));
            }
        }
        else if (node.effectType == "HEAL")
        {
            target->stats.modifyBaseStat("health", node.baseMagnitude);
            appendLog(std::format("{} heals {} for {} HP!", qa.user->name, target->name, static_cast<int>(node.baseMagnitude)));
        }
        else if (node.effectType == "SHIELD")
        {
            qa.user->stats.modifyBaseStat("health", node.baseMagnitude);
            appendLog(std::format("{} bolsters a shield barrier of {} HP!", qa.user->name, static_cast<int>(node.baseMagnitude)));
        }
    }
}

bool combatEngine::isCombatOver() const
{
    return isPlayerVictory() || std::all_of(m_playerParty.begin(), m_playerParty.end(), [](const CombatParticipant& p) {
        return !p.character || p.character->getStat("health") <= 0.0f;
    });
}

bool combatEngine::isPlayerVictory() const
{
    if (m_enemyParty.empty()) return false;

    bool allEnemiesDown = std::all_of(m_enemyParty.begin(), m_enemyParty.end(), [](const CombatParticipant& p) {
        return !p.character || p.character->getStat("health") <= 0.0f || p.character->getStat("lust") >= 100.0f;
    });

    bool anyPlayerAlive = std::any_of(m_playerParty.begin(), m_playerParty.end(), [](const CombatParticipant& p) {
        return p.character && p.character->getStat("health") > 0.0f;
    });

    return allEnemiesDown && anyPlayerAlive;
}

void combatEngine::appendLog(const std::string& message)
{
    m_combatLog.push_back(message);
}