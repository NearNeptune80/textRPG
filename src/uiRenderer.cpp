#include "game.h"
#include "uiRenderer.h"
#include "uiWidget.h"

void game::updateLayoutBounds(int w, int h)
{
    int padding = 12;
    int topBarH = (int)(h * 0.05f);
    int mapSize = (int)(h * 0.30f);

    int colStartY = padding + topBarH + padding;
    int colEndY = h - padding;

    int leftColW = mapSize;
    int rightColW = mapSize;
    int centerColW = w - (leftColW + rightColW + (4 * padding));

    int leftX = padding;
    int centerX = leftX + leftColW + padding;

    layout.mapRect = { leftX, colEndY - mapSize, mapSize, mapSize };

    int timeH = (int)(mapSize * 0.28f);
    int timeY = colEndY - mapSize - padding - timeH;
    layout.timeRect = { leftX, timeY, leftColW, timeH };

    int charH = (int)(mapSize * 0.82f);
    layout.charRect = { leftX, colStartY, leftColW, charH };

    int midY = colStartY + charH + padding;
    int midH = timeY - padding - midY;
    layout.companionRect = { leftX, midY, leftColW, midH };

    int btnH = (int)(h * 0.15f);
    layout.textMainRect = { centerX, colStartY, centerColW, (colEndY - btnH - padding) - colStartY };
    layout.actionGridRect = { (float)centerX, (float)(colEndY - btnH), (float)centerColW, (float)btnH };

    layout.equipRect = layout.mapRect;
    layout.inventoryRect = layout.textMainRect;
}

void game::renderDashboardLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) return;

    updateLayoutBounds(w, h);

    int padding = 12;
    int topBarH = (int)(h * 0.05f);

    int leftX = layout.charRect.x;
    int leftW = layout.charRect.w;
    int centerX = layout.textMainRect.x;
    int centerW = layout.textMainRect.w;
    int rightX = layout.textMainRect.x + layout.textMainRect.w + padding;
    int rightW = leftW;

    SDL_Rect slotTitle1 = { leftX, padding, leftW, topBarH };
    SDL_Rect slotTitle2 = { centerX, padding, centerW, topBarH };
    SDL_Rect slotTitle3 = { rightX, padding, rightW, topBarH };

    int rightColW = layout.mapRect.w;
    int colStartY = layout.charRect.y;
    int colEndY = h - padding;
    int rightAvailableH = colEndY - colStartY;
    int rightStackH = (rightAvailableH - (2 * padding)) / 3;

    SDL_Rect slotTopRight = { rightX, colStartY, rightColW, rightStackH };
    SDL_Rect slotMidRight = { rightX, colStartY + rightStackH + padding, rightColW, rightStackH };
    SDL_Rect slotBotRight = { rightX, colStartY + (rightStackH + padding) * 2, rightColW, rightAvailableH - (rightStackH * 2 + padding * 2) };

    renderTitleBar(slotTitle1, slotTitle2, slotTitle3);
    renderCharacterPanel({ (float)layout.charRect.x, (float)layout.charRect.y, (float)layout.charRect.w, (float)layout.charRect.h }, Player);
    renderCompanionPanel(layout.companionRect);
    renderTimePanel(layout.timeRect);
    renderActionGrid(layout.actionGridRect);

    switch (currentState)
    {
        case GameState::EVENT:
        case GameState::EXPLORATION:
            renderMapPanel(layout.mapRect, padding);
            renderTextPanel(layout.textMainRect);
            renderRightColumn(slotTopRight, slotMidRight, slotBotRight);
            break;

        case GameState::INVENTORY:
            renderEquipmentPanel(layout.equipRect, padding);
            renderInventoryPanel(layout.inventoryRect);
            renderRightColumn(slotTopRight, slotMidRight, slotBotRight);
            break;

        default: break;
    }

    float charPadX = layout.charRect.w * 0.04f;
    float charPadY = layout.charRect.h * 0.04f;
    float avatarSize = layout.charRect.h * 0.16f;

    SDL_FRect avatarScreenRect = {
        (float)layout.charRect.x + charPadX,
        (float)layout.charRect.y + charPadY,
        avatarSize,
        avatarSize
    };

    float winX, winY, mouseX, mouseY;
    SDL_GetMouseState(&winX, &winY);
    SDL_RenderCoordinatesFromWindow(renderer, winX, winY, &mouseX, &mouseY);

    if (UIGridHelper::contains(avatarScreenRect, mouseX, mouseY))
    {
        renderAnatomyTooltip(mouseX + 15.0f, mouseY + 15.0f);
    }
}

void game::renderMainMenuLayout()
{
    int w, h;
    SDL_RendererLogicalPresentation mode;
    if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode)) return;

    SDL_Rect menuRect = { w / 4, h / 4, w / 2, h / 2 };
    ViewportGuard vpGuard(renderer, &menuRect);

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)menuRect.w, (float)menuRect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 20, 60, 80, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 120, 160, 255 });
}

