#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "entities/entity.h"
#include "quest/quest.h"

class game;
class gameMap;
class questComponent;

struct ScheduleEntry
{
    int startHour{ 0 };
    int endHour{ 24 };
    std::vector<int> daysOfWeek; // Empty = every day
    std::string mapId;
    int x{ 0 };
    int y{ 0 };
    std::string activity; // e.g. "tending his market curio stall"
};

struct QuestLocationOverride
{
    std::string questId;
    int minStage{ 0 };
    int maxStage{ 999 };
    std::vector<gameCondition> conditions;
    std::string mapId;
    int x{ 0 };
    int y{ 0 };
    std::string activity;
    std::string overrideSceneId;
};

class NamedCharacter
{
public:
    std::string id;
    std::string name;
    std::string title;
    std::string defaultSceneId;
    bool isMerchant{ false };
    float buyMarkup{ 1.0f };
    float sellMarkdown{ 0.5f };

    std::shared_ptr<entity> characterEntity{ nullptr };
    std::vector<ScheduleEntry> schedules;
    std::vector<QuestLocationOverride> questOverrides;

    std::string currentMapId;
    int currentX{ 0 };
    int currentY{ 0 };
    std::string currentActivity;
    std::string activeSceneId;

    bool resolveLocation(int hour, int dayOfWeek, const questComponent* quests, const game* gameCtx);
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

class NamedCharacterManager
{
public:
    static bool loadFromDirectory(const std::string& dirPath = "data/characters");
    static std::shared_ptr<NamedCharacter> getCharacter(const std::string& id);
    static const std::unordered_map<std::string, std::shared_ptr<NamedCharacter>>& getAllCharacters();

    static void updateAllLocations(game* g);
    static void syncMapCharacters(gameMap* targetMap);

    static nlohmann::json saveStateToJson();
    static void loadStateFromJson(const nlohmann::json& j);

    static void clear();

private:
    static std::unordered_map<std::string, std::shared_ptr<NamedCharacter>> registry;
};
