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
#include "settings/gameSettings.h"

enum class BodyPresentation
{
	MASCULINE,
	FEMININE,
	ANDROGYNOUS
};

enum class RacialTier
{
	DOMINANT_MORPH,
	DUAL_HYBRID,
	CHAOTIC_CHIMERA
};

struct RacialClassification
{
	RacialTier tier = RacialTier::DOMINANT_MORPH;
	std::string primaryRace = "Human";
	std::string secondaryRace = "";
	std::string title = "Human";
	float primaryPercentage = 100.0f;
	float secondaryPercentage = 0.0f;
};

class anatomyComponent
{
public:
	float heightMeters = 1.75f;
	std::string bodySize = "Average";
	std::string muscleTone = "Toned";
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

	// Orifices and Fluids
	bool hasOrifice(bodySlot slot) const;
	OrificeData* getOrifice(bodySlot slot);
	const OrificeData* getOrifice(bodySlot slot) const;
	void transferFluidToOrifice(bodySlot slot, const std::string& fluidType, float amount);
	void stretchOrifice(bodySlot slot, float diameter);
	void processBiologicalRecovery(int minutesPassed);

	void setTattoo(tattooSlot slot, const tattoo& tat);
	void removeTattoo(tattooSlot slot);
	bool hasTattoo(tattooSlot slot) const;
	tattoo* getTattoo(tattooSlot slot);

	void addMutation(const anatomyMutation& mut);
	void processMutations(int minutesPassed);
	void applyTransformation(bodySlot slot, mutationType type, float amountOrVal,
							 const std::string& strVal, int durationMinutes, const std::string& mutName = "transformation");

	// 3-Tier Racial Classification & Archetype
	std::unordered_map<std::string, float> calculateRacePercentages() const;
	std::unordered_map<std::string, float> calculateWeightedRacePercentages() const;
	RacialClassification getRacialClassification() const;
	std::string getDominantRace() const;
	std::string getRacialTitle() const;
	bool isDualHybrid() const;
	bool isChaoticChimera() const;

	GenderArchetype getGenderArchetype() const;
	BodyPresentation getVisualPresentation() const;
	bool isFeminine() const { return getVisualPresentation() == BodyPresentation::FEMININE; }
	bool hasPenis() const;
	bool hasVagina() const;
	bool hasBreasts() const;

	float getAggregateStatBonus(const std::string& statName) const;
	void printDebug() const;

private:
	std::array<std::optional<bodyPart>, BODY_SLOT_COUNT> parts{};
	std::unordered_map<tattooSlot, tattoo> tattoos;
};