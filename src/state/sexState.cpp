#include "state/sexState.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "common/randomEngine.h"
#include "common/unitFormatter.h"
#include "core/contentFilterManager.h"
#include "core/game.h"
#include "entities/entity.h"
#include "state/explorationState.h"

namespace
{
	inline bodySlot stringToBodySlot(const std::string& str)
	{
		if (str == "MOUTH") return bodySlot::MOUTH;
		if (str == "BREASTS") return bodySlot::BREASTS;
		if (str == "NIPPLES") return bodySlot::NIPPLES;
		if (str == "HANDS") return bodySlot::HANDS;
		if (str == "FINGERS") return bodySlot::FINGERS;
		if (str == "GROIN" || str == "PENIS" || str == "VAGINA") return bodySlot::GROIN;
		if (str == "ASS" || str == "ANUS") return bodySlot::ASS;
		if (str == "TORSO") return bodySlot::TORSO;
		if (str == "ARMS") return bodySlot::ARMS;
		if (str == "FEET") return bodySlot::FEET;
		if (str == "LEGS") return bodySlot::LEGS;
		return bodySlot::GROIN;
	}

	struct ConjugationRules
	{
		std::vector<std::pair<std::string, std::string>> verbFixes;
		std::vector<std::pair<std::string, std::string>> pronounFixes;
	};

	static const ConjugationRules& getConjugationRules()
	{
		static ConjugationRules rules = []() {
			ConjugationRules r;
			std::ifstream file("data/config/conjugation_rules.json");
			if (file.is_open())
			{
				try
				{
					nlohmann::json j;
					file >> j;
					if (j.contains("verbFixes"))
					{
						for (const auto& item : j["verbFixes"])
						{
							r.verbFixes.emplace_back(item.value("from", ""), item.value("to", ""));
						}
					}
					if (j.contains("pronounFixes"))
					{
						for (const auto& item : j["pronounFixes"])
						{
							r.pronounFixes.emplace_back(item.value("from", ""), item.value("to", ""));
						}
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "[Grammar] Failed to parse conjugation_rules.json: " << e.what() << "\n";
				}
			}
			return r;
		}();
		return rules;
	}

	static std::string sanitizeNarrativeGrammar(const std::string& input)
	{
		std::string text = input;

		auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
			size_t pos = 0;
			while ((pos = str.find(from, pos)) != std::string::npos) {
				str.replace(pos, from.length(), to);
				pos += to.length();
			}
		};

		replaceAll(text, "You's", "your");
		replaceAll(text, "you's", "your");

		const auto& rules = getConjugationRules();
		for (const auto& [from, to] : rules.verbFixes)
		{
			replaceAll(text, from, to);
		}

		if (text.rfind("You ", 0) == 0)
		{
			for (const auto& [from, to] : rules.pronounFixes)
			{
				replaceAll(text, from, to);
			}
		}

		return text;
	}

	struct IntimateNarrativeConfig
	{
		std::string introSolo = "You find a private, quiet space to indulge in intimate solitary pleasure.";
		std::string introPairDominant = "Encounter begins in {} position. You assume control over {}, dictating the pace of the encounter.";
		std::string introPairSubmissive = "Encounter begins in {} position. {} takes control of the situation, holding you in a submissive position.";
		std::string introGroup = "An intense group intimate encounter begins in {} position with {} participants.";

		std::string strugglePlead = "You plead softly with {}, asking for gentle consideration.";
		std::string struggleEndure = "You close your eyes and passively endure {}'s touch, letting the sensations wash over you.";
		std::string struggleBegClimax = "You whimper helplessly, begging your partner to let you climax!";
		std::string struggleSuccess = "With a sudden burst of strength, you forcefully shove {} back, breaking free from their hold and seizing control!";
		std::string struggleOverpowered = "You thrash against {}'s grip, but they effortlessly pin your wrists and overpower your resistance!";

		std::string climaxPlayer = "A violent shudder wracks your body as an overwhelming orgasm surges through your core!";
		std::string climaxOther = "A violent shudder wracks {}'s body as an overwhelming orgasm surges through their core!";
		std::string ejaculateOrificePlayer = "You ejaculate {} of warm cum directly into {}'s orifice!";
		std::string ejaculateOrificeTargetPlayer = "{} ejaculates {} of warm cum directly into your orifice!";
		std::string ejaculateOrificeOther = "{} ejaculates {} of warm cum directly into {}'s orifice!";
		std::string ejaculateSurfacePlayer = "You ejaculate {} of warm cum across {}'s body surface!";
		std::string ejaculateSurfaceTargetPlayer = "{} ejaculates {} of warm cum across your body surface!";
		std::string ejaculateSurfaceOther = "{} ejaculates {} of warm cum across {}'s body surface!";
		std::string conceptionPlayer = "Your womb receives the potent seed, conceiving a new offspring!";
		std::string conceptionOther = "{}'s womb receives the potent seed, conceiving a new offspring!";
		std::string lactation = "{}'s engorged breasts leak {} of sweet milk in response to their intense orgasm!";
	};

