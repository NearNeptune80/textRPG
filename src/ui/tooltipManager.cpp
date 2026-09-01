#include "ui/tooltipManager.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>

std::optional<TooltipData> TooltipManager::s_activeTooltip = std::nullopt;

void TooltipManager::setTooltip(const TooltipData& data)
{
    s_activeTooltip = data;
}

void TooltipManager::setTooltip(const std::string& title,
                                const std::string& description,
                                const std::string& subtitle,
                                const std::string& hotkey,
                                const SDL_Color* titleColor)
{
    TooltipData data;
    data.title = title;
    data.description = description;
    data.subtitle = subtitle;
    data.hotkey = hotkey;
    if (titleColor) data.titleColor = *titleColor;
    s_activeTooltip = data;
}

bool TooltipManager::setHoverTooltip(const SDL_FRect& hoverRect,
                                     const TooltipPoint& mousePos,
                                     const std::string& title,
                                     const std::string& description,
                                     const std::string& subtitle,
                                     const std::string& hotkey,
                                     const SDL_Color* titleColor)
{
    if (mousePos.x >= hoverRect.x && mousePos.x <= hoverRect.x + hoverRect.w &&
        mousePos.y >= hoverRect.y && mousePos.y <= hoverRect.y + hoverRect.h)
    {
        TooltipData data;
        data.title = title;
        data.description = description;
        data.subtitle = subtitle;
        data.hotkey = hotkey;
        if (titleColor) data.titleColor = *titleColor;
        data.hoverBounds = hoverRect;
        s_activeTooltip = data;
        return true;
    }
    return false;
}

bool TooltipManager::setHoverTooltip(const SDL_FRect& hoverRect,
                                     const TooltipPoint& mousePos,
                                     const TooltipData& data)
{
    if (mousePos.x >= hoverRect.x && mousePos.x <= hoverRect.x + hoverRect.w &&
        mousePos.y >= hoverRect.y && mousePos.y <= hoverRect.y + hoverRect.h)
    {
        TooltipData copy = data;
        copy.hoverBounds = hoverRect;
        s_activeTooltip = copy;
        return true;
    }
    return false;
}

void TooltipManager::clear()
{
    s_activeTooltip = std::nullopt;
}

bool TooltipManager::hasActiveTooltip()
{
    return s_activeTooltip.has_value();
}

