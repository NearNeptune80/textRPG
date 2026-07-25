#include "game.h"
#include "uiRenderer.h"
#include "uiWidget.h"

// --- Atomic Primitive Implementations ---

void UI::DrawProgressBar(SDL_Renderer* renderer, game* g, SDL_FRect bounds, float currentVal, float maxVal, SDL_Color fillColor, SDL_Color bgColor)
{
    renderFillRoundedRect(renderer, bounds, 3.0f, bgColor);
    float pct = std::clamp(currentVal / maxVal, 0.0f, 1.0f);
    if (pct > 0.0f)
    {
        SDL_FRect fillRect = { bounds.x, bounds.y, bounds.w * pct, bounds.h };
        renderFillRoundedRect(renderer, fillRect, 3.0f, fillColor);
    }
}

void UI::DrawVitalRow(SDL_Renderer* renderer, game* g, SDL_FRect bounds, float currentVal, float maxVal, SDL_Color barColor)
{
    float iconRadius = bounds.h;
    float barHeight = bounds.h * 0.80f;
    float padX = bounds.w * 0.02f;

    SDL_FRect iconRect = { bounds.x, bounds.y, iconRadius, iconRadius };
    renderFillRoundedRect(renderer, iconRect, 3.0f, { 45, 40, 50, 255 });

    float valueTextWidth = bounds.w * 0.18f;
    float barW = bounds.w - iconRadius - valueTextWidth - padX;
    float fillX = bounds.x + iconRadius + padX;
    float barY = bounds.y + (iconRadius - barHeight) / 2.0f;

    SDL_FRect bgRect = { fillX, barY, barW, barHeight };
    DrawProgressBar(renderer, g, bgRect, currentVal, maxVal, barColor);

    SDL_FRect textRect = { fillX + barW + padX, bounds.y, valueTextWidth, iconRadius };
    g->drawTextFit(std::to_string(static_cast<int>(currentVal)), textRect, { 240, 240, 240, 255 });
}

void UI::DrawEquipmentGrid(SDL_Renderer* renderer, game* g, SDL_FRect bounds, entity* targetEntity, equipSlot selectedSlot, int padding)
{
    SDL_Rect viewRect = { static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.w), static_cast<int>(bounds.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 25, 20, 30, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 100, 50, 150, 255 });

    int cols = 6, rows = 6;
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int slotIdx = (r * cols) + c;
            SDL_FRect slot = UIGridHelper::getEquipmentSlotRect(viewRect, c, r, cols, rows, 4, padding);

            // Adjust to viewport local coords
            slot.x -= bounds.x;
            slot.y -= bounds.y;

            bool isOccupied = false;
            std::string equippedName = "";
            bool isSelectedEquip = false;

            if (targetEntity)
            {
                for (const auto& [eSlot, eqItem] : targetEntity->inventory.equipped)
                {
                    if (g->getEquipmentGridIndex(eSlot) == slotIdx && eqItem && !eqItem->id.empty())
                    {
                        isOccupied = true;
                        equippedName = eqItem->name;
                        if (eSlot == selectedSlot) isSelectedEquip = true;
                        break;
                    }
                }
            }

            SDL_Color fillCol = isOccupied ? SDL_Color{ 100, 60, 160, 255 } : SDL_Color{ 45, 40, 50, 255 };
            renderFillRoundedRect(renderer, slot, GLOBAL_CORNER_RADIUS, fillCol);

            SDL_Color borderCol = isSelectedEquip ? SDL_Color{ 255, 215, 0, 255 } : SDL_Color{ 80, 75, 95, 255 };
            renderDrawRoundedRect(renderer, slot, GLOBAL_CORNER_RADIUS, borderCol);

            if (isOccupied && !equippedName.empty())
            {
                g->drawTextFit(equippedName, slot, { 255, 215, 0, 255 });
            }
        }
    }
}

