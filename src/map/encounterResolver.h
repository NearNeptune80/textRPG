#pragma once

#include <memory>
#include <string>

#include "core/timeManager.h"
#include "quest/questDatabase.h"
#include "settings/gameSettings.h"

class game;
class entity;

class encounterResolver
{
public:
	static bool shouldTriggerEncounter(int dangerLevel, TimePhase phase, float playerStealth = 0.0f);
	static std::shared_ptr<entity> createEncounterNPC(int dangerLevel, const GameSettings& settings);
	static questScene buildEncounterScene(game* g, std::shared_ptr<entity> npc);
	static questScene buildVictoryScene(game* g, entity* targetNPC, const std::string& mapId);

private:
	static std::string selectFlavorText(const std::string& mapId, entity* target);
};