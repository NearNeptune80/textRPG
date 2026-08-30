#pragma once

#include <SDL3/SDL.h>

class game;

namespace EntityListWidgets
{
    float renderWidgetCharactersPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
}
