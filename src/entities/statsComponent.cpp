#include "entities/statsComponent.h"

#include <algorithm>
#include <iostream>

/**
 * Awards XP to character and processes level advancement.
 * Returns true if character leveled up.
 */
bool statsComponent::addXp(float amount)
{
	currentXp += amount;
	bool leveledUp = false;
	while (currentXp >= getRequiredXp())
	{
		currentXp -= getRequiredXp();
		level++;
		leveledUp = true;
	}
	return leveledUp;
}

void statsComponent::setBaseStat(const std::string& name, float value)
{
	baseValues[name] = value;
}

/**
 * Retrieves the base value of a named stat (single hash lookup).
 */
float statsComponent::getBaseStat(const std::string& name) const
{
	auto it = baseValues.find(name);
	if (it != baseValues.end()) return it->second;
	return 0.0f;
}

void statsComponent::modifyBaseStat(const std::string& name, float amount)
{
	baseValues[name] += amount;
}

/**
 * Calculates effective stat combining base value, flat modifiers, and percentage buffs.
 * Formula: (base + flatMod) * (1.0 + percentMod)
 */
float statsComponent::getEffectiveStat(const std::string& name, const std::vector<StatusEffect>& activeEffects) const
{
	float base = getBaseStat(name);
	float flatMod = 0.0f;
	float percentMod = 0.0f;

	for (const auto& fx : activeEffects)
	{
		for (const auto& mod : fx.statModifiers)
		{
			if (mod.statName == name)
			{
				flatMod += mod.flatValue;
				percentMod += mod.percentValue;
			}
		}
	}

	return std::max(0.0f, (base + flatMod) * (1.0f + percentMod));
}

void statsComponent::printDebug() const
{
	std::cout << "\n=== STATS DEBUG ===\nLevel: " << level << " | XP: " << currentXp << "\n===================\n\n";
}