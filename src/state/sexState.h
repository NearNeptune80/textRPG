#pragma once

#include <memory>
#include <string>
#include <unordered_map>
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
	LAP_SITTING,
	COWGIRL,
	LOTUS,
	PRONE_BONE
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
		case SexStance::COWGIRL:       return "Cowgirl";
		case SexStance::LOTUS:         return "Lotus Embrace";
		case SexStance::PRONE_BONE:    return "Prone Bone";
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
	if (str == "Cowgirl" || str == "COWGIRL")         return SexStance::COWGIRL;
	if (str == "Lotus" || str == "LOTUS")             return SexStance::LOTUS;
	if (str == "Prone Bone" || str == "PRONE_BONE")   return SexStance::PRONE_BONE;
	return SexStance::MISSIONARY;
}

enum class SexRole
{
	DOMINANT,
	SUBMISSIVE,
	SWITCH
};

inline std::string sexRoleToString(SexRole role)
{
	switch (role)
	{
		case SexRole::DOMINANT:   return "Dominant";
		case SexRole::SUBMISSIVE: return "Submissive";
		case SexRole::SWITCH:     return "Switch";
		default:                  return "Switch";
	}
}

enum class SexPhase
{
	FOREPLAY,
	ESCALATION,
	PENETRATION,
	CLIMAX_EDGE,
	AFTERCARE
};

