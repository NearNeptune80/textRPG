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
		modifyBaseStat("perk_points", 1.0f);
		leveledUp = true;
	}
	statsVersion++;
	return leveledUp;
}

void statsComponent::setBaseStat(const std::string& name, float value)
{
	baseValues[name] = value;
	if (name == "lust") baseValues["arousal"] = value;
	else if (name == "arousal") baseValues["lust"] = value;
	statsVersion++;
}

float statsComponent::getBaseStat(const std::string& name) const
{
	auto it = baseValues.find(name);
	if (it != baseValues.end()) return it->second;

	if (name == "arousal")
	{
		auto lustIt = baseValues.find("lust");
		if (lustIt != baseValues.end()) return lustIt->second;
	}
	else if (name == "lust")
	{
		auto arIt = baseValues.find("arousal");
		if (arIt != baseValues.end()) return arIt->second;
	}

	if (name == "max_health")
	{
		auto hpIt = baseValues.find("health");
		if (hpIt != baseValues.end() && hpIt->second > 0.0f) return std::max(hpIt->second, 100.0f);
		return 100.0f;
	}
	if (name == "max_mana")
	{
		auto mpIt = baseValues.find("mana");
		if (mpIt != baseValues.end() && mpIt->second > 0.0f) return std::max(mpIt->second, 100.0f);
		return 100.0f;
	}
	if (name == "max_lust" || name == "max_arousal")
	{
		return 100.0f;
	}

	return 0.0f;
}

void statsComponent::modifyBaseStat(const std::string& name, float amount)
{
	baseValues[name] += amount;
	if (name == "lust") baseValues["arousal"] = baseValues[name];
	else if (name == "arousal") baseValues["lust"] = baseValues[name];
	statsVersion++;
}

float statsComponent::getEffectiveStat(const std::string& name, const std::vector<StatusEffect>& activeEffects) const
{
	float base = getBaseStat(name);
	float flatMod = 0.0f;
	float percentMod = 0.0f;

	for (const auto& fx : activeEffects)
	{
		for (const auto& mod : fx.statModifiers)
		{
			bool matches = (mod.statName == name) ||
			               ((name == "lust" || name == "arousal") && (mod.statName == "lust" || mod.statName == "arousal"));
			if (matches)
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