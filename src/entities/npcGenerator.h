#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "settings/gameSettings.h"

class entity;

struct NPCTemplate
{
	std::string id;
	std::string name;
	int levelMin{1};
	int levelMax{1};

	std::unordered_map<std::string, float> baseStats;
	std::vector<std::string> tags;
	std::vector<std::string> possibleRaces;
	std::vector<std::string> guaranteedItems;
	std::vector<std::string> randomItems;
};

class npcGenerator
{
public:
	static bool loadTemplates(const std::string& filePath);
	static std::shared_ptr<entity> generateFromTemplate(const std::string& templateId, const GameSettings* settings = nullptr);
	static std::shared_ptr<entity> generateRandomNPC(const GameSettings* settings = nullptr);

private:
	static std::unordered_map<std::string, NPCTemplate> registry;
	static void applyDemographicConfiguration(entity* npc, const GameSettings* settings);
};