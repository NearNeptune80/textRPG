#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "entities/statusEffect.h"

class statsComponent
{
public:
	int level = 1;
	float currentXp = 0.0f;

	float getRequiredXp() const { return level * 100.0f; }
	bool addXp(float amount);

	void setBaseStat(const std::string& name, float value);
	float getBaseStat(const std::string& name) const;
	void modifyBaseStat(const std::string& name, float amount);
	const std::unordered_map<std::string, float>& getAllBaseStats() const { return baseValues; }

	float getEffectiveStat(const std::string& name, const std::vector<StatusEffect>& activeEffects = {}) const;
	void printDebug() const;

private:
	std::unordered_map<std::string, float> baseValues;
};