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

    void renderTopBar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderLeftPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderCenterPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderRightPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);

    // View sub-renderers for Center Pane
    void renderSceneView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderSexView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderCombatView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderResolutionView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderInventoryView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderExplorationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);

    // Per-panel scroll tracking
    std::unordered_map<std::string, float> m_panelScrollY;
    std::unordered_map<std::string, float> m_panelMaxScrollY;
    void drawScrollbar(SDL_Renderer* renderer, const SDL_FRect& panelRect, float contentHeight, float currentScroll, float uiScale);

    // Modular atomic widget renderers
    float renderWidgetCharOverview(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetVitals(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetAttributes(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetAnatomy(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderWidgetTarget(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
};