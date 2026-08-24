#pragma once

#include <string>
#include "settings/gameSettings.h"

class settingsManager
{
public:
    static bool loadFromFile(GameSettings& outSettings, const std::string& filePath = "data/settings.json");
    static bool saveToFile(const GameSettings& settings, const std::string& filePath = "data/settings.json");
    static GameSettings getDefaultSettings();
};
