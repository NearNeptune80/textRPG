#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <string>

constexpr float GLOBAL_CORNER_RADIUS = 8.0f;

inline SDL_Color getColorFromName(const std::string& colorName)
{
    std::string lower = colorName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "fair" || lower == "flesh") return { 240, 190, 170, 255 };
    if (lower == "brown") return { 160, 90, 44, 255 };
    if (lower == "blue") return { 80, 160, 255, 255 };
    if (lower == "pink") return { 255, 130, 180, 255 };
    if (lower == "scarlet" || lower == "red") return { 230, 40, 50, 255 };
    if (lower == "yellow") return { 255, 215, 0, 255 };
    if (lower == "purple") return { 180, 100, 255, 255 };
    if (lower == "green") return { 60, 200, 80, 255 };
    if (lower == "black" || lower == "dark") return { 100, 100, 110, 255 };
    if (lower == "white") return { 240, 240, 240, 255 };

    return { 220, 220, 230, 255 };
}

inline void renderFillRoundedRect(SDL_Renderer* renderer, SDL_FRect rect, float radius, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    radius = std::clamp(radius, 0.0f, std::min(rect.w * 0.5f, rect.h * 0.5f));

    if (radius <= 1.0f)
    {
        SDL_RenderFillRect(renderer, &rect);
        return;
    }

    SDL_FRect innerH = { rect.x + radius, rect.y, rect.w - (2.0f * radius), rect.h };
    SDL_FRect innerV = { rect.x, rect.y + radius, rect.w, rect.h - (2.0f * radius) };
    SDL_RenderFillRect(renderer, &innerH);
    SDL_RenderFillRect(renderer, &innerV);

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

    fillCorner(rect.x + radius, rect.y + radius, rect.x, rect.x + radius, rect.y, rect.y + radius);
    fillCorner(rect.x + rect.w - radius, rect.y + radius, rect.x + rect.w - radius, rect.x + rect.w, rect.y, rect.y + radius);
    fillCorner(rect.x + radius, rect.y + rect.h - radius, rect.x, rect.x + radius, rect.y + rect.h - radius, rect.y + rect.h);
    fillCorner(rect.x + rect.w - radius, rect.y + rect.h - radius, rect.x + rect.w - radius, rect.x + rect.w, rect.y + rect.h - radius, rect.y + rect.h);
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

    SDL_RenderLine(renderer, rect.x + radius, rect.y, x2 - radius, rect.y);
    SDL_RenderLine(renderer, rect.x + radius, y2, x2 - radius, y2);
    SDL_RenderLine(renderer, rect.x, rect.y + radius, rect.x, y2 - radius);
    SDL_RenderLine(renderer, x2, rect.y + radius, x2, y2 - radius);

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