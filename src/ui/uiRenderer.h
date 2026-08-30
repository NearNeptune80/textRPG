#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>

#include "ui/layoutEngine.h"

class game;

/**
 * Isolated View Presentation Renderer.
 * Interprets layout JSON (data/layouts/*.json) and theme JSON (data/themes/*.json),
 * dispatches modular view and widget components, and sends commands back to the engine.
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
    static constexpr int BUTTONS_PER_PAGE = 15;

    // Per-panel scroll tracking
    std::unordered_map<std::string, float> m_panelScrollY;
    std::unordered_map<std::string, float> m_panelMaxScrollY;

    void renderTopBar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    void renderBottomActionGrid(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float uiScale);
    float renderCenterPane(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    void drawScrollbar(SDL_Renderer* renderer, const SDL_FRect& panelRect, float contentHeight, float currentScroll, float uiScale);
};