void UI::DrawInventoryGrid(SDL_Renderer* renderer, game* g, SDL_FRect bounds, entity* targetEntity, int selectedIndex)
{
    SDL_Rect viewRect = { static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.w), static_cast<int>(bounds.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 35, 35, 45, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 60, 75, 255 });

    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderLine(renderer, bounds.w / 2.0f, 20.0f, bounds.w / 2.0f, bounds.h - 20.0f);

    int cols = 6, rows = 5;
    int maxSlots = cols * rows * 2;

    for (int i = 0; i < maxSlots; i++)
    {
        SDL_FRect slot = UIGridHelper::getInventorySlotRect(viewRect, i, cols, rows);
        slot.x -= bounds.x;
        slot.y -= bounds.y;

        bool hasItem = (targetEntity != nullptr) && (i < static_cast<int>(targetEntity->inventory.backpack.size()));

        SDL_Color bgCol = hasItem ? SDL_Color{ 60, 70, 90, 255 } : SDL_Color{ 50, 50, 60, 255 };
        renderFillRoundedRect(renderer, slot, GLOBAL_CORNER_RADIUS, bgCol);

        SDL_Color borderCol = (i == selectedIndex) ? SDL_Color{ 255, 215, 0, 255 } : SDL_Color{ 70, 70, 85, 255 };
        renderDrawRoundedRect(renderer, slot, GLOBAL_CORNER_RADIUS, borderCol);

        if (hasItem)
        {
            std::string displayName = targetEntity->inventory.backpack[i]->name;
            if (displayName.length() > 8) displayName = displayName.substr(0, 7) + ".";

            SDL_FRect textSlot = { slot.x + 2.0f, slot.y + 2.0f, slot.w - 4.0f, slot.h - 4.0f };
            g->drawTextFit(displayName, textSlot, { 255, 255, 255, 255 });
        }
    }
}

void UI::DrawEntitySummaryCard(SDL_Renderer* renderer, game* g, SDL_FRect bounds, entity* targetEntity, bool isEnemy)
{
    SDL_Rect viewRect = { static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.w), static_cast<int>(bounds.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    SDL_FRect cardRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    renderFillRoundedRect(renderer, cardRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, cardRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });

    if (!targetEntity) return;

    float padX = bounds.w * 0.04f;
    float padY = bounds.h * 0.04f;
    float contentW = bounds.w - (padX * 2.0f);
    float currentY = padY;
    float dividerGap = bounds.h * 0.025f;

    auto drawHorizontalDivider = [&](float y)
        {
            SDL_SetRenderDrawColor(renderer, 50, 46, 55, 255);
            SDL_RenderLine(renderer, padX, y, bounds.w - padX, y);
        };

    float avatarSize = bounds.h * 0.16f;
    SDL_FRect avatarRect = { padX, currentY, avatarSize, avatarSize };
    renderFillRoundedRect(renderer, avatarRect, 4.0f, { 50, 50, 60, 255 });

    SDL_Color avatarBorder = isEnemy ? SDL_Color{ 255, 120, 170, 255 } : SDL_Color{ 90, 90, 105, 255 };
    renderDrawRoundedRect(renderer, avatarRect, 4.0f, avatarBorder);

    float headerTextX = avatarRect.x + avatarSize + (padX * 0.8f);
    std::string headerStr = targetEntity->name + " - Level " + std::to_string(targetEntity->stats.level);
    float headerTextH = avatarSize * 0.55f;
    SDL_FRect headerTextRect = { headerTextX, currentY, bounds.w - headerTextX - padX, headerTextH };

    SDL_Color nameColor = isEnemy ? SDL_Color{ 255, 120, 170, 255 } : SDL_Color{ 160, 200, 255, 255 };
    g->drawTextFit(headerStr, headerTextRect, nameColor, "title_font");

    if (!isEnemy)
    {
        float xpBarY = currentY + headerTextH + (padY * 0.3f);
        float xpBarW = bounds.w - headerTextX - padX;
        float xpBarH = avatarSize * 0.18f;

        SDL_FRect xpBg = { headerTextX, xpBarY, xpBarW, xpBarH };
        float currentXp = targetEntity->stats.getBaseStat("xp");
        DrawProgressBar(renderer, g, xpBg, currentXp, 100.0f, { 80, 200, 230, 255 });

        currentY += avatarSize + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;

        float halfWidth = contentW / 2.0f;
        float currencyH = bounds.h * 0.08f;
        g->drawTextFit("¤ " + std::to_string(static_cast<int>(targetEntity->stats.getBaseStat("currency"))), { padX, currentY, halfWidth, currencyH }, { 255, 215, 0, 255 });
        g->drawTextFit("★ " + std::to_string(static_cast<int>(targetEntity->stats.getBaseStat("gems"))), { padX + halfWidth, currentY, halfWidth, currencyH }, { 255, 100, 220, 255 });

        currentY += currencyH + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;

        float colWidth = contentW / 3.0f;
        float miniStatH = bounds.h * 0.09f;

        auto drawMiniStat = [&](int colIndex, const std::string& statName, SDL_Color textColor)
            {
                float colX = padX + (colIndex * colWidth);
                SDL_FRect iconBox = { colX, currentY, miniStatH, miniStatH };
                renderFillRoundedRect(renderer, iconBox, 3.0f, { 45, 42, 50, 255 });

                int val = static_cast<int>(targetEntity->stats.getBaseStat(statName));
                SDL_FRect valRect = { colX + miniStatH + 4.0f, currentY, colWidth - miniStatH - 4.0f, miniStatH };
                g->drawTextFit(std::to_string(val), valRect, textColor);
            };

        drawMiniStat(0, "physique", { 255, 50, 120, 255 });
        drawMiniStat(1, "arcane", { 180, 110, 255, 255 });
        drawMiniStat(2, "corruption", { 100, 200, 255, 255 });

        currentY += miniStatH + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;
    }
    else
    {
        currentY += avatarSize + dividerGap;
    }

    float barHeight = bounds.h * 0.065f;
    float iconRadius = barHeight * 1.20f;
    float barGap = bounds.h * 0.02f;

    SDL_FRect healthRow = { padX, currentY, contentW, iconRadius };
    DrawVitalRow(renderer, g, healthRow, targetEntity->getStat("health"), 100.0f, { 255, 60, 90, 255 });
    currentY += iconRadius + barGap;

    SDL_FRect manaRow = { padX, currentY, contentW, iconRadius };
    DrawVitalRow(renderer, g, manaRow, targetEntity->getStat("mana"), 100.0f, { 220, 130, 255, 255 });
    currentY += iconRadius + barGap;

    SDL_FRect lustRow = { padX, currentY, contentW, iconRadius };
    DrawVitalRow(renderer, g, lustRow, targetEntity->getStat("lust"), 100.0f, { 230, 50, 150, 255 });
}