	static IntimateNarrativeConfig loadIntimateNarrativeConfig()
	{
		IntimateNarrativeConfig cfg;
		std::ifstream file("data/scenes/intimate/narratives.json");
		if (file.is_open())
		{
			try
			{
				nlohmann::json j;
				file >> j;
				if (j.contains("intros"))
				{
					const auto& in = j["intros"];
					cfg.introSolo = in.value("solo", cfg.introSolo);
					cfg.introPairDominant = in.value("pair_dominant", cfg.introPairDominant);
					cfg.introPairSubmissive = in.value("pair_submissive", cfg.introPairSubmissive);
					cfg.introGroup = in.value("group", cfg.introGroup);
				}
				if (j.contains("struggle_actions"))
				{
					const auto& st = j["struggle_actions"];
					cfg.strugglePlead = st.value("plead", cfg.strugglePlead);
					cfg.struggleEndure = st.value("endure", cfg.struggleEndure);
					cfg.struggleBegClimax = st.value("beg_climax", cfg.struggleBegClimax);
					cfg.struggleSuccess = st.value("struggle_success", cfg.struggleSuccess);
					cfg.struggleOverpowered = st.value("struggle_overpowered", cfg.struggleOverpowered);
				}
				if (j.contains("climax"))
				{
					const auto& cl = j["climax"];
					cfg.climaxPlayer = cl.value("climax_player", cfg.climaxPlayer);
					cfg.climaxOther = cl.value("climax_other", cfg.climaxOther);
					cfg.ejaculateOrificePlayer = cl.value("ejaculate_orifice_player", cfg.ejaculateOrificePlayer);
					cfg.ejaculateOrificeTargetPlayer = cl.value("ejaculate_orifice_target_player", cfg.ejaculateOrificeTargetPlayer);
					cfg.ejaculateOrificeOther = cl.value("ejaculate_orifice_other", cfg.ejaculateOrificeOther);
					cfg.ejaculateSurfacePlayer = cl.value("ejaculate_surface_player", cfg.ejaculateSurfacePlayer);
					cfg.ejaculateSurfaceTargetPlayer = cl.value("ejaculate_surface_target_player", cfg.ejaculateSurfaceTargetPlayer);
					cfg.ejaculateSurfaceOther = cl.value("ejaculate_surface_other", cfg.ejaculateSurfaceOther);
					cfg.conceptionPlayer = cl.value("conception_player", cfg.conceptionPlayer);
					cfg.conceptionOther = cl.value("conception_other", cfg.conceptionOther);
					cfg.lactation = cl.value("lactation", cfg.lactation);
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error loading data/scenes/intimate/narratives.json: " << e.what() << "\n";
			}
		}
		return cfg;
	}

	static const IntimateNarrativeConfig& getIntimateNarratives()
	{
		static const auto cfg = loadIntimateNarrativeConfig();
		return cfg;
	}

	static std::vector<SexAction> parseSexActionsFromFile(const std::string& path)
	{
		std::vector<SexAction> db;
		std::ifstream file(path);
		if (!file.is_open()) return db;

		try
		{
			nlohmann::json j;
			file >> j;
			if (j.contains("actions") && j["actions"].is_array())
			{
				for (const auto& aJson : j["actions"])
				{
					SexAction act;
					act.id = aJson.value("id", "");
					act.name = aJson.value("name", "");
					act.description = aJson.value("description", "");
					act.category = aJson.value("category", "Generic");
					act.isSolo = aJson.value("isSolo", false);
					act.isContinuous = aJson.value("isContinuous", false);
					act.continuousNarrative = aJson.value("continuousNarrative", "");
					act.minDominance = aJson.value("minDominance", -100.0f);
					act.maxDominance = aJson.value("maxDominance", 100.0f);
					act.narrative = aJson.value("narrative", "");

					if (aJson.contains("actorSlot")) act.actorSlot = stringToBodySlot(aJson["actorSlot"].get<std::string>());
					if (aJson.contains("targetSlot")) act.targetSlot = stringToBodySlot(aJson["targetSlot"].get<std::string>());
					act.handsRequired = aJson.value("handsRequired", 0);

					if (aJson.contains("values"))
					{
						const auto& v = aJson["values"];
						act.arousalGainSelf = v.value("arousalSelf", 15.0f);
						act.arousalGainPartner = v.value("arousalPartner", 15.0f);
						act.dominanceShift = v.value("dominanceShift", 0.0f);
						act.fluidProducedMl = v.value("fluidProducedMl", 1.0f);
					}

					if (aJson.contains("requirements"))
					{
						const auto& r = aJson["requirements"];
						if (r.contains("minDominance")) act.minDominance = r.value("minDominance", act.minDominance);
						if (r.contains("maxDominance")) act.maxDominance = r.value("maxDominance", act.maxDominance);
						act.requiresPenis = r.value("hasPenis", false);
						act.requiresVagina = r.value("hasVagina", false);
						act.requiresBreasts = r.value("hasBreasts", false);
						act.actorRequiresPenis = r.value("actorHasPenis", false);
						act.actorRequiresVagina = r.value("actorHasVagina", false);
						act.actorRequiresBreasts = r.value("actorHasBreasts", false);
						act.targetRequiresPenis = r.value("targetHasPenis", false);
						act.targetRequiresVagina = r.value("targetHasVagina", false);
						act.targetRequiresBreasts = r.value("targetHasBreasts", false);
						act.minArousal = r.value("minArousal", 0.0f);
					}

					if (aJson.contains("validStances") && aJson["validStances"].is_array())
					{
						for (const auto& st : aJson["validStances"])
						{
							act.validStances.push_back(stringToSexStance(st.get<std::string>()));
						}
					}

					db.push_back(act);
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error loading actions from " << path << ": " << e.what() << "\n";
		}
		return db;
	}

	static std::string formatTemplate(std::string text, const std::vector<std::string>& args)
	{
		for (const auto& arg : args)
		{
			size_t pos = text.find("{}");
			if (pos != std::string::npos)
			{
				text.replace(pos, 2, arg);
			}
		}
		return text;
	}
}

std::string SexParticipant::getName() const
{
	if (isPlayer) return "You";
	auto* e = getEntity();
	return e ? e->name : "Partner";
}

sexState::sexState()
{
	SexParticipant player;
	player.isPlayer = true;
	player.side = 0;
	player.dominanceScore = 20.0f;
	player.stamina = 100.0f;
	m_participants.push_back(player);

	m_stance = SexStance::SOLO_BED;
	m_isConsensual = true;
}

sexState::sexState(std::shared_ptr<entity> partner, SexStance initialStance, float initialDominance)
{
	SexParticipant player;
	player.isPlayer = true;
	player.side = 0;
	player.dominanceScore = initialDominance;
	player.stamina = (initialDominance < -30.0f) ? 15.0f : 100.0f;
	m_participants.push_back(player);

	if (partner)
	{
		SexParticipant part;
		part.character = partner;
		part.side = 1;
		part.dominanceScore = -initialDominance;
		part.stamina = 100.0f;
		m_participants.push_back(part);
	}

	m_stance = initialStance;
	m_isConsensual = (std::abs(initialDominance) < 40.0f);
	m_selectedTargetIndex = (m_participants.size() > 1) ? 1 : 0;
}

sexState::sexState(entity* partnerPtr, SexStance initialStance, float initialDominance)
{
	SexParticipant player;
	player.isPlayer = true;
	player.side = 0;
	player.dominanceScore = initialDominance;
	player.stamina = (initialDominance < -30.0f) ? 15.0f : 100.0f;
	m_participants.push_back(player);

	if (partnerPtr)
	{
		SexParticipant part;
		part.rawCharacter = partnerPtr;
		part.side = 1;
		part.dominanceScore = -initialDominance;
		part.stamina = 100.0f;
		m_participants.push_back(part);
	}

	m_stance = initialStance;
	m_isConsensual = (std::abs(initialDominance) < 40.0f);
	m_selectedTargetIndex = (m_participants.size() > 1) ? 1 : 0;
}

sexState::sexState(const std::vector<std::shared_ptr<entity>>& partners, SexStance initialStance, float initialDominance)
{
	SexParticipant player;
	player.isPlayer = true;
	player.side = 0;
	player.dominanceScore = initialDominance;
	player.stamina = (initialDominance < -30.0f) ? 15.0f : 100.0f;
	m_participants.push_back(player);

	for (const auto& p : partners)
	{
		if (!p) continue;
		SexParticipant part;
		part.character = p;
		part.side = 1;
		part.dominanceScore = -initialDominance;
		part.stamina = 100.0f;
		m_participants.push_back(part);
	}

	m_stance = initialStance;
	m_isConsensual = (std::abs(initialDominance) < 40.0f);
	m_selectedTargetIndex = (m_participants.size() > 1) ? 1 : 0;
}

sexState::sexState(const std::vector<std::shared_ptr<entity>>& allies, const std::vector<std::shared_ptr<entity>>& foes, SexStance initialStance, float initialDominance)
{
	SexParticipant player;
	player.isPlayer = true;
	player.side = 0;
	player.dominanceScore = initialDominance;
	player.stamina = (initialDominance < -30.0f) ? 15.0f : 100.0f;
	m_participants.push_back(player);

	for (const auto& a : allies)
	{
		if (!a) continue;
		SexParticipant part;
		part.character = a;
		part.side = 0;
		part.dominanceScore = initialDominance;
		part.stamina = 100.0f;
		m_participants.push_back(part);
	}

	for (const auto& f : foes)
	{
		if (!f) continue;
		SexParticipant part;
		part.character = f;
		part.side = 1;
		part.dominanceScore = -initialDominance;
		part.stamina = 100.0f;
		m_participants.push_back(part);
	}

	m_stance = initialStance;
	m_isConsensual = (std::abs(initialDominance) < 40.0f);
	m_selectedTargetIndex = (m_participants.size() > 1) ? 1 : 0;
}

void sexState::initialise(game* gameContext)
{
	if (!gameContext) return;

	if (auto* playerP = getPlayerParticipant())
	{
		playerP->rawCharacter = gameContext->getPlayer();
	}

	if (auto* targetP = getTargetParticipant())
	{
		if (targetP->character) gameContext->activeTargetNPC = targetP->character;
	}

	updateAllAttitudes();
	syncCategories(gameContext);

	const auto& narrCfg = getIntimateNarratives();
	if (isSolo())
	{
		appendNarrativeEntry(narrCfg.introSolo, "Scene", "Encounter Start", false, true);
	}
	else if (m_participants.size() == 2)
	{
		SexParticipant* targetP = getTargetParticipant();
		std::string pName = targetP ? targetP->getName() : "your partner";
		if (isPlayerDominant())
		{
			appendNarrativeEntry(formatTemplate(narrCfg.introPairDominant, { sexStanceToString(m_stance), pName }), "Scene", "Encounter Start", false, true);
		}
		else
		{
			appendNarrativeEntry(formatTemplate(narrCfg.introPairSubmissive, { sexStanceToString(m_stance), pName }), "Scene", "Encounter Start", false, true);
		}
	}
	else
	{
		appendNarrativeEntry(formatTemplate(narrCfg.introGroup, { sexStanceToString(m_stance), std::to_string(m_participants.size()) }), "Scene", "Encounter Start", false, true);
	}
}

void sexState::onEnter(game* gameContext)
{
	if (gameContext)
	{
		gameContext->selectedEquipmentSlot = equipSlot::NONE;
	}
	syncCategories(gameContext);
}

void sexState::onExit(game* gameContext)
{
	if (gameContext)
	{
		gameContext->clearActionCategories();
		gameContext->selectedEquipmentSlot = equipSlot::NONE;
	}
}

void sexState::update(game* gameContext, float /*deltaTime*/)
{
	// Event-driven turn-based state: no per-frame tick required
	(void)gameContext;
}

void sexState::appendNarrativeEntry(const std::string& rawText, const std::string& actorName, const std::string& actionName, bool isPlayer, bool isSystem)
{
	std::string cleanText = sanitizeNarrativeGrammar(rawText);

	if (!m_narrativeLog.empty())
	{
		m_narrativeLog += "\n\n";
	}
	m_narrativeLog += cleanText;

	SexLogEntry entry;
	entry.actorName = actorName.empty() ? (isPlayer ? "You" : (isSystem ? "Scene" : "Partner")) : actorName;
	entry.actionName = actionName;
	entry.narrative = cleanText;
	entry.isPlayer = isPlayer;
	entry.isPartner = (!isPlayer && !isSystem);
	entry.isSystem = isSystem;
	entry.turn = (isSystem && m_narrativeEntries.empty()) ? 0 : m_turnCount;
	m_narrativeEntries.push_back(entry);
	m_hasNewNarrative = true;
}

void sexState::appendNarrative(const std::string& text)
{
	bool isPlayer = (text.starts_with("You ") || text.starts_with("Your "));
	bool isSystem = (text.starts_with("An intense group") || text.starts_with("The encounter") ||
	                 text.find("climax") != std::string::npos || text.find("orgasm") != std::string::npos ||
	                 text.starts_with("Shifting position") || text.starts_with("Ongoing action") ||
	                 text.find("fluid") != std::string::npos || text.find("ejaculates") != std::string::npos);
	std::string actor = isPlayer ? "You" : (isSystem ? "Scene" : "Partner");
	std::string action = "";
	if (text.find("position") != std::string::npos) action = "Stance";
	else if (text.find("climax") != std::string::npos || text.find("orgasm") != std::string::npos) action = "Climax";

	appendNarrativeEntry(text, actor, action, isPlayer, isSystem);
}

float sexState::getPlayerDominance() const
{
	if (const auto* p = getPlayerParticipant()) return p->dominanceScore;
	return 20.0f;
}

bool sexState::isPlayerDominant() const
{
	return getPlayerDominance() >= 0.0f;
}

float sexState::getPlayerArousal() const
{
	if (const auto* p = getPlayerParticipant()) return p->arousal;
	return 0.0f;
}

float sexState::getPartnerArousal() const
{
	if (const auto* p = getTargetParticipant()) return p->arousal;
	return 0.0f;
}

entity* sexState::getPartner() const
{
	if (const auto* p = getTargetParticipant()) return p->getEntity();
	return nullptr;
}

SexParticipant* sexState::getPlayerParticipant()
{
	for (auto& p : m_participants)
	{
		if (p.isPlayer) return &p;
	}
	return nullptr;
}

const SexParticipant* sexState::getPlayerParticipant() const
{
	for (const auto& p : m_participants)
	{
		if (p.isPlayer) return &p;
	}
	return nullptr;
}

SexParticipant* sexState::getTargetParticipant()
{
	if (m_participants.empty()) return nullptr;
	if (m_selectedTargetIndex >= m_participants.size() || m_participants[m_selectedTargetIndex].isPlayer)
	{
		for (size_t i = 0; i < m_participants.size(); ++i)
		{
			if (!m_participants[i].isPlayer)
			{
				m_selectedTargetIndex = i;
				return &m_participants[i];
			}
		}
		m_selectedTargetIndex = 0;
	}
	return &m_participants[m_selectedTargetIndex];
}

const SexParticipant* sexState::getTargetParticipant() const
{
	if (m_participants.empty()) return nullptr;
	if (m_selectedTargetIndex >= m_participants.size() || m_participants[m_selectedTargetIndex].isPlayer)
	{
		for (size_t i = 0; i < m_participants.size(); ++i)
		{
			if (!m_participants[i].isPlayer) return &m_participants[i];
		}
		return &m_participants[0];
	}
	return &m_participants[m_selectedTargetIndex];
}

void sexState::selectTargetParticipant(size_t index)
{
	if (index < m_participants.size() && !m_participants[index].isPlayer)
	{
		m_selectedTargetIndex = index;
	}
}

SexPhase sexState::getScenePhase() const
{
	float highestArousal = 0.0f;
	int totalClimaxes = 0;
	for (const auto& p : m_participants)
	{
		highestArousal = std::max(highestArousal, p.arousal);
		totalClimaxes += p.climaxes;
	}

	if (totalClimaxes >= static_cast<int>(m_participants.size())) return SexPhase::AFTERCARE;
	if (highestArousal >= 85.0f) return SexPhase::CLIMAX_EDGE;
	if (highestArousal >= 60.0f) return SexPhase::PENETRATION;
	if (highestArousal >= 30.0f) return SexPhase::ESCALATION;
	return SexPhase::FOREPLAY;
}

std::vector<SexAction> sexState::buildMasterActionDatabase() const
{
	auto db = parseSexActionsFromFile("data/scenes/sex_actions.json");
	if (!db.empty()) return db;

	// Safe fallback behavior loading from data/scenes/intimate/fallback_actions.json
	db = parseSexActionsFromFile("data/scenes/intimate/fallback_actions.json");
	if (!db.empty()) return db;

	SexAction kiss;
	kiss.id = "partner_kiss";
	kiss.name = "Passionate Kiss";
	kiss.category = "Partner";
	kiss.isContinuous = true;
	kiss.actorSlot = bodySlot::MOUTH;
	kiss.targetSlot = bodySlot::MOUTH;
	kiss.arousalGainSelf = 12.0f;
	kiss.arousalGainPartner = 14.0f;
	kiss.validStances = { SexStance::MISSIONARY, SexStance::KNEELING, SexStance::STANDING, SexStance::LAP_SITTING };
	db.push_back(kiss);

	return db;
}

bool sexState::isActionValidForStance(const SexAction& action) const
{
	if (action.validStances.empty()) return true;
	return std::find(action.validStances.begin(), action.validStances.end(), m_stance) != action.validStances.end();
}

bool sexState::isActionValidForParticipants(const SexAction& action, const SexParticipant& actor, const SexParticipant& target) const
{
	if (!isActionValidForStance(action)) return false;
	if (actor.arousal < action.minArousal) return false;
	if (actor.dominanceScore < action.minDominance || actor.dominanceScore > action.maxDominance) return false;

	entity* actorEnt = actor.getEntity();
	entity* targetEnt = target.getEntity();

	if (action.requiresPenis && (!actorEnt || !actorEnt->anatomy.hasPenis())) return false;
	if (action.requiresVagina && (!targetEnt || !targetEnt->anatomy.hasVagina())) return false;
	if (action.requiresBreasts && (!targetEnt || !targetEnt->anatomy.hasBreasts())) return false;

	if (action.actorRequiresPenis && (!actorEnt || !actorEnt->anatomy.hasPenis())) return false;
	if (action.actorRequiresVagina && (!actorEnt || !actorEnt->anatomy.hasVagina())) return false;
	if (action.actorRequiresBreasts && (!actorEnt || !actorEnt->anatomy.hasBreasts())) return false;

	if (action.targetRequiresPenis && (!targetEnt || !targetEnt->anatomy.hasPenis())) return false;
	if (action.targetRequiresVagina && (!targetEnt || !targetEnt->anatomy.hasVagina())) return false;
	if (action.targetRequiresBreasts && (!targetEnt || !targetEnt->anatomy.hasBreasts())) return false;

	return true;
}

std::vector<SexAction> sexState::getAvailableActions() const
{
	auto allActions = buildMasterActionDatabase();
	std::vector<SexAction> valid;

	const SexParticipant* playerP = getPlayerParticipant();
	if (!playerP) return valid;

	if (isSolo())
	{
		for (const auto& act : allActions)
		{
			if (!act.isSolo && act.category != "Self" && act.category != "Generic") continue;
			if (!isActionValidForParticipants(act, *playerP, *playerP)) continue;
			valid.push_back(act);
		}
		return valid;
	}

	const SexParticipant* targetP = getTargetParticipant();
	if (!targetP) return valid;

	for (const auto& act : allActions)
	{
		if (act.category == "Self")
		{
			if (isActionValidForParticipants(act, *playerP, *playerP))
			{
				valid.push_back(act);
			}
		}
		else if (act.category == "Generic")
		{
			if (act.isSolo) continue;
			if (act.id == "sub_struggle")
			{
				if (canParticipantStruggle(0) && isActionValidForParticipants(act, *playerP, *targetP))
				{
					valid.push_back(act);
				}
			}
			else if (isActionValidForParticipants(act, *playerP, *targetP))
			{
				valid.push_back(act);
			}
		}
		else
		{
			if (act.isSolo) continue;
			if (isActionValidForParticipants(act, *playerP, *targetP))
			{
				valid.push_back(act);
			}
		}
	}

	return valid;
}

BodyPartUsage sexState::getActionBodyPartUsage(const SexAction& action) const
{
	if (action.actorSlot == bodySlot::MOUTH) return BodyPartUsage::MOUTH;
	if (action.actorSlot == bodySlot::GROIN) return BodyPartUsage::GROIN;
	if (action.actorSlot == bodySlot::TORSO) return BodyPartUsage::TORSO;
	if (action.actorSlot == bodySlot::FEET) return BodyPartUsage::FEET;
	if (action.actorSlot == bodySlot::HANDS || action.actorSlot == bodySlot::FINGERS)
	{
		if (action.handsRequired == 2 || action.id == "partner_choke_hold" || action.id == "generic_restrict_movement")
		{
			return BodyPartUsage::BOTH_HANDS;
		}
		return BodyPartUsage::ONE_HAND;
	}
	return BodyPartUsage::NONE;
}

void sexState::undressEntity(entity* ent)
{
	if (!ent) return;
	for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
	{
		equipSlot slot = static_cast<equipSlot>(i);
		if (ent->inventory.isEquipped(slot))
		{
			switch (slot)
			{
				case equipSlot::TORSO_OVER:
					ent->inventory.setDisplacement(slot, DisplacementMode::OPEN);
					break;
				case equipSlot::TORSO_UNDER:
					ent->inventory.setDisplacement(slot, DisplacementMode::UNBUTTON);
					break;
				case equipSlot::CHEST_WEAR:
				case equipSlot::NIPPLES_WEAR:
					ent->inventory.setDisplacement(slot, DisplacementMode::PULL_ASIDE);
					break;
				case equipSlot::LEGS_OUTER:
					ent->inventory.setDisplacement(slot, DisplacementMode::PULL_DOWN);
					break;
				case equipSlot::GROIN_OVER:
				case equipSlot::PENIS_WEAR:
				case equipSlot::VAGINA_WEAR:
					ent->inventory.setDisplacement(slot, DisplacementMode::PULL_ASIDE);
					break;
				case equipSlot::ASS_WEAR:
				case equipSlot::HIPS_WEAR:
					ent->inventory.setDisplacement(slot, DisplacementMode::PULL_DOWN);
					break;
				case equipSlot::STOMACH_WEAR:
					ent->inventory.setDisplacement(slot, DisplacementMode::LIFT_UP);
					break;
				case equipSlot::MOUTHWEAR:
					ent->inventory.setDisplacement(slot, DisplacementMode::OPEN);
					break;
				default:
					break;
			}
		}
	}
}

entity* sexState::getTargetEntity() const
{
	const SexParticipant* p = getTargetParticipant();
	return p ? p->getEntity() : nullptr;
}

void sexState::handleClothingAction(game* gameContext, int side, equipSlot slot, DisplacementMode mode, bool isRemove)
{
	if (!gameContext) return;
	SexParticipant* playerP = getPlayerParticipant();
	SexParticipant* targetP = isSolo() ? playerP : getTargetParticipant();
	if (!playerP || !targetP) return;

	entity* targetEnt = (side == 0) ? playerP->getEntity() : targetP->getEntity();
	if (!targetEnt) return;

	auto eqItem = targetEnt->inventory.getEquippedItem(slot);
	if (!eqItem) return;

	std::string itemName = eqItem->name;
	std::string actionTitle;
	std::string narrative;

	if (isRemove)
	{
		actionTitle = "Remove " + itemName;
		if (side == 0)
		{
			narrative = std::format("You strip off your {}, casting it aside.", itemName);
		}
		else
		{
			narrative = std::format("You peel {}'s {} from their body and cast it aside.", targetP->getName(), itemName);
		}
		targetEnt->inventory.unequipItem(slot);
		gameContext->selectedEquipmentSlot = equipSlot::NONE;
	}
	else if (mode == DisplacementMode::NONE)
	{
		actionTitle = "Restore " + itemName;
		if (side == 0)
		{
			narrative = std::format("You adjust your {}, smoothing it back into its proper fit.", itemName);
		}
		else
		{
			narrative = std::format("You smooth down {}'s {}, restoring its proper fit.", targetP->getName(), itemName);
		}
		targetEnt->inventory.setDisplacement(slot, DisplacementMode::NONE);
	}
	else
	{
		targetEnt->inventory.setDisplacement(slot, mode);
		std::string modeVerb = displacementModeToString(mode);
		actionTitle = modeVerb + " " + itemName;

		if (side == 0)
		{
			if (mode == DisplacementMode::PULL_DOWN) narrative = std::format("You pull down your {}, baring your hips and groin.", itemName);
			else if (mode == DisplacementMode::PULL_ASIDE) narrative = std::format("You slide your {} aside, granting open intimate access.", itemName);
			else if (mode == DisplacementMode::LIFT_UP) narrative = std::format("You pull your {} up, baring your chest and torso.", itemName);
			else if (mode == DisplacementMode::UNBUTTON) narrative = std::format("You unbutton your {}, loosening the fabric.", itemName);
			else if (mode == DisplacementMode::OPEN) narrative = std::format("You part your {} open, exposing your skin beneath.", itemName);
			else narrative = std::format("You adjust your {}, displacing it.", itemName);
		}
		else
		{
			if (mode == DisplacementMode::PULL_DOWN) narrative = std::format("You pull down {}'s {}, baring their hips and lower body.", targetP->getName(), itemName);
			else if (mode == DisplacementMode::PULL_ASIDE) narrative = std::format("You push {}'s {} aside, granting direct intimate access.", targetP->getName(), itemName);
			else if (mode == DisplacementMode::LIFT_UP) narrative = std::format("You lift {}'s {} up, baring their chest and torso.", targetP->getName(), itemName);
			else if (mode == DisplacementMode::UNBUTTON) narrative = std::format("You unfasten the buttons on {}'s {}, opening it up.", targetP->getName(), itemName);
			else if (mode == DisplacementMode::OPEN) narrative = std::format("You open {}'s {}, spreading the garment wide.", targetP->getName(), itemName);
			else narrative = std::format("You displace {}'s {}.", targetP->getName(), itemName);
		}
	}

	if (side == 1)
	{
		playerP->dominanceScore = std::clamp(playerP->dominanceScore + 2.0f, -100.0f, 100.0f);
		targetP->dominanceScore = std::clamp(targetP->dominanceScore - 2.0f, -100.0f, 100.0f);
		targetP->arousal = std::clamp(targetP->arousal + 5.0f, 0.0f, 100.0f);
		playerP->arousal = std::clamp(playerP->arousal + 4.0f, 0.0f, 100.0f);
	}
	else
	{
		playerP->arousal = std::clamp(playerP->arousal + 3.0f, 0.0f, 100.0f);
		targetP->arousal = std::clamp(targetP->arousal + 4.0f, 0.0f, 100.0f);
	}

	appendNarrativeEntry(narrative, "You", actionTitle, true, false);

	for (auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == 0)
		{
			oa.turnsActive++;
			playerP->arousal = std::clamp(playerP->arousal + oa.arousalGainActor * 0.35f, 0.0f, 100.0f);
			if (!isSolo() && targetP)
			{
				targetP->arousal = std::clamp(targetP->arousal + oa.arousalGainTarget * 0.35f, 0.0f, 100.0f);
			}
		}
	}

	if (isSolo())
	{
		m_turnCount++;
		updateAllAttitudes();
		syncCategories(gameContext);
		gameContext->refreshActionGrid();
		return;
	}

	processAllNpcTurns(gameContext);
	checkAllOrgasms(gameContext);

	m_turnCount++;
	updateAllAttitudes();
	syncCategories(gameContext);
	gameContext->refreshActionGrid();
}

void sexState::handleUndressAll(game* gameContext, bool targetIsPlayer)
{
	if (!gameContext) return;
	SexParticipant* playerP = getPlayerParticipant();
	SexParticipant* targetP = isSolo() ? playerP : getTargetParticipant();
	if (!playerP || !targetP) return;

	entity* targetEnt = targetIsPlayer ? playerP->getEntity() : targetP->getEntity();
	if (!targetEnt) return;

	std::string actionTitle = targetIsPlayer ? "Undress All (Self)" : std::format("Undress All ({})", targetP->getName());
	std::string narrative;

	if (targetIsPlayer)
	{
		undressEntity(targetEnt);
		narrative = "You strip away all of your remaining clothing, casting it aside and exposing yourself completely.";
		playerP->arousal = std::clamp(playerP->arousal + 5.0f, 0.0f, 100.0f);
		targetP->arousal = std::clamp(targetP->arousal + 8.0f, 0.0f, 100.0f);
	}
	else
	{
		undressEntity(targetEnt);
		narrative = std::format("You strip away all of {}'s remaining clothing, leaving them completely bare and exposed to your touch.", targetP->getName());
		playerP->dominanceScore = std::clamp(playerP->dominanceScore + 6.0f, -100.0f, 100.0f);
		targetP->dominanceScore = std::clamp(targetP->dominanceScore - 6.0f, -100.0f, 100.0f);
		playerP->arousal = std::clamp(playerP->arousal + 8.0f, 0.0f, 100.0f);
		targetP->arousal = std::clamp(targetP->arousal + 10.0f, 0.0f, 100.0f);
	}

	gameContext->selectedEquipmentSlot = equipSlot::NONE;
	appendNarrativeEntry(narrative, "You", actionTitle, true, false);

	for (auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == 0)
		{
			oa.turnsActive++;
			playerP->arousal = std::clamp(playerP->arousal + oa.arousalGainActor * 0.35f, 0.0f, 100.0f);
			if (!isSolo() && targetP)
			{
				targetP->arousal = std::clamp(targetP->arousal + oa.arousalGainTarget * 0.35f, 0.0f, 100.0f);
			}
		}
	}

	if (isSolo())
	{
		m_turnCount++;
		updateAllAttitudes();
		syncCategories(gameContext);
		gameContext->refreshActionGrid();
		return;
	}

	processAllNpcTurns(gameContext);
	checkAllOrgasms(gameContext);

	m_turnCount++;
	updateAllAttitudes();
	syncCategories(gameContext);
	gameContext->refreshActionGrid();
}

bool sexState::isActionAccessible(const SexAction& action, const SexParticipant& actor, const SexParticipant& target, std::string* outInaccessibleReason) const
{
	entity* actorEnt = actor.getEntity();
	entity* targetEnt = target.getEntity();

	if (action.id == "partner_strip" || action.id == "generic_cmd_strip")
	{
		if (targetEnt && targetEnt->inventory.isFullyExposed())
		{
			if (outInaccessibleReason) *outInaccessibleReason = std::format("{} is already unclothed.", target.getName());
			return false;
		}
		return true;
	}
	if (action.id == "self_strip" || action.id == "sub_strip_offer")
	{
		if (actorEnt && actorEnt->inventory.isFullyExposed())
		{
			if (outInaccessibleReason) *outInaccessibleReason = "You are already unclothed.";
			return false;
		}
		return true;
	}
	if (action.id == "generic_undress_mutual")
	{
		bool actorBare = !actorEnt || actorEnt->inventory.isFullyExposed();
		bool targetBare = !targetEnt || targetEnt->inventory.isFullyExposed();
		if (actorBare && targetBare)
		{
			if (outInaccessibleReason) *outInaccessibleReason = "Both of you are already unclothed.";
			return false;
		}
		return true;
	}

	bool needsTargetGroin = (action.targetSlot == bodySlot::GROIN || action.requiresExposedGenitals ||
	                         action.id.find("penetrate") != std::string::npos ||
	                         (action.id.find("oral") != std::string::npos && action.id != "partner_oral_receive") ||
	                         action.id.find("cunnilingus") != std::string::npos ||
	                         action.id.find("fellatio") != std::string::npos ||
	                         action.id.find("finger_vagina") != std::string::npos ||
	                         action.id.find("ride") != std::string::npos ||
	                         action.id.find("deepthroat") != std::string::npos ||
	                         action.id.find("clit") != std::string::npos ||
	                         action.id == "generic_cmd_oral" || action.id == "generic_req_oral" || action.id == "generic_req_penetrate");
	if (needsTargetGroin && targetEnt)
	{
		if (!targetEnt->inventory.isSlotExposed(bodySlot::GROIN))
		{
			if (outInaccessibleReason)
			{
				*outInaccessibleReason = target.isPlayer ? "Your groin is covered by clothing." : std::format("{}'s groin is covered by clothing.", target.getName());
			}
			return false;
		}
	}

	bool needsTargetAss = (action.targetSlot == bodySlot::ASS || action.id.find("anal") != std::string::npos ||
	                       action.id.find("rimjob") != std::string::npos || action.id.find("finger_anus") != std::string::npos);
	if (needsTargetAss && targetEnt)
	{
		if (!targetEnt->inventory.isSlotExposed(bodySlot::ASS))
		{
			if (outInaccessibleReason)
			{
				*outInaccessibleReason = target.isPlayer ? "Your rear is covered by clothing." : std::format("{}'s rear is covered by clothing.", target.getName());
			}
			return false;
		}
	}

	bool needsTargetBreasts = (action.targetSlot == bodySlot::BREASTS || action.targetSlot == bodySlot::NIPPLES ||
	                           action.id.find("breast") != std::string::npos || action.id.find("nipple") != std::string::npos);
	if (needsTargetBreasts && targetEnt)
	{
		if (!targetEnt->inventory.isSlotExposed(bodySlot::BREASTS))
		{
			if (outInaccessibleReason)
			{
				*outInaccessibleReason = target.isPlayer ? "Your chest is covered by clothing." : std::format("{}'s chest is covered by clothing.", target.getName());
			}
			return false;
		}
	}

	bool needsActorGroin = (action.actorSlot == bodySlot::GROIN || action.id.find("penetrate") != std::string::npos ||
	                        action.id.find("ride") != std::string::npos || action.id.find("oral_receive") != std::string::npos ||
	                        action.id.find("deepthroat") != std::string::npos ||
	                        (action.category == "Self" && (action.targetSlot == bodySlot::GROIN || action.targetSlot == bodySlot::ASS)));
	if (needsActorGroin && actorEnt)
	{
		if (!actorEnt->inventory.isSlotExposed(bodySlot::GROIN))
		{
			if (outInaccessibleReason)
			{
				*outInaccessibleReason = actor.isPlayer ? "Your groin is covered by clothing." : std::format("{}'s groin is covered by clothing.", actor.getName());
			}
			return false;
		}
	}

	bool needsActorBreasts = (action.category == "Self" && (action.targetSlot == bodySlot::BREASTS || action.targetSlot == bodySlot::NIPPLES));
	if (needsActorBreasts && actorEnt)
	{
		if (!actorEnt->inventory.isSlotExposed(bodySlot::BREASTS))
		{
			if (outInaccessibleReason)
			{
				*outInaccessibleReason = actor.isPlayer ? "Your chest is covered by clothing." : std::format("{}'s chest is covered by clothing.", actor.getName());
			}
			return false;
		}
	}

	if (action.actorSlot == bodySlot::MOUTH && actorEnt && !actorEnt->inventory.isSlotExposed(bodySlot::MOUTH))
	{
		if (outInaccessibleReason) *outInaccessibleReason = actor.isPlayer ? "Your mouth is obstructed." : std::format("{}'s mouth is obstructed.", actor.getName());
		return false;
	}
	if (action.targetSlot == bodySlot::MOUTH && targetEnt && !targetEnt->inventory.isSlotExposed(bodySlot::MOUTH))
	{
		if (outInaccessibleReason) *outInaccessibleReason = target.isPlayer ? "Your mouth is obstructed." : std::format("{}'s mouth is obstructed.", target.getName());
		return false;
	}

	return true;
}

void sexState::applyClothingDisplacementsForAction(const SexAction& action, entity* actor, entity* target)
{
	(void)actor;
	if (!target) return;

	if (action.actorSlot == bodySlot::GROIN || action.targetSlot == bodySlot::GROIN ||
	    action.targetSlot == bodySlot::ASS || action.id.find("penetrate") != std::string::npos ||
	    action.id.find("oral") != std::string::npos || action.id.find("fellatio") != std::string::npos ||
	    action.id.find("cunnilingus") != std::string::npos || action.id.find("ride") != std::string::npos)
	{
		target->inventory.setDisplacement(equipSlot::LEGS_OUTER, DisplacementMode::PULL_DOWN);
		target->inventory.setDisplacement(equipSlot::GROIN_OVER, DisplacementMode::PULL_ASIDE);
	}

	if (action.targetSlot == bodySlot::BREASTS || action.targetSlot == bodySlot::NIPPLES ||
	    action.id.find("breast") != std::string::npos || action.id.find("nipple") != std::string::npos)
	{
		target->inventory.setDisplacement(equipSlot::TORSO_UNDER, DisplacementMode::UNBUTTON);
		target->inventory.setDisplacement(equipSlot::CHEST_WEAR, DisplacementMode::PULL_ASIDE);
	}
}

void sexState::syncCategories(game* gameContext)
{
	if (!gameContext) return;
	gameContext->setActionCategories({ "Generic", "Self", "Partner", "Position" }, "Generic");
}

std::string sexState::interpolateNarrative(const std::string& templateStr, const SexParticipant& actor, const SexParticipant& target, const SexAction& action) const
{
	(void)action;
	std::string result = templateStr;

	auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
		size_t pos = 0;
		while ((pos = str.find(from, pos)) != std::string::npos) {
			str.replace(pos, from.length(), to);
			pos += to.length();
		}
	};