void TooltipManager::render(SDL_Renderer* renderer, float uiScale, float windowW, float windowH, const TooltipPoint& mousePos)
{
    if (!s_activeTooltip.has_value()) return;

    const auto& tt = s_activeTooltip.value();
    if (tt.title.empty() && tt.description.empty()) return;

    float innerPad = 8.0f * uiScale;
    float maxCardW = 320.0f * uiScale;
    float minCardW = 180.0f * uiScale;

    // Sizing estimation
    float titleFontScale = uiScale * 0.78f;
    float subFontScale = uiScale * 0.68f;
    float descFontScale = uiScale * 0.72f;
    float hotkeyFontScale = uiScale * 0.62f;

    float titleW = UIWidget::getTextWidth(tt.title, titleFontScale);
    float subW = tt.subtitle.empty() ? 0.0f : UIWidget::getTextWidth(tt.subtitle, subFontScale);
    float hotkeyW = tt.hotkey.empty() ? 0.0f : (UIWidget::getTextWidth(tt.hotkey, hotkeyFontScale) + (10.0f * uiScale));

    float headerRowW = titleW + (tt.hotkey.empty() ? 0.0f : (hotkeyW + (10.0f * uiScale)));
    float idealW = std::max({ minCardW, headerRowW + (innerPad * 2.0f), subW + (innerPad * 2.0f) });
    if (!tt.description.empty())
    {
        idealW = std::max(idealW, 250.0f * uiScale);
    }
    float cardW = std::min(maxCardW, idealW);
    float contentW = cardW - (innerPad * 2.0f);

    // Compute vertical height using exact font-metric measurements
    float cardH = innerPad;
    cardH += 18.0f * uiScale; // Title row

    if (!tt.subtitle.empty())
    {
        cardH += 16.0f * uiScale; // Subtitle row
    }

    if (!tt.description.empty())
    {
        cardH += 6.0f * uiScale; // Divider gap
        float measuredDescH = UIWidget::getTextWrappedHeight(tt.description, contentW, descFontScale);
        cardH += measuredDescH + (4.0f * uiScale);
    }

    if (!tt.stats.empty())
    {
        cardH += (tt.stats.size() * (15.0f * uiScale)) + (4.0f * uiScale);
    }

    cardH += innerPad + (4.0f * uiScale); // Safe bottom margin

    // Positioning without overlapping the inspected element
    float posX = 0.0f;
    float posY = 0.0f;

    if (tt.hasCustomAnchor)
    {
        posX = tt.customAnchor.x;
        posY = tt.customAnchor.y;
    }
    else if (tt.hoverBounds.has_value())
    {
        const SDL_FRect& b = tt.hoverBounds.value();
        // 1. Bottom action commands (y > 60% of screen): Place ABOVE the button
        if (b.y > windowH * 0.60f)
        {
            posY = b.y - cardH - (6.0f * uiScale);
            posX = b.x + ((b.w - cardW) / 2.0f);
        }
        // 2. Left sidebar panels (x < 28% of screen): Place to the RIGHT
        else if (b.x < windowW * 0.28f)
        {
            posX = b.x + b.w + (8.0f * uiScale);
            posY = b.y;
            if (posY + cardH > windowH - (8.0f * uiScale))
            {
                posY = windowH - cardH - (8.0f * uiScale);
            }
        }
        // 3. Right sidebar panels (x > 72% of screen): Place to the LEFT
        else if (b.x + b.w > windowW * 0.72f)
        {
            posX = b.x - cardW - (8.0f * uiScale);
            posY = b.y;
            if (posY + cardH > windowH - (8.0f * uiScale))
            {
                posY = windowH - cardH - (8.0f * uiScale);
            }
        }
        // 4. Central inventory/content grid: Place beside slot
        else
        {
            if (b.x + b.w + cardW + (8.0f * uiScale) <= windowW * 0.82f)
            {
                posX = b.x + b.w + (6.0f * uiScale);
            }
            else
            {
                posX = b.x - cardW - (6.0f * uiScale);
            }
            posY = b.y;
            if (posY + cardH > windowH - (8.0f * uiScale))
            {
                posY = windowH - cardH - (8.0f * uiScale);
            }
        }
    }
    else
    {
        // Cursor fallback
        posX = mousePos.x + (14.0f * uiScale);
        posY = mousePos.y + (14.0f * uiScale);

        if (posX + cardW > windowW - (8.0f * uiScale))
        {
            posX = mousePos.x - cardW - (10.0f * uiScale);
        }
        if (posY + cardH > windowH - (8.0f * uiScale))
        {
            posY = mousePos.y - cardH - (10.0f * uiScale);
        }
    }

    // Always clamp to screen boundaries
    posX = std::clamp(posX, 6.0f * uiScale, windowW - cardW - (6.0f * uiScale));
    posY = std::clamp(posY, 6.0f * uiScale, windowH - cardH - (6.0f * uiScale));

    // 1. Drop Shadow
    SDL_FRect shadowRect = { posX + (3.0f * uiScale), posY + (3.0f * uiScale), cardW, cardH };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, &shadowRect);

    // 2. Main Card Panel
    SDL_FRect cardRect = { posX, posY, cardW, cardH };
    SDL_Color bgCol = SDL_Color{ 14, 16, 22, 250 };
    SDL_Color bdCol = SDL_Color{ 70, 85, 110, 255 };
    UIWidget::drawPanel(renderer, cardRect, bgCol, bdCol);

    // 3. Top Accent Line (Gold)
    SDL_FRect topBar = { posX + 1.0f, posY + 1.0f, cardW - 2.0f, 2.0f * uiScale };
    SDL_SetRenderDrawColor(renderer, Theme::colors.textGold.r, Theme::colors.textGold.g, Theme::colors.textGold.b, 255);
    SDL_RenderFillRect(renderer, &topBar);

    // 4. Render Content
    float curY = posY + innerPad;
    float curX = posX + innerPad;

    // Title
    if (!tt.title.empty())
    {
        UIWidget::drawText(renderer, tt.title, curX, curY, tt.titleColor, titleFontScale);

        // Hotkey Badge
        if (!tt.hotkey.empty())
        {
            float hkBoxW = hotkeyW;
            float hkBoxH = 16.0f * uiScale;
            float hkX = posX + cardW - innerPad - hkBoxW;
            float hkY = curY + (1.0f * uiScale);
            SDL_FRect hkRect = { hkX, hkY, hkBoxW, hkBoxH };
            UIWidget::drawPanel(renderer, hkRect, SDL_Color{ 30, 36, 48, 255 }, Theme::colors.borderSelected);
            float txtW = UIWidget::getTextWidth(tt.hotkey, hotkeyFontScale);
            UIWidget::drawText(renderer, tt.hotkey, hkX + ((hkBoxW - txtW) / 2.0f), hkY + (1.0f * uiScale), Theme::colors.textGold, hotkeyFontScale);
        }

        curY += (18.0f * uiScale);
    }

    // Subtitle / Tag
    if (!tt.subtitle.empty())
    {
        UIWidget::drawText(renderer, tt.subtitle, curX, curY, tt.subtitleColor, subFontScale);
        curY += (16.0f * uiScale);
    }

    // Divider Line
    if (!tt.description.empty() || !tt.stats.empty())
    {
        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255);
        SDL_RenderLine(renderer, curX, curY + (2.0f * uiScale), curX + contentW, curY + (2.0f * uiScale));
        curY += (6.0f * uiScale);
    }

    // Description
    if (!tt.description.empty())
    {
        float textH = UIWidget::drawTextWrapped(renderer, tt.description, curX, curY, contentW, Theme::colors.textPrimary, descFontScale);
        curY += textH + (4.0f * uiScale);
    }

    // Stats Key-Value Table
    for (const auto& [statKey, statVal] : tt.stats)
    {
        UIWidget::drawText(renderer, statKey, curX, curY, Theme::colors.textSecondary, subFontScale);
        float vW = UIWidget::getTextWidth(statVal, subFontScale);
        UIWidget::drawText(renderer, statVal, curX + contentW - vW, curY, Theme::colors.textGold, subFontScale);
        curY += (15.0f * uiScale);
    }
}
