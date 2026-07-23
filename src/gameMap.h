#ifndef GAMEMAP_H
#define GAMEMAP_H

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>

class entity; // Forward declaration

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
    std::string sourceId; // e.g. "quest_curfew" or "status_repellent"
    int dangerDelta;      // +1 (more dangerous), -1 (safer)
    int durationTurns;    // Remaining duration (-1 for infinite)
};

struct TileRuntimeData
{
    std::shared_ptr<entity> persistentNPC = nullptr; // Persistent enemy on this tile
    std::string iconId = "";                         // Navigation icon (e.g. "icon_stairs")
    int baseDangerLevel = 0;                         // 0 = Safe, 1+ = Unsafe

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

class gameMap
{
public:
    gameMap();
    ~gameMap();

    bool loadFromFile(const std::string& filePath);
    bool isWalkable(int x, int y) const;
    void updateDiscovery(int playerX, int playerY);
    Tile getTile(int x, int y) const;
    bool checkWarp(int x, int y, MapWarp& outWarp) const;

    // --- Dynamic Tile Data Helpers ---
    TileRuntimeData& getRuntimeData(int x, int y);
    std::string getTileKey(int x, int y) const;

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
    std::unordered_map<std::string, TileRuntimeData> runtimeData;
};

#endif