void UI::DrawAnatomyTooltip(SDL_Renderer* renderer, game* g, entity* targetEntity, float mouseX, float mouseY)
{
    if (!targetEntity) return;

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
        const bodyPart* part = targetEntity->anatomy.getPart(slot);

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
            g->getOrRenderText(tok.text, "button_font", tok.color, srcW, srcH);
            if (srcH > 0.0f) rowW += srcW * (fontH / srcH);
        }

        if (rowW > maxContentW) maxContentW = rowW;
        rows.push_back(row);
    }

    float textStartXOffset = bulletSize + (padding * 0.8f);
    float boxWidth = maxContentW + textStartXOffset + (padding * 2.5f);

    int itemLines = static_cast<int>(rows.size());
    float boxHeight = headerH + subHeaderH + padding + (itemLines * lineH) + padding;

    float boxX = mouseX + 15.0f;
    float boxY = mouseY;

    if (boxX + boxWidth > screenW) boxX = mouseX - boxWidth - 12.0f;
    if (boxY + boxHeight > screenH) boxY = screenH - boxHeight - 10.0f;

    SDL_FRect tooltipRect = { boxX, boxY, boxWidth, boxHeight };

    SDL_Color borderColor = (targetEntity == g->Player) ? SDL_Color{ 140, 110, 200, 255 } : SDL_Color{ 255, 120, 170, 255 };
    renderFillRoundedRect(renderer, tooltipRect, GLOBAL_CORNER_RADIUS, { 25, 23, 30, 250 });
    renderDrawRoundedRect(renderer, tooltipRect, GLOBAL_CORNER_RADIUS, borderColor);

    SDL_FRect titleRect = { boxX + padding, boxY + padding, boxWidth - (padding * 2.0f), headerH };
    g->drawTextFit(targetEntity->name, titleRect, borderColor, "title_font");

    char subTitleBuffer[128];
    snprintf(subTitleBuffer, sizeof(subTitleBuffer), "Masculine | Fit body | %.2fm tall", targetEntity->anatomy.heightMeters);

    SDL_FRect subTitleRect = { boxX + padding, boxY + padding + headerH, boxWidth - (padding * 2.0f), subHeaderH };
    g->drawTextFit(subTitleBuffer, subTitleRect, { 100, 200, 255, 255 }, "button_font");

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
        g->renderTextLeftSegment(row.tokens, textX, textY, fontH, "button_font");

        currentY += lineH;
    }
}

