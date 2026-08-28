#pragma once

#include <memory>
#include <string>
#include <vector>
#include <SDL3/SDL.h>

#include "ui/layoutEngine.h"

class game;

/**
 * Isolated View Presentation Renderer.
 * Reads game snapshot getters exclusively and draws responsive multi-pane UI to SDL_Renderer.
 * Dispatches UI commands back to the headless engine on user interaction.
 */
class uiRenderer
{
public:
    uiRenderer();
    ~uiRenderer();

    void render(SDL_Renderer* renderer, game* gameContext);
    layoutEngine& getLayoutEngine() { return m_layoutEngine; }

private:
    layoutEngine m_layoutEngine;
    int m_currentPage = 0;
    static constexpr int BUTTONS_PER_PAGE = 10;

    void renderTopBar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderLeftPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderCenterPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderRightPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);

    // View sub-renderers for Center Pane
    void renderSceneView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderSexView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderCombatView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderResolutionView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderInventoryView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
    void renderExplorationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect);
};