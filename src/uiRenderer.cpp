#include "uiRenderer.h"
#include "game.h"
#include "uiWidget.h"

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
    renderFillRoundedRect(renderer, iconRect, 3.0f, Theme::colors.bgHeader);

    float valueTextWidth = bounds.w * 0.18f;
    float barW = bounds.w - iconRadius - valueTextWidth - padX;
    float fillX = bounds.x + iconRadius + padX;
    float barY = bounds.y + (iconRadius - barHeight) / 2.0f;

    SDL_FRect bgRect = { fillX, barY, barW, barHeight };
    DrawProgressBar(renderer, g, bgRect, currentVal, maxVal, barColor);

    SDL_FRect textRect = { fillX + barW + padX, bounds.y, valueTextWidth, iconRadius };
    g->renderTextAligned(std::to_string(static_cast<int>(currentVal)), textRect, TextAlignment::CENTER, true, "button_font", Theme::colors.textSecondary);
}

void UI::DrawEquipmentGrid(SDL_Renderer* renderer, game* g, SDL_FRect bounds, entity* targetEntity, equipSlot selectedSlot, int padding)
{
    ViewportGuard vpGuard(renderer, bounds);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    DrawPanel(renderer, panelRect, Theme::colors.bgPanel, Theme::colors.borderNormal);

    constexpr int cols = 6, rows = 6;
    SDL_FRect localBounds = { 0.0f, 0.0f, bounds.w, bounds.h };

    int targetSide = (targetEntity == g->Player) ? 0 : 1;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int slotIdx = (r * cols) + c;
            SDL_FRect slot = UIGridHelper::getEquipmentSlotRect(localBounds, c, r, cols, rows, 4.0f, static_cast<float>(padding));

            bool isOccupied = false;
            std::string equippedName;
            bool isSelectedEquip = false;

            if (targetEntity)
            {
                for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
                {
                    equipSlot eSlot = static_cast<equipSlot>(i);
                    const auto& eqItem = targetEntity->inventory.equipped[i];

                    if (g->getEquipmentGridIndex(eSlot) == slotIdx && eqItem && !eqItem->id.empty())
                    {
                        isOccupied = true;
                        equippedName = eqItem->name;
                        if (eSlot == selectedSlot && g->selectedInventorySide == targetSide)
                        {
                            isSelectedEquip = true;
                        }
                        break;
                    }
                }
            }

            SDL_Color fillCol = isOccupied ? Theme::colors.bgSlotOccupied : Theme::colors.bgSlot;
            SDL_Color borderCol = isSelectedEquip ? Theme::colors.borderSelected : Theme::colors.borderNormal;

            DrawPanel(renderer, slot, fillCol, borderCol, GLOBAL_CORNER_RADIUS);

            if (isOccupied && !equippedName.empty())
            {
                SDL_FRect textRect = { slot.x + 2.0f, slot.y + 2.0f, slot.w - 4.0f, slot.h - 4.0f };
                g->renderTextAligned(equippedName, textRect, TextAlignment::CENTER, true, "button_font", Theme::colors.textGold);
            }
        }
    }
}

