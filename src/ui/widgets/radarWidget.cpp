#include "ui/widgets/radarWidget.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "state/characterCreationState.h"
#include <format>
#include <vector>
#include <algorithm>
#include <cmath>

namespace RadarWidgets
{
    float renderWidgetRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        if (!gameContext->getPlayer())
        {
            return 0.0f;
        }

        const gameMap* m = gameContext->getActiveMap();
        if (!m) return 0.0f;

        float startY = curY;
        const int radius = 2; // 5x5 grid
        const int gridSize = (radius * 2) + 1; // 5
        const float availableW = std::max(20.0f, rect.w - (16.0f * uiScale));
        const float availableH = std::max(20.0f, rect.h - (10.0f * uiScale));
        const float maxDimension = std::min(availableW, availableH);
        const float tileSize = std::max(6.0f, std::min(22.0f * uiScale, maxDimension / static_cast<float>(gridSize)));
        const float totalGridW = tileSize * static_cast<float>(gridSize);

        const float padX = rect.x + ((rect.w - totalGridW) / 2.0f);

        for (int dy = -radius; dy <= radius; ++dy)
        {
            for (int dx = -radius; dx <= radius; ++dx)
            {
                SDL_FRect tileRect = {
                    padX + static_cast<float>(dx + radius) * tileSize,
                    curY + static_cast<float>(dy + radius) * tileSize,
                    std::max(1.0f, tileSize - (1.5f * uiScale)),
                    std::max(1.0f, tileSize - (1.5f * uiScale))
                };

                SDL_Color tileColor = SDL_Color{ 20, 22, 28, 255 };
                SDL_Color borderColor = SDL_Color{ 35, 38, 48, 255 };
                SDL_Color textCol = Theme::colors.textGold;
                std::string label = "";

                if (dx == 0 && dy == 0)
                {
                    // Player Center Tile (Corridor location pin)
                    tileColor = SDL_Color{ 25, 42, 60, 255 };
                    borderColor = SDL_Color{ 96, 175, 255, 255 };
                    label = "📍";
                    textCol = SDL_Color{ 100, 190, 255, 255 };
                }
                else if (dx == 0 && dy == -1)
                {
                    tileColor = SDL_Color{ 28, 34, 44, 255 };
                    borderColor = SDL_Color{ 60, 75, 95, 255 };
                    label = "W";
                    textCol = SDL_Color{ 130, 200, 255, 255 };
                }
                else if (dx == -1 && dy == 0)
                {
                    tileColor = SDL_Color{ 28, 34, 44, 255 };
                    borderColor = SDL_Color{ 60, 75, 95, 255 };
                    label = "A";
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
                    tileColor = SDL_Color{ 45, 25, 30, 255 };
                    borderColor = SDL_Color{ 160, 50, 60, 255 };
                    label = "D";
                    textCol = SDL_Color{ 255, 120, 130, 255 };
                }
                else if (std::abs(dx) == 2 || std::abs(dy) == 2 || (std::abs(dx) == 1 && std::abs(dy) == 1))
                {
                    tileColor = SDL_Color{ 18, 20, 26, 255 };
                    borderColor = SDL_Color{ 32, 35, 44, 255 };
                    label = "🛏";
                    textCol = SDL_Color{ 110, 115, 130, 200 };
                }

                // Draw tile border and fill
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                SDL_RenderRect(renderer, &tileRect);

                SDL_SetRenderDrawColor(renderer, tileColor.r, tileColor.g, tileColor.b, tileColor.a);
                SDL_RenderFillRect(renderer, &tileRect);

                if (!label.empty() && tileSize >= 12.0f * uiScale)
                {
                    float lScale = (label == "📍" || label == "🛏") ? (uiScale * 0.7f) : (uiScale * 0.75f);
                    UIWidget::drawText(renderer, label, tileRect.x + (tileSize * 0.2f), tileRect.y + (tileSize * 0.1f), textCol, lScale);
                }
            }
        }

