#include "core/weatherManager.h"

#include <fstream>
#include <iostream>
#include <random>

WeatherType weatherManager::s_currentWeather = WeatherType::CLEAR;
bool weatherManager::s_isOutdoor = true;
std::string weatherManager::s_currentMapId = "overworld";
std::unordered_map<WeatherType, WeatherEffect> weatherManager::s_effects;
std::unordered_map<std::string, WeatherRegion> weatherManager::s_regions;
bool weatherManager::s_loaded = false;

bool weatherManager::loadConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[WeatherManager] Could not open config file: " << path << std::endl;
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;

        s_effects.clear();
        if (j.contains("effects") && j["effects"].is_object())
        {
            for (const auto& [wStr, eJson] : j["effects"].items())
            {
                WeatherType wt = stringToWeatherType(wStr);
                WeatherEffect eff;
                eff.staminaDrain = eJson.value("staminaDrain", 0.0f);
                eff.radarRadius = eJson.value("radarRadius", 3);
                s_effects[wt] = eff;
            }
        }

        s_regions.clear();
        if (j.contains("regions") && j["regions"].is_object())
        {
            for (const auto& [regKey, regJson] : j["regions"].items())
            {
                WeatherRegion reg;
                reg.isOutdoor = regJson.value("isOutdoor", true);
                if (regJson.contains("weights") && regJson["weights"].is_object())
                {
                    for (const auto& [wStr, weightVal] : regJson["weights"].items())
                    {
                        WeatherType wt = stringToWeatherType(wStr);
                        reg.weights.emplace_back(wt, weightVal.get<int>());
                    }
                }
                s_regions[regKey] = reg;
            }
        }

        s_loaded = true;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[WeatherManager] Error parsing config " << path << ": " << e.what() << std::endl;
        return false;
    }
}

void weatherManager::onMapChange(const std::string& mapId, bool isOutdoorOverride)
{
    if (!s_loaded)
    {
        loadConfig();
    }

    s_currentMapId = mapId;

    auto it = s_regions.find(mapId);
    if (it == s_regions.end())
    {
        it = s_regions.find("default");
    }

    if (it != s_regions.end())
    {
        s_isOutdoor = it->second.isOutdoor;
        const auto& weights = it->second.weights;
        if (!weights.empty())
        {
            int totalWeight = 0;
            for (const auto& [wType, w] : weights)
            {
                totalWeight += std::max(0, w);
            }

            if (totalWeight > 0)
            {
                static std::mt19937 rng{ std::random_device{}() };
                std::uniform_int_distribution<int> dist(0, totalWeight - 1);
                int roll = dist(rng);
                int acc = 0;
                for (const auto& [wType, w] : weights)
                {
                    acc += std::max(0, w);
                    if (roll < acc)
                    {
                        s_currentWeather = wType;
                        break;
                    }
                }
            }
            else
            {
                s_currentWeather = weights.front().first;
            }
        }
        else
        {
            s_currentWeather = WeatherType::CLEAR;
        }
    }
    else
    {
        s_isOutdoor = isOutdoorOverride;
        s_currentWeather = WeatherType::CLEAR;
    }
}

void weatherManager::setWeather(WeatherType type)
{
    s_currentWeather = type;
}

void weatherManager::setWeatherByName(const std::string& name)
{
    s_currentWeather = stringToWeatherType(name);
}

WeatherType weatherManager::getCurrentWeather()
{
    return s_currentWeather;
}

std::string weatherManager::getCurrentWeatherName()
{
    return weatherTypeToString(s_currentWeather);
}

float weatherManager::getMovementStaminaDrain()
{
    if (!s_loaded) loadConfig();
    if (!s_isOutdoor) return 0.0f;
    auto it = s_effects.find(s_currentWeather);
    if (it != s_effects.end())
    {
        return it->second.staminaDrain;
    }
    return 0.0f;
}

int weatherManager::getRadarDiscoveryRadius()
{
    if (!s_loaded) loadConfig();
    auto it = s_effects.find(s_currentWeather);
    if (it != s_effects.end())
    {
        return it->second.radarRadius;
    }
    return 3;
}

bool weatherManager::isOutdoor()
{
    return s_isOutdoor;
}

void weatherManager::setOutdoor(bool outdoor)
{
    s_isOutdoor = outdoor;
}

nlohmann::json weatherManager::toJson()
{
    nlohmann::json j;
    j["currentWeather"] = getCurrentWeatherName();
    j["isOutdoor"] = s_isOutdoor;
    j["currentMapId"] = s_currentMapId;
    return j;
}

void weatherManager::fromJson(const nlohmann::json& j)
{
    if (j.contains("currentWeather"))
    {
        setWeatherByName(j["currentWeather"].get<std::string>());
    }
    s_isOutdoor = j.value("isOutdoor", true);
    s_currentMapId = j.value("currentMapId", "overworld");
}

void weatherManager::reset()
{
    s_currentWeather = WeatherType::CLEAR;
    s_isOutdoor = true;
    s_currentMapId = "overworld";
    s_loaded = false;
    loadConfig();
}