void game::renderTitleBar(SDL_Rect t1, SDL_Rect t2, SDL_Rect t3)
{
    SDL_Rect boxes[3] = { t1, t2, t3 };
    for (int i = 0; i < 3; i++)
    {
        ViewportGuard vpGuard(renderer, &boxes[i]);
        SDL_FRect r = { 0.0f, 0.0f, (float)boxes[i].w, (float)boxes[i].h };
        renderFillRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 45, 45, 52, 255 });
        renderDrawRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 65, 65, 75, 255 });
    }

    if (activeTargetNPC && activeTargetMode != TargetMode::NONE)
    {
        ViewportGuard vpGuard(renderer, &t3);
        SDL_Color headerColor = { 255, 100, 150, 255 };
        std::string headerTitle = "Enemy";

        if (activeTargetMode == TargetMode::DIALOGUE)
        {
            headerColor = { 100, 210, 255, 255 };
            headerTitle = "Interacting With";
        }
        else if (activeTargetMode == TargetMode::COMPANION)
        {
            headerColor = { 120, 240, 150, 255 };
            headerTitle = "Ally";
        }

        SDL_FRect titleRect = { 0.0f, 0.0f, (float)t3.w, (float)t3.h };
        drawTextFit(headerTitle, titleRect, headerColor, "title_font");
    }
}

void game::renderCompanionPanel(SDL_Rect rect)
{
    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });
}

void game::renderCharacterPanel(SDL_FRect rect, entity* playerObj)
{
    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, rect.w, rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });

    if (!playerObj) return;

    float padX = rect.w * 0.04f;
    float padY = rect.h * 0.04f;
    float contentW = rect.w - (padX * 2.0f);
    float currentY = padY;
    float dividerGap = rect.h * 0.025f;

    auto drawHorizontalDivider = [&](float y)
        {
            SDL_SetRenderDrawColor(renderer, 50, 46, 55, 255);
            SDL_RenderLine(renderer, padX, y, rect.w - padX, y);
        };

    float avatarSize = rect.h * 0.16f;
    SDL_FRect avatarRect = { padX, currentY, avatarSize, avatarSize };
    renderFillRoundedRect(renderer, avatarRect, 4.0f, { 50, 50, 60, 255 });
    renderDrawRoundedRect(renderer, avatarRect, 4.0f, { 90, 90, 105, 255 });

    float headerTextX = avatarRect.x + avatarSize + (padX * 0.8f);
    std::string headerStr = playerObj->name + " - Level " + std::to_string(playerObj->stats.level);
    float headerTextH = avatarSize * 0.55f;
    SDL_FRect headerTextRect = { headerTextX, currentY, rect.w - headerTextX - padX, headerTextH };
    drawTextFit(headerStr, headerTextRect, { 160, 200, 255, 255 });

    float xpBarY = currentY + headerTextH + (padY * 0.3f);
    float xpBarW = rect.w - headerTextX - padX;
    float xpBarH = avatarSize * 0.18f;

    SDL_FRect xpBg = { headerTextX, xpBarY, xpBarW, xpBarH };
    renderFillRoundedRect(renderer, xpBg, 3.0f, { 20, 18, 25, 255 });

    float currentXp = playerObj->stats.getBaseStat("xp");
    float xpFillPct = std::clamp(currentXp / 100.0f, 0.0f, 1.0f);
    SDL_FRect xpFill = { headerTextX, xpBarY, xpBarW * xpFillPct, xpBarH };
    renderFillRoundedRect(renderer, xpFill, 3.0f, { 80, 200, 230, 255 });

    currentY += avatarSize + dividerGap;
    drawHorizontalDivider(currentY);
    currentY += dividerGap;

    float halfWidth = contentW / 2.0f;
    float currencyH = rect.h * 0.08f;
    drawTextFit("¤ " + std::to_string((int)playerObj->stats.getBaseStat("currency")), { padX, currentY, halfWidth, currencyH }, { 255, 215, 0, 255 });
    drawTextFit("★ " + std::to_string((int)playerObj->stats.getBaseStat("gems")), { padX + halfWidth, currentY, halfWidth, currencyH }, { 255, 100, 220, 255 });

    currentY += currencyH + dividerGap;
    drawHorizontalDivider(currentY);
    currentY += dividerGap;

    float colWidth = contentW / 3.0f;
    float miniStatH = rect.h * 0.09f;

    auto drawMiniStat = [&](int colIndex, const std::string& statName, SDL_Color textColor)
        {
            float colX = padX + (colIndex * colWidth);
            SDL_FRect iconBox = { colX, currentY, miniStatH, miniStatH };
            renderFillRoundedRect(renderer, iconBox, 3.0f, { 45, 42, 50, 255 });

            int val = (int)playerObj->stats.getBaseStat(statName);
            SDL_FRect valRect = { colX + miniStatH + 4.0f, currentY, colWidth - miniStatH - 4.0f, miniStatH };
            drawTextFit(std::to_string(val), valRect, textColor);
        };

    drawMiniStat(0, "physique", { 255, 50, 120, 255 });
    drawMiniStat(1, "arcane", { 180, 110, 255, 255 });
    drawMiniStat(2, "corruption", { 100, 200, 255, 255 });

    currentY += miniStatH + dividerGap;
    drawHorizontalDivider(currentY);
    currentY += dividerGap;

    float barHeight = rect.h * 0.065f;
    float iconRadius = barHeight * 1.20f;
    float valueTextWidth = contentW * 0.18f;
    float barW = contentW - iconRadius - valueTextWidth - (padX * 0.5f);
    float barGap = rect.h * 0.02f;

    auto drawVitalBar = [&](float y, const std::string& statName, float maxVal, SDL_Color barColor)
        {
            SDL_FRect iconRect = { padX, y, iconRadius, iconRadius };
            renderFillRoundedRect(renderer, iconRect, 3.0f, { 45, 40, 50, 255 });

            float fillX = padX + iconRadius + (padX * 0.5f);
            float barY = y + (iconRadius - barHeight) / 2.0f;

            SDL_FRect bgRect = { fillX, barY, barW, barHeight };
            renderFillRoundedRect(renderer, bgRect, 3.0f, { 20, 18, 25, 255 });

            float currentVal = playerObj->stats.getBaseStat(statName);
            float fillPct = std::clamp(currentVal / maxVal, 0.0f, 1.0f);
            if (fillPct > 0.0f)
            {
                SDL_FRect fillRect = { fillX, barY, barW * fillPct, barHeight };
                renderFillRoundedRect(renderer, fillRect, 3.0f, barColor);
            }

            SDL_FRect textRect = { fillX + barW + (padX * 0.4f), y, valueTextWidth, iconRadius };
            drawTextFit(std::to_string((int)currentVal), textRect, { 240, 240, 240, 255 });
        };

    drawVitalBar(currentY, "health", 100.0f, { 255, 60, 90, 255 });
    currentY += iconRadius + barGap;

    drawVitalBar(currentY, "mana", 100.0f, { 220, 130, 255, 255 });
    currentY += iconRadius + barGap;

    drawVitalBar(currentY, "lust", 100.0f, { 230, 50, 150, 255 });
}

