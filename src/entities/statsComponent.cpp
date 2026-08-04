#include "entities/statsComponent.h"

#include <algorithm>
#include <iostream>

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

void statsComponent::setBaseStat(const std::string& name, float value) { baseValues[name] = value; }

float statsComponent::getBaseStat(const std::string& name) const
{
	if (baseValues.find(name) != baseValues.end()) return baseValues.at(name);
	return 0.0f;
}

void statsComponent::modifyBaseStat(const std::string& name, float amount) { baseValues[name] += amount; }

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