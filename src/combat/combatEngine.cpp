#include "combat/combatEngine.h"

#include <algorithm>
#include <format>
#include <cstddef>

#include "core/game.h"
#include "entities/entity.h"

int combatEngine::calculateParticipantMaxAp(const entity* ent) const
{
    if (!ent) return 3;

    // Base AP is 3. Modifiers can be derived from Physique/Agility or Status Effects
    float physique = ent->getStat("physique");
    int bonusAp = static_cast<int>(physique / 10.0f); // +1 AP for every 10 Physique

    int maxAp = std::max(1, 3 + bonusAp);
    return maxAp;
}

void combatEngine::initialiseCombat(const std::vector<std::shared_ptr<entity>>& playerParty,
                                    const std::vector<std::shared_ptr<entity>>& enemyParty)
{
    m_playerParty.clear();
    m_enemyParty.clear();
    m_combatLog.clear();
    m_currentRound = 0;

    for (const auto& ent : playerParty)
    {
        if (!ent) continue;
        CombatParticipant p;
        p.character = ent;
        p.isEnemy = false;
        p.maxAp = calculateParticipantMaxAp(ent.get());
        p.currentAp = p.maxAp;
        m_playerParty.push_back(p);
    }

    for (const auto& ent : enemyParty)
    {
        if (!ent) continue;
        CombatParticipant p;
        p.character = ent;
        p.isEnemy = true;
        p.maxAp = calculateParticipantMaxAp(ent.get());
        p.currentAp = p.maxAp;
        m_enemyParty.push_back(p);
    }

    appendLog("=== COMBAT STARTED ===");
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

    if (p.currentAp < apCost) return false; // Not enough AP

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
    // Basic AI loop for enemies: pick basic attacks on player until AP is depleted
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

    // Aggregate all queued actions into a flat timeline
    std::vector<QueuedAction> allActions;

    for (const auto& p : m_playerParty)
    {
        allActions.insert(allActions.end(), p.turnQueue.begin(), p.turnQueue.end());
    }
    for (const auto& p : m_enemyParty)
    {
        allActions.insert(allActions.end(), p.turnQueue.begin(), p.turnQueue.end());
    }

    // Process queued actions
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