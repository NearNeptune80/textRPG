#include "map/gameMap.h"

#include <cmath>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "entities/entity.h"
#include "items/itemDatabase.h"

using json = nlohmann::json;

gameMap::gameMap() = default;
gameMap::~gameMap() = default;

TileRuntimeData& gameMap::getRuntimeData(int x, int y)
{
    return runtimeData[makeTileKey(x, y)];
}

bool gameMap::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        json data;
        file >> data;

        mapId = data.value("id", "");
        mapName = data.value("name", "Unknown Map");
        width = data.at("width").get<int>();
        height = data.at("height").get<int>();

        grid.clear();
        grid.resize(height, std::vector<Tile>(width, { TILE_VOID, STATE_HIDDEN }));
        runtimeData.clear();

        auto tilesJson = data.at("tiles");
        for (size_t y = 0; y < static_cast<size_t>(height) && y < tilesJson.size(); ++y)
        {
            auto rowJson = tilesJson[y];
            for (size_t x = 0; x < static_cast<size_t>(width) && x < rowJson.size(); ++x)
            {
                int typeInt = rowJson[x].get<int>();
                grid[y][x] = { static_cast<TileType>(typeInt), STATE_HIDDEN };
            }
        }

        if (data.contains("dangerLevels"))
        {
            auto dangerJson = data.at("dangerLevels");
            for (size_t y = 0; y < static_cast<size_t>(height) && y < dangerJson.size(); ++y)
            {
                auto rowJson = dangerJson[y];
                for (size_t x = 0; x < static_cast<size_t>(width) && x < rowJson.size(); ++x)
                {
                    getRuntimeData(static_cast<int>(x), static_cast<int>(y)).baseDangerLevel = rowJson[x].get<int>();
                }
            }
        }

        warps.clear();
        if (data.contains("warps"))
        {
            for (const auto& wJson : data.at("warps"))
            {
                MapWarp w;
                w.x = wJson.at("x").get<int>();
                w.y = wJson.at("y").get<int>();
                w.targetMap = wJson.at("targetMap").get<std::string>();
                w.targetX = wJson.at("targetX").get<int>();
                w.targetY = wJson.at("targetY").get<int>();
                warps.push_back(w);

                getRuntimeData(w.x, w.y).iconId = "icon_door";
            }
        }

        triggers.clear();
        if (data.contains("triggers"))
        {
            for (const auto& tJson : data.at("triggers"))
            {
                MapTrigger trig;
                trig.id = tJson.value("id", "");
                trig.label = tJson.value("label", "Interact");
                trig.tooltip = tJson.value("tooltip", "");
                trig.description = tJson.value("description", "");
                trig.x = tJson.at("x").get<int>();
                trig.y = tJson.at("y").get<int>();
                trig.sceneId = tJson.at("sceneId").get<std::string>();

                if (tJson.contains("conditions"))
                {
                    for (const auto& cJson : tJson.at("conditions"))
                    {
                        gameCondition cond;
                        cond.type = cJson.at("type").get<std::string>();
                        cond.target = cJson.at("target").get<std::string>();
                        cond.requiredValue = cJson.value("requiredValue", 0);
                        trig.conditions.push_back(cond);
                    }
                }
                triggers.push_back(trig);
            }
        }

        // Descriptions & Tags
        defaultDescriptions.clear();
        if (data.contains("defaultDescriptions") && data["defaultDescriptions"].is_array())
        {
            for (const auto& d : data["defaultDescriptions"])
            {
                if (d.is_string()) defaultDescriptions.push_back(d.get<std::string>());
            }
        }
        else if (data.contains("description") && data["description"].is_string())
        {
            defaultDescriptions.push_back(data["description"].get<std::string>());
        }

        tileDescriptions.clear();
        if (data.contains("tileDescriptions") && data["tileDescriptions"].is_object())
        {
            for (const auto& [coordKey, val] : data["tileDescriptions"].items())
            {
                if (val.is_array())
                {
                    std::vector<std::string> list;
                    for (const auto& item : val) if (item.is_string()) list.push_back(item.get<std::string>());
                    tileDescriptions[coordKey] = list;
                }
                else if (val.is_string())
                {
                    tileDescriptions[coordKey] = { val.get<std::string>() };
                }
            }
        }

        tileTags.clear();
        if (data.contains("tileTags") && data["tileTags"].is_object())
        {
            for (const auto& [coordKey, val] : data["tileTags"].items())
            {
                if (val.is_array())
                {
                    std::vector<std::string> list;
                    for (const auto& item : val) if (item.is_string()) list.push_back(item.get<std::string>());
                    tileTags[coordKey] = list;
                }
                else if (val.is_string())
                {
                    tileTags[coordKey] = { val.get<std::string>() };
                }
            }
        }

        tagDescriptions.clear();
        if (data.contains("tagDescriptions") && data["tagDescriptions"].is_object())
        {
            for (const auto& [tagKey, val] : data["tagDescriptions"].items())
            {
                if (val.is_array())
                {
                    std::vector<std::string> list;
                    for (const auto& item : val) if (item.is_string()) list.push_back(item.get<std::string>());
                    tagDescriptions[tagKey] = list;
                }
                else if (val.is_string())
                {
                    tagDescriptions[tagKey] = { val.get<std::string>() };
                }
            }
        }

        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Map JSON Error (" << filePath << "): " << e.what() << "\n";
        return false;
    }
}