	if (target.isPlayer)
	{
		replaceAll(result, "{target}'s", "your");
		replaceAll(result, "{Target}'s", "Your");
		replaceAll(result, "{target}", "you");
		replaceAll(result, "{Target}", "You");
	}
	else
	{
		replaceAll(result, "{target}", target.getName());
	}

	if (actor.isPlayer)
	{
		replaceAll(result, "{actor}'s", "your");
		replaceAll(result, "{Actor}'s", "Your");
		replaceAll(result, "{actor}", "You");
		replaceAll(result, "{Actor}", "You");
	}
	else
	{
		replaceAll(result, "{actor}", actor.getName());
	}

	replaceAll(result, "{stance}", sexStanceToString(m_stance));
	replaceAll(result, "{actor_attitude}", actor.attitude);
	replaceAll(result, "{target_attitude}", target.attitude);

	return sanitizeNarrativeGrammar(result);
}

std::string sexState::computeHolisticAttitude(const SexParticipant& p) const
{
	entity* e = p.getEntity();
	SexRole role = p.getRole();

	if (p.isPlayer)
	{
		if (role == SexRole::DOMINANT) return "In Control";
		if (role == SexRole::SWITCH) return "Adaptive / Switch";
		return (p.stamina < 30.0f) ? "Exhausted & Yielding" : "Submissive";
	}

	if (!e) return "Attentive";

	if (p.climaxes >= 2) return "Spent & Blissful";

	if (e->hasTrait("Cruel"))
	{
		if (role == SexRole::DOMINANT) return "Cruel & Mocking";
		return "Resentful & Snarling";
	}
	if (e->hasTrait("Kind"))
	{
		if (role == SexRole::DOMINANT) return "Tender & Protective";
		return "Affectionate & Warm";
	}
	if (e->hasTrait("Innocent") || e->hasTrait("Shy"))
	{
		if (p.arousal < 40.0f) return "Flustered & Hesitant";
		return "Breathless & Melting";
	}
	if (e->hasTrait("Lewd"))
	{
		if (p.arousal >= 60.0f) return "Shameless & Craving";
		return "Eager & Seductive";
	}
	if (e->hasTrait("Brave"))
	{
		if (role == SexRole::SUBMISSIVE && p.stamina > 40.0f) return "Defiant";
		return "Composed";
	}
	if (e->hasTrait("Cowardly"))
	{
		if (role == SexRole::SUBMISSIVE) return "Fearful & Compliant";
	}

	if (role == SexRole::DOMINANT) return (p.arousal >= 70.0f) ? "Passionate & Commanding" : "Assertive";
	if (role == SexRole::SWITCH) return "Playful & Responsive";
	return (p.arousal >= 70.0f) ? "Lust-Dazed & Yielding" : "Submissive";
}

