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
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Map JSON Error (" << filePath << "): " << e.what() << "\n";
        return false;
    }
}

bool gameMap::isWalkable(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    TileType t = grid[y][x].type;
    return (t == TILE_FLOOR || t == TILE_DOOR);
}

void gameMap::updateDiscovery(int playerX, int playerY)
{
    if (playerX < 0 || playerX >= width || playerY < 0 || playerY >= height) return;

    if (grid[playerY][playerX].type != TILE_VOID && grid[playerY][playerX].type != TILE_WALL)
    {
        grid[playerY][playerX].discovery = STATE_REVEALED;
    }

    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (std::abs(dx) + std::abs(dy) > 1) continue;

            int nx = playerX + dx;
            int ny = playerY + dy;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height)
            {
                TileType type = grid[ny][nx].type;
                if (type != TILE_VOID && type != TILE_WALL && grid[ny][nx].discovery == STATE_HIDDEN)
                {
                    grid[ny][nx].discovery = STATE_PARTIAL;
                }
            }
        }
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
            for (const auto& itemPtr : runtime.droppedItems)
            {
                if (itemPtr)
                {
                    json itemEntry;
                    itemEntry["id"] = itemPtr->id;
                    itemEntry["count"] = itemPtr->count;
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

                auto itemPtr = itemDatabase::getItem(itemId);
                if (itemPtr)
                {
                    itemPtr->count = count;
                    runtimeData[key].droppedItems.push_back(itemPtr);
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