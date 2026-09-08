#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <vector>

class game;
class entity;

namespace EntityListWidgets
{
    bool isInteractingWithNPC(game* gameContext);
    std::vector<std::shared_ptr<entity>> getInteractingNPCs(game* gameContext);

    float renderWidgetNPCCard(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetCharactersPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetItemsPresent(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
    float renderWidgetEventLog(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale);
}
