#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "map/tile.h"
#include "quest/quest.h"

class entity;

class gameMap
{
public:
    gameMap();
    ~gameMap();

    bool loadFromFile(const std::string& filePath);
    bool isWalkable(int x, int y) const;
    bool isOpaque(int x, int y) const;
    void updateDiscovery(int playerX, int playerY, int visionRadius = 3);
    Tile getTile(int x, int y) const;
    bool checkWarp(int x, int y, MapWarp& outWarp) const;

    nlohmann::json saveStateToJson() const;
    void loadStateFromJson(const nlohmann::json& j);

    bool isStorageSafe(int x, int y) { return getRuntimeData(x, y).getIsEffectiveStorageSafe(); }
    void clearUnsafeItems(int x, int y)
    {
        if (!isStorageSafe(x, y))
        {
            getRuntimeData(x, y).droppedItems.clear();
        }
    }

    void processTimePassage(int minutesPassed);

    TileRuntimeData& getRuntimeData(int x, int y);
    const std::vector<MapTrigger>& getTriggers() const { return triggers; }
    std::vector<MapTrigger> getTriggersAt(int x, int y) const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    std::string getId() const { return mapId; }
    std::string getName() const { return mapName; }

private:
    int width = 0;
    int height = 0;
    std::string mapId;
    std::string mapName;

    std::vector<std::vector<Tile>> grid;
    std::vector<MapWarp> warps;
    std::vector<MapTrigger> triggers;
    std::unordered_map<uint64_t, TileRuntimeData> runtimeData;
};