#pragma once
#include <string>
#include <unordered_map>
#include "quest.h"

class questDatabase
{
private:
    static std::unordered_map<std::string, questScene> registry;

public:
    static bool loadDatabase(const std::string& filePath);
    static bool exists(const std::string& id);
    static questScene getScene(const std::string& id);
};