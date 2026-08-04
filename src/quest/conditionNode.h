#pragma once

#include <string>
#include <vector>

class game;

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

	bool evaluate(const game* gameContext) const;
};