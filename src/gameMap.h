#ifndef GAMEMAP_H
#define GAMEMAP_H

#include <vector>

enum TileType
{
    TILE_VOID,
    TILE_FLOOR,
    TILE_WALL
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

class gameMap
{
public:
    static const int WIDTH = 10;
    static const int HEIGHT = 10;

    gameMap();
    ~gameMap();

    bool isWalkable(int x, int y);
    void updateDiscovery(int playerX, int playerY);
    Tile getTile(int x, int y) { return grid[y][x]; }

private:
    Tile grid[HEIGHT][WIDTH];
};

#endif