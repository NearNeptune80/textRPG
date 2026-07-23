#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

// Global UI Radius Constant for perfect consistency across every panel
constexpr float GLOBAL_CORNER_RADIUS = 8.0f;

inline void renderFillRoundedRect(SDL_Renderer* renderer, SDL_FRect rect, float radius, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    radius = std::clamp(radius, 0.0f, std::min(rect.w * 0.5f, rect.h * 0.5f));

    if (radius <= 1.0f)
    {
        SDL_RenderFillRect(renderer, &rect);
        return;
    }

    // 1. Fill Inner Cross (Horizontal + Vertical)
    SDL_FRect innerH = { rect.x + radius, rect.y, rect.w - (2.0f * radius), rect.h };
    SDL_FRect innerV = { rect.x, rect.y + radius, rect.w, rect.h - (2.0f * radius) };
    SDL_RenderFillRect(renderer, &innerH);
    SDL_RenderFillRect(renderer, &innerV);

    // 2. Fill Corners Cleanly without edge overshoot
    float r2 = radius * radius;
    auto fillCorner = [&](float cx, float cy, float startX, float endX, float startY, float endY)
        {
            for (float y = startY; y < endY; y += 1.0f)
            {
                float dy = (y + 0.5f) - cy;
                for (float x = startX; x < endX; x += 1.0f)
                {
                    float dx = (x + 0.5f) - cx;
                    if ((dx * dx + dy * dy) <= r2)
                    {
                        SDL_RenderPoint(renderer, x, y);
                    }
                }
            }
        };

    fillCorner(rect.x + radius, rect.y + radius, rect.x, rect.x + radius, rect.y, rect.y + radius); // Top-Left
    fillCorner(rect.x + rect.w - radius, rect.y + radius, rect.x + rect.w - radius, rect.x + rect.w, rect.y, rect.y + radius); // Top-Right
    fillCorner(rect.x + radius, rect.y + rect.h - radius, rect.x, rect.x + radius, rect.y + rect.h - radius, rect.y + rect.h); // Bottom-Left
    fillCorner(rect.x + rect.w - radius, rect.y + rect.h - radius, rect.x + rect.w - radius, rect.x + rect.w, rect.y + rect.h - radius, rect.y + rect.h); // Bottom-Right
}

inline void renderDrawRoundedRect(SDL_Renderer* renderer, SDL_FRect rect, float radius, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    radius = std::clamp(radius, 0.0f, std::min(rect.w * 0.5f, rect.h * 0.5f));

    if (radius <= 1.0f)
    {
        SDL_RenderRect(renderer, &rect);
        return;
    }

    float x2 = rect.x + rect.w - 1.0f;
    float y2 = rect.y + rect.h - 1.0f;

    // 1. Straight Outer Border Edges
    SDL_RenderLine(renderer, rect.x + radius, rect.y, x2 - radius, rect.y);
    SDL_RenderLine(renderer, rect.x + radius, y2, x2 - radius, y2);
    SDL_RenderLine(renderer, rect.x, rect.y + radius, rect.x, y2 - radius);
    SDL_RenderLine(renderer, x2, rect.y + radius, x2, y2 - radius);

    // 2. Corner Arcs
    auto drawArc = [&](float cx, float cy, float quadX, float quadY)
        {
            float prevX = cx + quadX * radius;
            float prevY = cy;

            for (int i = 1; i <= 12; ++i)
            {
                float rad = (i * (90.0f / 12.0f)) * (3.14159265f / 180.0f);
                float currX = cx + quadX * radius * std::cos(rad);
                float currY = cy + quadY * radius * std::sin(rad);

                SDL_RenderLine(renderer, prevX, prevY, currX, currY);
                prevX = currX;
                prevY = currY;
            }
        };

    drawArc(rect.x + radius, rect.y + radius, -1.0f, -1.0f);
    drawArc(x2 - radius, rect.y + radius, 1.0f, -1.0f);
    drawArc(rect.x + radius, y2 - radius, -1.0f, 1.0f);
    drawArc(x2 - radius, y2 - radius, 1.0f, 1.0f);
}