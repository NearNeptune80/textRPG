#pragma once

#include <SDL3/SDL.h>

class game;

namespace PaperdollWidgets
{
    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetItemInspector(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
}