void game::renderTimePanel(SDL_Rect rect)
{
    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });

    float padX = rect.w * 0.04f;
    float gapX = rect.w * 0.05f;

    float leftColX = padX;
    float leftColW = (rect.w - (2.0f * padX) - gapX) * 0.54f;
    float rightColX = leftColX + leftColW + gapX;
    float rightColW = rect.w - rightColX - padX;

    float topHeaderY = rect.h * 0.10f;
    float headerH = rect.h * 0.32f;
    float bottomWidgetY = topHeaderY + headerH + (rect.h * 0.06f);
    float bottomWidgetH = rect.h * 0.44f;

    SDL_FRect dateBox = { leftColX, topHeaderY, leftColW, headerH };
    drawTextFit(gameTime.getFormattedDate(), dateBox, { 220, 225, 240, 255 });

    const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
    float daySlotW = leftColW / 7.0f;

    SDL_FRect trackBg = { leftColX, bottomWidgetY, leftColW, bottomWidgetH };
    renderFillRoundedRect(renderer, trackBg, 4.0f, { 20, 18, 24, 255 });
    renderDrawRoundedRect(renderer, trackBg, 4.0f, { 48, 44, 56, 255 });

    SDL_SetRenderDrawColor(renderer, 38, 35, 46, 255);
    for (int i = 1; i < 7; i++)
    {
        float lineX = leftColX + (i * daySlotW);
        SDL_RenderLine(renderer, lineX, bottomWidgetY + 2.0f, lineX, bottomWidgetY + bottomWidgetH - 2.0f);
    }

    for (int i = 0; i < 7; i++)
    {
        float slotX = leftColX + (i * daySlotW);
        SDL_FRect textFitRect = { slotX + 1.0f, bottomWidgetY + (bottomWidgetH * 0.15f), daySlotW - 2.0f, bottomWidgetH * 0.70f };

        if (i == gameTime.dayOfWeek)
        {
            float pillMargin = 2.0f;
            SDL_FRect pillRect = { slotX + pillMargin, bottomWidgetY + pillMargin, daySlotW - (pillMargin * 2.0f), bottomWidgetH - (pillMargin * 2.0f) };
            renderFillRoundedRect(renderer, pillRect, 3.0f, { 70, 60, 95, 255 });
            renderDrawRoundedRect(renderer, pillRect, 3.0f, { 160, 140, 210, 255 });
            drawTextFit(days[i], textFitRect, { 255, 255, 255, 255 });
        }
        else
        {
            drawTextFit(days[i], textFitRect, { 110, 110, 125, 255 });
        }
    }

    SDL_FRect timeBox = { rightColX, topHeaderY, rightColW, headerH };
    drawTextFit(gameTime.getFormattedTime(), timeBox, { 255, 220, 130, 255 });

    float barW = rightColW;
    float barX = rightColX;
    float barH = std::clamp((float)rect.h * 0.18f, 6.0f, 12.0f);
    float barY = bottomWidgetY + (bottomWidgetH - barH) * 0.5f;

    float sunrisePct = gameTime.getSunriseHour() / 24.0f;
    float sunsetPct = gameTime.getSunsetHour() / 24.0f;

    SDL_FRect nightPre = { barX, barY, barW * sunrisePct, barH };
    renderFillRoundedRect(renderer, nightPre, 2.0f, { 55, 55, 85, 255 });

    SDL_FRect dayLight = { barX + (barW * sunrisePct), barY, barW * (sunsetPct - sunrisePct), barH };
    renderFillRoundedRect(renderer, dayLight, 2.0f, { 140, 185, 225, 255 });

    SDL_FRect nightPost = { barX + (barW * sunsetPct), barY, barW * (1.0f - sunsetPct), barH };
    renderFillRoundedRect(renderer, nightPost, 2.0f, { 55, 55, 85, 255 });

    SDL_FRect barOutline = { barX, barY, barW, barH };
    renderDrawRoundedRect(renderer, barOutline, 2.0f, { 20, 18, 25, 255 });

    float currentTimePct = gameTime.getDayProgress();
    float needleX = barX + (barW * currentTimePct);
    float needleW = std::max(2.0f, (float)rect.w * 0.006f);

    SDL_FRect needle = { needleX - (needleW / 2.0f), barY - 2.0f, needleW, barH + 4.0f };
    renderFillRoundedRect(renderer, needle, 1.0f, { 255, 255, 255, 255 });
}