void UI::DrawMapGrid(SDL_Renderer* renderer, game* g, SDL_FRect bounds, gameMap* map, int playerX, int playerY, int padding)
{
    SDL_Rect viewRect = { static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.w), static_cast<int>(bounds.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 25, 25, 28, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 55, 55, 65, 255 });

    if (!map) return;

    int mapW = map->getWidth();
    int mapH = map->getHeight();

    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            int mapX = playerX + x;
            int mapY = playerY + y;

            if (mapX < 0 || mapX >= mapW || mapY < 0 || mapY >= mapH) continue;

            Tile t = map->getTile(mapX, mapY);
            if (t.type == TILE_VOID || t.type == TILE_WALL || t.discovery == STATE_HIDDEN) continue;

            int renderX = x + 2;
            int renderY = y + 2;

            // Line 411: Passes SDL_FRect bounds directly
            SDL_FRect r = UIGridHelper::getMapTileRect(bounds, renderX, renderY, static_cast<float>(padding));
            r.x -= bounds.x;
            r.y -= bounds.y;

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

    // Line 435: Passes SDL_FRect bounds directly
    SDL_FRect p = UIGridHelper::getMapTileRect(bounds, 2, 2, static_cast<float>(padding));
    p.x -= bounds.x;
    p.y -= bounds.y;
    renderFillRoundedRect(renderer, p, 3.0f, { 0, 200, 255, 255 });
}

void UI::DrawActionGrid(SDL_Renderer* renderer, game* g, SDL_FRect bounds, const std::vector<actionButton>& buttons)
{
    SDL_Rect viewRect = { static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.w), static_cast<int>(bounds.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    // 1. Draw action panel background card
    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 25, 25, 30, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 50, 50, 60, 255 });

    float padding = 8.0f; // Larger overall padding
    float arrowW = bounds.w * 0.03f; // Side arrow width

    // 2. Render Left (<) and Right (>) Navigation Arrows
    SDL_FRect leftArrowRect = { padding, padding, arrowW, bounds.h - (2.0f * padding) };
    SDL_FRect rightArrowRect = { bounds.w - arrowW - padding, padding, arrowW, bounds.h - (2.0f * padding) };

    renderFillRoundedRect(renderer, leftArrowRect, GLOBAL_CORNER_RADIUS, { 35, 35, 42, 255 });
    renderDrawRoundedRect(renderer, leftArrowRect, GLOBAL_CORNER_RADIUS, { 50, 50, 60, 255 });
    g->renderTextCentered("<", leftArrowRect, "button_font", { 120, 120, 140, 255 });

    renderFillRoundedRect(renderer, rightArrowRect, GLOBAL_CORNER_RADIUS, { 35, 35, 42, 255 });
    renderDrawRoundedRect(renderer, rightArrowRect, GLOBAL_CORNER_RADIUS, { 50, 50, 60, 255 });
    g->renderTextCentered(">", rightArrowRect, "button_font", { 120, 120, 140, 255 });

    // 3. Grid area inset between arrows
    float gridX = padding + arrowW + (padding * 0.5f);
    float gridW = bounds.w - (2.0f * (padding + arrowW + (padding * 0.5f)));
    SDL_FRect gridBounds = { gridX, 0.0f, gridW, bounds.h };

    int cols = 5, rows = 3;
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int index = (r * cols) + c;
            SDL_FRect btn = UIGridHelper::getActionButtonRect(gridBounds, c, r, cols, rows, padding);

            if (index < static_cast<int>(buttons.size()))
            {
                renderFillRoundedRect(renderer, btn, 4.0f, { 70, 100, 140, 255 });
                renderDrawRoundedRect(renderer, btn, 4.0f, { 100, 140, 190, 255 });
                g->renderTextCentered(buttons[index].label, btn, "button_font");
            }
            else
            {
                renderFillRoundedRect(renderer, btn, 4.0f, { 35, 35, 42, 255 });
                renderDrawRoundedRect(renderer, btn, 4.0f, { 50, 50, 60, 255 });
            }
        }
    }
}

