#pragma once

#include <string>
#include <unordered_map>

class questComponent
{
public:
	std::unordered_map<std::string, int> activeQuests;

	void setQuestStage(const std::string& questId, int stage);
	int getQuestStage(const std::string& questId) const;
};