inline std::string sexPhaseToString(SexPhase phase)
{
	switch (phase)
	{
		case SexPhase::FOREPLAY:    return "Foreplay";
		case SexPhase::ESCALATION:  return "Escalation";
		case SexPhase::PENETRATION: return "Penetration";
		case SexPhase::CLIMAX_EDGE: return "Climax / Edge";
		case SexPhase::AFTERCARE:   return "Aftercare";
		default:                    return "Foreplay";
	}
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

struct SexParticipant
{
	std::shared_ptr<entity> character = nullptr;
	entity* rawCharacter = nullptr;
	int side = 0; // 0 = Player/Allies, 1 = Opposing/Foes
	bool isPlayer = false;
	float dominanceScore = 0.0f; // -100 to +100
	float arousal = 0.0f;        // 0 to 100
	float stamina = 100.0f;      // 0 to 100
	int climaxes = 0;
	size_t targetIndex = 0;
	std::string attitude = "neutral";
	bool beggedClimax = false;
	bool resistedLastTurn = false;
	bool hasOpeningToStruggle = false;
	std::unordered_map<std::string, int> perceivedFetishes;

	entity* getEntity() const { return character ? character.get() : rawCharacter; }
	std::string getName() const;
	SexRole getRole() const
	{
		if (dominanceScore > 25.0f) return SexRole::DOMINANT;
		if (dominanceScore < -25.0f) return SexRole::SUBMISSIVE;
		return SexRole::SWITCH;
	}
	bool isDominant() const { return dominanceScore >= 0.0f; }
};

enum class BodyPartUsage
{
	NONE,
	ONE_HAND,
	BOTH_HANDS,
	MOUTH,
	GROIN,
	TORSO,
	FEET
};

struct OngoingSexAction
{
	std::string actionId;
	std::string actionName;
	size_t actorIndex = 0;
	size_t targetIndex = 0;
	bodySlot actorSlot = bodySlot::GROIN;
	bodySlot targetSlot = bodySlot::GROIN;
	BodyPartUsage bodyPartUsage = BodyPartUsage::NONE;
	int handAssigned = 0; // 0 = not a hand, 1 = Hand 1, 2 = Hand 2, 3 = Both Hands
	float arousalGainActor = 10.0f;
	float arousalGainTarget = 10.0f;
	float dominanceShift = 0.0f;
	float fluidProducedMl = 1.0f;
	std::string continuousNarrative;
	int turnsActive = 0;
};

struct SexLogEntry
{
	std::string actorName;
	std::string actionName;
	std::string narrative;
	bool isPlayer = false;
	bool isPartner = false;
	bool isSystem = false;
	int turn = 0;
};

struct SexAction
{
	std::string id;
	std::string name;
	std::string description;
	std::string category = "Generic";
	SexActionType type = SexActionType::SOLO_ACTION;
	bool isSolo = false;
	bool isContinuous = false; // Persistent action across turns until stopped or slot superseded

	float minDominance = -100.0f; // Minimum player dominance score (-100 to 100)
	float maxDominance = 100.0f;  // Maximum player dominance score (-100 to 100)

	float arousalGainSelf = 15.0f;
	float arousalGainPartner = 15.0f;
	float dominanceShift = 5.0f;
	float fluidProducedMl = 1.0f;

	bodySlot actorSlot = bodySlot::MOUTH;
	bodySlot targetSlot = bodySlot::MOUTH;
	int handsRequired = 0; // 0 = none/unspecified, 1 = one hand, 2 = both hands

	std::vector<SexStance> validStances;
	bool requiresPenetration = false;
	bool requiresExposedGenitals = false;
	bool requiresPenis = false;
	bool requiresVagina = false;
	bool requiresBreasts = false;

	bool actorRequiresPenis = false;
	bool actorRequiresVagina = false;
	bool actorRequiresBreasts = false;
	bool targetRequiresPenis = false;
	bool targetRequiresVagina = false;
	bool targetRequiresBreasts = false;

	float minArousal = 0.0f;
	std::string narrative;
	std::string continuousNarrative; // Narrative prose emitted on subsequent turns while continuous
};

class sexState : public iGameState
{
public:
	sexState();
	explicit sexState(std::shared_ptr<entity> partner, SexStance initialStance = SexStance::MISSIONARY, float initialDominance = 20.0f);
	explicit sexState(entity* partnerPtr, SexStance initialStance = SexStance::MISSIONARY, float initialDominance = 20.0f);
	explicit sexState(const std::vector<std::shared_ptr<entity>>& partners, SexStance initialStance = SexStance::MISSIONARY, float initialDominance = 20.0f);
	explicit sexState(const std::vector<std::shared_ptr<entity>>& allies, const std::vector<std::shared_ptr<entity>>& foes, SexStance initialStance = SexStance::MISSIONARY, float initialDominance = 20.0f);
	~sexState() override = default;

	void initialise(game* gameContext) override;
	void handleCommand(game* gameContext, const UICommand& cmd) override;
	void update(game* gameContext, float deltaTime) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;

	bool executeAction(game* gameContext, const SexAction& action);
	void changeStance(SexStance newStance);
	std::vector<SexAction> buildMasterActionDatabase() const;

	void handlePleadSuggest(game* gameContext);
	void handleEndure(game* gameContext);
	void handleBegClimax(game* gameContext);
	void handleStruggle(game* gameContext);

	void processPartnerTurn(game* gameContext);
	void processAllNpcTurns(game* gameContext);
	void processSingleNpcTurn(game* gameContext, size_t npcIdx);

	void processOrgasm(game* gameContext, entity* orgasmingEntity, entity* receivingEntity, bodySlot receivingSlot);
	void checkAllOrgasms(game* gameContext);

	std::vector<SexAction> getAvailableActions() const;
	bool isActionValidForStance(const SexAction& action) const;
	bool isActionValidForParticipants(const SexAction& action, const SexParticipant& actor, const SexParticipant& target) const;
	bool isActionAccessible(const SexAction& action, const SexParticipant& actor, const SexParticipant& target, std::string* outInaccessibleReason = nullptr) const;
	BodyPartUsage getActionBodyPartUsage(const SexAction& action) const;
	void undressEntity(entity* ent);
	void applyClothingDisplacementsForAction(const SexAction& action, entity* actor, entity* target);
	entity* getTargetEntity() const;
	void handleClothingAction(game* gameContext, int side, equipSlot slot, DisplacementMode mode, bool isRemove);
	void handleUndressAll(game* gameContext, bool targetIsPlayer);

	bool isSolo() const { return m_participants.size() <= 1; }
	SexStance getStance() const { return m_stance; }
	float getPlayerDominance() const;
	bool isPlayerDominant() const;
	float getPlayerArousal() const;
	float getPartnerArousal() const;
	entity* getPartner() const;
	const std::string& getNarrativeLog() const { return m_narrativeLog; }
	const std::vector<SexLogEntry>& getNarrativeEntries() const { return m_narrativeEntries; }
	bool checkAndResetNewNarrative() { bool val = m_hasNewNarrative; m_hasNewNarrative = false; return val; }

	size_t getParticipantCount() const { return m_participants.size(); }
	const std::vector<SexParticipant>& getParticipants() const { return m_participants; }
	std::vector<SexParticipant>& getParticipants() { return m_participants; }
	SexParticipant* getPlayerParticipant();
	const SexParticipant* getPlayerParticipant() const;
	SexParticipant* getTargetParticipant();
	const SexParticipant* getTargetParticipant() const;
	size_t getTargetParticipantIndex() const { return m_selectedTargetIndex; }
	void selectTargetParticipant(size_t index);

	SexPhase getScenePhase() const;
	bool canParticipantStruggle(size_t participantIdx, std::string* outReason = nullptr) const;
	std::string computeHolisticAttitude(const SexParticipant& p) const;
	void updateAllAttitudes();

	void setConsensual(bool consensual) { m_isConsensual = consensual; }
	bool isConsensual() const { return m_isConsensual; }

	const std::vector<OngoingSexAction>& getOngoingActions() const { return m_ongoingActions; }
	bool hasOngoingAction(size_t participantIdx) const;
	const OngoingSexAction* getOngoingActionForParticipant(size_t participantIdx) const;
	std::vector<const OngoingSexAction*> getOngoingActionsForParticipant(size_t participantIdx) const;
	void stopOngoingAction(size_t participantIdx, game* gameContext = nullptr);
	void stopActionById(const std::string& actionId, game* gameContext = nullptr);
	void clearAllOngoingActions();
	void processOngoingActions(game* gameContext);

	void appendNarrative(const std::string& text);
	void appendNarrativeEntry(const std::string& rawText, const std::string& actorName, const std::string& actionName, bool isPlayer, bool isSystem);

private:
	std::vector<SexParticipant> m_participants;
	std::vector<OngoingSexAction> m_ongoingActions;
	std::vector<SexLogEntry> m_narrativeEntries;
	size_t m_selectedTargetIndex = 0;
	SexStance m_stance = SexStance::SOLO_BED;
	bool m_isConsensual = true;
	bool m_hasNewNarrative = false;
	int m_turnCount = 1;

	std::string m_narrativeLog;
	void syncCategories(game* gameContext);
	std::string interpolateNarrative(const std::string& templateStr, const SexParticipant& actor, const SexParticipant& target, const SexAction& action) const;
};