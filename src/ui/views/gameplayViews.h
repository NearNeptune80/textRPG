#pragma once

#include <SDL3/SDL.h>

class game;

namespace GameplayViews
{
    float renderSceneView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderSexView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderPhoneAppView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderCombatView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderResolutionView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderInventoryView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderExplorationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderShopView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderTransformationView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
    float renderEnchantingView(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale);
}