        curY += (totalGridW + 4.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetTimeBar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext->getPlayer())
        {
            return 0.0f;
        }

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);
        std::string dateStr = inPrologue ? "29th August" : "Unknown date";
        std::string timeStr = inPrologue ? "20:37" : "21:47";

        // Calendar icon + Date + Watch icon + Time
        UIWidget::drawText(renderer, "📅", padX, curY + (2.0f * uiScale), inPrologue ? Theme::colors.textGold : SDL_Color{ 255, 110, 120, 255 }, uiScale * 0.85f);
        UIWidget::drawText(renderer, dateStr, padX + (16.0f * uiScale), curY + (2.0f * uiScale), inPrologue ? Theme::colors.textPrimary : SDL_Color{ 255, 110, 120, 255 }, uiScale * 0.82f);
        UIWidget::drawText(renderer, "⌚", padX + availableW - (60.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textGold, uiScale * 0.85f);
        UIWidget::drawText(renderer, timeStr, padX + availableW - (45.0f * uiScale), curY + (2.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.82f);
        curY += (18.0f * uiScale);

        // Weekdays tracker: M T W T [F] S S
        static const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
        float dayW = (availableW - (6 * 3.0f * uiScale)) / 7.0f;
        for (int d = 0; d < 7; ++d)
        {
            SDL_FRect dRect = { padX + (d * (dayW + 3.0f * uiScale)), curY, dayW, 14.0f * uiScale };
            bool isActiveDay = inPrologue && (d == 4); // Friday selected during prologue
            if (isActiveDay)
            {
                SDL_SetRenderDrawColor(renderer, 45, 55, 65, 255);
                SDL_RenderFillRect(renderer, &dRect);
                SDL_SetRenderDrawColor(renderer, 100, 160, 255, 255);
                SDL_RenderRect(renderer, &dRect);
            }
            UIWidget::drawText(renderer, days[d], dRect.x + ((dRect.w - (5.0f * uiScale)) / 2.0f), dRect.y + (1.0f * uiScale), isActiveDay ? Theme::colors.textPrimary : Theme::colors.textMuted, uiScale * 0.68f);
        }

        curY += (18.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetOptionsToolbar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext->getPlayer())
        {
            return 0.0f;
        }

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        static const std::vector<std::pair<std::string, CommandType>> tools = {
            { "⚙", CommandType::OPEN_SETTINGS },
            { "📱", CommandType::OPEN_PHONE },
            { "🛡", CommandType::OPEN_INVENTORY },
            { "👥", CommandType::OPEN_INVENTORY },
            { "🔍", CommandType::OPEN_TRANSFORMATION }
        };

        float btnGap = 4.0f * uiScale;
        float btnW = (availableW - (btnGap * (tools.size() - 1))) / tools.size();
        float btnH = 22.0f * uiScale;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        for (size_t i = 0; i < tools.size(); ++i)
        {
            SDL_FRect btnRect = { padX + (i * (btnW + btnGap)), curY, btnW, btnH };
            bool hovered = (mousePos.x >= btnRect.x && mousePos.x <= btnRect.x + btnRect.w &&
                            mousePos.y >= btnRect.y && mousePos.y <= btnRect.y + btnRect.h);

            bool isActive = (i == 1 && gameContext->isPhoneMenuOpen);
            UIWidget::drawButton(renderer, btnRect, tools[i].first, hovered, true, isActive, uiScale * 0.8f);

            if (hovered && clicked)
            {
                gameContext->handleCommand(UICommand{ tools[i].second });
                gameContext->input.consumeMouseClick();
            }
        }

        curY += btnH + (4.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetDpadRadar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext->getPlayer())
        {
            return 0.0f;
        }

        float startY = curY;
        curY += renderWidgetTimeBar(renderer, gameContext, curX, curY, innerW, uiScale);
        curY += renderWidgetRadar(renderer, gameContext, { curX, curY, innerW, 120.0f * uiScale }, curY, uiScale);
        curY += renderWidgetOptionsToolbar(renderer, gameContext, curX, curY, innerW, uiScale);
        return (curY - startY);
    }
}
