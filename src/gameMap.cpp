#include "gameMap.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

gameMap::gameMap() {}
gameMap::~gameMap() {}

bool gameMap::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open map file: " << filePath << "\n";
        return false;
    }

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
            }
        }

        std::cout << "Successfully loaded map '" << mapName << "' (" << width << "x" << height << ").\n";
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Map JSON Parsing Error (" << filePath << "): " << e.what() << "\n";
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

                // Ignore both VOID and WALL tiles completely
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
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        return { TILE_VOID, STATE_HIDDEN };
    }
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