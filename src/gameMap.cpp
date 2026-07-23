#include "gameMap.h"
#include "entity.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

gameMap::gameMap() {}
gameMap::~gameMap() {}

std::string gameMap::getTileKey(int x, int y) const
{
    return std::to_string(x) + "_" + std::to_string(y);
}

TileRuntimeData& gameMap::getRuntimeData(int x, int y)
{
    std::string key = getTileKey(x, y);
    return runtimeData[key];
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
        for (size_t y = 0; y < (size_t)height && y < tilesJson.size(); ++y)
        {
            auto rowJson = tilesJson[y];
            for (size_t x = 0; x < (size_t)width && x < rowJson.size(); ++x)
            {
                int typeInt = rowJson[x].get<int>();
                TileType tType = static_cast<TileType>(typeInt);
                grid[y][x] = { tType, STATE_HIDDEN };
            }
        }

        // Parse optional danger levels
        if (data.contains("dangerLevels"))
        {
            auto dangerJson = data.at("dangerLevels");
            for (size_t y = 0; y < (size_t)height && y < dangerJson.size(); ++y)
            {
                auto rowJson = dangerJson[y];
                for (size_t x = 0; x < (size_t)width && x < rowJson.size(); ++x)
                {
                    getRuntimeData((int)x, (int)y).baseDangerLevel = rowJson[x].get<int>();
                }
            }
        }

        // Parse Warps
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

        // Parse Dynamic Map Triggers
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