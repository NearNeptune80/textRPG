#include "ui/widgets/radarWidget.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "core/game.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "map/tile.h"
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

        // Date & Time Card
        float cardH = 46.0f * uiScale;
        SDL_FRect timeRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, timeRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerPad = 8.0f * uiScale;
        float tX = padX + innerPad;
        float tW = availableW - (innerPad * 2.0f);

        UIWidget::drawText(renderer, dateStr, tX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
        float timeTextW = UIWidget::getTextWidth(timeStr, uiScale * 0.80f);
        UIWidget::drawText(renderer, timeStr, tX + tW - timeTextW, curY + (5.0f * uiScale), Theme::colors.companion, uiScale * 0.80f);

        // Weekdays tracker: [M] [T] [W] [T] [F] [S] [S]
        static const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
        float dayW = (tW - (6 * 3.0f * uiScale)) / 7.0f;
        float dayY = curY + (24.0f * uiScale);

        for (int d = 0; d < 7; ++d)
        {
            SDL_FRect dRect = { tX + (d * (dayW + 3.0f * uiScale)), dayY, dayW, 14.0f * uiScale };
            bool isActiveDay = (d == 4); // Friday
            SDL_Color dColor = isActiveDay ? Theme::colors.textGold : Theme::colors.textMuted;
            SDL_Color dBorder = isActiveDay ? Theme::colors.borderSelected : Theme::colors.borderButton;
            SDL_Color dFill = isActiveDay ? SDL_Color{ 45, 55, 68, 255 } : Theme::colors.bgDark;

            UIWidget::drawPanel(renderer, dRect, dFill, dBorder);
            float txtW = UIWidget::getTextWidth(days[d], uiScale * 0.65f);
            UIWidget::drawText(renderer, days[d], dRect.x + ((dRect.w - txtW) / 2.0f), dRect.y + (1.0f * uiScale), dColor, uiScale * 0.65f);
        }

        curY += cardH + (10.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = rect.x + (8.0f * uiScale);
        float availableW = rect.w - (16.0f * uiScale);

        const gameMap* map = gameContext->getActiveMap();
        int pX = gameContext->gridX;
        int pY = gameContext->gridY;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Calculate card height for Radar + D-Pad + Toolbar
        const int radius = 2; // 5x5 grid
        const int gridSize = (radius * 2) + 1; // 5
        const float tileSize = std::clamp((availableW - (16.0f * uiScale)) / static_cast<float>(gridSize), 18.0f * uiScale, 28.0f * uiScale);
        const float totalGridW = tileSize * static_cast<float>(gridSize);

        float dpadBtnH = 24.0f * uiScale;
        float toolH = 22.0f * uiScale;
        float cardH = (26.0f * uiScale) + totalGridW + (10.0f * uiScale) + dpadBtnH + (8.0f * uiScale) + toolH + (10.0f * uiScale);

        // ==========================================
        // CARD: Mini-Map Radar & Navigation Suite
        // ==========================================
        SDL_FRect mainCardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, mainCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerX = padX + (8.0f * uiScale);
        float innerW = availableW - (16.0f * uiScale);
        float cardCurY = curY + (6.0f * uiScale);

        UIWidget::drawText(renderer, "MINI-MAP RADAR", innerX, cardCurY, Theme::colors.textGold, uiScale * 0.78f);
        cardCurY += (20.0f * uiScale);

        // Center the 5x5 grid inside the card
        float gridStartX = padX + ((availableW - totalGridW) / 2.0f);
        SDL_FRect radarBox = { gridStartX - (2.0f * uiScale), cardCurY - (2.0f * uiScale), totalGridW + (4.0f * uiScale), totalGridW + (4.0f * uiScale) };
        UIWidget::drawPanel(renderer, radarBox, Theme::colors.bgDark, Theme::colors.borderButton);

        for (int dy = -radius; dy <= radius; ++dy)
        {
            for (int dx = -radius; dx <= radius; ++dx)
            {
                int targetX = pX + dx;
                int targetY = pY + dy;

                SDL_FRect tileRect = {
                    gridStartX + static_cast<float>(dx + radius) * tileSize,
                    cardCurY + static_cast<float>(dy + radius) * tileSize,
                    tileSize - (2.0f * uiScale),
                    tileSize - (2.0f * uiScale)
                };

                SDL_Color tileColor = SDL_Color{ 14, 16, 20, 255 };
                SDL_Color borderColor = SDL_Color{ 28, 30, 38, 255 };
                SDL_Color textCol = Theme::colors.textMuted;
                std::string label = "";

                bool isPlayer = (dx == 0 && dy == 0);
                bool inBounds = (map && targetX >= 0 && targetX < map->getWidth() && targetY >= 0 && targetY < map->getHeight());

                if (isPlayer)
                {
                    tileColor = SDL_Color{ 25, 55, 85, 255 };
                    borderColor = Theme::colors.borderSelected;
                    label = "YOU";
                    textCol = Theme::colors.textGold;
                }
                else if (inBounds)
                {
                    Tile t = map->getTile(targetX, targetY);
                    bool walkable = map->isWalkable(targetX, targetY);

                    if (t.type == TILE_WALL)
                    {
                        tileColor = SDL_Color{ 35, 38, 48, 255 };
                        borderColor = SDL_Color{ 50, 55, 68, 255 };
                        label = "#";
                        textCol = SDL_Color{ 80, 85, 100, 255 };
                    }
                    else if (t.type == TILE_DOOR)
                    {
                        tileColor = SDL_Color{ 55, 45, 30, 255 };
                        borderColor = Theme::colors.textGold;
                        label = "+";
                        textCol = Theme::colors.textGold;
                    }
                    else if (walkable)
                    {
                        // Check if entity / item / warp present
                        MapWarp warp;
                        if (map->checkWarp(targetX, targetY, warp))
                        {
                            tileColor = SDL_Color{ 20, 50, 60, 255 };
                            borderColor = Theme::colors.companion;
                            label = "W";
                            textCol = Theme::colors.companion;
                        }
                        else
                        {
                            tileColor = SDL_Color{ 22, 26, 34, 255 };
                            borderColor = SDL_Color{ 45, 50, 65, 255 };
                            label = "·";
                            textCol = SDL_Color{ 100, 110, 130, 255 };
                        }
                    }
                }

                bool tileHovered = (mousePos.x >= tileRect.x && mousePos.x <= tileRect.x + tileRect.w &&
                                    mousePos.y >= tileRect.y && mousePos.y <= tileRect.y + tileRect.h);

                if (tileHovered && !isPlayer && inBounds && map->isWalkable(targetX, targetY))
                {
                    borderColor = Theme::colors.textGold;
                    if (clicked)
                    {
                        gameContext->movePlayer(targetX, targetY);
                        gameContext->input.consumeMouseClick();
                    }
                }

                UIWidget::drawPanel(renderer, tileRect, tileColor, borderColor);

                if (!label.empty())
                {
                    float lW = UIWidget::getTextWidth(label, uiScale * 0.65f);
                    UIWidget::drawText(renderer, label, tileRect.x + ((tileRect.w - lW) / 2.0f), tileRect.y + (2.0f * uiScale), textCol, uiScale * 0.65f);
                }
            }
        }

        cardCurY += totalGridW + (10.0f * uiScale);

        // Directional Movement D-Pad (< W, ^ N, v S, E >)
        float dpadBtnW = (innerW - (3 * 4.0f * uiScale)) / 4.0f;
        struct DpadButton { std::string label; int dx; int dy; };
        static const DpadButton dpadBtns[4] = {
            { "< W", -1, 0 },
            { "^ N", 0, -1 },
            { "v S", 0, 1 },
            { "E >", 1, 0 }
        };

        for (int i = 0; i < 4; ++i)
        {
            SDL_FRect bRect = { innerX + (i * (dpadBtnW + (4.0f * uiScale))), cardCurY, dpadBtnW, dpadBtnH };
            bool hov = (mousePos.x >= bRect.x && mousePos.x <= bRect.x + bRect.w &&
                        mousePos.y >= bRect.y && mousePos.y <= bRect.y + bRect.h);

            UIWidget::drawButton(renderer, bRect, dpadBtns[i].label, hov, true, false, uiScale * 0.76f);

            if (hov && clicked)
            {
                gameContext->movePlayer(pX + dpadBtns[i].dx, pY + dpadBtns[i].dy);
                gameContext->input.consumeMouseClick();
            }
        }
        cardCurY += dpadBtnH + (8.0f * uiScale);

        // Quick Navigation Toolbar
        static const std::vector<std::pair<std::string, CommandType>> tools = {
            { "Inv", CommandType::OPEN_INVENTORY },
            { "Phone", CommandType::OPEN_PHONE },
            { "TF", CommandType::OPEN_TRANSFORMATION },
            { "Opt", CommandType::OPEN_SETTINGS }
        };

        float toolW = (innerW - (3 * 4.0f * uiScale)) / 4.0f;

        for (size_t i = 0; i < tools.size(); ++i)
        {
            SDL_FRect tRect = { innerX + (i * (toolW + (4.0f * uiScale))), cardCurY, toolW, toolH };
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

        curY += cardH + (10.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetOptionsToolbar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        return 0.0f;
    }

    float renderWidgetDpadRadar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        curY += renderWidgetTimeBar(renderer, gameContext, curX, curY, innerW, uiScale);
        curY += renderWidgetRadar(renderer, gameContext, { curX, curY, innerW, 200.0f * uiScale }, curY, uiScale);
        return (curY - startY);
    }
}
