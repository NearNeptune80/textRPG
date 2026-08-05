#pragma once

#include <functional>
#include <string>
#include <vector>

class game;
class entity;

enum class EffectTargetType {
	SELF,
	SINGLE_TARGET,
	ALL_ENEMIES,
	ALL_ALLIES
};

struct SpellEffectNode {
	std::string effectType; // "DAMAGE", "HEAL", "MUTATION", "AP_MODIFIER", "STATUS_EFFECT"
	std::string element;    // "Fire", "Arcane", "Lust", "Physical"
	float baseMagnitude = 0.0f;
	float scalingFactor = 1.0f;
	std::string statusEffectId;
};

struct CombatAction {
	std::string id;
	std::string name;
	std::string description;

	int baseApCost = 1;
	float manaCost = 0.0f;
	float lustCost = 0.0f;

	EffectTargetType targetType = EffectTargetType::SINGLE_TARGET;
	bool canTargetSelf = true;
	bool canTargetAllies = true;
	bool canTargetEnemies = true;

	std::vector<SpellEffectNode> effectNodes;
	std::function<void(entity* user, entity* target, game* g)> customExecute = nullptr;
};

struct QueuedAction {
	CombatAction action;
	entity* user = nullptr;
	entity* target = nullptr;
	int actualApCost = 1;
};