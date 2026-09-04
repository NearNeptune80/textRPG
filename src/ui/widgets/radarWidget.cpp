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
#include "state/inventoryState.h"
#include "state/phoneAppsState.h"
#include "state/mainMenuState.h"
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
                                        "Diurnal environmental cycle. Influences monster encounters, shop hours, and intimate interactions.",
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
            SDL_Color dFill = isActiveDay ? Theme::colors.bgSlotOccupied : Theme::colors.bgDark;

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

        float cardH = SidebarGeometry::getSquareCardH(rect, uiScale);
        SDL_FRect radarRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, radarRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        TooltipPoint mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        gameMap* map = gameContext->map;
        int pX = gameContext->gridX;
        int pY = gameContext->gridY;

        float headerH = 18.0f * uiScale;
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        std::string mapTitle = map ? map->getName() : "Local Area";
        UIWidget::drawHeader(renderer, headerRect, mapTitle, Theme::colors.bgHeader, Theme::colors.textAccent, uiScale * 0.72f);

        curY += headerH + (4.0f * uiScale);

        float cardCurY = curY;
        float innerPad = 8.0f * uiScale;
        float innerX = padX + innerPad;
        float innerW = availableW - (innerPad * 2.0f);

        // Render 5x5 Local Radar Grid
        float toolH = 20.0f * uiScale;
        float boxSize = availableW - (innerPad * 2.0f);
        float tileSize = (boxSize - (4 * 2.0f * uiScale)) / 5.0f;

        for (int dy = -2; dy <= 2; ++dy)
        {
            for (int dx = -2; dx <= 2; ++dx)
            {
                int targetX = pX + dx;
                int targetY = pY + dy;

                float tileX = innerX + ((dx + 2) * (tileSize + (2.0f * uiScale)));
                float tileY = cardCurY + ((dy + 2) * (tileSize + (2.0f * uiScale)));

                SDL_FRect tileRect = {
                    tileX,
                    tileY,
                    tileSize,
                    tileSize
                };

                SDL_Color tileColor = Theme::colors.bgDark;
                SDL_Color borderColor = Theme::colors.slotEmptyBorder;
                SDL_Color textCol = Theme::colors.textMuted;
                std::string label = "";

                bool isPlayer = (dx == 0 && dy == 0);
                bool inBounds = (map && targetX >= 0 && targetX < map->getWidth() && targetY >= 0 && targetY < map->getHeight());

                if (isPlayer)
                {
                    tileColor = Theme::colors.bgSlotSelected;
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
                        tileColor = Theme::colors.bgSlot;
                        borderColor = Theme::colors.borderNormal;
                        label = "#";
                        textCol = Theme::colors.textDisabled;
                        TooltipManager::setHoverTooltip(tileRect, mousePos, "Impassable Wall", "Solid boundary wall or barrier.", std::format("Grid ({}, {})", targetX, targetY));
                    }
                    else if (t.type == TILE_DOOR)
                    {
                        tileColor = Theme::colors.bgSlotOccupied;
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
                            tileColor = Theme::colors.bgSlotOccupied;
                            borderColor = Theme::colors.companion;
                            label = "W";
                            textCol = Theme::colors.companion;
                            TooltipManager::setHoverTooltip(tileRect, mousePos, "Zone Transition", "Passage connecting to another sector or room.", std::format("Grid ({}, {})", targetX, targetY));
                        }
                        else
                        {
                            tileColor = Theme::colors.bgInput;
                            borderColor = Theme::colors.borderMuted;
                            label = "·";
                            textCol = Theme::colors.textMuted;
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

        // Quick Navigation Toolbar (Inv, Phone, Main Menu)
        static const std::vector<std::pair<std::string, CommandType>> tools = {
            { "Inv", CommandType::OPEN_INVENTORY },
            { "Phone", CommandType::OPEN_PHONE },
            { "Main Menu", CommandType::OPEN_MAIN_MENU }
        };

        float toolW = (boxSize - (2 * 4.0f * uiScale)) / 3.0f;

        for (size_t i = 0; i < tools.size(); ++i)
        {
            SDL_FRect tRect = { innerX + (i * (toolW + (4.0f * uiScale))), cardCurY, toolW, toolH };
            bool hov = (mousePos.x >= tRect.x && mousePos.x <= tRect.x + tRect.w &&
                        mousePos.y >= tRect.y && mousePos.y <= tRect.y + tRect.h);
            bool isActive = false;
            if (i == 0 && dynamic_cast<inventoryState*>(gameContext->getActiveState())) isActive = true;
            else if (i == 1 && dynamic_cast<phoneAppsState*>(gameContext->getActiveState())) isActive = true;
            else if (i == 2 && dynamic_cast<mainMenuState*>(gameContext->getActiveState())) isActive = true;

            UIWidget::drawButton(renderer, tRect, tools[i].first, hov, true, isActive, uiScale * 0.70f);

            if (i == 0) TooltipManager::setHoverTooltip(tRect, mousePos, "Inventory & Storage", "Opens dual 5x4 player inventory and ground loot storage.", "Storage", "[ I ]");
            else if (i == 1) TooltipManager::setHoverTooltip(tRect, mousePos, "Phone & In-Game Actions", "Access smartphone apps, transformations, resting, and regional map.", "Communication", "[ P ]");
            else if (i == 2) TooltipManager::setHoverTooltip(tRect, mousePos, "Main Menu", "Open main menu, save/load, settings, and game options.", "System", "[ ESC ]");

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