void sexState::updateAllAttitudes()
{
	for (auto& p : m_participants)
	{
		p.attitude = computeHolisticAttitude(p);
	}
}

bool sexState::canParticipantStruggle(size_t participantIdx, std::string* outReason) const
{
	if (participantIdx >= m_participants.size()) return false;
	const auto& p = m_participants[participantIdx];

	if (!m_isConsensual)
	{
		if (p.stamina < 20.0f && !p.hasOpeningToStruggle)
		{
			if (outReason) *outReason = "Exhausted from combat defeat; you must catch your breath or wait for an opening.";
			return false;
		}
		return true;
	}

	entity* e = p.getEntity();
	if (e)
	{
		if (e->hasFetish("BDSM") || e->hasFetish("Dominance") || e->hasFetish("Submission") ||
			e->hasTrait("Brave") || p.resistedLastTurn)
		{
			return true;
		}
	}

	if (outReason) *outReason = "In a consensual encounter, struggling requires a rough/roleplay fetish or boundary violation.";
	return false;
}

void sexState::changeStance(SexStance newStance)
{
	m_stance = newStance;
	SexParticipant* playerP = getPlayerParticipant();
	SexParticipant* targetP = getTargetParticipant();

	if (isSolo())
	{
		appendNarrativeEntry(std::format("You adjust your position to {}.", sexStanceToString(m_stance)), "You", "Position: " + sexStanceToString(m_stance), true, false);
	}
	else
	{
		std::string tName = targetP ? targetP->getName() : "your partner";
		std::string aName = playerP ? playerP->getName() : "You";
		if (playerP && playerP->dominanceScore < -25.0f)
		{
			appendNarrativeEntry(std::format("You present yourself submissively, offering the {} position to {}.", sexStanceToString(m_stance), tName), "You", "Offer Position", true, false);
		}
		else
		{
			appendNarrativeEntry(std::format("You maneuver {} into the {} position.", tName, sexStanceToString(m_stance)), "You", "Shift Position", true, false);
		}
	}

	auto allActions = buildMasterActionDatabase();
	auto it = m_ongoingActions.begin();
	while (it != m_ongoingActions.end())
	{
		auto actIt = std::find_if(allActions.begin(), allActions.end(), [&](const SexAction& a) {
			return a.id == it->actionId;
		});
		if (actIt != allActions.end() && !actIt->validStances.empty())
		{
			bool valid = (std::find(actIt->validStances.begin(), actIt->validStances.end(), newStance) != actIt->validStances.end());
			if (!valid)
			{
				appendNarrativeEntry(std::format("Shifting position ends {}.", it->actionName), "Scene", "End Ongoing", false, true);
				it = m_ongoingActions.erase(it);
				continue;
			}
		}
		++it;
	}
}

