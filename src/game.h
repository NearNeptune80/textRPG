#pragma once
#include <SDL3/SDL.h>
#include "gameMap.h"
#include "entity.h"
#include "itemDatabase.h"
#include <iostream>

enum class GameState
{
    MAIN_MENU,
    EXPLORATION,
    INVENTORY
};

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
    entity* player;

    int gridX, gridY;
    GameState currentState;

    // --- Layout Orchestrators ---
    void renderDashboardLayout();
    void renderMainMenuLayout();

    // --- UI Widgets ---
    void renderTitleBar(SDL_Rect t1, SDL_Rect t2, SDL_Rect t3);
    void renderPCPanel(SDL_Rect rect);
    void renderCompanionPanel(SDL_Rect rect);
    void renderMapPanel(SDL_Rect rect, int padding);
    void renderTextPanel(SDL_Rect rect);
    void renderButtons(SDL_Rect rect);
    void renderRightColumn(SDL_Rect top, SDL_Rect mid, SDL_Rect bot);

    // New Widgets for Inventory State
    void renderEquipmentPanel(SDL_Rect rect, int padding);
    void renderInventoryPanel(SDL_Rect rect);
};