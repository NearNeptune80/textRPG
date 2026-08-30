#pragma once

#include <SDL3/SDL.h>

class game;

namespace MainMenuView
{
    float render(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
}
