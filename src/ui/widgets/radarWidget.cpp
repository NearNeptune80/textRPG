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
#include "state/explorationState.h"
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
        auto* cc = dynamic_cast<characterCreationState*>(gameContext->getActiveState());
        bool inPrologue = (cc != nullptr);

        static constexpr std::string_view months[13] = {
            "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        std::string_view mName = (tm.month >= 1 && tm.month <= 12) ? months[tm.month] : "Month";
        std::string dateStr = inPrologue ? std::format("Day 29, {}", cc->startMonth.substr(0, 3)) : std::format("Day {}, {}", tm.day, mName);
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

        float cardCurY = curY + headerH;
        float boxSize = SidebarGeometry::getBoxSize(rect, uiScale);
        float gridStartX = SidebarGeometry::getGridStartX(rect, uiScale);

        // Centered 5x5 Grid Container Box matching Mini-Map Radar & Equipment Grid
        SDL_FRect gridBox = { gridStartX - (1.0f * uiScale), cardCurY - (1.0f * uiScale), boxSize + (2.0f * uiScale), boxSize + (2.0f * uiScale) };
        UIWidget::drawPanel(renderer, gridBox, Theme::colors.bgDark, Theme::colors.borderButton);

        // Render 5x5 Local Radar Grid
        float tileSize = (boxSize - (4 * 2.0f * uiScale)) / 5.0f;

        for (int dy = -2; dy <= 2; ++dy)
        {
            for (int dx = -2; dx <= 2; ++dx)
            {
                int targetX = pX + dx;
                int targetY = pY + dy;

                float tileX = gridStartX + ((dx + 2) * (tileSize + (2.0f * uiScale)));
                float tileY = cardCurY + ((dy + 2) * (tileSize + (2.0f * uiScale)));

                SDL_FRect tileRect = {
                    tileX,
                    tileY,
                    tileSize,
                    tileSize
                };
                bool isPlayer = (dx == 0 && dy == 0);
                bool isAdjacent = (std::abs(dx) + std::abs(dy) == 1);
                bool isExploration = (dynamic_cast<explorationState*>(gameContext->getActiveState()) != nullptr);
                bool inBounds = (map && targetX >= 0 && targetX < map->getWidth() && targetY >= 0 && targetY < map->getHeight());

                // Completely suppress rendering for coordinates outside the map or void tiles
                if (!inBounds) continue;
                Tile t = map->getTile(targetX, targetY);
                if (t.type == TILE_VOID) continue;

                SDL_Color tileColor = Theme::colors.bgDark;
                SDL_Color borderColor = Theme::colors.slotEmptyBorder;
                SDL_Color textCol = Theme::colors.textMuted;
                std::string label = "";

                // Query danger and ambush runtime data
                TileRuntimeData& rData = map->getRuntimeData(targetX, targetY);
                int danger = rData.getEffectiveDangerLevel();
                bool hasAmbush = (!rData.ambushState.isDefeated && !rData.ambushState.isPermanentlyRemoved &&
                                 (rData.ambushState.npc != nullptr || !rData.ambushState.templateId.empty() || !rData.ambushState.templatePool.empty()));
                bool isDangerous = (danger > 0 || hasAmbush);

                if (isPlayer)
                {
                    tileColor = isDangerous ? SDL_Color{ 60, 24, 28, 255 } : Theme::colors.bgSlotSelected;
                    borderColor = isDangerous ? Theme::colors.enemy : Theme::colors.borderSelected;
                    label = "YOU";
                    textCol = isDangerous ? Theme::colors.enemy : Theme::colors.textGold;
                    std::string playerDesc = isDangerous ? "Current grid position. WARNING: Hostile territory or ambush threat!" : "Your current grid position on this map.";
                    TooltipManager::setHoverTooltip(tileRect, mousePos, isDangerous ? "Player Location [Hazard Zone]" : "Player Location", playerDesc, std::format("Grid ({}, {})", pX, pY));
                }
                else
                {
                    bool walkable = map->isWalkable(targetX, targetY);
                    MapWarp warp;
                    bool isWarp = walkable && map->checkWarp(targetX, targetY, warp);

                    if (t.type == TILE_WALL) label = "#";
                    else if (t.type == TILE_DOOR) label = "+";
                    else if (isWarp) label = "W";
                    else if (walkable) label = isDangerous ? "!" : "·";

                    // 3-Tier Discovery System:
                    // 1. Undiscovered: Player has never stood on it and never stood next to it (Dark, but visible)
                    // 2. Partially Discovered: Player has stood on an adjacent tile, but not on this tile (Lighter)
                    // 3. Fully Discovered / Visited: Player has physically walked on this tile (Brightest)
                    if (t.visited || t.discovery == STATE_REVEALED)
                    {
                        // Visited / Fully shown
                        if (isDangerous)
                        {
                            tileColor = { 56, 22, 28, 255 };
                            borderColor = { 200, 60, 65, 255 };
                            textCol = (label == "W") ? Theme::colors.companion : (label == "+" ? Theme::colors.textGold : Theme::colors.enemy);

                            std::string desc = isAdjacent ? (isExploration ? "Click to navigate. DANGER: Hostile encounter or ambush!" : "Dangerous tile. Movement locked during encounter/event.") : "Explored hostile terrain. Must be adjacent to move here.";
                            std::string title = (label == "#") ? "Explored Wall" : (label == "+" ? "Explored Door" : (isWarp ? "Hazardous Passage" : "Dangerous Floor"));
                            TooltipManager::setHoverTooltip(tileRect, mousePos, title, desc, std::format("Grid ({}, {}) [Danger: {}]", targetX, targetY, danger > 0 ? danger : 1));
                        }
                        else
                        {
                            tileColor = { 38, 44, 62, 255 };
                            borderColor = Theme::colors.borderSelected;
                            textCol = (label == "W") ? Theme::colors.companion : (label == "+" ? Theme::colors.textGold : Theme::colors.textGold);

                            std::string desc = isAdjacent ? (isExploration ? "Click to navigate to this adjacent tile." : "Explored tile. Movement locked during encounter/event.") : "Explored terrain. Must be adjacent to move here.";
                            std::string title = (label == "#") ? "Explored Wall" : (label == "+" ? "Explored Door" : (isWarp ? "Zone Transition" : "Explored Floor"));
                            TooltipManager::setHoverTooltip(tileRect, mousePos, title, desc, std::format("Grid ({}, {})", targetX, targetY));
                        }
                    }
                    else if (t.discovery == STATE_PARTIAL)
                    {
                        // Partially discovered (stood adjacent to)
                        if (isDangerous)
                        {
                            tileColor = { 38, 18, 23, 255 };
                            borderColor = { 150, 48, 54, 255 };
                            textCol = (label == "W") ? Theme::colors.companion : (label == "+" ? Theme::colors.textGold : Theme::colors.enemy);

                            std::string desc = isAdjacent ? (isExploration ? "Click to navigate. Signs of threat or danger surveyed!" : "Surveyed hazard. Movement locked during encounter/event.") : "Surveyed hazard tile. Must be adjacent to move here.";
                            std::string title = (label == "#") ? "Surveyed Wall" : (label == "+" ? "Surveyed Door" : (isWarp ? "Surveyed Passage" : "Surveyed Hazard"));
                            TooltipManager::setHoverTooltip(tileRect, mousePos, title, desc, std::format("Grid ({}, {}) [Hazard]", targetX, targetY));
                        }
                        else
                        {
                            tileColor = { 26, 30, 42, 255 };
                            borderColor = Theme::colors.borderMuted;
                            textCol = (label == "W") ? Theme::colors.companion : (label == "+" ? Theme::colors.textGold : Theme::colors.textSecondary);

                            std::string desc = isAdjacent ? (isExploration ? "Click to navigate to this adjacent tile." : "Surveyed tile. Movement locked during encounter/event.") : "Surveyed terrain tile. Must be adjacent to move here.";
                            std::string title = (label == "#") ? "Surveyed Wall" : (label == "+" ? "Surveyed Door" : (isWarp ? "Surveyed Passage" : "Surveyed Floor"));
                            TooltipManager::setHoverTooltip(tileRect, mousePos, title, desc, std::format("Grid ({}, {})", targetX, targetY));
                        }
                    }
                    else
                    {
                        // Undiscovered (STATE_HIDDEN): Dark, but still visible
                        if (isDangerous)
                        {
                            tileColor = { 28, 14, 18, 255 };
                            borderColor = { 78, 28, 33, 255 };
                            label = "!";
                            textCol = { 155, 54, 58, 255 };

                            TooltipManager::setHoverTooltip(tileRect, mousePos, "Unexplored Hazard", "Terrain appears treacherous or ominous. Proceed with caution.", std::format("Grid ({}, {})", targetX, targetY));
                        }
                        else
                        {
                            tileColor = { 16, 17, 23, 255 };
                            borderColor = { 32, 34, 46, 255 };
                            textCol = { 75, 80, 95, 255 };

                            TooltipManager::setHoverTooltip(tileRect, mousePos, "Undiscovered Area", "Terrain tile not yet approached. Move closer to inspect.", std::format("Grid ({}, {})", targetX, targetY));
                        }
                    }

                    // Handle adjacent navigation clicking
                    bool tileHovered = (mousePos.x >= tileRect.x && mousePos.x <= tileRect.x + tileRect.w &&
                                        mousePos.y >= tileRect.y && mousePos.y <= tileRect.y + tileRect.h);
                    if (tileHovered && isAdjacent && isExploration && walkable)
                    {
                        borderColor = isDangerous ? Theme::colors.enemy : Theme::colors.textGold;
                        if (clicked)
                        {
                            gameContext->movePlayer(targetX, targetY);
                            gameContext->input.consumeMouseClick();
                        }
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

        // Quick Navigation Toolbar (Inv, Phone, Main Menu)
        SidebarGeometry::renderNavigationToolbar(renderer, gameContext, rect, startY, uiScale);

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
