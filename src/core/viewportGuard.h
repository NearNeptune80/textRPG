#pragma once

#include <SDL3/SDL.h>

/**
 * RAII Guard to handle temporary viewport changes cleanly.
 */
class ViewportGuard
{
public:
    ViewportGuard(SDL_Renderer* renderer, const SDL_Rect* newViewport) : m_renderer(renderer)
    {
        SDL_GetRenderViewport(m_renderer, &m_oldViewport);
        SDL_SetRenderViewport(m_renderer, newViewport);
    }

    ViewportGuard(SDL_Renderer* renderer, const SDL_FRect& fBounds) : m_renderer(renderer)
    {
        SDL_Rect viewRect = { static_cast<int>(fBounds.x), static_cast<int>(fBounds.y), static_cast<int>(fBounds.w), static_cast<int>(fBounds.h) };
        SDL_GetRenderViewport(m_renderer, &m_oldViewport);
        SDL_SetRenderViewport(m_renderer, &viewRect);
    }

    ~ViewportGuard()
    {
        SDL_SetRenderViewport(m_renderer, &m_oldViewport);
    }

private:
    SDL_Renderer* m_renderer;
    SDL_Rect m_oldViewport;
};