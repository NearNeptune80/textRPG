#pragma once

#include <SDL3/SDL.h>

class game;

namespace CharacterCardWidget
{
    float renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
}
