#include "ui/widgets/radarWidget.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "core/game.h"
#include "core/timeManager.h"
#include "entities/entity.h"
#include "map/gameMap.h"
#include "map/tile.h"
#include "state/characterCreationState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"
#include "ui/widgets/sidebarGeometry.h"

namespace RadarWidgets
{
    float renderWidgetTimeBar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        SDL_FRect dummyRect = { curX, curY, innerW, 0.0f };
        float padX = SidebarGeometry::getPadX(dummyRect, uiScale);
        float availableW = SidebarGeometry::getAvailableW(dummyRect, uiScale);

        const timeManager& tm = gameContext->getTime();
        bool inPrologue = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);

        static constexpr std::string_view months[13] = {
            "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        std::string_view mName = (tm.month >= 1 && tm.month <= 12) ? months[tm.month] : "Month";
        std::string dateStr = inPrologue ? "Day 29, Aug" : std::format("Day {}, {}", tm.day, mName);
        std::string timeStr = inPrologue ? "20:37 (Night)" : std::format("{} ({})", tm.getFormattedTime(), tm.getPhaseString());

        // 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday
        // Map to Monday-first index [M, T, W, T, F, S, S] (0..6)
        int activeDayIdx = inPrologue ? 4 : ((tm.dayOfWeek == 0) ? 6 : (tm.dayOfWeek - 1));

        // Date & Time Card
        float cardH = SidebarGeometry::getTimeBarH(uiScale);
        SDL_FRect timeRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, timeRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
        TooltipManager::setHoverTooltip(timeRect, gameContext->input.getMousePosition(),
                                        std::format("Calendar: {}", dateStr),
                                        "Dominion diurnal cycle. Influences monster encounters, shop hours, and intimate interactions.",
                                        timeStr);

        float innerPad = 8.0f * uiScale;
        float tX = padX + innerPad;
        float tW = availableW - (innerPad * 2.0f);

        UIWidget::drawText(renderer, dateStr, tX, curY + (5.0f * uiScale), Theme::colors.textGold, uiScale * 0.82f);
        float timeTextW = UIWidget::getTextWidth(timeStr, uiScale * 0.80f);
        UIWidget::drawText(renderer, timeStr, tX + tW - timeTextW, curY + (5.0f * uiScale), Theme::colors.companion, uiScale * 0.80f);

        // Weekdays tracker: [M] [T] [W] [T] [F] [S] [S]
        static const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
        static const char* fullDays[7] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
        float dayW = (tW - (6 * 3.0f * uiScale)) / 7.0f;
        float dayY = curY + (24.0f * uiScale);

        for (int d = 0; d < 7; ++d)
        {
            SDL_FRect dRect = { tX + (d * (dayW + 3.0f * uiScale)), dayY, dayW, 14.0f * uiScale };
            bool isActiveDay = (d == activeDayIdx);
            SDL_Color dColor = isActiveDay ? Theme::colors.textGold : Theme::colors.textMuted;
            SDL_Color dBorder = isActiveDay ? Theme::colors.borderSelected : Theme::colors.borderButton;
            SDL_Color dFill = isActiveDay ? SDL_Color{ 45, 55, 68, 255 } : Theme::colors.bgDark;

            UIWidget::drawPanel(renderer, dRect, dFill, dBorder);
            float txtW = UIWidget::getTextWidth(days[d], uiScale * 0.65f);
            UIWidget::drawText(renderer, days[d], dRect.x + ((dRect.w - txtW) / 2.0f), dRect.y + (1.0f * uiScale), dColor, uiScale * 0.65f);

            TooltipManager::setHoverTooltip(dRect, gameContext->input.getMousePosition(), fullDays[d],
                                            isActiveDay ? "Current in-game day of the week." : "Day of the in-game week.",
                                            "Calendar");
        }

        curY += cardH;
        return (curY - startY);
    }

    float renderWidgetRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& rect, float curY, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = SidebarGeometry::getPadX(rect, uiScale);
        float availableW = SidebarGeometry::getAvailableW(rect, uiScale);

        const gameMap* map = gameContext->getActiveMap();
        int pX = gameContext->gridX;
        int pY = gameContext->gridY;

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Calculate square dimensions for 5x5 tile grid filling the container
        const int radius = 2; // 5x5 grid
        const int gridSize = (radius * 2) + 1; // 5
        const float boxSize = SidebarGeometry::getBoxSize(rect, uiScale);
        const float tileSize = boxSize / static_cast<float>(gridSize);

        float toolH = SidebarGeometry::getToolbarH(uiScale);
        float cardH = SidebarGeometry::getSquareCardH(rect, uiScale);

        // ==========================================
        // CARD: Mini-Map Radar & Quick Toolbar
        // ==========================================
        SDL_FRect mainCardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, mainCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerX = padX + (4.0f * uiScale);
        float cardCurY = curY + (3.0f * uiScale);

        UIWidget::drawText(renderer, "MINI-MAP RADAR", innerX, cardCurY, Theme::colors.textGold, uiScale * 0.74f);
        cardCurY += (15.0f * uiScale);

        // Center the 5x5 grid inside the card
        float gridStartX = SidebarGeometry::getGridStartX(rect, uiScale);
        SDL_FRect radarBox = { gridStartX - (1.0f * uiScale), cardCurY - (1.0f * uiScale), boxSize + (2.0f * uiScale), boxSize + (2.0f * uiScale) };
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
                    tileSize,
                    tileSize
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
                    TooltipManager::setHoverTooltip(tileRect, mousePos, "Player Location", "Your current grid position on this map.", std::format("Grid ({}, {})", pX, pY));
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
                        TooltipManager::setHoverTooltip(tileRect, mousePos, "Impassable Wall", "Solid boundary wall or barrier.", std::format("Grid ({}, {})", targetX, targetY));
                    }
                    else if (t.type == TILE_DOOR)
                    {
                        tileColor = SDL_Color{ 55, 45, 30, 255 };
                        borderColor = Theme::colors.textGold;
                        label = "+";
                        textCol = Theme::colors.textGold;
                        TooltipManager::setHoverTooltip(tileRect, mousePos, "Door / Portal", "Click to move toward or open this doorway.", std::format("Grid ({}, {})", targetX, targetY));
                    }
                    else if (walkable)
                    {
                        MapWarp warp;
                        if (map->checkWarp(targetX, targetY, warp))
                        {
                            tileColor = SDL_Color{ 20, 50, 60, 255 };
                            borderColor = Theme::colors.companion;
                            label = "W";
                            textCol = Theme::colors.companion;
                            TooltipManager::setHoverTooltip(tileRect, mousePos, "Zone Transition", "Passage connecting to another sector or room.", std::format("Grid ({}, {})", targetX, targetY));
                        }
                        else
                        {
                            tileColor = SDL_Color{ 22, 26, 34, 255 };
                            borderColor = SDL_Color{ 45, 50, 65, 255 };
                            label = "·";
                            textCol = SDL_Color{ 100, 110, 130, 255 };
                            TooltipManager::setHoverTooltip(tileRect, mousePos, "Open Floor", "Walkable terrain tile. Click to navigate.", std::format("Grid ({}, {})", targetX, targetY));
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

        cardCurY += boxSize + (5.0f * uiScale);

        // Quick Navigation Toolbar (Inv, Phone, TF, Opt)
        static const std::vector<std::pair<std::string, CommandType>> tools = {
            { "Inv", CommandType::OPEN_INVENTORY },
            { "Phone", CommandType::OPEN_PHONE },
            { "TF", CommandType::OPEN_TRANSFORMATION },
            { "Opt", CommandType::OPEN_SETTINGS }
        };

        float toolW = (boxSize - (3 * 4.0f * uiScale)) / 4.0f;

        for (size_t i = 0; i < tools.size(); ++i)
        {
            SDL_FRect tRect = { innerX + (i * (toolW + (4.0f * uiScale))), cardCurY, toolW, toolH };
            bool hov = (mousePos.x >= tRect.x && mousePos.x <= tRect.x + tRect.w &&
                        mousePos.y >= tRect.y && mousePos.y <= tRect.y + tRect.h);
            bool isActive = (i == 1 && gameContext->isPhoneMenuOpen);

            UIWidget::drawButton(renderer, tRect, tools[i].first, hov, true, isActive, uiScale * 0.74f);

            if (i == 0) TooltipManager::setHoverTooltip(tRect, mousePos, "Inventory & Storage", "Opens dual 5x4 player inventory and ground loot storage.", "Storage", "[ I ]");
            else if (i == 1) TooltipManager::setHoverTooltip(tRect, mousePos, "Phone & Messaging", "Access smartphone apps, contacts, messages, and map.", "Communication", "[ P ]");
            else if (i == 2) TooltipManager::setHoverTooltip(tRect, mousePos, "Transformations & Mutations", "Inspect body changes, demon morphs, horns, wings, and anatomy editor.", "Biology", "[ M ]");
            else if (i == 3) TooltipManager::setHoverTooltip(tRect, mousePos, "Game Options & Settings", "Adjust gameplay options, content filters, keybindings, and themes.", "System", "[ O / ESC ]");

            if (hov && clicked)
            {
                gameContext->handleCommand(UICommand{ tools[i].second });
                gameContext->input.consumeMouseClick();
            }
        }

        curY += cardH;
        return (curY - startY);
    }

    float renderWidgetOptionsToolbar(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        return 0.0f;
    }

    float renderWidgetDpadRadar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float curY, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float bottomPinnedY = SidebarGeometry::getBottomPinnedY(panelRect, uiScale);
        curY = std::max(curY, bottomPinnedY);

        float startY = curY;
        curY += renderWidgetTimeBar(renderer, gameContext, panelRect.x, curY, panelRect.w, uiScale);
        curY += SidebarGeometry::getGapBetweenCards(uiScale);
        curY += renderWidgetRadar(renderer, gameContext, panelRect, curY, uiScale);
        return (curY - startY);
    }
}
