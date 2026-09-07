#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "quest/quest.h"

class questDatabase {
public:
    static std::unordered_map<std::string, questScene> registry;
    static std::vector<MapTrigger> globalTriggers;
    static std::unordered_map<std::string, QuestDefinition> quests;
    static std::vector<QuestNPCRelocation> globalRelocations;

    static bool loadDatabase(const std::string& directoryPath);
    static bool exists(const std::string& id);
    static questScene getScene(const std::string& id);
    static std::vector<MapTrigger> getTriggersForLocation(const std::string& mapId, int x, int y);
    static std::vector<MapTrigger> getTriggersForNPC(const std::string& npcId);
    static std::vector<QuestNPCRelocation> getRelocationsForNPC(const std::string& npcId);

    static const QuestDefinition* getQuest(const std::string& id);
    static std::vector<QuestDefinition> getAllQuests();
};