void game::renderMapPanel(SDL_Rect rect, int padding)
{
    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 25, 25, 28, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 55, 55, 65, 255 });

    if (!map) return;

    int mapW = map->getWidth();
    int mapH = map->getHeight();

    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            int mapX = gridX + x;
            int mapY = gridY + y;

            if (mapX < 0 || mapX >= mapW || mapY < 0 || mapY >= mapH) continue;

            Tile t = map->getTile(mapX, mapY);
            if (t.type == TILE_VOID || t.type == TILE_WALL || t.discovery == STATE_HIDDEN) continue;

            int renderX = x + 2;
            int renderY = y + 2;

            SDL_FRect r = UIGridHelper::getMapTileRect({ 0, 0, rect.w, rect.h }, renderX, renderY, padding);
            int danger = map->getRuntimeData(mapX, mapY).getEffectiveDangerLevel();

            if (t.discovery == STATE_PARTIAL)
            {
                SDL_Color col = (danger > 0) ? SDL_Color{ 45, 40, 50, 255 } : SDL_Color{ 70, 70, 80, 255 };
                renderFillRoundedRect(renderer, r, 3.0f, col);
            }
            else if (t.type == TILE_FLOOR || t.type == TILE_DOOR)
            {
                SDL_Color col = (danger > 0) ? SDL_Color{ 100, 95, 110, 255 } : SDL_Color{ 210, 210, 215, 255 };
                renderFillRoundedRect(renderer, r, 3.0f, col);

                if (t.type == TILE_DOOR)
                {
                    renderDrawRoundedRect(renderer, r, 3.0f, { 240, 180, 50, 255 });
                }
            }
        }
    }

    SDL_FRect p = UIGridHelper::getMapTileRect({ 0, 0, rect.w, rect.h }, 2, 2, padding);
    renderFillRoundedRect(renderer, p, 3.0f, { 0, 200, 255, 255 });
}

void game::renderEquipmentPanel(SDL_Rect rect, int padding, entity* targetEntity)
{
    if (!targetEntity) targetEntity = Player;

    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 25, 20, 30, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 100, 50, 150, 255 });

    int cols = 6, rows = 6;
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int slotIdx = (r * cols) + c;
            SDL_FRect slot = UIGridHelper::getEquipmentSlotRect({ 0, 0, rect.w, rect.h }, c, r, cols, rows, 4, padding);

            bool isOccupied = false;
            std::string equippedName = "";
            bool isSelectedEquip = false;

            if (targetEntity)
            {
                for (const auto& [eSlot, eqItem] : targetEntity->inventory.equipped)
                {
                    if (getEquipmentGridIndex(eSlot) == slotIdx && !eqItem->id.empty())
                    {
                        isOccupied = true;
                        equippedName = eqItem->name;
                        if (targetEntity == Player && eSlot == selectedEquipmentSlot) isSelectedEquip = true;
                        break;
                    }
                }
            }

            SDL_Color fillCol = isOccupied ? SDL_Color{ 100, 60, 160, 255 } : SDL_Color{ 45, 40, 50, 255 };
            renderFillRoundedRect(renderer, slot, 4.0f, fillCol);

            if (isSelectedEquip)
            {
                renderDrawRoundedRect(renderer, slot, 4.0f, { 255, 215, 0, 255 });
            }
            else
            {
                renderDrawRoundedRect(renderer, slot, 4.0f, { 80, 75, 95, 255 });
            }

            if (isOccupied && !equippedName.empty())
            {
                drawTextFit(equippedName, slot, { 255, 215, 0, 255 });
            }
        }
    }
}

void game::renderInventoryPanel(SDL_Rect rect)
{
    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 35, 35, 45, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 60, 75, 255 });

    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderLine(renderer, rect.w / 2.0f, 20.0f, rect.w / 2.0f, rect.h - 20.0f);

    int cols = 6, rows = 5;
    int maxSlots = cols * rows * 2;

    for (int i = 0; i < maxSlots; i++)
    {
        SDL_FRect slot = UIGridHelper::getInventorySlotRect({ 0, 0, rect.w, rect.h }, i, cols, rows);
        bool hasItem = (Player != nullptr) && (i < (int)Player->inventory.backpack.size());

        SDL_Color bgCol = hasItem ? SDL_Color{ 60, 70, 90, 255 } : SDL_Color{ 50, 50, 60, 255 };
        renderFillRoundedRect(renderer, slot, 4.0f, bgCol);

        if (i == selectedInventoryIndex)
        {
            renderDrawRoundedRect(renderer, slot, 4.0f, { 255, 215, 0, 255 });
        }
        else
        {
            renderDrawRoundedRect(renderer, slot, 4.0f, { 70, 70, 85, 255 });
        }

        if (hasItem)
        {
            std::string displayName = Player->inventory.backpack[i]->name;
            if (displayName.length() > 8) displayName = displayName.substr(0, 7) + ".";

            SDL_FRect textSlot = { slot.x + 2.0f, slot.y + 2.0f, slot.w - 4.0f, slot.h - 4.0f };
            drawTextFit(displayName, textSlot, { 255, 255, 255, 255 });
        }
    }
}