void UI::DrawInventoryGrid(SDL_Renderer* renderer, game* g, SDL_FRect bounds, entity* targetEntity, int selectedIndex)
{
    ViewportGuard vpGuard(renderer, bounds);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    DrawPanel(renderer, panelRect, Theme::colors.bgPanel, Theme::colors.borderNormal);

    static const char* tabLabels[7] = { "I", "II", "III", "IV", "V", "VI", "Key" };
    float halfW = bounds.w * 0.5f;
    float headerH = bounds.h * 0.08f;
    entity* rightEntity = g->activeTargetNPC ? g->activeTargetNPC : nullptr;

    constexpr int cols = 6, rows = 5;
    constexpr int itemsPerPage = cols * rows;

    for (int side = 0; side < 2; side++)
    {
        int activePage = (side == 0) ? g->currentInventoryPage : g->currentRightInventoryPage;
        std::string titleStr = (side == 0)
            ? "Your Inventory | Page " + std::string(tabLabels[activePage])
            : (rightEntity ? rightEntity->name + "'s Inventory | Page " + std::string(tabLabels[activePage])
                : "In Area | Page " + std::string(tabLabels[activePage]));

        SDL_FRect headerBox = { side * halfW, bounds.h * 0.02f, halfW, headerH };
        g->renderTextAligned(titleStr, headerBox, TextAlignment::CENTER, true, "button_font", Theme::colors.textSecondary);

        // Calculate page occupation states
        for (int t = 0; t < 7; t++)
        {
            bool hasItemsOnPage = false;
            int pageStart = t * itemsPerPage;

            for (int slotOffset = 0; slotOffset < itemsPerPage; slotOffset++)
            {
                InventorySlotInfo slotInfo = g->getInventorySlotItem(side, pageStart + slotOffset);
                if (slotInfo.isValid && slotInfo.itemPtr)
                {
                    hasItemsOnPage = true;
                    break;
                }
            }

            SDL_FRect tabRect = UIGridHelper::getInventoryTabRect(panelRect, side, t);
            bool isSelected = (activePage == t);

            SDL_Color bgCol = isSelected ? Theme::colors.bgSlotSelected : Theme::colors.bgSlot;
            SDL_Color borderCol = isSelected ? Theme::colors.textAccent : Theme::colors.borderNormal;
            SDL_Color textCol = isSelected ? Theme::colors.textPrimary : (hasItemsOnPage ? Theme::colors.textSecondary : Theme::colors.textMuted);

            if (!hasItemsOnPage && !isSelected)
            {
                bgCol = Theme::colors.bgHeader;
                borderCol = Theme::colors.borderButtonDisabled;
            }

            DrawPanel(renderer, tabRect, bgCol, borderCol, 3.0f);
            g->renderTextAligned(tabLabels[t], tabRect, TextAlignment::CENTER, true, "button_font", textCol);
        }
    }

    SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, 255);
    SDL_RenderLine(renderer, halfW, 10.0f, halfW, bounds.h - 10.0f);

    for (int side = 0; side < 2; side++)
    {
        int activePage = (side == 0) ? g->currentInventoryPage : g->currentRightInventoryPage;
        int pageOffset = activePage * itemsPerPage;

        for (int i = 0; i < itemsPerPage; i++)
        {
            int gridSlotIdx = (side * itemsPerPage) + i;
            int absoluteItemIdx = pageOffset + i;

            SDL_FRect slot = UIGridHelper::getInventorySlotRect(panelRect, gridSlotIdx, cols, rows);
            InventorySlotInfo slotInfo = g->getInventorySlotItem(side, absoluteItemIdx);

            SDL_Color bgCol = slotInfo.isValid ? Theme::colors.bgSlotOccupied : Theme::colors.bgSlot;
            bool isSelected = (side == g->selectedInventorySide) && (absoluteItemIdx == selectedIndex);
            SDL_Color borderCol = isSelected ? Theme::colors.borderSelected : Theme::colors.borderNormal;

            DrawPanel(renderer, slot, bgCol, borderCol, 4.0f);

            if (slotInfo.isValid && slotInfo.itemPtr)
            {
                std::string itemName = slotInfo.itemPtr->name;
                if (itemName.length() > 8) itemName = itemName.substr(0, 7) + ".";

                SDL_FRect textSlot = { slot.x + 2.0f, slot.y + 2.0f, slot.w - 4.0f, slot.h - 4.0f };
                g->renderTextAligned(itemName, textSlot, TextAlignment::CENTER, true, "button_font", Theme::colors.textPrimary);

                if (slotInfo.count > 1)
                {
                    std::string countStr = "x" + std::to_string(slotInfo.count);
                    SDL_FRect countRect = { slot.x + slot.w * 0.50f, slot.y + slot.h * 0.65f, slot.w * 0.45f, slot.h * 0.30f };
                    g->renderTextAligned(countStr, countRect, TextAlignment::CENTER, true, "button_font", Theme::colors.textGold);
                }
            }
        }
    }
}

