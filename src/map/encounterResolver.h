#pragma once

#include <string>

#include "quest/questDatabase.h"

class game;
class entity;

class encounterResolver
{
public:
	static questScene buildVictoryScene(game* g, entity* targetNPC, const std::string& mapId);

private:
	static std::string selectFlavorText(const std::string& mapId, entity* target);
};