const std::vector<std::string>& gameMap::getTileTags(int x, int y) const
{
    static const std::vector<std::string> emptyTags;
    std::string key = std::to_string(x) + "," + std::to_string(y);
    auto it = tileTags.find(key);
    if (it != tileTags.end()) return it->second;
    return emptyTags;
}

std::string gameMap::getTileDescription(int x, int y) const
{
    std::string key = std::to_string(x) + "," + std::to_string(y);
    auto pickStable = [x, y](const std::vector<std::string>& list) -> std::string {
        if (list.empty()) return "";
        uint32_t h = (static_cast<uint32_t>(x) * 73856093u) ^ (static_cast<uint32_t>(y) * 19349663u);
        return list[h % list.size()];
    };

    // 1. Specific coordinate description
    auto itDesc = tileDescriptions.find(key);
    if (itDesc != tileDescriptions.end() && !itDesc->second.empty())
    {
        return pickStable(itDesc->second);
    }

    // 2. Tag-based descriptions
    auto itTags = tileTags.find(key);
    if (itTags != tileTags.end())
    {
        for (const auto& tag : itTags->second)
        {
            auto itTagDesc = tagDescriptions.find(tag);
            if (itTagDesc != tagDescriptions.end() && !itTagDesc->second.empty())
            {
                return pickStable(itTagDesc->second);
            }
        }
    }

    // 3. Default map descriptions
    if (!defaultDescriptions.empty())
    {
        return pickStable(defaultDescriptions);
    }

    return "You are in " + mapName + ".";
}

bool gameMap::isWalkable(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    TileType t = grid[y][x].type;
    return (t == TILE_FLOOR || t == TILE_DOOR);
}

bool gameMap::isOpaque(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return true;
    TileType t = grid[y][x].type;
    return (t == TILE_WALL || t == TILE_VOID);
}

void gameMap::updateDiscovery(int playerX, int playerY, int visionRadius)
{
    if (playerX < 0 || playerX >= width || playerY < 0 || playerY >= height) return;

    // The tile the player has physically walked on is marked as visited & revealed
    if (grid[playerY][playerX].type != TILE_VOID)
    {
        grid[playerY][playerX].visited = true;
        grid[playerY][playerX].discovery = STATE_REVEALED;
    }
}

