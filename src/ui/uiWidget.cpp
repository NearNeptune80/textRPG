#include "ui/uiWidget.h"
#include "ui/fontManager.h"

#include <algorithm>
#include <sstream>
#include <string_view>

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

    void drawHeader(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view title, SDL_Color headerColor, SDL_Color textColor, float scale)
    {
        if (!renderer) return;

        SDL_SetRenderDrawColor(renderer, headerColor.r, headerColor.g, headerColor.b, headerColor.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, Theme::colors.borderNormal.a);
        SDL_RenderRect(renderer, &rect);

        drawText(renderer, title, rect.x + (8.0f * scale), rect.y + ((rect.h - (10.0f * scale)) / 2.0f), textColor, scale);
    }

    void drawCenteredHeaderCard(SDL_Renderer* renderer, float centerX, float curY, float cardW, float cardH, std::string_view title, SDL_Color textColor, float scale)
    {
        if (!renderer) return;

        SDL_FRect cardRect = { centerX - (cardW / 2.0f), curY, cardW, cardH };
        SDL_SetRenderDrawColor(renderer, Theme::colors.bgInput.r, Theme::colors.bgInput.g, Theme::colors.bgInput.b, 240);
        SDL_RenderFillRect(renderer, &cardRect);

        SDL_SetRenderDrawColor(renderer, Theme::colors.borderMuted.r, Theme::colors.borderMuted.g, Theme::colors.borderMuted.b, Theme::colors.borderMuted.a);
        SDL_RenderRect(renderer, &cardRect);

        float textW = title.size() * (8.5f * scale);
        float textX = centerX - (textW / 2.0f);
        drawText(renderer, title, textX, curY + ((cardH - (14.0f * scale)) / 2.0f), textColor, scale * 1.1f);
    }

    void drawProgressBar(SDL_Renderer* renderer, const SDL_FRect& rect, float currentValue, float maxValue, SDL_Color fillColor, SDL_Color bgColor, std::string_view label, float scale)
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

    bool drawButton(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view label, bool isHovered, bool isEnabled, bool isSelected, float scale)
    {
        if (!renderer) return false;

        SDL_Color bg = isEnabled ? (isSelected ? Theme::colors.bgSlotSelected : (isHovered ? Theme::colors.borderButton : Theme::colors.bgButton)) : Theme::colors.bgButtonDisabled;
        SDL_Color border = isEnabled ? (isSelected ? Theme::colors.borderSelected : (isHovered ? Theme::colors.textGold : Theme::colors.borderButton)) : Theme::colors.borderButtonDisabled;
        SDL_Color textCol = isEnabled ? (isHovered ? Theme::colors.textGold : (isSelected ? Theme::colors.textGold : Theme::colors.textPrimary)) : Theme::colors.textMuted;

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderRect(renderer, &rect);

        if (!label.empty())
        {
            float labelW = getTextWidth(label, scale);
            float labelH = 13.0f * scale;
            float textX = rect.x + std::max(2.0f * scale, (rect.w - labelW) / 2.0f);
            float textY = rect.y + std::max(1.0f * scale, (rect.h - labelH) / 2.0f);
            drawText(renderer, label, textX, textY, textCol, scale);
        }
        return isEnabled && isHovered;
    }

    bool drawColoredButton(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view label, SDL_Color bgColor, SDL_Color textColor, bool isSelected, float scale)
    {
        if (!renderer) return false;

        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_Color border = isSelected ? Theme::colors.textGold : Theme::colors.borderButton;
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderRect(renderer, &rect);

        if (!label.empty())
        {
            float labelW = getTextWidth(label, scale);
            float labelH = 13.0f * scale;
            float textX = rect.x + std::max(2.0f * scale, (rect.w - labelW) / 2.0f);
            float textY = rect.y + std::max(1.0f * scale, (rect.h - labelH) / 2.0f);
            drawText(renderer, label, textX, textY, textColor, scale);
        }
        return true;
    }

    bool drawColorSwatch(SDL_Renderer* renderer, const SDL_FRect& rect, SDL_Color color, bool isSelected, bool isHovered, float scale)
    {
        if (!renderer) return false;

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);

        if (isSelected)
        {
            SDL_SetRenderDrawColor(renderer, Theme::colors.textGold.r, Theme::colors.textGold.g, Theme::colors.textGold.b, 255);
            SDL_RenderRect(renderer, &rect);
            SDL_FRect outer = { rect.x - (2.0f * scale), rect.y - (2.0f * scale), rect.w + (4.0f * scale), rect.h + (4.0f * scale) };
            SDL_RenderRect(renderer, &outer);
        }
        else if (isHovered)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
            SDL_RenderRect(renderer, &rect);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, Theme::colors.slotEmptyBorder.r, Theme::colors.slotEmptyBorder.g, Theme::colors.slotEmptyBorder.b, Theme::colors.slotEmptyBorder.a);
            SDL_RenderRect(renderer, &rect);
        }

        return isHovered;
    }

    bool drawLTActionButton(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view label, std::string_view hotkey, bool isHovered, bool isEnabled, bool isSelected, float scale)
    {
        if (!renderer) return false;

        SDL_Color bg = isEnabled ? (isSelected ? Theme::colors.bgSlotSelected : (isHovered ? Theme::colors.bgButtonHover : Theme::colors.bgSlot)) : Theme::colors.bgButtonDisabled;
        SDL_Color border = isEnabled ? (isSelected ? Theme::colors.borderSelected : (isHovered ? Theme::colors.borderButtonHover : Theme::colors.borderButton)) : Theme::colors.borderButtonDisabled;

        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderRect(renderer, &rect);

        // Draw subtle hotkey badge in top right corner
        if (!hotkey.empty())
        {
            float hkScale = scale * 0.65f;
            float hotkeyW = getTextWidth(hotkey, hkScale);
            SDL_Color hkCol = isEnabled ? (isSelected ? Theme::colors.textGold : Theme::colors.textSecondary) : Theme::colors.textDisabled;
            drawText(renderer, hotkey, rect.x + rect.w - hotkeyW - (6.0f * scale), rect.y + (3.0f * scale), hkCol, hkScale);
        }

        // Draw centered button label
        if (!label.empty())
        {
            float labelScale = scale * 0.85f;
            float labelW = getTextWidth(label, labelScale);
            float labelX = rect.x + std::max(6.0f * scale, (rect.w - labelW) / 2.0f);
            float labelY = rect.y + ((rect.h - (12.0f * scale)) / 2.0f);

            if (!isEnabled)
            {
                // Unconditionally greyed out when requirements are unmet
                drawText(renderer, label, labelX, labelY, Theme::colors.textMuted, labelScale);
            }
            else
            {
                SDL_Color textCol = isSelected ? Theme::colors.textGold : (isHovered ? Theme::colors.textGold : Theme::colors.textPrimary);

                if (label == "Reset")
                {
                    drawText(renderer, label, labelX, labelY, Theme::colors.enemy, labelScale);
                }
                else if (label == "Quests" || label == "Encyclopedia")
                {
                    drawText(renderer, label, labelX, labelY, Theme::colors.textGold, labelScale);
                }
                else if (label == "Masturbate")
                {
                    drawText(renderer, label, labelX, labelY, Theme::colors.lust, labelScale);
                }
                else if (label.find(": OFF") != std::string_view::npos)
                {
                    size_t colonPos = label.find(':');
                    std::string_view prefix = label.substr(0, colonPos + 2);
                    drawText(renderer, prefix, labelX, labelY, textCol, labelScale);
                    float prefixW = getTextWidth(prefix, labelScale);
                    drawText(renderer, "OFF", labelX + prefixW, labelY, Theme::colors.enemy, labelScale);
                }
                else if (label.find(": ON") != std::string_view::npos)
                {
                    size_t colonPos = label.find(':');
                    std::string_view prefix = label.substr(0, colonPos + 2);
                    drawText(renderer, prefix, labelX, labelY, textCol, labelScale);
                    float prefixW = getTextWidth(prefix, labelScale);
                    drawText(renderer, "ON", labelX + prefixW, labelY, Theme::colors.companion, labelScale);
                }
                else
                {
                    drawText(renderer, label, labelX, labelY, textCol, labelScale);
                }
            }
        }

        return isEnabled && isHovered;
    }

    void drawText(SDL_Renderer* renderer, std::string_view text, float x, float y, SDL_Color color, float scale)
    {
        ::fontManager::getInstance().drawText(renderer, std::string(text), x, y, color, scale);
    }

    float drawTextWrapped(SDL_Renderer* renderer, std::string_view text, float x, float y, float maxWidth, SDL_Color color, float scale)
    {
        return ::fontManager::getInstance().drawTextWrapped(renderer, std::string(text), x, y, maxWidth, color, scale);
    }

    float getTextWrappedHeight(std::string_view text, float maxWidth, float scale)
    {
        return ::fontManager::getInstance().getTextWrappedHeight(std::string(text), maxWidth, scale);
    }

    float getTextWidth(std::string_view text, float scale)
    {
        return ::fontManager::getInstance().getTextWidth(std::string(text), scale);
    }

    float getLineHeight(float scale)
    {
        return ::fontManager::getInstance().getLineHeight(scale);
    }
}