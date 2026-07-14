#include "gameMap.h"

gameMap::gameMap()
{
    // Initialize everything as VOID
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            grid[y][x] = { TILE_VOID, STATE_HIDDEN };
        }
    }

    // Set up a simple room
    for (int y = 1; y < 8; ++y)
    {
        for (int x = 1; x < 8; ++x)
        {
            grid[y][x] = { TILE_FLOOR, STATE_HIDDEN };
        }
    }
}

gameMap::~gameMap() {}

bool gameMap::isWalkable(int x, int y)
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return false;
    return (grid[y][x].type == TILE_FLOOR);
}

void gameMap::updateDiscovery(int playerX, int playerY)
{
    // Reveal current tile regardless of type (assumes player is on a valid tile)
    grid[playerY][playerX].discovery = STATE_REVEALED;

    // Partial reveal neighbors
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int nx = playerX + dx;
            int ny = playerY + dy;

            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT)
            {
                // ADDED CHECK: Only mark as PARTIAL if it is NOT VOID
                if (grid[ny][nx].type != TILE_VOID && grid[ny][nx].discovery == STATE_HIDDEN)
                {
                    grid[ny][nx].discovery = STATE_PARTIAL;
                }
            }
        }
    }
}