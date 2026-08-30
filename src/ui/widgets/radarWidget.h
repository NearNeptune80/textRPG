#pragma once

#include <SDL3/SDL.h>

class game;

namespace RadarWidgets
{
    float renderWidgetRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderWidgetTimeBar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetOptionsToolbar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetDpadRadar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
}
