#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
	static std::shared_ptr<entity> generateFromTemplate(const std::string& templateId);
	static std::shared_ptr<entity> generateRandomNPC();

private:
	static std::unordered_map<std::string, NPCTemplate> registry;
};