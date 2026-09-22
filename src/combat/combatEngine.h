#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "combat/combatAction.h"

class entity;
class game;

struct CombatParticipant
{
	std::shared_ptr<entity> character = nullptr;
	int maxAp = 3;
	int currentAp = 3;
	float maxStamina = 100.0f;
	float currentStamina = 100.0f;
	bool isEnemy = false;
	bool isGuarding = false;
	bool isDodging = false;
	bool isParrying = false;
	std::string currentElementStatus = ""; // e.g. "soaked", "scorched", "frozen", "mired"

	std::vector<QueuedAction> turnQueue;
	std::vector<CombatAction> preparedActions; // Primary Grid (10 slots max)
};

struct ElementalReactionDef
{
	std::string name;
	float damageMultiplier = 1.0f;
	std::string effect;
	std::string description;
};

class combatEngine
{
public:
	combatEngine();
	~combatEngine() = default;

	void initialiseCombat(const std::vector<std::shared_ptr<entity>>& playerParty,
						  const std::vector<std::shared_ptr<entity>>& enemyParty);

	void startNewRound();
	bool queuePlayerAction(size_t participantIndex, const CombatAction& action, entity* target, bool isFromSecondaryGrid = false);
	void clearPlayerQueue(size_t participantIndex);

	void resolveTurn(game* g);

	int calculateParticipantMaxAp(const entity* ent) const;
	float calculateParticipantMaxStamina(const entity* ent) const;

	std::vector<CombatParticipant>& getPlayerParty() { return m_playerParty; }
	const std::vector<CombatParticipant>& getPlayerParty() const { return m_playerParty; }
	std::vector<CombatParticipant>& getEnemyParty() { return m_enemyParty; }
	const std::vector<CombatParticipant>& getEnemyParty() const { return m_enemyParty; }

	const std::vector<std::string>& getCombatLog() const { return m_combatLog; }
	void appendLog(const std::string& message);

	bool isCombatOver() const;
	bool isPlayerVictory() const;
	bool isLustSurrender() const { return m_lustSurrenderAchieved; }
	int getCurrentRound() const { return m_currentRound; }

	void executeAction(const QueuedAction& qa, game* g);

	float getElementalMultiplier(const std::string& attackElement, const std::string& targetElement) const;
	void loadElements(const std::string& path = "data/elements.json");

private:
	std::vector<CombatParticipant> m_playerParty;
	std::vector<CombatParticipant> m_enemyParty;
	std::vector<std::string> m_combatLog;

	int m_currentRound = 0;
	bool m_lustSurrenderAchieved = false;

	std::unordered_map<std::string, std::unordered_map<std::string, float>> m_elementalMatchups;
	std::unordered_map<std::string, ElementalReactionDef> m_elementalReactions;

	void generateNpcQueues();
};