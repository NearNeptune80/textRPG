#pragma once

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/enums.h"
#include "entities/bodyPart.h"
#include "entities/mutation.h"
#include "entities/tattoo.h"

enum class BodyPresentation
{
	MASCULINE,
	FEMININE,
	ANDROGYNOUS
};

class anatomyComponent
{
public:
	float heightMeters = 1.75f;
	std::vector<anatomyMutation> activeMutations;

	void setPart(bodySlot slot, const bodyPart& part);
	void removePart(bodySlot slot);
	bool hasPart(bodySlot slot) const;
	bodyPart* getPart(bodySlot slot);
	const bodyPart* getPart(bodySlot slot) const;

	bool hasTag(bodySlot slot, const std::string& tag) const;
	bool hasGlobalTag(const std::string& tag) const;
	std::vector<std::string> getAllTags() const;
	const std::array<std::optional<bodyPart>, BODY_SLOT_COUNT>& getAllParts() const { return parts; }

	void setTattoo(tattooSlot slot, const tattoo& tat);
	void removeTattoo(tattooSlot slot);
	bool hasTattoo(tattooSlot slot) const;
	tattoo* getTattoo(tattooSlot slot);

	void addMutation(const anatomyMutation& mut);
	void processMutations(int minutesPassed);
	void applyTransformation(bodySlot slot, mutationType type, float amountOrVal,
							 const std::string& strVal, int durationMinutes, const std::string& mutName = "transformation");

	std::unordered_map<std::string, float> calculateRacePercentages() const;
	std::string getDominantRace() const;
	BodyPresentation getVisualPresentation() const;
	bool isFeminine() const { return getVisualPresentation() == BodyPresentation::FEMININE; }
	float getAggregateStatBonus(const std::string& statName) const;

	void printDebug() const;

private:
	std::array<std::optional<bodyPart>, BODY_SLOT_COUNT> parts{};
	std::unordered_map<tattooSlot, tattoo> tattoos;
};