void UI::DrawItemDetailPanel(SDL_Renderer* renderer, game* g, SDL_FRect bounds, entity* targetEntity, int selectedIndex)
{
    ViewportGuard vpGuard(renderer, bounds);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    DrawPanel(renderer, panelRect, Theme::colors.bgPanel, Theme::colors.borderNormal);

    float pad = bounds.h * 0.04f;
    std::shared_ptr<item> selectedItem = nullptr;

    if (g->selectedInventoryIndex >= 0)
    {
        selectedItem = g->getInventorySlotItem(g->selectedInventorySide, g->selectedInventoryIndex).itemPtr;
    }
    else if (g->selectedEquipmentSlot != equipSlot::NONE)
    {
        entity* currentTarget = (g->selectedInventorySide == 1 && g->activeTargetNPC) ? g->activeTargetNPC : targetEntity;
        if (currentTarget && currentTarget->inventory.isEquipped(g->selectedEquipmentSlot))
        {
            selectedItem = currentTarget->inventory.getEquippedItem(g->selectedEquipmentSlot);
        }
    }

    if (!selectedItem)
    {
        SDL_FRect emptyBox = { pad, pad, bounds.w - (2.0f * pad), bounds.h - (2.0f * pad) };
        g->renderTextAligned("Select an item to view details", emptyBox, TextAlignment::CENTER, false, "button_font", Theme::colors.textMuted);
        return;
    }

    float previewSize = bounds.h - (2.0f * pad);
    float previewX = bounds.w - previewSize - pad;
    SDL_FRect previewRect = { previewX, pad, previewSize, previewSize };

    DrawPanel(renderer, previewRect, Theme::colors.bgHeader, Theme::colors.borderNormal, 4.0f);
    g->renderTextAligned("No Image", previewRect, TextAlignment::CENTER, false, "button_font", Theme::colors.textMuted);

    float textW = previewX - (2.0f * pad);
    float currentY = pad;

    float titleH = bounds.h * 0.12f;
    SDL_FRect nameRect = { pad, currentY, textW, titleH };
    g->renderTextAligned(selectedItem->name, nameRect, TextAlignment::CENTER, true, "title_font", Theme::colors.textGold);
    currentY += titleH + (pad * 0.2f);

    float subH = bounds.h * 0.08f;
    std::string typeStr = selectedItem->isEquippable ? "Equippable Item" : (selectedItem->isConsumable ? "Consumable" : "Misc Item");
    SDL_FRect typeRect = { pad, currentY, textW, subH };
    g->renderTextAligned(typeStr, typeRect, TextAlignment::CENTER, true, "button_font", Theme::colors.textAccent);
    currentY += subH + (pad * 0.3f);

    if (!selectedItem->requiredTags.empty())
    {
        std::string reqStr = "Requires: ";
        for (const auto& tag : selectedItem->requiredTags) reqStr += tag + " ";

        float reqH = bounds.h * 0.06f;
        SDL_FRect reqRect = { pad, currentY, textW, reqH };
        g->renderTextAligned(reqStr, reqRect, TextAlignment::CENTER, true, "button_font", Theme::colors.arcane);
        currentY += reqH + (pad * 0.2f);
    }

    float descH = bounds.h - currentY - pad;

    {
        SDL_FRect descClip = { bounds.x + pad, bounds.y + currentY, textW, descH };
        ViewportGuard descGuard(renderer, descClip);
        SDL_FRect descContentRect = { 0.0f, -g->descriptionScrollY, textW, descH };
        std::string descText = selectedItem->description.empty() ? "No description available." : selectedItem->description;
        g->renderTextWrapped(descText, descContentRect, "button_font", Theme::colors.textSecondary);
    }
}

