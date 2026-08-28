#include "ui/uiWidget.h"
#include "ui/fontManager.h"

#include <algorithm>
#include <sstream>

namespace UIWidget
{
    void drawPanel(SDL_Renderer* renderer, const SDL_FRect& rect, SDL_Color bgColor, SDL_Color borderColor)
    {
        if (!renderer) return;

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderRect(renderer, &rect);
    }

    void drawHeader(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& title, SDL_Color headerColor, SDL_Color textColor, float scale)
    {
        if (!renderer) return;

        SDL_SetRenderDrawColor(renderer, headerColor.r, headerColor.g, headerColor.b, headerColor.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, Theme::colors.borderNormal.a);
        SDL_RenderRect(renderer, &rect);

        drawText(renderer, title, rect.x + (8.0f * scale), rect.y + ((rect.h - (10.0f * scale)) / 2.0f), textColor, scale);
    }

    void drawProgressBar(SDL_Renderer* renderer, const SDL_FRect& rect, float currentValue, float maxValue, SDL_Color fillColor, SDL_Color bgColor, const std::string& label, float scale)
    {
        if (!renderer) return;

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &rect);

        float pct = (maxValue > 0.0f) ? std::clamp(currentValue / maxValue, 0.0f, 1.0f) : 0.0f;
        SDL_FRect fillRect = { rect.x, rect.y, rect.w * pct, rect.h };

        SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        SDL_RenderFillRect(renderer, &fillRect);

        SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, Theme::colors.borderNormal.a);
        SDL_RenderRect(renderer, &rect);

        if (!label.empty())
        {
            drawText(renderer, label, rect.x + (6.0f * scale), rect.y + ((rect.h - (10.0f * scale)) / 2.0f), Theme::colors.textPrimary, scale * 0.9f);
        }
    }

    bool drawButton(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& label, bool isHovered, bool isEnabled, bool isSelected, float scale)
    {
        if (!renderer) return false;

        SDL_Color bg = isEnabled ? (isSelected ? Theme::colors.bgSlotSelected : (isHovered ? Theme::colors.borderButton : Theme::colors.bgButton)) : Theme::colors.bgButtonDisabled;
        SDL_Color border = isEnabled ? (isSelected ? Theme::colors.borderSelected : (isHovered ? Theme::colors.textGold : Theme::colors.borderButton)) : Theme::colors.borderButtonDisabled;
        SDL_Color textCol = isEnabled ? (isHovered ? Theme::colors.textGold : Theme::colors.textPrimary) : Theme::colors.textMuted;

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderRect(renderer, &rect);

        drawText(renderer, label, rect.x + (8.0f * scale), rect.y + ((rect.h - (10.0f * scale)) / 2.0f), textCol, scale);
        return isEnabled && isHovered;
    }

    void drawText(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color, float scale)
    {
        ::fontManager::getInstance().drawText(renderer, text, x, y, color, scale);
    }

    float drawTextWrapped(SDL_Renderer* renderer, const std::string& text, float x, float y, float maxWidth, SDL_Color color, float scale)
    {
        return ::fontManager::getInstance().drawTextWrapped(renderer, text, x, y, maxWidth, color, scale);
    }
}