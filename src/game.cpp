#include "game.h"
#include <iostream>

game::game() : isRunning(false), window(nullptr), renderer(nullptr), map(nullptr) {}
game::~game() { delete map; }

void game::init(const char* title, int width, int height, bool fullscreen)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return;

    // ADDED: SDL_WINDOW_RESIZABLE
    SDL_WindowFlags windowFlags = (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);

    window = SDL_CreateWindow(title, width, height, windowFlags);
    renderer = SDL_CreateRenderer(window, NULL);

    // ADDED: This ensures the "internal" resolution stays 800x600 
    // even if the user drags the window to be massive.
    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

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
        if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            int newWidth = event.window.data1;
            int newHeight = event.window.data2;

            // Set the logical size to match the window so everything scales
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

    // 1. Get current Logical resolution
    int logicalW, logicalH;
    SDL_RendererLogicalPresentation mode;
    float scale;
    SDL_GetRenderLogicalPresentation(renderer, &logicalW, &logicalH, &mode);

    // 2. Layout calculations (The "Manager")
    int padding = 20;
    int mapSize = (int)(logicalH * 0.3f);

    SDL_Rect mapRect = { padding, logicalH - mapSize - padding, mapSize, mapSize };
    SDL_Rect portraitRect = { logicalW - mapSize - padding, logicalH - mapSize - padding, mapSize, mapSize };

    int textX = mapRect.x + mapRect.w + padding;
    int textW = portraitRect.x - textX - padding;
    SDL_Rect textRect = { textX, logicalH - mapSize - padding, textW, mapSize };

    // 3. Orchestrate
    renderMapPanel(mapRect);
    renderPortraitPanel(portraitRect);
    renderTextPanel(textRect);

    SDL_SetRenderViewport(renderer, NULL);
    SDL_RenderPresent(renderer);
}

// Helper function to calculate pixel rect from relative rect
SDL_Rect game::getPixelRect(RelativeRect rel)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return {
        (int)(w * rel.x),
        (int)(h * rel.y),
        (int)(w * rel.w),
        (int)(h * rel.h)
    };
}

void game::drawMapTiles(int panelW, int panelH)
{
    // Calculate tile size so the map fits perfectly in the panel
    int tileSize = panelW / gameMap::WIDTH;

    for (int y = 0; y < gameMap::HEIGHT; y++)
    {
        for (int x = 0; x < gameMap::WIDTH; x++)
        {
            Tile t = map->getTile(x, y);
            if (t.discovery == STATE_HIDDEN) continue;

            SDL_FRect r = {
                (float)(x * tileSize),
                (float)(y * tileSize),
                (float)tileSize - 1.0f,
                (float)tileSize - 1.0f
            };

            // ... (Your rendering logic remains the same)
            SDL_RenderFillRect(renderer, &r);
        }
    }
}

SDL_Rect game::getSquareRect(float sizePercentage)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    // 1. Calculate size (Square based on min dimension)
    int side = (int)(std::min(w, h) * sizePercentage);

    // 2. Define constant padding
    int padding = 20;

    // 3. Pin to Bottom-Left
    // X = padding
    // Y = WindowHeight - SideSize - Padding
    return {
        padding,
        h - side - padding,
        side,
        side
    };
}

void game::renderMapPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, NULL);

    int tileSize = rect.w / 6; // slightly smaller to create natural padding
    int offset = tileSize / 2; // centers the grid

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

                SDL_FRect r = {
                    (float)(offset + ((x + 2) * tileSize)),
                    (float)(offset + ((y + 2) * tileSize)),
                    (float)tileSize - 4.0f,
                    (float)tileSize - 4.0f
                };

                if (t.discovery == STATE_PARTIAL) SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                else if (t.type == TILE_FLOOR) SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                else SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
    // Player
    SDL_FRect p = { (float)(offset + (2 * tileSize)), (float)(offset + (2 * tileSize)), (float)tileSize - 4.0f, (float)tileSize - 4.0f };
    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderFillRect(renderer, &p);
}

void game::renderPortraitPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, NULL);

    // Draw placeholder X
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLine(renderer, 0, 0, rect.w, rect.h);
    SDL_RenderLine(renderer, rect.w, 0, 0, rect.h);
}

void game::renderTextPanel(SDL_Rect rect)
{
    SDL_SetRenderViewport(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(renderer, NULL);
}

void game::clean()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}