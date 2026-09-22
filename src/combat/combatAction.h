#pragma once

#include <functional>
#include <string>
#include <vector>

class game;
class entity;

enum class CombatFocus {
	ROOT,
	WEAPON,
	MAGIC,
	DEFENSE,
	SEDUCTION,
	COMPANION,
	ITEM
};

enum class EffectTargetType {
	SELF,
	SINGLE_TARGET,
	ALL_ENEMIES,
	ALL_ALLIES
};

struct SpellEffectNode {
	std::string effectType; // "DAMAGE", "HEAL", "MUTATION", "AP_MODIFIER", "STATUS_EFFECT", "LUSTRATING"
	std::string element;    // "fire", "water", "air", "earth", "arcane", "lust", "physical"
	float baseMagnitude = 0.0f;
	float scalingFactor = 1.0f;
	std::string statusEffectId;
};

struct CombatAction {
	std::string id;
	std::string name;
	std::string description;

	int baseApCost = 1;
	float staminaCost = 0.0f;
	float manaCost = 0.0f;
	float lustCost = 0.0f;
	float lustDamage = 0.0f;

	float damageMultiplier = 1.0f;
	std::string damageType = "physical"; // "slashing", "piercing", "bludgeoning"
	bool isAoe = false;
	bool isGuarding = false;
	bool isDodging = false;
	bool isParrying = false;

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