void game::renderTextPanel(SDL_Rect rect)
{
    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, (float)rect.w, (float)rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 40, 40, 40, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 65, 65, 65, 255 });

    if (currentState == GameState::EVENT)
    {
        SDL_FRect nameRect = { 0.0f, 0.0f, (float)rect.w, 40.0f };
        renderTextCentered(currentScene.speakerName, nameRect, "title_font", { 255, 200, 100, 255 });

        SDL_FRect bodyRect = { 0.0f, 40.0f, (float)rect.w, (float)rect.h - 40.0f };
        renderTextWrapped(currentScene.bodyText, bodyRect, "button_font", { 220, 220, 220, 255 });
    }
}

void game::renderRightColumn(SDL_Rect top, SDL_Rect mid, SDL_Rect bot)
{
    if (activeTargetNPC && activeTargetMode != TargetMode::NONE)
    {
        float rightX = (float)top.x;
        float rightY = (float)layout.charRect.y;
        float rightW = (float)top.w;

        float cardH = (float)layout.charRect.h;
        renderNPCTargetPanel(rightX, rightY, rightW, cardH);

        SDL_Rect equipBox = { (int)rightX, layout.mapRect.y, (int)rightW, layout.mapRect.h };
        renderEquipmentPanel(equipBox, 12, activeTargetNPC);

        float avatarSize = cardH * 0.16f;
        float charPadX = rightW * 0.04f;
        float charPadY = cardH * 0.04f;

        SDL_FRect avatarRect = {
            rightX + charPadX,
            rightY + charPadY,
            avatarSize,
            avatarSize
        };

        float winX, winY, mouseX, mouseY;
        SDL_GetMouseState(&winX, &winY);
        SDL_RenderCoordinatesFromWindow(renderer, winX, winY, &mouseX, &mouseY);

        if (UIGridHelper::contains(avatarRect, mouseX, mouseY))
        {
            renderNPCAnatomyTooltip(mouseX, mouseY);
        }
        return;
    }

    SDL_Rect boxes[3] = { top, mid, bot };
    for (int i = 0; i < 3; i++)
    {
        ViewportGuard vpGuard(renderer, &boxes[i]);
        SDL_FRect r = { 0.0f, 0.0f, (float)boxes[i].w, (float)boxes[i].h };
        renderFillRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 40, 40, 40, 255 });
        renderDrawRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 65, 65, 65, 255 });
    }
}

void game::renderActionGrid(SDL_FRect rect)
{
    ViewportGuard vpGuard(renderer, &rect);

    SDL_FRect panelRect = { 0.0f, 0.0f, rect.w, rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 25, 25, 30, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 50, 50, 60, 255 });

    int cols = 5, rows = 3;
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int index = (r * cols) + c;
            SDL_FRect btn = UIGridHelper::getActionButtonRect({ 0.0f, 0.0f, rect.w, rect.h }, c, r, cols, rows);

            if (index < (int)activeButtons.size())
            {
                renderFillRoundedRect(renderer, btn, 4.0f, { 70, 100, 140, 255 });
                renderTextCentered(activeButtons[index].label, btn, "button_font");
            }
            else
            {
                renderFillRoundedRect(renderer, btn, 4.0f, { 40, 40, 45, 255 });
            }
            renderDrawRoundedRect(renderer, btn, 4.0f, { 60, 60, 70, 255 });
        }
    }
}

