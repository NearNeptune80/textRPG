#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <SDL3/SDL.h>
#include "ui/theme.h"

namespace UIWidget
{
    void drawPanel(SDL_Renderer* renderer, const SDL_FRect& rect, SDL_Color bgColor = Theme::colors.bgPanel, SDL_Color borderColor = Theme::colors.borderNormal);
    void drawHeader(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view title, SDL_Color headerColor = Theme::colors.bgHeader, SDL_Color textColor = Theme::colors.textGold, float scale = 1.0f);
    void drawCenteredHeaderCard(SDL_Renderer* renderer, float centerX, float curY, float cardW, float cardH, std::string_view title, SDL_Color textColor = Theme::colors.textPrimary, float scale = 1.0f);
    void drawProgressBar(SDL_Renderer* renderer, const SDL_FRect& rect, float currentValue, float maxValue, SDL_Color fillColor, SDL_Color bgColor = Theme::colors.bgDark, std::string_view label = "", float scale = 1.0f);
    bool drawButton(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view label, bool isHovered, bool isEnabled = true, bool isSelected = false, float scale = 1.0f);
    bool drawColoredButton(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view label, SDL_Color bgColor, SDL_Color textColor, bool isSelected = false, float scale = 1.0f);
    bool drawLTActionButton(SDL_Renderer* renderer, const SDL_FRect& rect, std::string_view label, std::string_view hotkey, bool isHovered, bool isEnabled = true, bool isSelected = false, float scale = 1.0f);

    void drawText(SDL_Renderer* renderer, std::string_view text, float x, float y, SDL_Color color = Theme::colors.textPrimary, float scale = 1.0f);
    float drawTextWrapped(SDL_Renderer* renderer, std::string_view text, float x, float y, float maxWidth, SDL_Color color = Theme::colors.textPrimary, float scale = 1.0f);

    float getTextWidth(std::string_view text, float scale = 1.0f);
    float getLineHeight(float scale = 1.0f);
}