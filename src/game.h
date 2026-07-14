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

    void renderMapPanel(SDL_Rect rect, int padding);
    void renderTitleBar(SDL_Rect t1, SDL_Rect t2, SDL_Rect t3);
    void renderPCPanel(SDL_Rect rect);
    void renderCompanionPanel(SDL_Rect rect);
    void renderTextPanel(SDL_Rect rect);
    void renderButtons(SDL_Rect rect);
    void renderRightColumn(SDL_Rect charRect, SDL_Rect itemRect, SDL_Rect logRect);
};

#endif