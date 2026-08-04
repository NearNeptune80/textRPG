#include "quest/conditionNode.h"

#include "core/game.h"

bool conditionNode::evaluate(const game* gameContext) const
{
	if (!gameContext) return false;

	switch (op)
	{
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
			return children.empty();
		}
		case conditionOperator::NOT: {
			if (children.empty()) return true;
			return !children.front().evaluate(gameContext);
		}
		case conditionOperator::LEAF:
		default: {
			return gameContext->checkSingleCondition(condition);
		}
	}
}