void UI::DrawTimePanel(SDL_Renderer* renderer, game* g, SDL_FRect bounds, const timeManager& gameTime)
{
    SDL_Rect viewRect = { static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.w), static_cast<int>(bounds.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    // 1. Panel Background
    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });

    float padX = bounds.w * 0.04f;
    float gapX = bounds.w * 0.05f;

    float leftColX = padX;
    float leftColW = (bounds.w - (2.0f * padX) - gapX) * 0.52f;
    float rightColX = leftColX + leftColW + gapX;
    float rightColW = bounds.w - rightColX - padX;

    // Compact Vertical Row Positions
    float row1Y = bounds.h * 0.08f;
    float row1H = bounds.h * 0.44f;
    float row2Y = bounds.h * 0.56f;
    float row2H = bounds.h * 0.36f;

    // Left Side: Date Text
    SDL_FRect dateBox = { leftColX, row1Y, leftColW, row1H };
    g->drawTextFit(gameTime.getFormattedDate(), dateBox, { 220, 225, 240, 255 });

    // Left Side: Days Row (No outer track box)
    const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
    float daySlotW = leftColW / 7.0f;

    for (int i = 0; i < 7; i++)
    {
        float slotX = leftColX + (i * daySlotW);
        SDL_FRect textFitRect = { slotX, row2Y, daySlotW, row2H };

        if (i == gameTime.dayOfWeek)
        {
            float pillMargin = 1.0f;
            SDL_FRect pillRect = { slotX + pillMargin, row2Y, daySlotW - (pillMargin * 2.0f), row2H };
            renderFillRoundedRect(renderer, pillRect, 3.0f, { 55, 50, 75, 255 });
            renderDrawRoundedRect(renderer, pillRect, 3.0f, { 140, 130, 180, 255 });
            g->drawTextFit(days[i], textFitRect, { 255, 255, 255, 255 });
        }
        else
        {
            g->drawTextFit(days[i], textFitRect, { 100, 100, 115, 255 });
        }
    }

    // Right Side: Time Text
    SDL_FRect timeBox = { rightColX, row1Y, rightColW, row1H };
    g->drawTextFit(gameTime.getFormattedTime(), timeBox, { 255, 220, 130, 255 });

    // Right Side: Time Bar
    float barW = rightColW;
    float barX = rightColX;
    float barH = std::clamp(row2H * 0.45f, 4.0f, 8.0f);
    float barY = row2Y + (row2H - barH) * 0.5f;

    float sunrisePct = gameTime.getSunriseHour() / 24.0f;
    float sunsetPct = gameTime.getSunsetHour() / 24.0f;

    SDL_FRect fullBar = { barX, barY, barW, barH };
    renderFillRoundedRect(renderer, fullBar, 2.0f, { 55, 55, 85, 255 });

    float dayX = barX + (barW * sunrisePct);
    float dayW = barW * (sunsetPct - sunrisePct);
    if (dayW > 0.0f)
    {
        SDL_FRect dayLight = { dayX, barY, dayW, barH };
        SDL_SetRenderDrawColor(renderer, 140, 185, 225, 255);
        SDL_RenderFillRect(renderer, &dayLight);
    }

    SDL_SetRenderDrawColor(renderer, 20, 18, 25, 255);
    SDL_RenderRect(renderer, &fullBar);

    float currentTimePct = gameTime.getDayProgress();
    float needleX = barX + (barW * currentTimePct);
    float needleW = 2.0f;

    SDL_FRect needle = { needleX - 1.0f, barY - 2.0f, needleW, barH + 4.0f };
    renderFillRoundedRect(renderer, needle, 1.0f, { 255, 255, 255, 255 });
}

// --- High-Level Member Layout Drawer Implementations ---

