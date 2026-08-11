#pragma once

#include <SDL3/SDL.h>

class game;

/**
 * Isolated Presentation View Renderer.
 * Reads headless game snapshots and draws the visual state to an SDL_Renderer context.
 */
class uiRenderer
{
public:
    uiRenderer() = default;
    ~uiRenderer() = default;

    void render(SDL_Renderer* renderer, game* gameContext);
};