#pragma once
#include <SDL3/SDL.h>

struct ViewportGuard
{
    SDL_Renderer* renderer;

    ViewportGuard(SDL_Renderer* r, const SDL_Rect* rect) : renderer(r)
    {
        SDL_SetRenderViewport(renderer, rect);
    }

    ViewportGuard(SDL_Renderer* r, const SDL_FRect* frect) : renderer(r)
    {
        if (frect)
        {
            SDL_Rect rect = { (int)frect->x, (int)frect->y, (int)frect->w, (int)frect->h };
            SDL_SetRenderViewport(renderer, &rect);
        }
    }

    ~ViewportGuard()
    {
        SDL_SetRenderViewport(renderer, NULL); // Automatically resets viewport on exit!
    }
};