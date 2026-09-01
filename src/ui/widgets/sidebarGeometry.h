#pragma once
#include <SDL3/SDL.h>
#include <cmath>

namespace SidebarGeometry
{
    inline float getPadX(const SDL_FRect& panelRect, float uiScale)
    {
        return panelRect.x + (5.0f * uiScale);
    }

    inline float getAvailableW(const SDL_FRect& panelRect, float uiScale)
    {
        return panelRect.w - (10.0f * uiScale);
    }

    inline float getBoxSize(const SDL_FRect& panelRect, float uiScale)
    {
        return std::floor(getAvailableW(panelRect, uiScale) - (8.0f * uiScale));
    }

    inline float getToolbarH(float uiScale)
    {
        return 20.0f * uiScale;
    }

    inline float getTimeBarH(float uiScale)
    {
        return 46.0f * uiScale;
    }

    inline float getGapBetweenCards(float uiScale)
    {
        return 8.0f * uiScale;
    }

    inline float getSquareCardH(const SDL_FRect& panelRect, float uiScale)
    {
        float boxSize = getBoxSize(panelRect, uiScale);
        return (18.0f * uiScale) + boxSize + (5.0f * uiScale) + getToolbarH(uiScale) + (5.0f * uiScale);
    }

    inline float getTotalNavH(const SDL_FRect& panelRect, float uiScale)
    {
        return getTimeBarH(uiScale) + getGapBetweenCards(uiScale) + getSquareCardH(panelRect, uiScale);
    }

    inline float getBottomPinnedY(const SDL_FRect& panelRect, float uiScale)
    {
        return panelRect.y + panelRect.h - getTotalNavH(panelRect, uiScale) - (6.0f * uiScale);
    }

    inline float getGridStartX(const SDL_FRect& panelRect, float uiScale)
    {
        float padX = getPadX(panelRect, uiScale);
        float availableW = getAvailableW(panelRect, uiScale);
        float boxSize = getBoxSize(panelRect, uiScale);
        return padX + ((availableW - boxSize) / 2.0f);
    }
}
