#pragma once

#include <SDL3/SDL.h>

#include <vector>
#include "ui/tooltipManager.h"

class game;
struct StatusEffect;

namespace CharacterCardWidget
{
    float renderWidgetCharacterCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    void renderStatusEffectsGrid(SDL_Renderer* renderer, const std::vector<StatusEffect>& effects, float s2ContentX, float s2Y, float s2ContentW, float chipSize, float chipGap, int chipsPerRow, float uiScale, const TooltipPoint& mousePos);
}
