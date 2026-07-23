#ifndef GAMEMAP_H
#define GAMEMAP_H

#include <vector>
#include <string>

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
};

#endif