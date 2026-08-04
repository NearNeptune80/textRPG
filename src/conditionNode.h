#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class game; // Forward declaration

enum class conditionOperator { LEAF, AND, OR, NOT };

struct gameCondition {
	std::string type;
	std::string target;
	int requiredValue{0};
};

struct conditionNode {
	conditionOperator op{conditionOperator::LEAF};
	gameCondition condition{};
	std::vector<conditionNode> children{};

	conditionNode() = default;
	conditionNode(const gameCondition& cond)
		: op(conditionOperator::LEAF), condition(cond) {}

	// Evaluated in game.cpp where full 'game' definition is available
	bool evaluate(const game* gameContext) const;
};