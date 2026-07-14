#ifndef GAME_H
#define GAME_H

#include <SDL3/SDL.h>
#include "gameMap.h"

class game
{
public:
    game();
    ~game();

    void init(const char* title, int width, int height, bool fullscreen);
    void handleEvents();
    void update();
    void render();
    
    void clean();

    bool running() { return isRunning; }

private:
    bool isRunning;
    SDL_Window* window;
    SDL_Renderer* renderer;

    gameMap* map;
    int gridX = 1;
    int gridY = 1;
    const int TILE_SIZE = 40;

    struct RelativeRect
    {
        float x, y, w, h;
    };

    // Layout configuration (The "Dashboard" setup)
    const RelativeRect mapPanel = { 0.05f, 0.10f, 0.30f, 0.30f }; // Left side
    const RelativeRect textPanel = { 0.35f, 0.10f, 0.60f, 0.35f }; // Center
    SDL_Rect getPixelRect(RelativeRect rel);
    void drawMapTiles(int panelW, int panelH);
    SDL_Rect getSquareRect(float sizePercentage);

    void renderMapPanel(SDL_Rect rect);
    void renderPortraitPanel(SDL_Rect rect);
    void renderTextPanel(SDL_Rect rect);
};

#endif