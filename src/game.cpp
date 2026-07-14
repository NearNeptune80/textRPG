#include "game.h"
#include <iostream>
#include <algorithm>

game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr), gridX(1), gridY(1) {}

game::~game()
{
    delete map;
}

void game::init(const char* title, int width, int height, bool fullscreen)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return;

    window = SDL_CreateWindow(title, width, height, (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
    renderer = SDL_CreateRenderer(window, NULL);

    // Set initial logical presentation
    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_STRETCH);

    map = new gameMap();
    map->updateDiscovery(gridX, gridY);
    isRunning = true;
}

void game::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT) isRunning = false;

        // Dynamic scaling: Update logical presentation when window is resized
        if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            int newWidth = event.window.data1;
            int newHeight = event.window.data2;
            SDL_SetRenderLogicalPresentation(renderer, newWidth, newHeight, SDL_LOGICAL_PRESENTATION_STRETCH);
        }

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            int nextX = gridX, nextY = gridY;
            switch (event.key.key)
            {
                case SDLK_UP:    nextY--; break;
                case SDLK_DOWN:  nextY++; break;
                case SDLK_LEFT:  nextX--; break;
                case SDLK_RIGHT: nextX++; break;
            }
            if (map->isWalkable(nextX, nextY))
            {
                gridX = nextX;
                gridY = nextY;
                map->updateDiscovery(gridX, gridY);
            }
        }
    }
}

void game::update() {}

void game::render()
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    int w, h;
    SDL_RendererLogicalPresentation mode;
    float scale;
    SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode);

    // --- 1. SINGLE SOURCE OF TRUTH ---
    int padding = 12;
    int topBarH = (int)(h * 0.08f);
    int mapSize = (int)(h * 0.30f);

    // --- 2. VERTICAL BOUNDARIES ---
    // The top row
    int topRowY = padding;

    // This adds the missing padding between the top bar and the main columns!
    int colStartY = topRowY + topBarH + padding;

    // The absolute lowest point any box can reach (ensures flush bottoms)
    int colEndY = h - padding;

    // --- 3. HORIZONTAL BOUNDARIES ---
    int leftColW = mapSize;
    int rightColW = mapSize;
    int centerColW = w - (leftColW + rightColW + (4 * padding));

    int leftX = padding;
    int centerX = leftX + leftColW + padding;
    int rightX = centerX + centerColW + padding;

    // --- 4. RECT DEFINITIONS ---
    // Top Bar
    int titleW = (w - (4 * padding)) / 3;
    renderTitleBar(
        { padding, topRowY, titleW, topBarH },
        { padding + titleW + padding, topRowY, titleW, topBarH },
        { padding + (titleW + padding) * 2, topRowY, titleW, topBarH }
    );

    // -- LEFT COLUMN --
    // Map is anchored to the absolute bottom
    renderMapPanel({ leftX, colEndY - mapSize, mapSize, mapSize }, padding);

    // PC/Companion fill the space above the map
    int leftAvailableH = (colEndY - mapSize - padding) - colStartY;
    int leftStackH = (leftAvailableH - padding) / 2;

    renderPCPanel({ leftX, colStartY, leftColW, leftStackH });

    // Calculate exact remaining height for Companion to avoid 1px rounding gaps
    int compH = leftAvailableH - leftStackH - padding;
    renderCompanionPanel({ leftX, colStartY + leftStackH + padding, leftColW, compH });

    // -- CENTER COLUMN --
    // Buttons anchored to the absolute bottom
    int btnH = (int)(h * 0.15f);
    renderButtons({ centerX, colEndY - btnH, centerColW, btnH });

    // Text box stretches to fill everything between the top boundary and the buttons
    int textH = (colEndY - btnH - padding) - colStartY;
    renderTextPanel({ centerX, colStartY, centerColW, textH });

    // -- RIGHT COLUMN --
    int rightAvailableH = colEndY - colStartY;
    int rightStackH = (rightAvailableH - (2 * padding)) / 3;

    renderRightColumn(
        { rightX, colStartY, rightColW, rightStackH },
        { rightX, colStartY + rightStackH + padding, rightColW, rightStackH },
        // Log box takes the exact remaining space to sit perfectly flush at the bottom
        { rightX, colStartY + (rightStackH + padding) * 2, rightColW, rightAvailableH - (rightStackH * 2 + padding * 2) }
    );

    SDL_SetRenderViewport(renderer, NULL);
    SDL_RenderPresent(renderer);
}

// --- UI MODULAR FUNCTIONS ---

void game::renderMapPanel(SDL_Rect rect, int padding)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, NULL);

    // 1. Define internal spacing explicitly
    int tileGap = 2; // The exact pixel gap between tiles

    // 2. Calculate the exact size of a drawn tile
    // Take the full width, subtract the padding on both sides, 
    // and subtract the 4 gaps that exist between the 5 tiles.
    int availableForTiles = rect.w - (2 * padding) - (4 * tileGap);
    int drawnTileSize = availableForTiles / 5;

    // 3. Calculate the exact total width/height of the tile cluster
    int totalGridSize = (drawnTileSize * 5) + (tileGap * 4);

    // 4. Center the entire cluster within the viewport
    int offsetX = (rect.w - totalGridSize) / 2;
    int offsetY = (rect.h - totalGridSize) / 2;

    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            int mapX = gridX + x;
            int mapY = gridY + y;

            if (mapX >= 0 && mapX < gameMap::WIDTH && mapY >= 0 && mapY < gameMap::HEIGHT)
            {
                Tile t = map->getTile(mapX, mapY);
                if (t.discovery == STATE_HIDDEN) continue;

                // Map -2 to 2 into a 0 to 4 render index
                int renderX = x + 2;
                int renderY = y + 2;

                // Draw exactly at the offset, plus the number of tiles/gaps before this one
                SDL_FRect r = {
                    (float)(offsetX + (renderX * (drawnTileSize + tileGap))),
                    (float)(offsetY + (renderY * (drawnTileSize + tileGap))),
                    (float)drawnTileSize,
                    (float)drawnTileSize
                };

                // Color Logic
                if (t.discovery == STATE_PARTIAL) SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                else if (t.type == TILE_FLOOR) SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                else SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);

                SDL_RenderFillRect(renderer, &r);
            }
        }
    }

    // Player (Always at render index 2,2)
    SDL_FRect p = {
        (float)(offsetX + (2 * (drawnTileSize + tileGap))),
        (float)(offsetY + (2 * (drawnTileSize + tileGap))),
        (float)drawnTileSize,
        (float)drawnTileSize
    };
    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderFillRect(renderer, &p);
}

void game::renderTitleBar(SDL_Rect t1, SDL_Rect t2, SDL_Rect t3)
{
    SDL_Rect boxes[3] = { t1, t2, t3 };
    for (int i = 0; i < 3; i++)
    {
        SDL_SetRenderViewport(renderer, &boxes[i]);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, NULL);
    }
}

void game::renderPCPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, NULL);
}

void game::renderCompanionPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, NULL);
}

void game::renderTextPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, NULL);
}

void game::renderButtons(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, NULL);
}

void game::renderRightColumn(SDL_Rect charRect, SDL_Rect itemRect, SDL_Rect logRect)
{
    SDL_Rect boxes[3] = { charRect, itemRect, logRect };
    for (int i = 0; i < 3; i++)
    {
        SDL_SetRenderViewport(renderer, &boxes[i]);
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, NULL);
    }
}

void game::clean()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}