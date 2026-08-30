#pragma once

#include <SDL3/SDL.h>

class game;

namespace StatusWidgets
{
    float renderWidgetCharOverview(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetVitals(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetAttributes(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetAnatomy(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetTarget(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
}