bool sexState::hasOngoingAction(size_t participantIdx) const
{
	for (const auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == participantIdx)
		{
			return true;
		}
	}
	return false;
}

std::vector<const OngoingSexAction*> sexState::getOngoingActionsForParticipant(size_t participantIdx) const
{
	std::vector<const OngoingSexAction*> result;
	for (const auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == participantIdx)
		{
			result.push_back(&oa);
		}
	}
	return result;
}

const OngoingSexAction* sexState::getOngoingActionForParticipant(size_t participantIdx) const
{
	for (const auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == participantIdx)
		{
			return &oa;
		}
	}
	return nullptr;
}

void sexState::stopOngoingAction(size_t participantIdx, game* gameContext)
{
	std::string stoppedName;
	bool stoppedAny = false;

	auto it = m_ongoingActions.begin();
	while (it != m_ongoingActions.end())
	{
		if (it->actorIndex == participantIdx)
		{
			if (stoppedName.empty()) stoppedName = it->actionName;
			it = m_ongoingActions.erase(it);
			stoppedAny = true;
		}
		else
		{
			++it;
		}
	}

	if (stoppedAny)
	{
		bool isPl = (participantIdx < m_participants.size()) ? m_participants[participantIdx].isPlayer : (participantIdx == 0);
		std::string pName = (participantIdx < m_participants.size()) ? m_participants[participantIdx].getName() : "You";

		if (isPl)
		{
			appendNarrativeEntry(std::format("You cease and disengage from {}.", stoppedName), "You", "Disengage", true, false);
			if (gameContext)
			{
				processAllNpcTurns(gameContext);
				checkAllOrgasms(gameContext);
				m_turnCount++;
			}
		}
		else
		{
			appendNarrativeEntry(std::format("{} ceases and disengages from {}.", pName, stoppedName), pName, "Disengage", false, false);
		}
		updateAllAttitudes();
		syncCategories(gameContext);
		if (gameContext) gameContext->refreshActionGrid();
	}
}

void sexState::stopActionById(const std::string& actionId, game* gameContext)
{
	std::string stoppedName;
	bool stoppedAny = false;
	size_t actorIdx = 0;
	auto it = m_ongoingActions.begin();
	while (it != m_ongoingActions.end())
	{
		if (it->actionId == actionId)
		{
			if (stoppedName.empty()) stoppedName = it->actionName;
			actorIdx = it->actorIndex;
			it = m_ongoingActions.erase(it);
			stoppedAny = true;
		}
		else
		{
			++it;
		}
	}

	if (stoppedAny)
	{
		bool isPl = (actorIdx < m_participants.size()) ? m_participants[actorIdx].isPlayer : (actorIdx == 0);
		if (isPl)
		{
			appendNarrativeEntry(std::format("You cease and disengage from {}.", stoppedName), "You", "Disengage", true, false);
			if (gameContext)
			{
				processAllNpcTurns(gameContext);
				checkAllOrgasms(gameContext);
				m_turnCount++;
			}
		}
		else
		{
			std::string pName = (actorIdx < m_participants.size()) ? m_participants[actorIdx].getName() : "Partner";
			appendNarrativeEntry(std::format("{} ceases and disengages from {}.", pName, stoppedName), pName, "Disengage", false, false);
		}
		updateAllAttitudes();
		syncCategories(gameContext);
		if (gameContext) gameContext->refreshActionGrid();
	}
}

void sexState::clearAllOngoingActions()
{
	m_ongoingActions.clear();
}

void sexState::processOngoingActions(game* gameContext)
{
	if (m_ongoingActions.empty()) return;

	auto allActions = buildMasterActionDatabase();
	std::vector<OngoingSexAction> remaining;

	for (auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex >= m_participants.size()) continue;

		auto actIt = std::find_if(allActions.begin(), allActions.end(), [&oa](const SexAction& a) {
			return a.id == oa.actionId;
		});

		if (actIt != allActions.end() && !actIt->validStances.empty())
		{
			bool stanceOk = (std::find(actIt->validStances.begin(), actIt->validStances.end(), m_stance) != actIt->validStances.end());
			if (!stanceOk)
			{
				appendNarrativeEntry(std::format("{} can no longer be maintained in the {} position.", oa.actionName, sexStanceToString(m_stance)), "Scene", "End Ongoing", false, true);
				continue;
			}
		}

		oa.turnsActive++;
		auto& actor = m_participants[oa.actorIndex];

		if (isSolo() || oa.actorIndex == oa.targetIndex)
		{
			actor.arousal = std::clamp(actor.arousal + oa.arousalGainActor, 0.0f, 100.0f);
			if (actor.getEntity())
			{
				actor.getEntity()->stats.modifyBaseStat("arousal", oa.arousalGainActor);
				actor.getEntity()->stats.modifyBaseStat("lust", oa.arousalGainActor);
			}

			std::string narr = !oa.continuousNarrative.empty()
				? interpolateNarrative(oa.continuousNarrative, actor, actor, actIt != allActions.end() ? *actIt : SexAction{})
				: std::format("You continue {}, pleasure steadily mounting.", oa.actionName);
			appendNarrativeEntry(narr, "You", oa.actionName + " (Ongoing)", true, false);

			if (actor.arousal >= 100.0f)
			{
				actor.climaxes++;
				actor.arousal = 0.0f;
				if (actor.getEntity())
				{
					actor.getEntity()->stats.setBaseStat("lust", 0.0f);
					actor.getEntity()->stats.setBaseStat("arousal", 0.0f);
				}
				appendNarrativeEntry("The continuous stimulation pushes you over the edge into a breathless climax!", "You", "Climax", true, true);
			}
			remaining.push_back(oa);
			continue;
		}

		if (oa.targetIndex >= m_participants.size()) continue;
		auto& target = m_participants[oa.targetIndex];

		actor.arousal = std::clamp(actor.arousal + oa.arousalGainActor, 0.0f, 100.0f);
		target.arousal = std::clamp(target.arousal + oa.arousalGainTarget, 0.0f, 100.0f);

		actor.dominanceScore = std::clamp(actor.dominanceScore + oa.dominanceShift, -100.0f, 100.0f);
		target.dominanceScore = std::clamp(target.dominanceScore - oa.dominanceShift, -100.0f, 100.0f);

		std::string narr = !oa.continuousNarrative.empty()
			? interpolateNarrative(oa.continuousNarrative, actor, target, actIt != allActions.end() ? *actIt : SexAction{})
			: std::format("{} continues {} on {}, intensifying sensations.", actor.getName(), oa.actionName, target.getName());
		appendNarrativeEntry(narr, actor.getName(), oa.actionName + " (Ongoing)", actor.isPlayer, false);

		if (oa.fluidProducedMl > 0.0f && gameContext)
		{
			entity* actorEnt = actor.getEntity();
			entity* targetEnt = target.getEntity();
			if (actorEnt && targetEnt && (oa.actorSlot == bodySlot::GROIN || oa.targetSlot == bodySlot::GROIN || oa.targetSlot == bodySlot::MOUTH || oa.targetSlot == bodySlot::ASS))
			{
				bodyPart* aGroin = actorEnt->anatomy.getPart(bodySlot::GROIN);
				if (aGroin && aGroin->currentFluidMl > 0.0f && targetEnt->anatomy.hasOrifice(oa.targetSlot))
				{
					float transfer = std::min(aGroin->currentFluidMl, oa.fluidProducedMl);
					aGroin->currentFluidMl -= transfer;
					targetEnt->anatomy.transferFluidToOrifice(oa.targetSlot, "pre", transfer);
				}
			}
		}

		if (actor.arousal >= 100.0f)
		{
			processOrgasm(gameContext, actor.getEntity(), target.getEntity(), oa.targetSlot);
		}
		if (target.arousal >= 100.0f)
		{
			processOrgasm(gameContext, target.getEntity(), actor.getEntity(), oa.actorSlot);
		}

		remaining.push_back(oa);
	}

	m_ongoingActions = std::move(remaining);
}

