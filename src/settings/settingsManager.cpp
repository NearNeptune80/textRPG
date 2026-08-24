#include "settings/settingsManager.h"

#include <fstream>
#include <iostream>

GameSettings settingsManager::getDefaultSettings()
{
    return GameSettings{};
}

bool settingsManager::loadFromFile(GameSettings& outSettings, const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cout << "[Settings] Could not open " << filePath << ", using default settings.\n";
        outSettings = getDefaultSettings();
        saveToFile(outSettings, filePath);
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        outSettings.fromJson(j);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Settings] Error parsing " << filePath << ": " << e.what() << "\n";
        outSettings = getDefaultSettings();
        return false;
    }
}

bool settingsManager::saveToFile(const GameSettings& settings, const std::string& filePath)
{
    try
    {
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "[Settings] Could not create file " << filePath << "\n";
            return false;
        }

        nlohmann::json j = settings.toJson();
        file << j.dump(4);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Settings] Error writing " << filePath << ": " << e.what() << "\n";
        return false;
    }
}
