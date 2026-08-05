#pragma once

#include <memory>
#include <vector>

class entity;

enum class CombatOutcome {
	VICTORY,
	DEFEAT,
	ESCAPE,
	SURRENDER
};

struct CombatEndedEvent {
	CombatOutcome outcome;
	std::vector<std::shared_ptr<entity>> playerParty;
	std::vector<std::shared_ptr<entity>> enemyParty;
	int currencyLost = 0;
	int xpGained = 0;
};