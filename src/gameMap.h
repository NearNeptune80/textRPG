#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "quest.h"
#include "item.h"

class entity;

enum TileType
{
    TILE_VOID = 0,
    TILE_FLOOR = 1,
    TILE_WALL = 2,
    TILE_DOOR = 3
};

enum DiscoveryState
{
    STATE_HIDDEN,
    STATE_PARTIAL,
    STATE_REVEALED
};

struct Tile
{
    TileType type;
    DiscoveryState discovery;
};

struct MapWarp
{
    int x;
    int y;
    std::string targetMap;
    int targetX;
    int targetY;
};

struct TemporarySafetyModifier
{
    std::string sourceId;
    int dangerDelta;
    int durationTurns;
};

struct TileRuntimeData
{
    std::shared_ptr<entity> persistentNPC = nullptr;
    std::vector<std::shared_ptr<item>> droppedItems;
    std::string iconId = "";
    int baseDangerLevel = 0;
    bool isStorageSafe = false;

    std::vector<TemporarySafetyModifier> safetyModifiers;

    int getEffectiveDangerLevel() const
    {
        int effective = baseDangerLevel;
        for (const auto& mod : safetyModifiers)
        {
            effective += mod.dangerDelta;
        }
        return std::max(0, effective);
    }
};

constexpr uint64_t makeTileKey(int32_t x, int32_t y) noexcept
{
    return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
}

class gameMap
{
public:
    gameMap();
    ~gameMap();

    bool isStorageSafe(int x, int y)
    {
        return getRuntimeData(x, y).isStorageSafe;
    }

    void clearUnsafeItems(int x, int y)
    {
        if (!isStorageSafe(x, y))
        {
            getRuntimeData(x, y).droppedItems.clear();
        }
    }

    bool loadFromFile(const std::string& filePath);
    bool isWalkable(int x, int y) const;
    void updateDiscovery(int playerX, int playerY);
    Tile getTile(int x, int y) const;
    bool checkWarp(int x, int y, MapWarp& outWarp) const;

    nlohmann::json saveStateToJson() const;
    void loadStateFromJson(const nlohmann::json& j);

    TileRuntimeData& getRuntimeData(int x, int y);
    const std::vector<MapTrigger>& getTriggers() const { return triggers; }

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