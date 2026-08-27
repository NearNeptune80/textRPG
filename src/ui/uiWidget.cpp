#include "ui/uiWidget.h"

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

    void drawHeader(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& title, SDL_Color headerColor, SDL_Color textColor)
    {
        if (!renderer) return;

        SDL_SetRenderDrawColor(renderer, headerColor.r, headerColor.g, headerColor.b, headerColor.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, Theme::colors.borderNormal.a);
        SDL_RenderRect(renderer, &rect);

        drawText(renderer, title, rect.x + 8.0f, rect.y + 6.0f, textColor, 1.0f);
    }

    void drawProgressBar(SDL_Renderer* renderer, const SDL_FRect& rect, float currentValue, float maxValue, SDL_Color fillColor, SDL_Color bgColor, const std::string& label)
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
            drawText(renderer, label, rect.x + 6.0f, rect.y + 3.0f, Theme::colors.textPrimary, 1.0f);
        }
    }

    bool drawButton(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& label, bool isHovered, bool isEnabled, bool isSelected)
    {
        if (!renderer) return false;

        SDL_Color bg = isEnabled ? (isSelected ? Theme::colors.bgSlotSelected : (isHovered ? Theme::colors.borderButton : Theme::colors.bgButton)) : Theme::colors.bgButtonDisabled;
        SDL_Color border = isEnabled ? (isSelected ? Theme::colors.borderSelected : (isHovered ? Theme::colors.textGold : Theme::colors.borderButton)) : Theme::colors.borderButtonDisabled;
        SDL_Color textCol = isEnabled ? (isHovered ? Theme::colors.textGold : Theme::colors.textPrimary) : Theme::colors.textMuted;

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderRect(renderer, &rect);

        drawText(renderer, label, rect.x + 8.0f, rect.y + ((rect.h - 10.0f) / 2.0f), textCol, 1.0f);
        return isEnabled && isHovered;
    }

    void drawText(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color, float scale)
    {
        if (!renderer || text.empty()) return;

        float charWidth = 8.0f * scale;
        float charHeight = 10.0f * scale;
        float curX = x;
        float curY = y;

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        for (char c : text)
        {
            if (c == '\n')
            {
                curY += charHeight + 2.0f;
                curX = x;
                continue;
            }

            // High-contrast clean block rendering for primary text characters
            SDL_FRect charRect = { curX, curY, charWidth - 1.0f, charHeight - 2.0f };
            if (c != ' ')
            {
                SDL_RenderFillRect(renderer, &charRect);
            }

            curX += charWidth;
        }
    }

    void drawTextWrapped(SDL_Renderer* renderer, const std::string& text, float x, float y, float maxWidth, SDL_Color color, float scale)
    {
        if (!renderer || text.empty()) return;

        std::istringstream stream(text);
        std::string line;
        float curY = y;
        float charWidth = 8.0f * scale;
        float lineHeight = 12.0f * scale;

        while (std::getline(stream, line))
        {
            std::istringstream lineStream(line);
            std::string word;
            std::string currentLine = "";

            while (lineStream >> word)
            {
                std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
                if ((testLine.length() * charWidth) > maxWidth && !currentLine.empty())
                {
                    drawText(renderer, currentLine, x, curY, color, scale);
                    curY += lineHeight;
                    currentLine = word;
                }
                else
                {
                    currentLine = testLine;
                }
            }

            if (!currentLine.empty())
            {
                drawText(renderer, currentLine, x, curY, color, scale);
                curY += lineHeight;
            }
        }
    }
}