void game::renderNPCTargetPanel(float x, float y, float w, float h)
{
    if (!activeTargetNPC) return;

    SDL_Rect panelBox = { (int)x, (int)y, (int)w, (int)h };
    ViewportGuard vpGuard(renderer, &panelBox);

    SDL_FRect cardRect = { 0.0f, 0.0f, w, h };
    renderFillRoundedRect(renderer, cardRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, cardRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });

    float padX = w * 0.04f;
    float padY = h * 0.04f;
    float contentW = w - (padX * 2.0f);
    float currentY = padY;
    float dividerGap = h * 0.025f;

    float avatarSize = h * 0.16f;
    SDL_FRect avatarRect = { padX, currentY, avatarSize, avatarSize };
    renderFillRoundedRect(renderer, avatarRect, 4.0f, { 50, 50, 60, 255 });
    renderDrawRoundedRect(renderer, avatarRect, 4.0f, { 255, 120, 170, 255 });

    float headerTextX = avatarRect.x + avatarSize + (padX * 0.8f);
    std::string nameLevelStr = activeTargetNPC->name + " - Level " + std::to_string(activeTargetNPC->stats.level);
    float headerTextH = avatarSize * 0.55f;

    SDL_FRect nameRect = { headerTextX, currentY, w - headerTextX - padX, headerTextH };
    drawTextFit(nameLevelStr, nameRect, { 255, 120, 170, 255 }, "title_font");

    currentY += avatarSize + dividerGap;

    float barHeight = h * 0.065f;
    float iconRadius = barHeight * 1.20f;
    float valueTextWidth = contentW * 0.18f;
    float barW = contentW - iconRadius - valueTextWidth - (padX * 0.5f);
    float barGap = h * 0.02f;

    auto drawVitalBar = [&](float barYPos, const std::string& statName, float maxVal, SDL_Color barColor)
        {
            SDL_FRect iconRect = { padX, barYPos, iconRadius, iconRadius };
            renderFillRoundedRect(renderer, iconRect, 3.0f, { 45, 40, 50, 255 });

            float fillX = padX + iconRadius + (padX * 0.5f);
            float fillY = barYPos + (iconRadius - barHeight) / 2.0f;

            SDL_FRect bgRect = { fillX, fillY, barW, barHeight };
            renderFillRoundedRect(renderer, bgRect, 3.0f, { 20, 18, 25, 255 });

            float currentVal = activeTargetNPC->getStat(statName);
            float fillPct = std::clamp(currentVal / maxVal, 0.0f, 1.0f);
            if (fillPct > 0.0f)
            {
                SDL_FRect fillRect = { fillX, fillY, barW * fillPct, barHeight };
                renderFillRoundedRect(renderer, fillRect, 3.0f, barColor);
            }

            SDL_FRect textRect = { fillX + barW + (padX * 0.4f), barYPos, valueTextWidth, iconRadius };
            drawTextFit(std::to_string((int)currentVal), textRect, { 240, 240, 240, 255 });
        };

    drawVitalBar(currentY, "health", 100.0f, { 255, 60, 90, 255 });
    currentY += iconRadius + barGap;

    drawVitalBar(currentY, "mana", 100.0f, { 220, 130, 255, 255 });
    currentY += iconRadius + barGap;

    drawVitalBar(currentY, "lust", 100.0f, { 230, 50, 150, 255 });
}

