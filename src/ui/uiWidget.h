#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include "ui/theme.h"

namespace UIWidget
{
    void drawPanel(SDL_Renderer* renderer, const SDL_FRect& rect, SDL_Color bgColor = Theme::colors.bgPanel, SDL_Color borderColor = Theme::colors.borderNormal);
    void drawHeader(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& title, SDL_Color headerColor = Theme::colors.bgHeader, SDL_Color textColor = Theme::colors.textGold, float scale = 1.0f);
    void drawCenteredHeaderCard(SDL_Renderer* renderer, float centerX, float curY, float cardW, float cardH, const std::string& title, SDL_Color textColor = Theme::colors.textPrimary, float scale = 1.0f);
    void drawProgressBar(SDL_Renderer* renderer, const SDL_FRect& rect, float currentValue, float maxValue, SDL_Color fillColor, SDL_Color bgColor = Theme::colors.bgDark, const std::string& label = "", float scale = 1.0f);
    bool drawButton(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& label, bool isHovered, bool isEnabled = true, bool isSelected = false, float scale = 1.0f);
    bool drawColoredButton(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& label, SDL_Color bgColor, SDL_Color textColor, bool isSelected = false, float scale = 1.0f);
    bool drawLTActionButton(SDL_Renderer* renderer, const SDL_FRect& rect, const std::string& label, const std::string& hotkey, bool isHovered, bool isEnabled = true, bool isSelected = false, float scale = 1.0f);

    void drawText(SDL_Renderer* renderer, const std::string& text, float x, float y, SDL_Color color = Theme::colors.textPrimary, float scale = 1.0f);
    float drawTextWrapped(SDL_Renderer* renderer, const std::string& text, float x, float y, float maxWidth, SDL_Color color = Theme::colors.textPrimary, float scale = 1.0f);
}