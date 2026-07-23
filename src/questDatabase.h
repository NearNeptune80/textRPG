#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "quest.h"

class questDatabase
{
private:
    static std::unordered_map<std::string, questScene> registry;
    static std::vector<MapTrigger> globalTriggers;

public:
    static bool loadDatabase(const std::string& pathStr);
    static bool exists(const std::string& id);
    static questScene getScene(const std::string& id);
    static std::vector<MapTrigger> getTriggersForLocation(const std::string& mapId, int x, int y);
};