void game::renderNPCAnatomyTooltip(float mouseX, float mouseY)
{
    if (!activeTargetNPC) return;

    int screenW = 1280, screenH = 720;
    SDL_GetRenderOutputSize(renderer, &screenW, &screenH);

    static const std::vector<bodySlot> anatomicalOrder = {
        bodySlot::HAIR, bodySlot::HEAD, bodySlot::EYES, bodySlot::EARS, bodySlot::HORNS,
        bodySlot::MOUTH, bodySlot::NECK, bodySlot::TORSO, bodySlot::BREASTS, bodySlot::STOMACH,
        bodySlot::BACK, bodySlot::ARMS, bodySlot::HANDS, bodySlot::FINGERS, bodySlot::HIPS,
        bodySlot::GROIN, bodySlot::ASS, bodySlot::TAIL, bodySlot::LEGS, bodySlot::FEET,
        bodySlot::WINGS, bodySlot::TENTACLES, bodySlot::ANTENNAE
    };

    float headerH = screenH * 0.032f;
    float subHeaderH = screenH * 0.024f;
    float lineH = screenH * 0.025f;
    float fontH = lineH * 0.80f;
    float padding = screenW * 0.008f;
    float bulletSize = fontH * 0.45f;

    struct RenderRowData
    {
        bool isOccupied = false;
        SDL_Color bulletColor = { 100, 100, 110, 255 };
        std::vector<ColorToken> tokens;
    };

    std::vector<RenderRowData> rows;
    float maxContentW = screenW * 0.18f;

    for (bodySlot slot : anatomicalOrder)
    {
        RenderRowData row;
        const bodyPart* part = activeTargetNPC->anatomy.getPart(slot);

        if (part != nullptr)
        {
            row.isOccupied = true;
            row.bulletColor = getColorFromName(part->primaryColor);

            std::string prefixStr = "";
            if (slot == bodySlot::GROIN && part->length > 0.0f)
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "%s (%gcm long, %gcm diameter)", part->name.c_str(), part->length, part->diameter);
                prefixStr = std::string(buf);
            }
            else if (slot == bodySlot::BREASTS && part->cupSize > 0)
            {
                prefixStr = part->name + " (" + bodyPart::getCupSizeName(part->cupSize) + "-cup)";
            }
            else
            {
                if (part->count == 2) prefixStr = "Two ";
                else if (part->count > 2) prefixStr = std::to_string(part->count) + " ";

                if (!part->style.empty()) prefixStr += part->style + " ";
                prefixStr += part->name;
            }

            std::string coveringNoun = getCoveringNoun(part->covering);

            row.tokens.push_back({ prefixStr + ": ", { 220, 220, 230, 255 } });
            row.tokens.push_back({ part->race + " - ", { 180, 100, 255, 255 } });

            if (!part->secondaryColor.empty())
            {
                row.tokens.push_back({ part->secondaryColor, getColorFromName(part->secondaryColor) });
                row.tokens.push_back({ "-rimmed, ", { 220, 220, 230, 255 } });
            }

            row.tokens.push_back({ part->primaryColor + " ", row.bulletColor });
            row.tokens.push_back({ coveringNoun, { 200, 200, 210, 255 } });
        }
        else
        {
            row.isOccupied = false;
            row.bulletColor = { 65, 65, 75, 255 };

            std::string slotLabel = getSlotName(slot);
            row.tokens.push_back({ slotLabel + ": ", { 110, 110, 125, 255 } });
            row.tokens.push_back({ "None", { 140, 140, 150, 255 } });
        }

        float rowW = 0.0f;
        for (const auto& tok : row.tokens)
        {
            float srcW = 0.0f, srcH = 0.0f;
            getOrRenderText(tok.text, "button_font", tok.color, srcW, srcH);
            if (srcH > 0.0f) rowW += srcW * (fontH / srcH);
        }

        if (rowW > maxContentW) maxContentW = rowW;
        rows.push_back(row);
    }

    float textStartXOffset = bulletSize + (padding * 0.8f);
    float boxWidth = maxContentW + textStartXOffset + (padding * 2.5f);

    int itemLines = (int)rows.size();
    float boxHeight = headerH + subHeaderH + padding + (itemLines * lineH) + padding;

    float boxX = mouseX - boxWidth - 12.0f;
    float boxY = mouseY;

    if (boxX < 10.0f) boxX = mouseX + 12.0f;
    if (boxY + boxHeight > screenH) boxY = screenH - boxHeight - 10.0f;

    SDL_FRect tooltipRect = { boxX, boxY, boxWidth, boxHeight };

    renderFillRoundedRect(renderer, tooltipRect, 6.0f, { 25, 23, 30, 250 });
    renderDrawRoundedRect(renderer, tooltipRect, 6.0f, { 255, 120, 170, 255 });

    SDL_FRect titleRect = { boxX + padding, boxY + padding, boxWidth - (padding * 2.0f), headerH };
    drawTextFit(activeTargetNPC->name, titleRect, { 255, 120, 170, 255 }, "title_font");

    char subTitleBuffer[128];
    snprintf(subTitleBuffer, sizeof(subTitleBuffer), "Masculine | Fit body | %.2fm tall", activeTargetNPC->anatomy.heightMeters);

    SDL_FRect subTitleRect = { boxX + padding, boxY + padding + headerH, boxWidth - (padding * 2.0f), subHeaderH };
    drawTextFit(subTitleBuffer, subTitleRect, { 100, 200, 255, 255 }, "button_font");

    float dividerY = boxY + padding + headerH + subHeaderH + (padding * 0.4f);
    SDL_SetRenderDrawColor(renderer, 60, 50, 75, 255);
    SDL_RenderLine(renderer, boxX + padding, dividerY, boxX + boxWidth - padding, dividerY);

    float currentY = dividerY + (padding * 0.4f);

    for (const auto& row : rows)
    {
        float textY = currentY + (lineH - fontH) * 0.5f;
        float bulletY = textY + (fontH - bulletSize) * 0.5f;

        SDL_FRect colorBullet = { boxX + padding, bulletY, bulletSize, bulletSize };
        renderFillRoundedRect(renderer, colorBullet, 2.0f, row.bulletColor);

        float textX = boxX + padding + textStartXOffset;
        renderTextLeftSegment(row.tokens, textX, textY, fontH, "button_font");

        currentY += lineH;
    }
}

float game::renderTextLeftSegment(const std::vector<ColorToken>& tokens, float startX, float startY, float maxH, const std::string& fontId)
{
    float currentX = startX;

    for (const auto& token : tokens)
    {
        if (token.text.empty()) continue;

        float srcW = 0.0f, srcH = 0.0f;
        SDL_Texture* texture = getOrRenderText(token.text, fontId, token.color, srcW, srcH);

        if (texture && srcH > 0.0f)
        {
            float scale = maxH / srcH;
            float drawW = srcW * scale;
            float drawH = maxH;

            SDL_FRect renderDst = { currentX, startY, drawW, drawH };
            SDL_RenderTexture(renderer, texture, NULL, &renderDst);

            currentX += drawW;
        }
    }

    return currentX - startX;
}

