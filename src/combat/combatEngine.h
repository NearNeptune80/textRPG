#pragma once

#include <memory>
#include <string>
#include <vector>

#include "combat/combatAction.h"

class entity;
class game;

struct CombatParticipant
{
	std::shared_ptr<entity> character = nullptr;
	int maxAp = 3;
	int currentAp = 3;
	bool isEnemy = false;

	std::vector<QueuedAction> turnQueue;
	std::vector<CombatAction> preparedActions; // Primary Grid (10 slots max)
};

class combatEngine
{
public:
	combatEngine() = default;
	~combatEngine() = default;

	void initialiseCombat(const std::vector<std::shared_ptr<entity>>& playerParty,
						  const std::vector<std::shared_ptr<entity>>& enemyParty);

	void startNewRound();
	bool queuePlayerAction(size_t participantIndex, const CombatAction& action, entity* target, bool isFromSecondaryGrid);
	void clearPlayerQueue(size_t participantIndex);

	void resolveTurn(game* g);

	int calculateParticipantMaxAp(const entity* ent) const;

	std::vector<CombatParticipant>& getPlayerParty() { return m_playerParty; }
	std::vector<CombatParticipant>& getEnemyParty() { return m_enemyParty; }
	const std::vector<std::string>& getCombatLog() const { return m_combatLog; }
	void appendLog(const std::string& message); // Public access for combat log writing

	bool isCombatOver() const;
	bool isPlayerVictory() const;

private:
	std::vector<CombatParticipant> m_playerParty;
	std::vector<CombatParticipant> m_enemyParty;
	std::vector<std::string> m_combatLog;

	int m_currentRound = 0;

	void generateNpcQueues();
	void executeAction(const QueuedAction& qa, game* g);
};