void game::renderTitleBar(SDL_FRect t1, SDL_FRect t2, SDL_FRect t3)
{
    SDL_FRect boxes[3] = { t1, t2, t3 };
    for (int i = 0; i < 3; i++)
    {
        SDL_Rect viewRect = { static_cast<int>(boxes[i].x), static_cast<int>(boxes[i].y), static_cast<int>(boxes[i].w), static_cast<int>(boxes[i].h) };
        ViewportGuard vpGuard(renderer, &viewRect);
        SDL_FRect r = { 0.0f, 0.0f, boxes[i].w, boxes[i].h };
        renderFillRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 45, 45, 52, 255 });
        renderDrawRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 65, 65, 75, 255 });
    }

    if (activeTargetNPC && activeTargetMode != TargetMode::NONE)
    {
        SDL_Rect viewRect3 = { static_cast<int>(t3.x), static_cast<int>(t3.y), static_cast<int>(t3.w), static_cast<int>(t3.h) };
        ViewportGuard vpGuard(renderer, &viewRect3);
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

        SDL_FRect titleRect = { 0.0f, 0.0f, t3.w, t3.h };
        drawTextFit(headerTitle, titleRect, headerColor, "title_font");
    }
}

void game::renderCompanionPanel(SDL_FRect rect)
{
    SDL_Rect viewRect = { static_cast<int>(rect.x), static_cast<int>(rect.y), static_cast<int>(rect.w), static_cast<int>(rect.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    SDL_FRect panelRect = { 0.0f, 0.0f, rect.w, rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 30, 28, 35, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 60, 55, 65, 255 });
}

void game::renderTextPanel(SDL_FRect rect)
{
    SDL_Rect viewRect = { static_cast<int>(rect.x), static_cast<int>(rect.y), static_cast<int>(rect.w), static_cast<int>(rect.h) };
    ViewportGuard vpGuard(renderer, &viewRect);

    SDL_FRect panelRect = { 0.0f, 0.0f, rect.w, rect.h };
    renderFillRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 40, 40, 40, 255 });
    renderDrawRoundedRect(renderer, panelRect, GLOBAL_CORNER_RADIUS, { 65, 65, 65, 255 });

    if (currentState == GameState::EVENT)
    {
        SDL_FRect nameRect = { 0.0f, 0.0f, rect.w, 40.0f };
        renderTextCentered(currentScene.speakerName, nameRect, "title_font", { 255, 200, 100, 255 });

        SDL_FRect bodyRect = { 0.0f, 40.0f, rect.w, rect.h - 40.0f };
        renderTextWrapped(currentScene.bodyText, bodyRect, "button_font", { 220, 220, 220, 255 });
    }
}

void game::renderRightColumn(SDL_FRect top, SDL_FRect mid, SDL_FRect bot)
{
    if (activeTargetNPC && activeTargetMode != TargetMode::NONE)
    {
        // Target NPC Summary Card
        SDL_FRect targetCardFRect = { top.x, layout.charRect.y, top.w, layout.charRect.h };
        UI::DrawEntitySummaryCard(renderer, this, targetCardFRect, activeTargetNPC, true);

        // Target NPC Equipment Grid Widget
        SDL_FRect targetEquipFRect = { top.x, layout.mapRect.y, top.w, layout.mapRect.h };
        UI::DrawEquipmentGrid(renderer, this, targetEquipFRect, activeTargetNPC, equipSlot::NONE, 12);

        float winX, winY, mouseX, mouseY;
        SDL_GetMouseState(&winX, &winY);
        SDL_RenderCoordinatesFromWindow(renderer, winX, winY, &mouseX, &mouseY);

        if (UIGridHelper::contains(layout.targetAvatarRect, mouseX, mouseY))
        {
            UI::DrawAnatomyTooltip(renderer, this, activeTargetNPC, mouseX, mouseY);
        }
        return;
    }

    SDL_FRect boxes[3] = { top, mid, bot };
    for (int i = 0; i < 3; i++)
    {
        SDL_Rect viewRect = { static_cast<int>(boxes[i].x), static_cast<int>(boxes[i].y), static_cast<int>(boxes[i].w), static_cast<int>(boxes[i].h) };
        ViewportGuard vpGuard(renderer, &viewRect);
        SDL_FRect r = { 0.0f, 0.0f, boxes[i].w, boxes[i].h };
        renderFillRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 40, 40, 40, 255 });
        renderDrawRoundedRect(renderer, r, GLOBAL_CORNER_RADIUS, { 65, 65, 75, 255 });
    }
}