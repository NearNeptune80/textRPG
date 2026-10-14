#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

enum class WeatherType
{
    CLEAR,
    CLOUDY,
    RAIN,
    STORM,
    FOG,
    SNOW,
    HEATWAVE
};

inline std::string weatherTypeToString(WeatherType type)
{
    switch (type)
    {
        case WeatherType::CLEAR: return "CLEAR";
        case WeatherType::CLOUDY: return "CLOUDY";
        case WeatherType::RAIN: return "RAIN";
        case WeatherType::STORM: return "STORM";
        case WeatherType::FOG: return "FOG";
        case WeatherType::SNOW: return "SNOW";
        case WeatherType::HEATWAVE: return "HEATWAVE";
        default: return "CLEAR";
    }
}

inline WeatherType stringToWeatherType(const std::string& str)
{
    if (str == "CLEAR") return WeatherType::CLEAR;
    if (str == "CLOUDY") return WeatherType::CLOUDY;
    if (str == "RAIN") return WeatherType::RAIN;
    if (str == "STORM") return WeatherType::STORM;
    if (str == "FOG") return WeatherType::FOG;
    if (str == "SNOW") return WeatherType::SNOW;
    if (str == "HEATWAVE") return WeatherType::HEATWAVE;
    return WeatherType::CLEAR;
}

struct WeatherEffect
{
    float staminaDrain = 0.0f;
    int radarRadius = 3;
};

struct WeatherRegion
{
    bool isOutdoor = true;
    std::vector<std::pair<WeatherType, int>> weights;
};

class weatherManager
{
public:
    static bool loadConfig(const std::string& path = "data/config/weather.json");
    static void onMapChange(const std::string& mapId, bool isOutdoorOverride = true);
    static void setWeather(WeatherType type);
    static void setWeatherByName(const std::string& name);
    static WeatherType getCurrentWeather();
    static std::string getCurrentWeatherName();
    static float getMovementStaminaDrain();
    static int getRadarDiscoveryRadius();
    static bool isOutdoor();
    static void setOutdoor(bool outdoor);

    static nlohmann::json toJson();
    static void fromJson(const nlohmann::json& j);
    static void reset();

private:
    static WeatherType s_currentWeather;
    static bool s_isOutdoor;
    static std::string s_currentMapId;
    static std::unordered_map<WeatherType, WeatherEffect> s_effects;
    static std::unordered_map<std::string, WeatherRegion> s_regions;
    static bool s_loaded;
};