void gameMap::processTimePassage(int minutesPassed)
{
    if (minutesPassed <= 0) return;

    for (auto& [key, runtime] : runtimeData)
    {
        runtime.processItemDecay(minutesPassed);
    }
}

Tile gameMap::getTile(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return { TILE_VOID, STATE_HIDDEN };
    return grid[y][x];
}

bool gameMap::checkWarp(int x, int y, MapWarp& outWarp) const
{
    for (const auto& w : warps)
    {
        if (w.x == x && w.y == y)
        {
            outWarp = w;
            return true;
        }
    }
    return false;
}

std::vector<MapTrigger> gameMap::getTriggersAt(int x, int y) const
{
    std::vector<MapTrigger> result;
    for (const auto& trig : triggers)
    {
        if (trig.x == x && trig.y == y)
        {
            result.push_back(trig);
        }
    }
    return result;
}

nlohmann::json gameMap::saveStateToJson() const
{
    json j;
    j["mapId"] = mapId;

    json discoveryGrid = json::array();
    for (int y = 0; y < height; ++y)
    {
        json row = json::array();
        for (int x = 0; x < width; ++x)
        {
            row.push_back(static_cast<int>(grid[y][x].discovery));
        }
        discoveryGrid.push_back(row);
    }
    j["discovery"] = discoveryGrid;

    json tileItemsMap = json::object();
    json tileNPCsMap = json::object();

    for (const auto& [key, runtime] : runtimeData)
    {
        if (!runtime.droppedItems.empty())
        {
            json itemArray = json::array();
            for (const auto& entry : runtime.droppedItems)
            {
                if (entry.itemPtr)
                {
                    json itemEntry;
                    itemEntry["id"] = entry.itemPtr->id;
                    itemEntry["count"] = entry.itemPtr->count;
                    itemEntry["minutesRemaining"] = entry.minutesRemaining;
                    itemArray.push_back(itemEntry);
                }
            }
            tileItemsMap[std::to_string(key)] = itemArray;
        }

        if (runtime.persistentNPC)
        {
            tileNPCsMap[std::to_string(key)] = runtime.persistentNPC->toJson();
        }
    }

    j["tileItems"] = tileItemsMap;
    j["tileNPCs"] = tileNPCsMap;

    return j;
}

void gameMap::loadStateFromJson(const json& j)
{
    if (j.contains("discovery"))
    {
        const auto& dGrid = j["discovery"];
        for (size_t y = 0; y < static_cast<size_t>(height) && y < dGrid.size(); ++y)
        {
            const auto& row = dGrid[y];
            for (size_t x = 0; x < static_cast<size_t>(width) && x < row.size(); ++x)
            {
                grid[y][x].discovery = static_cast<DiscoveryState>(row[x].get<int>());
            }
        }
    }

    if (j.contains("tileItems"))
    {
        for (auto& [keyStr, itemsJson] : j["tileItems"].items())
        {
            uint64_t key = std::stoull(keyStr);
            runtimeData[key].droppedItems.clear();
            for (const auto& itemEntry : itemsJson)
            {
                std::string itemId = itemEntry.is_string() ? itemEntry.get<std::string>() : itemEntry.value("id", "");
                int count = itemEntry.is_object() ? itemEntry.value("count", 1) : 1;
                int mins = itemEntry.is_object() ? itemEntry.value("minutesRemaining", 120) : 120;

                auto itemPtr = itemDatabase::getItem(itemId);
                if (itemPtr)
                {
                    itemPtr->count = count;
                    runtimeData[key].droppedItems.push_back({ itemPtr, mins });
                }
            }
        }
    }

    if (j.contains("tileNPCs"))
    {
        for (auto& [keyStr, npcJson] : j["tileNPCs"].items())
        {
            uint64_t key = std::stoull(keyStr);
            auto npc = std::make_shared<entity>("npc_temp", "Unknown");
            npc->fromJson(npcJson);
            runtimeData[key].persistentNPC = npc;
        }
    }
}