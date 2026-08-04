#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "quest/quest.h"

class questDatabase {
public:
    static std::unordered_map<std::string, questScene> registry;
    static std::vector<MapTrigger> globalTriggers;

    static bool loadDatabase(const std::string& directoryPath);
    static bool exists(const std::string& id);
    static questScene getScene(const std::string& id);
    static std::vector<MapTrigger> getTriggersForLocation(const std::string& mapId, int x, int y);
};