bool sexState::executeAction(game* gameContext, const SexAction& action)
{
	if (!gameContext) return false;

	SexParticipant* playerP = getPlayerParticipant();
	if (!playerP) return false;

	entity* playerEnt = gameContext->getPlayer();
	if (!playerEnt) return false;

	if (action.id == "generic_stop_action")
	{
		stopOngoingAction(0, gameContext);
		return true;
	}

	if (action.id == "generic_end_solo")
	{
		appendNarrative(!action.narrative.empty() ? action.narrative : "You bring yourself to a composed finish, catch your breath, and readjust your clothing.");
		clearAllOngoingActions();
		gameContext->isPhoneMenuOpen = true;
		gameContext->changeState(std::make_unique<explorationState>());
		return true;
	}

	if (action.id == "generic_end_dominant" || action.id == "generic_end_submissive" || action.id == "generic_end_mutual")
	{
		SexParticipant* targetP = getTargetParticipant();
		if (targetP)
		{
			appendNarrative(interpolateNarrative(action.narrative, *playerP, *targetP, action));
		}
		else
		{
			appendNarrative("The encounter comes to an end.");
		}
		clearAllOngoingActions();
		gameContext->handleCommand({ CommandType::END_SEX_SCENE, 0, 0, "" });
		return true;
	}

	SexParticipant* targetP = isSolo() ? playerP : getTargetParticipant();
	if (!targetP) return false;

	entity* targetEnt = targetP->getEntity();

	std::string inaccReason;
	if (!isActionAccessible(action, *playerP, *targetP, &inaccReason))
	{
		appendNarrativeEntry(std::format("Cannot perform {}: {}", action.name, inaccReason), "Scene", "Inaccessible", true, true);
		return false;
	}

	if (action.id == "partner_strip" || action.id == "generic_cmd_strip")
	{
		undressEntity(targetEnt);
	}
	else if (action.id == "self_strip" || action.id == "sub_strip_offer")
	{
		undressEntity(playerEnt);
	}
	else if (action.id == "generic_undress_mutual")
	{
		undressEntity(playerEnt);
		undressEntity(targetEnt);
	}

	applyClothingDisplacementsForAction(action, playerEnt, targetEnt);

	float selfGain = action.arousalGainSelf;
	float partnerGain = action.arousalGainPartner;

	if (playerEnt && playerEnt->hasFetish(action.category))
	{
		selfGain *= 1.5f;
	}
	if (targetEnt && targetEnt->hasFetish(action.category))
	{
		partnerGain *= 1.5f;
		targetP->perceivedFetishes[action.category] = 4;
	}

	OngoingSexAction* existingOngoing = nullptr;
	for (auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == 0 && oa.actionId == action.id)
		{
			existingOngoing = &oa;
			break;
		}
	}

	if (existingOngoing)
	{
		selfGain *= 0.7f;
		partnerGain *= 0.7f;
		float domShift = action.dominanceShift * 0.5f;

		playerP->arousal = std::clamp(playerP->arousal + selfGain, 0.0f, 100.0f);
		targetP->arousal = std::clamp(targetP->arousal + partnerGain, 0.0f, 100.0f);
		playerP->dominanceScore = std::clamp(playerP->dominanceScore + domShift, -100.0f, 100.0f);
		targetP->dominanceScore = std::clamp(targetP->dominanceScore - domShift, -100.0f, 100.0f);
		existingOngoing->turnsActive++;

		std::string actionNarr = !action.continuousNarrative.empty()
			? interpolateNarrative(action.continuousNarrative, *playerP, *targetP, action)
			: interpolateNarrative(action.narrative, *playerP, *targetP, action);
		appendNarrativeEntry(actionNarr, "You", action.name + " (Ongoing)", true, false);
	}
	else
	{
		BodyPartUsage newUsage = getActionBodyPartUsage(action);
		int assignedHand = 0;

		if (newUsage == BodyPartUsage::BOTH_HANDS)
		{
			std::erase_if(m_ongoingActions, [](const auto& oa) {
				return oa.actorIndex == 0 && (oa.bodyPartUsage == BodyPartUsage::ONE_HAND ||
				                              oa.bodyPartUsage == BodyPartUsage::BOTH_HANDS ||
				                              oa.handAssigned > 0);
			});
			assignedHand = 3;
		}
		else if (newUsage == BodyPartUsage::ONE_HAND)
		{
			bool hadBothHands = false;
			for (const auto& oa : m_ongoingActions)
			{
				if (oa.actorIndex == 0 && (oa.bodyPartUsage == BodyPartUsage::BOTH_HANDS || oa.handAssigned == 3))
				{
					hadBothHands = true;
					break;
				}
			}

			if (hadBothHands)
			{
				std::erase_if(m_ongoingActions, [](const auto& oa) {
					return oa.actorIndex == 0 && (oa.bodyPartUsage == BodyPartUsage::BOTH_HANDS || oa.handAssigned == 3);
				});
				assignedHand = 1;
			}
			else
			{
				bool hand1InUse = false;
				bool hand2InUse = false;
				for (const auto& oa : m_ongoingActions)
				{
					if (oa.actorIndex == 0)
					{
						if (oa.handAssigned == 1) hand1InUse = true;
						if (oa.handAssigned == 2) hand2InUse = true;
					}
				}

				if (!hand1InUse)
				{
					assignedHand = 1;
				}
				else if (!hand2InUse)
				{
					assignedHand = 2;
				}
				else
				{
					bool replacedSameTarget = false;
					for (auto it = m_ongoingActions.begin(); it != m_ongoingActions.end(); ++it)
					{
						if (it->actorIndex == 0 && it->targetSlot == action.targetSlot && it->handAssigned > 0)
						{
							assignedHand = it->handAssigned;
							m_ongoingActions.erase(it);
							replacedSameTarget = true;
							break;
						}
					}
					if (!replacedSameTarget)
					{
						std::erase_if(m_ongoingActions, [](const auto& oa) {
							return oa.actorIndex == 0 && oa.handAssigned == 1;
						});
						assignedHand = 1;
					}
				}
			}
		}
		else if (newUsage == BodyPartUsage::MOUTH)
		{
			std::erase_if(m_ongoingActions, [](const auto& oa) {
				return oa.actorIndex == 0 && (oa.bodyPartUsage == BodyPartUsage::MOUTH || oa.actorSlot == bodySlot::MOUTH);
			});
		}
		else if (newUsage == BodyPartUsage::GROIN)
		{
			std::erase_if(m_ongoingActions, [](const auto& oa) {
				return oa.actorIndex == 0 && (oa.bodyPartUsage == BodyPartUsage::GROIN || oa.actorSlot == bodySlot::GROIN);
			});
		}
		else if (newUsage == BodyPartUsage::TORSO)
		{
			std::erase_if(m_ongoingActions, [](const auto& oa) { return oa.actorIndex == 0; });
		}

		if (action.targetSlot == bodySlot::MOUTH)
		{
			std::erase_if(m_ongoingActions, [&](const auto& oa) {
				return oa.targetIndex == (isSolo() ? 0 : m_selectedTargetIndex) && oa.targetSlot == bodySlot::MOUTH;
			});
		}
		if (action.targetSlot == bodySlot::GROIN && (action.id.find("penetrate") != std::string::npos || action.id.find("ride") != std::string::npos))
		{
			std::erase_if(m_ongoingActions, [&](const auto& oa) {
				return oa.actorIndex == 0 && oa.targetIndex == (isSolo() ? 0 : m_selectedTargetIndex) &&
				       oa.targetSlot == bodySlot::GROIN &&
				       (oa.actionId.find("penetrate") != std::string::npos || oa.actionId.find("ride") != std::string::npos);
			});
		}

		playerP->arousal = std::clamp(playerP->arousal + selfGain, 0.0f, 100.0f);
		targetP->arousal = std::clamp(targetP->arousal + partnerGain, 0.0f, 100.0f);
		playerP->dominanceScore = std::clamp(playerP->dominanceScore + action.dominanceShift, -100.0f, 100.0f);
		targetP->dominanceScore = std::clamp(targetP->dominanceScore - action.dominanceShift, -100.0f, 100.0f);
		playerP->stamina = std::clamp(playerP->stamina + 10.0f, 0.0f, 100.0f);

		if (action.isContinuous)
		{
			OngoingSexAction ongoing;
			ongoing.actionId = action.id;
			ongoing.actionName = action.name;
			ongoing.actorIndex = 0;
			ongoing.targetIndex = isSolo() ? 0 : m_selectedTargetIndex;
			ongoing.actorSlot = action.actorSlot;
			ongoing.targetSlot = action.targetSlot;
			ongoing.bodyPartUsage = newUsage;
			ongoing.handAssigned = assignedHand;
			ongoing.arousalGainActor = action.arousalGainSelf * 0.7f;
			ongoing.arousalGainTarget = action.arousalGainPartner * 0.7f;
			ongoing.dominanceShift = action.dominanceShift * 0.5f;
			ongoing.fluidProducedMl = action.fluidProducedMl;
			ongoing.continuousNarrative = action.continuousNarrative;
			ongoing.turnsActive = 1;
			m_ongoingActions.push_back(ongoing);
		}

		if ((action.actorSlot == bodySlot::GROIN || action.id.find("penetrate") != std::string::npos) && targetEnt)
		{
			bodyPart* pPenis = playerEnt->anatomy.getPart(bodySlot::GROIN);
			float penisDiam = pPenis ? pPenis->diameter : 3.5f;
			bodySlot targetSlot = (action.id.find("anal") != std::string::npos) ? bodySlot::ASS : bodySlot::GROIN;

			OrificeData* orifice = targetEnt->anatomy.getOrifice(targetSlot);
			if (orifice && penisDiam > orifice->currentStretch)
			{
				targetEnt->anatomy.stretchOrifice(targetSlot, penisDiam);
				const std::string& unitPref = gameContext->settings.gameplay.unitPreference;
				std::string dStr = unitFormatter::formatLength(penisDiam, unitPref);
				appendNarrative(std::format("Your {} thick organ stretches {}'s tight passage to accommodate your girth.",
											dStr, targetP->getName()));
			}
		}

		std::string actionNarr = !action.narrative.empty()
			? interpolateNarrative(action.narrative, *playerP, *targetP, action)
			: std::format("Using the {} position, you perform {} on {}.", sexStanceToString(m_stance), action.name, targetP->getName());
		appendNarrativeEntry(actionNarr, "You", action.name, true, false);
	}

	for (auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == 0 && oa.actionId != action.id)
		{
			oa.turnsActive++;
			playerP->arousal = std::clamp(playerP->arousal + oa.arousalGainActor * 0.35f, 0.0f, 100.0f);
			if (!isSolo() && targetP)
			{
				targetP->arousal = std::clamp(targetP->arousal + oa.arousalGainTarget * 0.35f, 0.0f, 100.0f);
			}
		}
	}

	if (isSolo())
	{
		if (playerP->arousal >= 100.0f || action.id == "solo_climax")
		{
			processOrgasm(gameContext, playerEnt, nullptr, action.targetSlot);
		}
		m_turnCount++;
		updateAllAttitudes();
		syncCategories(gameContext);
		gameContext->refreshActionGrid();
		return true;
	}

	processAllNpcTurns(gameContext);

	checkAllOrgasms(gameContext);

	m_turnCount++;
	updateAllAttitudes();
	syncCategories(gameContext);
	gameContext->refreshActionGrid();
	return true;
}

void sexState::processAllNpcTurns(game* gameContext)
{
	for (size_t i = 0; i < m_participants.size(); ++i)
	{
		if (!m_participants[i].isPlayer)
		{
			processSingleNpcTurn(gameContext, i);
		}
	}
}

