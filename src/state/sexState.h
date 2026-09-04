#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/enums.h"
#include "state/iGameState.h"

class game;
class entity;

enum class SexStance
{
	SOLO_BED,
	SOLO_CHAIR,
	SOLO_STANDING,
	MISSIONARY,
	FROM_BEHIND,
	KNEELING,
	STANDING,
	LAP_SITTING
};

inline std::string sexStanceToString(SexStance stance)
{
	switch (stance)
	{
		case SexStance::SOLO_BED:      return "In Bed";
		case SexStance::SOLO_CHAIR:    return "Seated";
		case SexStance::SOLO_STANDING: return "Standing (Solo)";
		case SexStance::MISSIONARY:    return "Missionary";
		case SexStance::FROM_BEHIND:   return "From Behind";
		case SexStance::KNEELING:      return "Kneeling";
		case SexStance::STANDING:      return "Standing";
		case SexStance::LAP_SITTING:   return "Lap Sitting";
		default:                       return "In Bed";
	}
}

inline SexStance stringToSexStance(const std::string& str)
{
	if (str == "SOLO_BED" || str == "In Bed") return SexStance::SOLO_BED;
	if (str == "SOLO_CHAIR" || str == "Seated") return SexStance::SOLO_CHAIR;
	if (str == "SOLO_STANDING" || str == "Standing (Solo)") return SexStance::SOLO_STANDING;
	if (str == "From Behind" || str == "FROM_BEHIND") return SexStance::FROM_BEHIND;
	if (str == "Kneeling" || str == "KNEELING")       return SexStance::KNEELING;
	if (str == "Standing" || str == "STANDING")       return SexStance::STANDING;
	if (str == "Lap Sitting" || str == "LAP_SITTING") return SexStance::LAP_SITTING;
	return SexStance::MISSIONARY;
}

enum class SexActionType
{
	KISS,
	CARESS_BODY,
	CARESS_BREASTS,
	ORAL_GIVE,
	ORAL_RECEIVE,
	VAGINAL_PENETRATION,
	ANAL_PENETRATION,
	MAMMARY_SEX,
	SPANK,
	COMMAND_PARTNER,
	PLEAD_SUGGEST,
	ENDURE,
	BEG_CLIMAX,
	STRUGGLE,
	SOLO_ACTION
};

struct SexAction
{
	std::string id;
	std::string name;
	std::string description;
	std::string category = "General";
	SexActionType type = SexActionType::SOLO_ACTION;
	bool isSolo = false;

	float arousalGainSelf = 15.0f;
	float arousalGainPartner = 15.0f;
	float dominanceShift = 5.0f;
	float fluidProducedMl = 1.0f;

	bodySlot actorSlot = bodySlot::MOUTH;
	bodySlot targetSlot = bodySlot::MOUTH;

	std::vector<SexStance> validStances;
	bool requiresPenetration = false;
	bool requiresExposedGenitals = false;
	bool requiresPenis = false;
	bool requiresVagina = false;
	bool requiresBreasts = false;
	float minArousal = 0.0f;
	std::string narrative;
};

/**
 * Headless state controller for CYOA interactive erotic encounters (`sexState`).
 * Manages physical stance proximity, dynamic dominance continuum, arousal, fluid transfer,
 * orifice stretch, narrative text generation, and post-scene clothing restoration.
 */
class sexState : public iGameState
{
public:
	sexState();
	explicit sexState(std::shared_ptr<entity> partner, SexStance initialStance = SexStance::MISSIONARY, float initialDominance = 20.0f);
	explicit sexState(entity* partnerPtr, SexStance initialStance = SexStance::MISSIONARY, float initialDominance = 20.0f);
	~sexState() override = default;

	void initialise(game* gameContext) override;
	void handleCommand(game* gameContext, const UICommand& cmd) override;
	void update(game* gameContext, float deltaTime) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;

	// Action Execution & Core Mechanics
	bool executeAction(game* gameContext, const SexAction& action);
	void changeStance(SexStance newStance);

	// Submissive Player Handlers
	void handlePleadSuggest(game* gameContext);
	void handleEndure(game* gameContext);
	void handleBegClimax(game* gameContext);
	void handleStruggle(game* gameContext);

	// NPC Partner AI Turn
	void processPartnerTurn(game* gameContext);

	// Dynamic Narrative & Orgasm Processing
	void processOrgasm(game* gameContext, entity* orgasmingEntity, entity* receivingEntity, bodySlot receivingSlot);

	// Helper Methods
	std::vector<SexAction> getAvailableActions() const;
	bool isActionValidForStance(const SexAction& action) const;
	void applyClothingDisplacementsForAction(const SexAction& action);

	// Read-only Snapshot Accessors
	bool isSolo() const { return m_partner == nullptr && m_partnerRaw == nullptr; }
	SexStance getStance() const { return m_stance; }
	float getPlayerDominance() const { return m_playerDominance; }
	bool isPlayerDominant() const { return m_playerDominance >= 0.0f; }
	float getPlayerArousal() const { return m_playerArousal; }
	float getPartnerArousal() const { return m_partnerArousal; }
	entity* getPartner() const { return m_partner ? m_partner.get() : m_partnerRaw; }
	const std::string& getNarrativeLog() const { return m_narrativeLog; }

private:
	std::shared_ptr<entity> m_partner = nullptr;
	entity* m_partnerRaw = nullptr;
	SexStance m_stance = SexStance::SOLO_BED;

	float m_playerDominance = 20.0f; // -100 (Max Submissive) to +100 (Max Dominant)
	float m_playerArousal = 0.0f;   // 0 to 100
	float m_partnerArousal = 0.0f;  // 0 to 100

	int m_playerClimaxes = 0;
	int m_partnerClimaxes = 0;

	std::string m_narrativeLog;

	void appendNarrative(const std::string& text);
	std::vector<SexAction> buildMasterActionDatabase() const;
};