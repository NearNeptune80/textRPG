#include "ui/widgets/radarWidget.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "state/characterCreationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"

namespace RadarWidgets
{
    float renderWidgetTimeBar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
        std::string dateStr = inPrologue ? "29th August" : "Day 29, August";
        std::string timeStr = inPrologue ? "20:37 (Night)" : "21:47 (Night)";

        // Date & Time Container
        SDL_FRect timeRect = { padX, curY, availableW, 36.0f * uiScale };
        UIWidget::drawPanel(renderer, timeRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        UIWidget::drawText(renderer, dateStr, padX + (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
        float timeW = UIWidget::getTextWidth(timeStr, uiScale * 0.80f);
        UIWidget::drawText(renderer, timeStr, padX + availableW - timeW - (8.0f * uiScale), curY + (4.0f * uiScale), Theme::colors.companion, uiScale * 0.80f);

        // Weekdays tracker: [M] [T] [W] [T] [F] [S] [S]
        static const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
        float dayW = (availableW - (16.0f * uiScale) - (6 * 3.0f * uiScale)) / 7.0f;
        float dayY = curY + (18.0f * uiScale);

        for (int d = 0; d < 7; ++d)
        {
            SDL_FRect dRect = { padX + (8.0f * uiScale) + (d * (dayW + 3.0f * uiScale)), dayY, dayW, 14.0f * uiScale };
            bool isActiveDay = (d == 4); // Friday
            SDL_Color dColor = isActiveDay ? Theme::colors.textGold : Theme::colors.textMuted;
            SDL_Color dBorder = isActiveDay ? Theme::colors.borderSelected : Theme::colors.borderButton;
            SDL_Color dFill = isActiveDay ? SDL_Color{ 45, 55, 68, 255 } : Theme::colors.bgDark;

            UIWidget::drawPanel(renderer, dRect, dFill, dBorder);
            float txtW = UIWidget::getTextWidth(days[d], uiScale * 0.65f);
            UIWidget::drawText(renderer, days[d], dRect.x + ((dRect.w - txtW) / 2.0f), dRect.y + (1.0f * uiScale), dColor, uiScale * 0.65f);
        }

        curY += timeRect.h + (8.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        const int radius = 2; // 5x5 grid
        const int gridSize = (radius * 2) + 1; // 5
        const float availableW = std::max(20.0f, rect.w - (16.0f * uiScale));
        const float tileSize = std::clamp(availableW / static_cast<float>(gridSize), 16.0f * uiScale, 26.0f * uiScale);
        const float totalGridW = tileSize * static_cast<float>(gridSize);

        const float padX = rect.x + ((rect.w - totalGridW) / 2.0f);

        SDL_FRect radarBox = { padX - (4.0f * uiScale), curY - (2.0f * uiScale), totalGridW + (8.0f * uiScale), totalGridW + (8.0f * uiScale) };
        UIWidget::drawPanel(renderer, radarBox, Theme::colors.bgDark, Theme::colors.borderButton);

        for (int dy = -radius; dy <= radius; ++dy)
        {
            for (int dx = -radius; dx <= radius; ++dx)
            {
                SDL_FRect tileRect = {
                    padX + static_cast<float>(dx + radius) * tileSize,
                    curY + static_cast<float>(dy + radius) * tileSize,
                    tileSize - (2.0f * uiScale),
                    tileSize - (2.0f * uiScale)
                };

                SDL_Color tileColor = SDL_Color{ 20, 22, 28, 255 };
                SDL_Color borderColor = SDL_Color{ 35, 38, 48, 255 };
                SDL_Color textCol = Theme::colors.textGold;
                std::string label = "";

                if (dx == 0 && dy == 0)
                {
                    // Player Center Tile
                    tileColor = SDL_Color{ 25, 50, 75, 255 };
                    borderColor = Theme::colors.companion;
                    label = "YOU";
                    textCol = Theme::colors.textGold;
                }
                else if (dx == 0 && dy == -1)
                {
                    tileColor = SDL_Color{ 28, 34, 44, 255 };
                    borderColor = SDL_Color{ 60, 75, 95, 255 };
                    label = "N";
                    textCol = SDL_Color{ 130, 200, 255, 255 };
                }
                else if (dx == -1 && dy == 0)
                {
                    tileColor = SDL_Color{ 28, 34, 44, 255 };
                    borderColor = SDL_Color{ 60, 75, 95, 255 };
                    label = "W";
                    textCol = SDL_Color{ 130, 200, 255, 255 };
                }
                else if (dx == 0 && dy == 1)
                {
                    tileColor = SDL_Color{ 28, 34, 44, 255 };
                    borderColor = SDL_Color{ 60, 75, 95, 255 };
                    label = "S";
                    textCol = SDL_Color{ 130, 200, 255, 255 };
                }
                else if (dx == 1 && dy == 0)
                {
                    tileColor = SDL_Color{ 28, 34, 44, 255 };
                    borderColor = SDL_Color{ 60, 75, 95, 255 };
                    label = "E";
                    textCol = SDL_Color{ 130, 200, 255, 255 };
                }
                else
                {
                    tileColor = SDL_Color{ 16, 18, 24, 255 };
                    borderColor = SDL_Color{ 28, 30, 38, 255 };
                    label = "·";
                    textCol = SDL_Color{ 70, 75, 90, 255 };
                }

                UIWidget::drawPanel(renderer, tileRect, tileColor, borderColor);

                if (!label.empty())
                {
                    float lW = UIWidget::getTextWidth(label, uiScale * 0.65f);
                    UIWidget::drawText(renderer, label, tileRect.x + ((tileRect.w - lW) / 2.0f), tileRect.y + (2.0f * uiScale), textCol, uiScale * 0.65f);
                }
            }
        }

        curY += totalGridW + (10.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetOptionsToolbar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        // Directional Movement Pad (North, West, South, East)
        float dpadBtnW = (availableW - (3 * 4.0f * uiScale)) / 4.0f;
        float dpadBtnH = 24.0f * uiScale;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        struct DpadButton { std::string label; int dx; int dy; };
        static const DpadButton dpadBtns[4] = {
            { "< W", -1, 0 },
            { "^ N", 0, -1 },
            { "v S", 0, 1 },
            { "E >", 1, 0 }
        };

        for (int i = 0; i < 4; ++i)
        {
            SDL_FRect bRect = { padX + (i * (dpadBtnW + (4.0f * uiScale))), curY, dpadBtnW, dpadBtnH };
            bool hov = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                        mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, dpadBtns[i].label, hov, true, false, uiScale * 0.76f);

            if (hov && clicked)
            {
                gameContext->movePlayer(gameContext->gridX + dpadBtns[i].dx, gameContext->gridY + dpadBtns[i].dy);
                gameContext->input.consumeMouseClick();
            }
        }
        curY += dpadBtnH + (8.0f * uiScale);

        // Quick Navigation Toolstrip
        static const std::vector<std::pair<std::string, CommandType>> tools = {
            { "Inv", CommandType::OPEN_INVENTORY },
            { "Phone", CommandType::OPEN_PHONE },
            { "TF", CommandType::OPEN_TRANSFORMATION },
            { "Opt", CommandType::OPEN_SETTINGS }
        };

        float toolW = (availableW - (3 * 4.0f * uiScale)) / 4.0f;
        float toolH = 22.0f * uiScale;

        for (size_t i = 0; i < tools.size(); ++i)
        {
            SDL_FRect tRect = { padX + (i * (toolW + (4.0f * uiScale))), curY, toolW, toolH };
            bool hov = (mousePos.x >= tRect.x && mousePos.x <= tRect.x + tRect.w &&
                        mousePos.y >= tRect.y && mousePos.y <= tRect.y + tRect.h);
            bool isActive = (i == 1 && gameContext->isPhoneMenuOpen);

            UIWidget::drawButton(renderer, tRect, tools[i].first, hov, true, isActive, uiScale * 0.74f);

            if (hov && clicked)
            {
                gameContext->handleCommand(UICommand{ tools[i].second });
                gameContext->input.consumeMouseClick();
            }
        }

        curY += toolH + (8.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetDpadRadar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        curY += renderWidgetTimeBar(renderer, gameContext, curX, curY, innerW, uiScale);
        curY += renderWidgetRadar(renderer, gameContext, { curX, curY, innerW, 130.0f * uiScale }, curY, uiScale);
        curY += renderWidgetOptionsToolbar(renderer, gameContext, curX, curY, innerW, uiScale);
        return (curY - startY);
    }
}
