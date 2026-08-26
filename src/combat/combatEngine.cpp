#include "combat/combatEngine.h"

#include <algorithm>
#include <format>
#include <cstddef>

#include "core/game.h"
#include "entities/entity.h"

int combatEngine::calculateParticipantMaxAp(const entity* ent) const
{
    if (!ent) return 3;

    float physique = ent->getStat("physique");
    float agility = ent->getStat("agility");
    int bonusAp = static_cast<int>((physique + agility) / 20.0f);

    return std::clamp(3 + bonusAp, 1, 6);
}

void combatEngine::initialiseCombat(const std::vector<std::shared_ptr<entity>>& playerParty,
                                    const std::vector<std::shared_ptr<entity>>& enemyParty)
{
    m_playerParty.clear();
    m_enemyParty.clear();
    m_combatLog.clear();
    m_currentRound = 0;

    size_t maxPartySize = 4; // Multi-party combat setup (up to 4v4)

    for (size_t i = 0; i < playerParty.size() && i < maxPartySize; ++i)
    {
        const auto& ent = playerParty[i];
        if (!ent) continue;
        CombatParticipant p;
        p.character = ent;
        p.isEnemy = false;
        p.maxAp = calculateParticipantMaxAp(ent.get());
        p.currentAp = p.maxAp;
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
    }

    for (auto& p : m_enemyParty)
    {
        p.maxAp = calculateParticipantMaxAp(p.character.get());
        p.currentAp = p.maxAp;
        p.turnQueue.clear();
    }
}

bool combatEngine::queuePlayerAction(size_t participantIndex, const CombatAction& action, entity* target, bool isFromSecondaryGrid)
{
    if (participantIndex >= m_playerParty.size()) return false;

    auto& p = m_playerParty[participantIndex];
    int apCost = action.baseApCost + (isFromSecondaryGrid ? 1 : 0);

    if (p.currentAp < apCost) return false;

    QueuedAction qa;
    qa.action = action;
    qa.user = p.character.get();
    qa.target = target;
    qa.actualApCost = apCost;

    p.turnQueue.push_back(qa);
    p.currentAp -= apCost;
    return true;
}

void combatEngine::clearPlayerQueue(size_t participantIndex)
{
    if (participantIndex >= m_playerParty.size()) return;
    auto& p = m_playerParty[participantIndex];
    p.turnQueue.clear();
    p.currentAp = p.maxAp;
}

void combatEngine::generateNpcQueues()
{
    for (auto& enemyP : m_enemyParty)
    {
        if (!enemyP.character || enemyP.character->getStat("health") <= 0) continue;
        if (m_playerParty.empty()) break;

        entity* playerTarget = m_playerParty[0].character.get();

        while (enemyP.currentAp >= 1)
        {
            CombatAction strike;
            strike.id = "action_basic_strike";
            strike.name = "Strike";
            strike.baseApCost = 1;

            SpellEffectNode dmgNode;
            dmgNode.effectType = "DAMAGE";
            dmgNode.element = "Physical";
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

    // Sort queued actions by user Agility descending
    std::sort(allActions.begin(), allActions.end(), [](const QueuedAction& a, const QueuedAction& b) {
        float agiA = a.user ? a.user->getStat("agility") : 0.0f;
        float agiB = b.user ? b.user->getStat("agility") : 0.0f;
        return agiA > agiB;
    });

    for (const auto& qa : allActions)
    {
        if (qa.user && qa.user->getStat("health") > 0)
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
    if (!qa.user || !qa.target) return;

    if (qa.action.customExecute)
    {
        qa.action.customExecute(qa.user, qa.target, g);
        return;
    }

    for (const auto& node : qa.action.effectNodes)
    {
        if (node.effectType == "DAMAGE")
        {
            float rawDamage = node.baseMagnitude;
            qa.target->stats.modifyBaseStat("health", -rawDamage);

            appendLog(std::format("{} uses {} on {} for {} {} damage!",
                qa.user->name, qa.action.name, qa.target->name,
                static_cast<int>(rawDamage), node.element));
        }
        else if (node.effectType == "HEAL")
        {
            qa.target->stats.modifyBaseStat("health", node.baseMagnitude);
            appendLog(std::format("{} heals {} for {} HP!", qa.user->name, qa.target->name, static_cast<int>(node.baseMagnitude)));
        }
    }
}

bool combatEngine::isCombatOver() const
{
    return isPlayerVictory() || std::all_of(m_playerParty.begin(), m_playerParty.end(), [](const CombatParticipant& p) {
        return p.character->getStat("health") <= 0;
    });
}

bool combatEngine::isPlayerVictory() const
{
    return std::all_of(m_enemyParty.begin(), m_enemyParty.end(), [](const CombatParticipant& p) {
        return p.character->getStat("health") <= 0;
    });
}

void combatEngine::appendLog(const std::string& message)
{
    m_combatLog.push_back(message);
}