void game::renderAnatomyTooltip(float mouseX, float mouseY)
{
    if (!Player) return;

    int screenW = 1280, screenH = 720;
    SDL_GetRenderOutputSize(renderer, &screenW, &screenH);

    static const std::vector<bodySlot> anatomicalOrder = {
        bodySlot::HAIR, bodySlot::HEAD, bodySlot::EYES, bodySlot::EARS, bodySlot::HORNS,
        bodySlot::MOUTH, bodySlot::NECK, bodySlot::TORSO, bodySlot::BREASTS, bodySlot::STOMACH,
        bodySlot::BACK, bodySlot::ARMS, bodySlot::HANDS, bodySlot::FINGERS, bodySlot::HIPS,
        bodySlot::GROIN, bodySlot::ASS, bodySlot::TAIL, bodySlot::LEGS, bodySlot::FEET,
        bodySlot::WINGS, bodySlot::TENTACLES, bodySlot::ANTENNAE
    };

    float headerH = screenH * 0.032f;
    float subHeaderH = screenH * 0.024f;
    float lineH = screenH * 0.025f;
    float fontH = lineH * 0.80f;
    float padding = screenW * 0.008f;
    float bulletSize = fontH * 0.45f;

    struct RenderRowData
    {
        bool isOccupied = false;
        SDL_Color bulletColor = { 100, 100, 110, 255 };
        std::vector<ColorToken> tokens;
    };

    std::vector<RenderRowData> rows;
    float maxContentW = screenW * 0.18f;

    for (bodySlot slot : anatomicalOrder)
    {
        RenderRowData row;
        const bodyPart* part = Player->anatomy.getPart(slot);

        if (part != nullptr)
        {
            row.isOccupied = true;
            row.bulletColor = getColorFromName(part->primaryColor);

            std::string prefixStr = "";
            if (slot == bodySlot::GROIN && part->length > 0.0f)
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "%s (%gcm long, %gcm diameter)", part->name.c_str(), part->length, part->diameter);
                prefixStr = std::string(buf);
            }
            else if (slot == bodySlot::BREASTS && part->cupSize > 0)
            {
                prefixStr = part->name + " (" + bodyPart::getCupSizeName(part->cupSize) + "-cup)";
            }
            else
            {
                if (part->count == 2) prefixStr = "Two ";
                else if (part->count > 2) prefixStr = std::to_string(part->count) + " ";

                if (!part->style.empty()) prefixStr += part->style + " ";
                prefixStr += part->name;
            }

            std::string coveringNoun = getCoveringNoun(part->covering);

            row.tokens.push_back({ prefixStr + ": ", { 220, 220, 230, 255 } });
            row.tokens.push_back({ part->race + " - ", { 180, 100, 255, 255 } });

            if (!part->secondaryColor.empty())
            {
                row.tokens.push_back({ part->secondaryColor, getColorFromName(part->secondaryColor) });
                row.tokens.push_back({ "-rimmed, ", { 220, 220, 230, 255 } });
            }

            row.tokens.push_back({ part->primaryColor + " ", row.bulletColor });
            row.tokens.push_back({ coveringNoun, { 200, 200, 210, 255 } });
        }
        else
        {
            row.isOccupied = false;
            row.bulletColor = { 65, 65, 75, 255 };

            std::string slotLabel = getSlotName(slot);
            row.tokens.push_back({ slotLabel + ": ", { 110, 110, 125, 255 } });
            row.tokens.push_back({ "None", { 140, 140, 150, 255 } });
        }

        float rowW = 0.0f;
        for (const auto& tok : row.tokens)
        {
            float srcW = 0.0f, srcH = 0.0f;
            getOrRenderText(tok.text, "button_font", tok.color, srcW, srcH);
            if (srcH > 0.0f) rowW += srcW * (fontH / srcH);
        }

        if (rowW > maxContentW) maxContentW = rowW;
        rows.push_back(row);
    }

    float textStartXOffset = bulletSize + (padding * 0.8f);
    float boxWidth = maxContentW + textStartXOffset + (padding * 2.5f);

    int itemLines = (int)rows.size();
    float boxHeight = headerH + subHeaderH + padding + (itemLines * lineH) + padding;

    if (mouseX + boxWidth > screenW) mouseX = screenW - boxWidth - 10.0f;
    if (mouseY + boxHeight > screenH) mouseY = screenH - boxHeight - 10.0f;

    SDL_FRect tooltipRect = { mouseX, mouseY, boxWidth, boxHeight };

    renderFillRoundedRect(renderer, tooltipRect, 6.0f, { 25, 23, 30, 250 });
    renderDrawRoundedRect(renderer, tooltipRect, 6.0f, { 140, 110, 200, 255 });

    SDL_FRect titleRect = { mouseX + padding, mouseY + padding, boxWidth - (padding * 2.0f), headerH };
    drawTextFit("Human", titleRect, { 210, 100, 255, 255 }, "title_font");

    float heightMeters = Player->stats.getBaseStat("height");
    if (heightMeters <= 0.0f) heightMeters = 1.75f;

    char subTitleBuffer[128];
    snprintf(subTitleBuffer, sizeof(subTitleBuffer), "Masculine | Fit body | %.2fm tall", Player->anatomy.heightMeters);

    SDL_FRect subTitleRect = { mouseX + padding, mouseY + padding + headerH, boxWidth - (padding * 2.0f), subHeaderH };
    drawTextFit(subTitleBuffer, subTitleRect, { 100, 200, 255, 255 }, "button_font");

    float dividerY = mouseY + padding + headerH + subHeaderH + (padding * 0.4f);
    SDL_SetRenderDrawColor(renderer, 60, 50, 75, 255);
    SDL_RenderLine(renderer, mouseX + padding, dividerY, mouseX + boxWidth - padding, dividerY);

    float currentY = dividerY + (padding * 0.4f);

    for (const auto& row : rows)
    {
        float textY = currentY + (lineH - fontH) * 0.5f;
        float bulletY = textY + (fontH - bulletSize) * 0.5f;

        SDL_FRect colorBullet = { mouseX + padding, bulletY, bulletSize, bulletSize };
        renderFillRoundedRect(renderer, colorBullet, 2.0f, row.bulletColor);

        float textX = mouseX + padding + textStartXOffset;
        renderTextLeftSegment(row.tokens, textX, textY, fontH, "button_font");

        currentY += lineH;
    }
}