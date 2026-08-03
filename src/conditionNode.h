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

	// Method Declaration
	bool evaluate(const game* gameContext) const;
};

// Method Implementation (Inline to avoid multiple definition errors across translation units)
inline bool conditionNode::evaluate(const game* gameContext) const {
	if (!gameContext) return false;

	switch (op) {
		case conditionOperator::AND: {
			for (const auto& child : children) {
				if (!child.evaluate(gameContext)) return false;
			}
			return true;
		}
		case conditionOperator::OR: {
			for (const auto& child : children) {
				if (child.evaluate(gameContext)) return true;
			}
			return children.empty(); // If OR has no children, default logic
		}
		case conditionOperator::NOT: {
			if (children.empty()) return true;
			return !children.front().evaluate(gameContext);
		}
		case conditionOperator::LEAF:
		default: {
			// Evaluates base condition against state/player flags in your gameContext
			// Adjust checks to fit your actual game evaluation methods:
			// e.g. return gameContext->checkLeafCondition(condition);
			return true;
		}
	}
}