void sexState::processSingleNpcTurn(game* gameContext, size_t npcIdx)
{
	if (npcIdx >= m_participants.size()) return;
	auto& npc = m_participants[npcIdx];
	entity* npcEnt = npc.getEntity();
	if (!npcEnt) return;

	size_t targetIdx = 0;
	auto& target = m_participants[targetIdx];
	entity* targetEnt = target.getEntity();

	SexRole role = npc.getRole();
	SexPhase phase = getScenePhase();
	auto allActions = buildMasterActionDatabase();

	OngoingSexAction* maintainableOa = nullptr;
	for (auto& oa : m_ongoingActions)
	{
		if (oa.actorIndex == npcIdx)
		{
			auto actIt = std::find_if(allActions.begin(), allActions.end(), [&](const SexAction& a) { return a.id == oa.actionId; });
			bool stanceOk = (actIt == allActions.end() || actIt->validStances.empty() ||
			                 std::find(actIt->validStances.begin(), actIt->validStances.end(), m_stance) != actIt->validStances.end());
			if (stanceOk)
			{
				bool shouldMaintain = (oa.turnsActive < 2) || (role == SexRole::SUBMISSIVE && oa.actionId == "sub_yield");
				if (shouldMaintain)
				{
					maintainableOa = &oa;
					break;
				}
			}
		}
	}

	if (maintainableOa)
	{
		auto actIt = std::find_if(allActions.begin(), allActions.end(), [&](const SexAction& a) { return a.id == maintainableOa->actionId; });
		npc.arousal = std::clamp(npc.arousal + maintainableOa->arousalGainActor, 0.0f, 100.0f);
		target.arousal = std::clamp(target.arousal + maintainableOa->arousalGainTarget, 0.0f, 100.0f);
		npc.dominanceScore = std::clamp(npc.dominanceScore + maintainableOa->dominanceShift, -100.0f, 100.0f);
		target.dominanceScore = std::clamp(target.dominanceScore - maintainableOa->dominanceShift, -100.0f, 100.0f);
		maintainableOa->turnsActive++;

		std::string npcNarr = !maintainableOa->continuousNarrative.empty()
			? interpolateNarrative(maintainableOa->continuousNarrative, npc, target, actIt != allActions.end() ? *actIt : SexAction{})
			: std::format("{} continues {} on {}.", npc.getName(), maintainableOa->actionName, target.getName());
		appendNarrativeEntry(npcNarr, npc.getName(), maintainableOa->actionName + " (Ongoing)", false, false);
		return;
	}

	std::vector<std::pair<SexAction, float>> scoredActions;

	for (const auto& act : allActions)
	{
		if (act.isSolo) continue;
		if (!isActionValidForParticipants(act, npc, target)) continue;
		if (!isActionAccessible(act, npc, target)) continue;

		float score = 10.0f;

		if (act.id == "partner_strip" || act.id == "generic_cmd_strip")
		{
			if (role == SexRole::DOMINANT || role == SexRole::SWITCH)
			{
				score += 35.0f;
			}
		}

		if (role == SexRole::DOMINANT)
		{
			score += act.dominanceShift * 2.5f;
			if (act.dominanceShift < -5.0f) score -= 40.0f;
		}
		else if (role == SexRole::SUBMISSIVE)
		{
			score -= act.dominanceShift * 2.5f;
			if (act.dominanceShift > 5.0f) score -= 40.0f;
		}

		if (phase == SexPhase::FOREPLAY)
		{
			if (act.arousalGainSelf <= 20.0f && act.actorSlot != bodySlot::GROIN) score += 20.0f;
		}
		else if (phase == SexPhase::ESCALATION)
		{
			if (act.actorSlot == bodySlot::MOUTH || act.targetSlot == bodySlot::GROIN) score += 20.0f;
		}
		else if (phase == SexPhase::PENETRATION)
		{
			if (act.actorSlot == bodySlot::GROIN || act.targetSlot == bodySlot::GROIN || act.targetSlot == bodySlot::ASS) score += 25.0f;
		}
		else if (phase == SexPhase::CLIMAX_EDGE)
		{
			if (role == SexRole::DOMINANT && npcEnt->hasTrait("Cruel") && act.id == "partner_deny_climax") score += 40.0f;
			else score += act.arousalGainSelf;
		}

		std::string fetishKey = (act.targetSlot == bodySlot::ASS) ? "Anal" :
								((act.targetSlot == bodySlot::MOUTH || act.actorSlot == bodySlot::MOUTH) ? "Oral" :
								((act.actorSlot == bodySlot::GROIN || act.targetSlot == bodySlot::GROIN) ? "Vaginal" :
								((act.targetSlot == bodySlot::BREASTS || act.targetSlot == bodySlot::NIPPLES) ? "Breasts" : "Intimacy")));
		int ownFetishDesire = npcEnt->getFetishDesire(fetishKey);
		if (ownFetishDesire >= 4) score += 25.0f;
		else if (ownFetishDesire == 3) score += 10.0f;
		else if (ownFetishDesire == 1) score -= 50.0f;

		auto it = npc.perceivedFetishes.find(fetishKey);
		if (it != npc.perceivedFetishes.end())
		{
			if (npcEnt->hasTrait("Kind")) score += 20.0f;
			if (npcEnt->hasTrait("Cruel") && it->second >= 4) score -= 10.0f;
		}

		scoredActions.push_back({ act, score });
	}

	if (scoredActions.empty())
	{
		npc.arousal = std::clamp(npc.arousal + 15.0f, 0.0f, 100.0f);
		target.arousal = std::clamp(target.arousal + 15.0f, 0.0f, 100.0f);
		appendNarrativeEntry(std::format("{} caresses {} with intimate intent.", npc.getName(), target.getName()), npc.getName(), "Caress", false, false);
		return;
	}

	std::sort(scoredActions.begin(), scoredActions.end(), [](const auto& a, const auto& b) {
		return a.second > b.second;
	});

	const auto& chosen = scoredActions.front().first;

	if (chosen.id == "partner_strip" || chosen.id == "generic_cmd_strip")
	{
		undressEntity(targetEnt);
	}
	else if (chosen.id == "self_strip")
	{
		undressEntity(npcEnt);
	}

	applyClothingDisplacementsForAction(chosen, npcEnt, targetEnt);

	BodyPartUsage newUsage = getActionBodyPartUsage(chosen);
	int assignedHand = 0;

	if (newUsage == BodyPartUsage::BOTH_HANDS)
	{
		std::erase_if(m_ongoingActions, [&](const auto& oa) {
			return oa.actorIndex == npcIdx && (oa.bodyPartUsage == BodyPartUsage::ONE_HAND ||
			                                  oa.bodyPartUsage == BodyPartUsage::BOTH_HANDS ||
			                                  oa.handAssigned > 0);
		});
		assignedHand = 3;
	}
	else if (newUsage == BodyPartUsage::ONE_HAND)
	{
		bool hadBoth = false;
		for (const auto& oa : m_ongoingActions)
		{
			if (oa.actorIndex == npcIdx && (oa.bodyPartUsage == BodyPartUsage::BOTH_HANDS || oa.handAssigned == 3))
			{
				hadBoth = true;
				break;
			}
		}

		if (hadBoth)
		{
			std::erase_if(m_ongoingActions, [&](const auto& oa) {
				return oa.actorIndex == npcIdx && (oa.bodyPartUsage == BodyPartUsage::BOTH_HANDS || oa.handAssigned == 3);
			});
			assignedHand = 1;
		}
		else
		{
			bool h1 = false, h2 = false;
			for (const auto& oa : m_ongoingActions)
			{
				if (oa.actorIndex == npcIdx)
				{
					if (oa.handAssigned == 1) h1 = true;
					if (oa.handAssigned == 2) h2 = true;
				}
			}
			if (!h1) assignedHand = 1;
			else if (!h2) assignedHand = 2;
			else
			{
				std::erase_if(m_ongoingActions, [&](const auto& oa) {
					return oa.actorIndex == npcIdx && oa.handAssigned == 1;
				});
				assignedHand = 1;
			}
		}
	}
	else if (newUsage == BodyPartUsage::MOUTH)
	{
		std::erase_if(m_ongoingActions, [&](const auto& oa) {
			return oa.actorIndex == npcIdx && (oa.bodyPartUsage == BodyPartUsage::MOUTH || oa.actorSlot == bodySlot::MOUTH);
		});
	}
	else if (newUsage == BodyPartUsage::GROIN)
	{
		std::erase_if(m_ongoingActions, [&](const auto& oa) {
			return oa.actorIndex == npcIdx && (oa.bodyPartUsage == BodyPartUsage::GROIN || oa.actorSlot == bodySlot::GROIN);
		});
	}
	else if (newUsage == BodyPartUsage::TORSO)
	{
		std::erase_if(m_ongoingActions, [&](const auto& oa) { return oa.actorIndex == npcIdx; });
	}

	if (chosen.isContinuous)
	{
		OngoingSexAction ongoing;
		ongoing.actionId = chosen.id;
		ongoing.actionName = chosen.name;
		ongoing.actorIndex = npcIdx;
		ongoing.targetIndex = targetIdx;
		ongoing.actorSlot = chosen.actorSlot;
		ongoing.targetSlot = chosen.targetSlot;
		ongoing.bodyPartUsage = newUsage;
		ongoing.handAssigned = assignedHand;
		ongoing.arousalGainActor = chosen.arousalGainSelf * 0.7f;
		ongoing.arousalGainTarget = chosen.arousalGainPartner * 0.7f;
		ongoing.dominanceShift = chosen.dominanceShift * 0.5f;
		ongoing.fluidProducedMl = chosen.fluidProducedMl;
		ongoing.continuousNarrative = chosen.continuousNarrative;
		ongoing.turnsActive = 1;
		m_ongoingActions.push_back(ongoing);
	}

	npc.arousal = std::clamp(npc.arousal + chosen.arousalGainSelf, 0.0f, 100.0f);
	target.arousal = std::clamp(target.arousal + chosen.arousalGainPartner, 0.0f, 100.0f);

	npc.dominanceScore = std::clamp(npc.dominanceScore + chosen.dominanceShift, -100.0f, 100.0f);
	target.dominanceScore = std::clamp(target.dominanceScore - chosen.dominanceShift, -100.0f, 100.0f);

	std::string npcNarr = !chosen.narrative.empty()
		? interpolateNarrative(chosen.narrative, npc, target, chosen)
		: std::format("{} performs {} on {}.", npc.getName(), chosen.name, target.getName());
	appendNarrativeEntry(npcNarr, npc.getName(), chosen.name, false, false);

	if (target.beggedClimax && role == SexRole::DOMINANT)
	{
		target.beggedClimax = false;
		if (npcEnt->hasTrait("Cruel"))
		{
			appendNarrativeEntry(std::format("{} smiles cruelly at {}'s desperate begging, intentionally slowing their pace to deny the release!", npc.getName(), target.getName()), npc.getName(), "Deny Climax", false, false);
		}
		else
		{
			appendNarrativeEntry(std::format("{} hears {}'s plea and drives their hips with passionate intensity, granting the promised climax!", npc.getName(), target.getName()), npc.getName(), "Grant Climax", false, false);
			target.arousal = 100.0f;
		}
	}
}

void sexState::checkAllOrgasms(game* gameContext)
{
	for (size_t i = 0; i < m_participants.size(); ++i)
	{
		if (m_participants[i].arousal >= 100.0f)
		{
			entity* orgEnt = m_participants[i].getEntity();
			size_t targetIdx = (i == 0) ? m_selectedTargetIndex : 0;
			entity* recEnt = (targetIdx < m_participants.size()) ? m_participants[targetIdx].getEntity() : nullptr;
			processOrgasm(gameContext, orgEnt, recEnt, bodySlot::GROIN);
		}
	}
}

