#pragma once

#include <SDL3/SDL.h>

class game;
class entity;

namespace PaperdollWidgets
{
    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float curY, float uiScale, entity* targetEntity = nullptr);
    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetItemInspector(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
}
