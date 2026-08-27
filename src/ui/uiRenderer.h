#pragma once

#include <memory>
#include <string>
#include <vector>
#include <SDL3/SDL.h>

class game;

/**
 * Isolated View Presentation Renderer.
 * Reads game snapshot getters exclusively and draws multi-pane UI to SDL_Renderer.
 * Dispatches UI commands back to the headless engine on user interaction.
 */
class uiRenderer
{
public:
    uiRenderer();
    ~uiRenderer();

    void render(SDL_Renderer* renderer, game* gameContext);

private:
    int m_currentPage = 0;
    static constexpr int BUTTONS_PER_PAGE = 10;

    void renderTopBar(SDL_Renderer* renderer, game* gameContext);
    void renderLeftPane(SDL_Renderer* renderer, game* gameContext);
    void renderCenterPane(SDL_Renderer* renderer, game* gameContext);
    void renderRightPane(SDL_Renderer* renderer, game* gameContext);
    void renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext);

    // View sub-renderers for Center Pane
    void renderSceneView(SDL_Renderer* renderer, game* gameContext);
    void renderSexView(SDL_Renderer* renderer, game* gameContext);
    void renderCombatView(SDL_Renderer* renderer, game* gameContext);
    void renderResolutionView(SDL_Renderer* renderer, game* gameContext);
    void renderInventoryView(SDL_Renderer* renderer, game* gameContext);
    void renderExplorationView(SDL_Renderer* renderer, game* gameContext);
};