void UI::DrawEntitySummaryCard(SDL_Renderer* renderer, game* g, SDL_FRect bounds, entity* targetEntity, bool isEnemy)
{
    ViewportGuard vpGuard(renderer, bounds);

    SDL_FRect cardRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    DrawPanel(renderer, cardRect, Theme::colors.bgPanel, Theme::colors.borderNormal);

    if (!targetEntity) return;

    float padX = bounds.w * 0.04f, padY = bounds.h * 0.04f;
    float contentW = bounds.w - (padX * 2.0f);
    float currentY = padY;
    float dividerGap = bounds.h * 0.025f;

    auto drawHorizontalDivider = [&](float y)
        {
            SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, 255);
            SDL_RenderLine(renderer, padX, y, bounds.w - padX, y);
        };

    float avatarSize = bounds.h * 0.16f;
    SDL_FRect avatarRect = { padX, currentY, avatarSize, avatarSize };
    SDL_Color avatarBorder = isEnemy ? Theme::colors.enemy : Theme::colors.borderNormal;
    DrawPanel(renderer, avatarRect, Theme::colors.bgHeader, avatarBorder, 4.0f);

    float headerTextX = avatarRect.x + avatarSize + (padX * 0.8f);
    std::string headerStr = targetEntity->name + " - Level " + std::to_string(targetEntity->stats.level);
    float headerTextH = avatarSize * 0.55f;
    SDL_FRect headerTextRect = { headerTextX, currentY, bounds.w - headerTextX - padX, headerTextH };

    SDL_Color nameColor = isEnemy ? Theme::colors.enemy : Theme::colors.friendly;
    g->renderTextAligned(headerStr, headerTextRect, TextAlignment::CENTER, true, "title_font", nameColor);

    if (!isEnemy)
    {
        float xpBarY = currentY + headerTextH + (padY * 0.3f);
        float xpBarW = bounds.w - headerTextX - padX;
        float xpBarH = avatarSize * 0.18f;

        SDL_FRect xpBg = { headerTextX, xpBarY, xpBarW, xpBarH };
        float currentXp = targetEntity->stats.getBaseStat("xp");
        DrawProgressBar(renderer, g, xpBg, currentXp, 100.0f, Theme::colors.corruption);

        currentY += avatarSize + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;

        float halfWidth = contentW / 2.0f;
        float currencyH = bounds.h * 0.08f;
        g->renderTextAligned("¤ " + std::to_string(static_cast<int>(targetEntity->stats.getBaseStat("currency"))), { padX, currentY, halfWidth, currencyH }, TextAlignment::CENTER, true, "button_font", Theme::colors.currency);
        g->renderTextAligned("★ " + std::to_string(static_cast<int>(targetEntity->stats.getBaseStat("gems"))), { padX + halfWidth, currentY, halfWidth, currencyH }, TextAlignment::CENTER, true, "button_font", Theme::colors.gems);

        currentY += currencyH + dividerGap;
        drawHorizontalDivider(currentY);
        currentY += dividerGap;

        float colWidth = contentW / 3.0f;
        float miniStatH = bounds.h * 0.09f;

        auto drawMiniStat = [&](int colIndex, const std::string& statName, SDL_Color textColor)
            {
                float colX = padX + (colIndex * colWidth);
                SDL_FRect iconBox = { colX, currentY, miniStatH, miniStatH };
                renderFillRoundedRect(renderer, iconBox, 3.0f, Theme::colors.bgHeader);

                int val = static_cast<int>(targetEntity->stats.getBaseStat(statName));
                SDL_FRect valRect = { colX + miniStatH + 4.0f, currentY, colWidth - miniStatH - 4.0f, miniStatH };
                g->renderTextAligned(std::to_string(val), valRect, TextAlignment::CENTER, true, "button_font", textColor);
            };

        drawMiniStat(0, "physique", Theme::colors.physique);
        drawMiniStat(1, "arcane", Theme::colors.arcane);
        drawMiniStat(2, "corruption", Theme::colors.corruption);

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
    DrawVitalRow(renderer, g, healthRow, targetEntity->getStat("health"), 100.0f, Theme::colors.health);
    currentY += iconRadius + barGap;

    SDL_FRect manaRow = { padX, currentY, contentW, iconRadius };
    DrawVitalRow(renderer, g, manaRow, targetEntity->getStat("mana"), 100.0f, Theme::colors.mana);
    currentY += iconRadius + barGap;

    SDL_FRect lustRow = { padX, currentY, contentW, iconRadius };
    DrawVitalRow(renderer, g, lustRow, targetEntity->getStat("lust"), 100.0f, Theme::colors.lust);
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

    float headerH = static_cast<float>(screenH) * 0.032f;
    float subHeaderH = static_cast<float>(screenH) * 0.024f;
    float lineH = static_cast<float>(screenH) * 0.025f;

    float fontH = lineH * 0.80f;
    float padding = static_cast<float>(screenW) * 0.008f;
    float bulletSize = fontH * 0.45f;

    struct RenderRowData
    {
        bool isOccupied = false;
        SDL_Color bulletColor = Theme::colors.textMuted;
        std::vector<ColorToken> tokens;
    };

    std::vector<RenderRowData> rows;
    float maxContentW = static_cast<float>(screenW) * 0.18f;

    for (bodySlot slot : anatomicalOrder)
    {
        RenderRowData row;
        const bodyPart* part = targetEntity->anatomy.getPart(slot);

        if (part != nullptr)
        {
            row.isOccupied = true;
            row.bulletColor = Theme::getColorFromName(part->primaryColor);

            std::string prefixStr;
            if (slot == bodySlot::GROIN && part->length > 0.0f)
            {
                char buf[64]; snprintf(buf, sizeof(buf), "%s (%gcm long, %gcm diameter)", part->name.c_str(), part->length, part->diameter);
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

            row.tokens.push_back({ prefixStr + ": ", Theme::colors.textSecondary });
            row.tokens.push_back({ part->race + " - ", Theme::colors.arcane });

            if (!part->secondaryColor.empty())
            {
                row.tokens.push_back({ part->secondaryColor, Theme::getColorFromName(part->secondaryColor) });
                row.tokens.push_back({ "-rimmed, ", Theme::colors.textSecondary });
            }

            row.tokens.push_back({ part->primaryColor + " ", row.bulletColor });
            row.tokens.push_back({ coveringNoun, Theme::colors.textSecondary });
        }
        else
        {
            row.isOccupied = false;
            row.bulletColor = Theme::colors.borderButtonDisabled;
            row.tokens.push_back({ getSlotName(slot) + ": ", Theme::colors.textMuted });
            row.tokens.push_back({ "None", Theme::colors.textMuted });
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
    float boxHeight = headerH + subHeaderH + padding + (static_cast<float>(rows.size()) * lineH) + padding;

    float boxX = mouseX + 15.0f, boxY = mouseY;
    if (boxX + boxWidth > static_cast<float>(screenW)) boxX = mouseX - boxWidth - 12.0f;
    if (boxY + boxHeight > static_cast<float>(screenH)) boxY = static_cast<float>(screenH) - boxHeight - 10.0f;

    SDL_FRect tooltipRect = { boxX, boxY, boxWidth, boxHeight };
    SDL_Color borderColor = (targetEntity == g->Player) ? Theme::colors.textAccent : Theme::colors.enemy;

    DrawPanel(renderer, tooltipRect, Theme::colors.bgPanel, borderColor);

    SDL_FRect titleRect = { boxX + padding, boxY + padding, boxWidth - (padding * 2.0f), headerH };
    g->renderTextAligned(targetEntity->name, titleRect, TextAlignment::CENTER, true, "title_font", borderColor);

    char subTitleBuffer[128];
    snprintf(subTitleBuffer, sizeof(subTitleBuffer), "Masculine | Fit body | %.2fm tall", targetEntity->anatomy.heightMeters);
    SDL_FRect subTitleRect = { boxX + padding, boxY + padding + headerH, boxWidth - (padding * 2.0f), subHeaderH };
    g->renderTextAligned(subTitleBuffer, subTitleRect, TextAlignment::CENTER, true, "button_font", Theme::colors.friendly);

    float dividerY = boxY + padding + headerH + subHeaderH + (padding * 0.4f);
    SDL_SetRenderDrawColor(renderer, Theme::colors.borderNormal.r, Theme::colors.borderNormal.g, Theme::colors.borderNormal.b, 255);
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
    ViewportGuard vpGuard(renderer, bounds);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    DrawPanel(renderer, panelRect, Theme::colors.bgHeader, Theme::colors.borderNormal);

    if (!map) return;
    int mapW = map->getWidth(), mapH = map->getHeight();
    SDL_FRect localBounds = { 0.0f, 0.0f, bounds.w, bounds.h };

    constexpr float tileGap = 4.0f;

    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            int mapX = playerX + x, mapY = playerY + y;
            if (mapX < 0 || mapX >= mapW || mapY < 0 || mapY >= mapH) continue;

            Tile t = map->getTile(mapX, mapY);
            if (t.type == TILE_VOID || t.type == TILE_WALL || t.discovery == STATE_HIDDEN) continue;

            SDL_FRect r = UIGridHelper::getMapTileRect(localBounds, x + 2, y + 2, static_cast<float>(padding), tileGap);
            int danger = map->getRuntimeData(mapX, mapY).getEffectiveDangerLevel();

            SDL_Color baseCol = Theme::colors.textSecondary;

            float brightness = 1.0f;
            if (t.discovery == STATE_PARTIAL) brightness *= 0.65f;
            if (danger > 0) brightness *= 0.80f;

            SDL_Color fillCol = {
                static_cast<Uint8>(static_cast<float>(baseCol.r) * brightness),
                static_cast<Uint8>(static_cast<float>(baseCol.g) * brightness),
                static_cast<Uint8>(static_cast<float>(baseCol.b) * brightness),
                255
            };

            SDL_Color borderCol = Theme::colors.borderNormal;
            if (t.type == TILE_DOOR) borderCol = Theme::colors.textGold;
            else if (danger > 0) borderCol = Theme::colors.enemy;

            DrawPanel(renderer, r, fillCol, borderCol, 4.0f);
        }
    }

    SDL_FRect p = UIGridHelper::getMapTileRect(localBounds, 2, 2, static_cast<float>(padding), tileGap);
    DrawPanel(renderer, p, Theme::colors.friendly, Theme::colors.textAccent, 4.0f);
}

void UI::DrawActionGrid(SDL_Renderer* renderer, game* g, SDL_FRect bounds, const std::vector<actionButton>& buttons)
{
    ViewportGuard vpGuard(renderer, bounds);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    DrawPanel(renderer, panelRect, Theme::colors.bgPanel, Theme::colors.borderNormal);

    auto [leftArrow, rightArrow] = UIGridHelper::getNavigationArrows(panelRect);

    bool hasPrevPage = (g->actionGridPage > 0);
    bool hasNextPage = false;

    int totalButtons = static_cast<int>(g->activeButtons.size());
    int maxButtonsOnCurrentPage = (g->actionGridPage + 1) * 15;
    if (totalButtons > maxButtonsOnCurrentPage)
    {
        hasNextPage = true;
    }

    SDL_Color leftBg = hasPrevPage ? Theme::colors.bgButton : Theme::colors.bgHeader;
    SDL_Color leftBorder = hasPrevPage ? Theme::colors.borderButton : Theme::colors.borderButtonDisabled;
    SDL_Color leftText = hasPrevPage ? Theme::colors.textPrimary : Theme::colors.textMuted;

    DrawPanel(renderer, leftArrow, leftBg, leftBorder, 4.0f);
    g->renderTextAligned("<", leftArrow, TextAlignment::CENTER, false, "button_font", leftText);

    SDL_Color rightBg = hasNextPage ? Theme::colors.bgButton : Theme::colors.bgHeader;
    SDL_Color rightBorder = hasNextPage ? Theme::colors.borderButton : Theme::colors.borderButtonDisabled;
    SDL_Color rightText = hasNextPage ? Theme::colors.textPrimary : Theme::colors.textMuted;

    DrawPanel(renderer, rightArrow, rightBg, rightBorder, 4.0f);
    g->renderTextAligned(">", rightArrow, TextAlignment::CENTER, false, "button_font", rightText);

    SDL_FRect gridBounds = UIGridHelper::getActionGridBounds(panelRect);
    auto currentSlots = g->getSlotsForCurrentActionPage();
    constexpr int cols = 5, rows = 3;

    for (int i = 0; i < cols * rows; i++)
    {
        int c = i % cols, r = i / cols;
        SDL_FRect btnRect = UIGridHelper::getActionButtonRect(gridBounds, c, r, cols, rows);
        const actionButton& activeBtn = currentSlots[i];

        if (!activeBtn.label.empty())
        {
            SDL_Color bgCol = activeBtn.isEnabled ? Theme::colors.bgButton : Theme::colors.bgButtonDisabled;
            SDL_Color borderCol = activeBtn.isEnabled ? Theme::colors.borderButton : Theme::colors.borderButtonDisabled;
            SDL_Color textCol = activeBtn.isEnabled ? Theme::colors.textPrimary : Theme::colors.textMuted;

            DrawPanel(renderer, btnRect, bgCol, borderCol, 4.0f);
            g->renderTextAligned(activeBtn.label, btnRect, TextAlignment::CENTER, false, "button_font", textCol);
        }
        else
        {
            DrawPanel(renderer, btnRect, Theme::colors.bgHeader, Theme::colors.borderButtonDisabled, 4.0f);
        }
    }
}

void UI::DrawTimePanel(SDL_Renderer* renderer, game* g, SDL_FRect bounds, const timeManager& gameTime)
{
    ViewportGuard vpGuard(renderer, bounds);

    SDL_FRect panelRect = { 0.0f, 0.0f, bounds.w, bounds.h };
    DrawPanel(renderer, panelRect, Theme::colors.bgPanel, Theme::colors.borderNormal);

    float padX = bounds.w * 0.04f, gapX = bounds.w * 0.05f;
    float leftColX = padX, leftColW = (bounds.w - (2.0f * padX) - gapX) * 0.52f;
    float rightColX = leftColX + leftColW + gapX, rightColW = bounds.w - rightColX - padX;

    float row1Y = bounds.h * 0.08f, row1H = bounds.h * 0.44f;
    float row2Y = bounds.h * 0.56f, row2H = bounds.h * 0.36f;

    SDL_FRect dateBox = { leftColX, row1Y, leftColW, row1H };
    g->renderTextAligned(gameTime.getFormattedDate(), dateBox, TextAlignment::CENTER, true, "button_font", Theme::colors.textSecondary);

    static const char* days[7] = { "M", "T", "W", "T", "F", "S", "S" };
    float daySlotW = leftColW / 7.0f;

    for (int i = 0; i < 7; i++)
    {
        float slotX = leftColX + (static_cast<float>(i) * daySlotW);
        SDL_FRect textFitRect = { slotX, row2Y, daySlotW, row2H };

        if (i == gameTime.dayOfWeek)
        {
            constexpr float pillMargin = 1.0f;
            SDL_FRect pillRect = { slotX + pillMargin, row2Y, daySlotW - (pillMargin * 2.0f), row2H };
            DrawPanel(renderer, pillRect, Theme::colors.bgSlotSelected, Theme::colors.textAccent, 3.0f);
            g->renderTextAligned(days[i], textFitRect, TextAlignment::CENTER, true, "button_font", Theme::colors.textPrimary);
        }
        else
        {
            g->renderTextAligned(days[i], textFitRect, TextAlignment::CENTER, true, "button_font", Theme::colors.textMuted);
        }
    }

    SDL_FRect timeBox = { rightColX, row1Y, rightColW, row1H };
    g->renderTextAligned(gameTime.getFormattedTime(), timeBox, TextAlignment::CENTER, true, "button_font", Theme::colors.textGold);

    float barW = rightColW, barX = rightColX;
    float barH = std::clamp(row2H * 0.45f, 4.0f, 8.0f);
    float barY = row2Y + (row2H - barH) * 0.5f;

    float sunrisePct = gameTime.getSunriseHour() / 24.0f;
    float sunsetPct = gameTime.getSunsetHour() / 24.0f;

    SDL_FRect fullBar = { barX, barY, barW, barH };
    renderFillRoundedRect(renderer, fullBar, 2.0f, Theme::colors.bgHeader);

    float dayX = barX + (barW * sunrisePct);
    float dayW = barW * (sunsetPct - sunrisePct);
    if (dayW > 0.0f)
    {
        SDL_FRect dayLight = { dayX, barY, dayW, barH };
        SDL_SetRenderDrawColor(renderer, Theme::colors.textAccent.r, Theme::colors.textAccent.g, Theme::colors.textAccent.b, 255);
        SDL_RenderFillRect(renderer, &dayLight);
    }

    SDL_SetRenderDrawColor(renderer, Theme::colors.bgDark.r, Theme::colors.bgDark.g, Theme::colors.bgDark.b, 255);
    SDL_RenderRect(renderer, &fullBar);

    float currentTimePct = gameTime.getDayProgress();
    float needleX = barX + (barW * currentTimePct);

    SDL_FRect needle = { needleX - 1.0f, barY - 2.0f, 2.0f, barH + 4.0f };
    renderFillRoundedRect(renderer, needle, 1.0f, Theme::colors.textPrimary);
}