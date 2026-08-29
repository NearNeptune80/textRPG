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

    void drawCenteredHeaderCard(SDL_Renderer* renderer, float centerX, float curY, float cardW, float cardH, const std::string& title, SDL_Color textColor, float scale)
    {
        if (!renderer) return;

        SDL_FRect cardRect = { centerX - (cardW / 2.0f), curY, cardW, cardH };
        SDL_SetRenderDrawColor(renderer, 22, 24, 28, 240);
        SDL_RenderFillRect(renderer, &cardRect);

        SDL_SetRenderDrawColor(renderer, 50, 54, 62, 255);
        SDL_RenderRect(renderer, &cardRect);

        float textW = title.size() * (8.5f * scale);
        float textX = centerX - (textW / 2.0f);
        drawText(renderer, title, textX, curY + ((cardH - (14.0f * scale)) / 2.0f), textColor, scale * 1.1f);
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

    bool drawLTActionButton(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& label, const std::string& hotkey, bool isHovered, bool isEnabled, bool isSelected, float scale)
    {
        if (!renderer) return false;

        SDL_Color bg = isEnabled ? (isSelected ? SDL_Color{ 36, 40, 48, 255 } : (isHovered ? SDL_Color{ 52, 56, 68, 255 } : SDL_Color{ 28, 30, 36, 255 })) : SDL_Color{ 18, 20, 24, 255 };
        SDL_Color border = isEnabled ? (isSelected ? Theme::colors.borderSelected : (isHovered ? Theme::colors.textGold : SDL_Color{ 55, 60, 72, 255 })) : SDL_Color{ 32, 34, 40, 255 };

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderRect(renderer, &rect);

        // Draw subtle hotkey badge in top right corner
        if (!hotkey.empty())
        {
            float hotkeyW = hotkey.size() * (5.5f * scale);
            drawText(renderer, hotkey, rect.x + rect.w - hotkeyW - (4.0f * scale), rect.y + (3.0f * scale), isSelected ? Theme::colors.textGold : SDL_Color{ 110, 115, 125, 190 }, scale * 0.65f);
        }

        // Draw centered button label
        if (!label.empty())
        {
            SDL_Color textCol = isEnabled ? (isSelected ? Theme::colors.textGold : (isHovered ? Theme::colors.textGold : Theme::colors.textPrimary)) : Theme::colors.textMuted;

            float labelW = label.size() * (7.0f * scale);
            float labelX = rect.x + std::max(6.0f * scale, (rect.w - labelW) / 2.0f);
            float labelY = rect.y + ((rect.h - (12.0f * scale)) / 2.0f);

            if (label == "Reset")
            {
                drawText(renderer, label, labelX, labelY, SDL_Color{ 255, 120, 140, 255 }, scale * 0.85f);
            }
            else if (label.find(": OFF") != std::string::npos)
            {
                size_t colonPos = label.find(':');
                std::string prefix = label.substr(0, colonPos + 2);
                drawText(renderer, prefix, labelX, labelY, textCol, scale * 0.85f);
                float prefixW = prefix.size() * (7.0f * scale);
                drawText(renderer, "OFF", labelX + prefixW, labelY, Theme::colors.enemy, scale * 0.85f);
            }
            else if (label.find(": ON") != std::string::npos)
            {
                size_t colonPos = label.find(':');
                std::string prefix = label.substr(0, colonPos + 2);
                drawText(renderer, prefix, labelX, labelY, textCol, scale * 0.85f);
                float prefixW = prefix.size() * (7.0f * scale);
                drawText(renderer, "ON", labelX + prefixW, labelY, Theme::colors.companion, scale * 0.85f);
            }
            else
            {
                drawText(renderer, label, labelX, labelY, textCol, scale * 0.85f);
            }
        }

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