void sexState::processPartnerTurn(game* gameContext)
{
	processAllNpcTurns(gameContext);
	checkAllOrgasms(gameContext);
	m_turnCount++;
	updateAllAttitudes();
	syncCategories(gameContext);
	if (gameContext) gameContext->refreshActionGrid();
}

void sexState::handlePleadSuggest(game* gameContext)
{
	SexParticipant* playerP = getPlayerParticipant();
	SexParticipant* targetP = getTargetParticipant();
	std::string pName = targetP ? targetP->getName() : "Partner";

	std::erase_if(m_ongoingActions, [](const auto& oa) { return oa.actorIndex == 0; });

	appendNarrativeEntry(formatTemplate(getIntimateNarratives().strugglePlead, { pName }), "You", "Plead", true, false);
	if (playerP) playerP->dominanceScore = std::clamp(playerP->dominanceScore - 4.0f, -100.0f, 100.0f);

	processPartnerTurn(gameContext);
}

void sexState::handleEndure(game* gameContext)
{
	SexParticipant* playerP = getPlayerParticipant();
	SexParticipant* targetP = getTargetParticipant();
	std::string pName = targetP ? targetP->getName() : "Partner";

	std::erase_if(m_ongoingActions, [](const auto& oa) { return oa.actorIndex == 0; });

	if (playerP)
	{
		playerP->arousal = std::clamp(playerP->arousal + 15.0f, 0.0f, 100.0f);
		playerP->dominanceScore = std::clamp(playerP->dominanceScore - 5.0f, -100.0f, 100.0f);
		playerP->stamina = std::clamp(playerP->stamina + 15.0f, 0.0f, 100.0f);
	}

	appendNarrativeEntry(formatTemplate(getIntimateNarratives().struggleEndure, { pName }), "You", "Endure", true, false);
	processPartnerTurn(gameContext);
}

void sexState::handleBegClimax(game* gameContext)
{
	SexParticipant* playerP = getPlayerParticipant();
	if (playerP)
	{
		playerP->beggedClimax = true;
		playerP->dominanceScore = std::clamp(playerP->dominanceScore - 6.0f, -100.0f, 100.0f);
	}

	appendNarrativeEntry(getIntimateNarratives().struggleBegClimax, "You", "Beg for Climax", true, false);
	processPartnerTurn(gameContext);
}

void sexState::handleStruggle(game* gameContext)
{
	SexParticipant* playerP = getPlayerParticipant();
	SexParticipant* targetP = getTargetParticipant();
	if (!playerP || !targetP) return;

	std::string reason;
	if (!canParticipantStruggle(0, &reason))
	{
		appendNarrativeEntry(reason, "You", "Struggle Blocked", true, true);
		if (gameContext) gameContext->refreshActionGrid();
		return;
	}

	entity* playerEnt = playerP->getEntity();
	entity* targetEnt = targetP->getEntity();

	float pPhys = playerEnt ? playerEnt->getStat("physique") : 10.0f;
	float targetPhys = targetEnt ? targetEnt->getStat("physique") : 10.0f;

	float rollPlayer = pPhys + dice::rollFloat(1.0f, 12.0f);
	float rollTarget = targetPhys + dice::rollFloat(1.0f, 10.0f);

	if (rollPlayer > rollTarget)
	{
		playerP->dominanceScore = std::clamp(playerP->dominanceScore + 35.0f, -100.0f, 100.0f);
		targetP->dominanceScore = std::clamp(targetP->dominanceScore - 35.0f, -100.0f, 100.0f);
		playerP->hasOpeningToStruggle = false;

		clearAllOngoingActions();

		appendNarrativeEntry(formatTemplate(getIntimateNarratives().struggleSuccess, { targetP->getName() }), "You", "Struggle Success", true, false);
		checkAllOrgasms(gameContext);
		m_turnCount++;
		updateAllAttitudes();
		syncCategories(gameContext);
		if (gameContext) gameContext->refreshActionGrid();
	}
	else
	{
		playerP->dominanceScore = std::clamp(playerP->dominanceScore - 10.0f, -100.0f, 100.0f);
		playerP->stamina = std::clamp(playerP->stamina - 20.0f, 0.0f, 100.0f);
		playerP->resistedLastTurn = true;

		appendNarrativeEntry(formatTemplate(getIntimateNarratives().struggleOverpowered, { targetP->getName() }), "You", "Struggle Overpowered", true, false);
		processPartnerTurn(gameContext);
	}
}

void sexState::processOrgasm(game* gameContext, entity* orgasmingEntity, entity* receivingEntity, bodySlot receivingSlot)
{
	if (!orgasmingEntity) return;

	std::string oName = orgasmingEntity->name;
	std::string rName = receivingEntity ? receivingEntity->name : "the floor";
	bool isPlayerOrg = (orgasmingEntity == (gameContext ? gameContext->getPlayer() : nullptr));
	bool isPlayerRec = (receivingEntity == (gameContext ? gameContext->getPlayer() : nullptr));

	const auto& narrCfg = getIntimateNarratives();

	std::string climNarr = isPlayerOrg
		? narrCfg.climaxPlayer
		: formatTemplate(narrCfg.climaxOther, { oName });
	appendNarrativeEntry(climNarr, isPlayerOrg ? "You" : oName, "Climax", isPlayerOrg, true);

	float fluidMult = (gameContext && gameContext->settings.content.fluidMultiplier > 0.0f) ? gameContext->settings.content.fluidMultiplier : 1.0f;
	const std::string& unitPref = gameContext ? gameContext->settings.gameplay.unitPreference : "Metric";

	bodyPart* groin = orgasmingEntity->anatomy.getPart(bodySlot::GROIN);
	if (groin && groin->currentFluidMl > 0.0f)
	{
		float cumAmount = std::min(groin->currentFluidMl, 15.0f * fluidMult);
		groin->currentFluidMl = std::max(0.0f, groin->currentFluidMl - cumAmount);
		std::string cumStr = unitFormatter::formatFluid(cumAmount, unitPref);

		if (receivingEntity && receivingEntity->anatomy.hasOrifice(receivingSlot))
		{
			receivingEntity->anatomy.transferFluidToOrifice(receivingSlot, "cum", cumAmount);
			std::string ejNarr;
			if (isPlayerOrg)
			{
				ejNarr = formatTemplate(narrCfg.ejaculateOrificePlayer, { cumStr, rName });
			}
			else
			{
				ejNarr = isPlayerRec
					? formatTemplate(narrCfg.ejaculateOrificeTargetPlayer, { oName, cumStr })
					: formatTemplate(narrCfg.ejaculateOrificeOther, { oName, cumStr, rName });
			}
			appendNarrativeEntry(ejNarr, isPlayerOrg ? "You" : oName, "Ejaculation", isPlayerOrg, true);

			if (receivingSlot == bodySlot::GROIN && receivingEntity->anatomy.hasVagina() && gameContext && gameContext->settings.content.pregnancyEnabled)
			{
				bool success = receivingEntity->gestation.impregnate(
					orgasmingEntity->id, orgasmingEntity->name,
					orgasmingEntity->anatomy.getDominantRace(),
					receivingEntity->anatomy.getDominantRace()
				);
				if (success)
				{
					std::string concNarr = isPlayerRec
						? narrCfg.conceptionPlayer
						: formatTemplate(narrCfg.conceptionOther, { rName });
					appendNarrativeEntry(concNarr, isPlayerRec ? "You" : rName, "Conception", isPlayerRec, true);
				}
			}
		}
		else
		{
			std::string ejNarr;
			if (isPlayerOrg)
			{
				ejNarr = formatTemplate(narrCfg.ejaculateSurfacePlayer, { cumStr, rName });
			}
			else
			{
				ejNarr = isPlayerRec
					? formatTemplate(narrCfg.ejaculateSurfaceTargetPlayer, { oName, cumStr })
					: formatTemplate(narrCfg.ejaculateSurfaceOther, { oName, cumStr, rName });
			}
			appendNarrativeEntry(ejNarr, isPlayerOrg ? "You" : oName, "Ejaculation", isPlayerOrg, true);
		}
	}

	bodyPart* breasts = orgasmingEntity->anatomy.getPart(bodySlot::BREASTS);
	if (breasts && breasts->isLactating && gameContext && gameContext->settings.content.lactationEnabled && breasts->currentFluidMl > 0.0f)
	{
		float milkAmount = std::min(breasts->currentFluidMl, 25.0f * fluidMult);
		breasts->currentFluidMl = std::max(0.0f, breasts->currentFluidMl - milkAmount);
		std::string milkStr = unitFormatter::formatFluid(milkAmount, unitPref);
		appendNarrativeEntry(formatTemplate(narrCfg.lactation, { isPlayerOrg ? "Your" : (oName + "'s"), milkStr }), isPlayerOrg ? "You" : oName, "Lactation", isPlayerOrg, true);
	}

	for (auto& p : m_participants)
	{
		if (p.getEntity() == orgasmingEntity || (p.isPlayer && isPlayerOrg))
		{
			p.arousal = 15.0f;
			p.climaxes++;
			p.stamina = std::clamp(p.stamina - 15.0f, 0.0f, 100.0f);
			if (p.getEntity())
			{
				p.getEntity()->stats.setBaseStat("arousal", 15.0f);
				p.getEntity()->stats.setBaseStat("lust", 15.0f);
			}
		}
	}
}

void sexState::handleCommand(game* gameContext, const UICommand& cmd)
{
	if (!gameContext) return;

	if (cmd.type == CommandType::EXECUTE_SEX_ACTION)
	{
		auto actions = getAvailableActions();
		int idx = cmd.intPayload1;
		SexAction act;
		bool found = false;

		if (!cmd.stringPayload.empty())
		{
			for (const auto& a : actions)
			{
				if (a.id == cmd.stringPayload)
				{
					act = a;
					found = true;
					break;
				}
			}
		}

		if (!found && idx >= 0 && static_cast<size_t>(idx) < actions.size())
		{
			act = actions[idx];
			found = true;
		}

		if (found)
		{
			if (act.type == SexActionType::PLEAD_SUGGEST || act.id == "sub_plead") handlePleadSuggest(gameContext);
			else if (act.type == SexActionType::ENDURE || act.id == "sub_endure") handleEndure(gameContext);
			else if (act.type == SexActionType::BEG_CLIMAX || act.id == "sub_beg_climax") handleBegClimax(gameContext);
			else if (act.type == SexActionType::STRUGGLE || act.id == "sub_struggle") handleStruggle(gameContext);
			else executeAction(gameContext, act);
		}
	}
	else if (cmd.type == CommandType::CHANGE_SEX_STANCE)
	{
		SexStance newStance = static_cast<SexStance>(cmd.intPayload1);
		changeStance(newStance);
		processPartnerTurn(gameContext);
	}
	else if (cmd.type == CommandType::SELECT_SEX_TARGET)
	{
		selectTargetParticipant(static_cast<size_t>(cmd.intPayload1));
		if (auto* targetP = getTargetParticipant())
		{
			if (targetP->character) gameContext->activeTargetNPC = targetP->character;
		}
		syncCategories(gameContext);
		gameContext->refreshActionGrid();
	}
	else if (cmd.type == CommandType::END_SEX_SCENE || cmd.type == CommandType::CLOSE_MENU)
	{
		if (!gameContext->sceneStack.empty())
		{
			gameContext->popScene();
		}
		else
		{
			gameContext->changeState(std::make_unique<explorationState>());
		}
	}
}