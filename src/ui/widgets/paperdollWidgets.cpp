#include "ui/widgets/paperdollWidgets.h"

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "core/game.h"
#include "entities/entity.h"
#include "items/item.h"
#include "items/clothingDisplacement.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"

namespace PaperdollWidgets
{
    struct GridSlotDef {
        std::string shortName;
        equipSlot slot;
        bool isActionButton = false;
    };

    static const std::array<GridSlotDef, 36> s_equipmentGrid = {{
        // Row 0
        { "EYES", equipSlot::EYEWEAR },
        { "HEAD", equipSlot::HEADWEAR },
        { "HAIR", equipSlot::HAIR_WEAR },
        { "HORNS", equipSlot::HORNS_SLOT },
        { "MAIN", equipSlot::WEAPON_MAIN },
        { "OFF", equipSlot::WEAPON_OFF },

        // Row 1
        { "MOUTH", equipSlot::MOUTHWEAR },
        { "NECK", equipSlot::NECKWEAR },
        { "COAT", equipSlot::TORSO_OVER },
        { "WINGS", equipSlot::WINGS_SLOT },
        { "P:EAR", equipSlot::PIERCING_EAR },
        { "P:NOSE", equipSlot::PIERCING_NOSE },

        // Row 2
        { "WRISTS", equipSlot::WRISTS },
        { "SHIRT", equipSlot::TORSO_UNDER },
        { "BRA", equipSlot::CHEST_WEAR },
        { "NIPPLE", equipSlot::NIPPLES_WEAR },
        { "P:LIP", equipSlot::PIERCING_LIP },
        { "P:TONG", equipSlot::PIERCING_TONGUE },

        // Row 3
        { "HANDS", equipSlot::HANDS },
        { "BELT", equipSlot::HIPS_WEAR },
        { "STOMACH", equipSlot::STOMACH_WEAR },
        { "RING", equipSlot::FINGER_PRIMARY },
        { "P:NIP", equipSlot::PIERCING_NIPPLE },
        { "P:NAV", equipSlot::PIERCING_NAVEL },

        // Row 4
        { "ANKLES", equipSlot::ANKLES },
        { "PANTS", equipSlot::LEGS_OUTER },
        { "UNDIES", equipSlot::GROIN_OVER },
        { "TAIL", equipSlot::TAIL_SLOT },
        { "P:COCK", equipSlot::PIERCING_COCK },
        { "P:VAG", equipSlot::PIERCING_VAGINA },

        // Row 5
        { "SOCKS", equipSlot::CALVES },
        { "SHOES", equipSlot::FEET },
        { "PLUG", equipSlot::ASS_WEAR },
        { "COCK", equipSlot::PENIS_WEAR },
        { "VAGINA", equipSlot::VAGINA_WEAR },
        { "STRIP", equipSlot::NONE, true }
    }};

    static bool isSlotAvailableForEntity(const entity* character, equipSlot slot)
    {
        if (!character) return false;

        switch (slot)
        {
            case equipSlot::HORNS_SLOT:
                return character->anatomy.hasPart(bodySlot::HORNS);

            case equipSlot::WINGS_SLOT:
                return character->anatomy.hasPart(bodySlot::WINGS);

            case equipSlot::TAIL_SLOT:
                return character->anatomy.hasPart(bodySlot::TAIL);

            case equipSlot::NIPPLES_WEAR:
            case equipSlot::PIERCING_NIPPLE:
                return character->anatomy.hasPart(bodySlot::BREASTS) || character->anatomy.hasPart(bodySlot::NIPPLES);

            case equipSlot::PIERCING_COCK:
            case equipSlot::PENIS_WEAR:
                return character->anatomy.hasTag(bodySlot::GROIN, "penis") ||
                       character->anatomy.hasTag(bodySlot::GROIN, "cock");

            case equipSlot::PIERCING_VAGINA:
            case equipSlot::VAGINA_WEAR:
                return character->anatomy.hasTag(bodySlot::GROIN, "vagina") ||
                       character->anatomy.hasTag(bodySlot::GROIN, "pussy");

            default:
                return true;
        }
    }

    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        entity* player = gameContext->getPlayer();
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Header Card
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        UIWidget::drawHeader(renderer, headerRect, "EQUIPMENT PAPERDOLL (6x6)", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.85f);
        curY += headerH + (8.0f * uiScale);

        // 6 Columns x 6 Rows Grid
        const int cols = 6;
        const int rows = 6;
        float slotGap = 4.0f * uiScale;
        float slotW = (availableW - (slotGap * (cols - 1))) / static_cast<float>(cols);
        float slotH = 46.0f * uiScale;

        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int index = (r * cols) + c;
                const auto& def = s_equipmentGrid[index];

                float slotX = padX + (c * (slotW + slotGap));
                float slotY = curY + (r * (slotH + slotGap));
                SDL_FRect slotRect = { slotX, slotY, slotW, slotH };

                bool isAvailable = def.isActionButton || isSlotAvailableForEntity(player, def.slot);
                bool isSelected = (!def.isActionButton && gameContext->selectedEquipmentSlot == def.slot);

                bool hovered = (mousePos.x >= slotRect.x && mousePos.x <= slotRect.x + slotRect.w &&
                                mousePos.y >= slotRect.y && mousePos.y <= slotRect.y + slotRect.h);

                if (def.isActionButton)
                {
                    // Special Action Button: [ Strip All ]
                    SDL_Color fillCol = hovered ? Theme::colors.bgHeader : Theme::colors.bgHeader;
                    SDL_Color borderCol = hovered ? Theme::colors.borderSelected : Theme::colors.borderButton;
                    UIWidget::drawPanel(renderer, slotRect, fillCol, borderCol);

                    float lblW = UIWidget::getTextWidth("ACTION", uiScale * 0.65f);
                    UIWidget::drawText(renderer, "ACTION", slotX + ((slotW - lblW) / 2.0f), slotY + (3.0f * uiScale), Theme::colors.textGold, uiScale * 0.65f);

                    float btnW = UIWidget::getTextWidth("Strip", uiScale * 0.72f);
                    UIWidget::drawText(renderer, "Strip", slotX + ((slotW - btnW) / 2.0f), slotY + (20.0f * uiScale), Theme::colors.health, uiScale * 0.72f);

                    if (hovered && clicked)
                    {
                        // Strip all equipped items
                        for (size_t s = 0; s < EQUIP_SLOT_COUNT; ++s)
                        {
                            player->inventory.unequipItem(static_cast<equipSlot>(s));
                        }
                        gameContext->selectedEquipmentSlot = equipSlot::NONE;
                        gameContext->refreshActionGrid();
                        gameContext->input.consumeMouseClick();
                    }
                }
                else if (!isAvailable)
                {
                    // Disabled / Anatomically Locked Slot
                    UIWidget::drawPanel(renderer, slotRect, SDL_Color{ 16, 18, 22, 255 }, SDL_Color{ 28, 30, 36, 255 });
                    float lblW = UIWidget::getTextWidth(def.shortName, uiScale * 0.65f);
                    UIWidget::drawText(renderer, def.shortName, slotX + ((slotW - lblW) / 2.0f), slotY + (3.0f * uiScale), SDL_Color{ 65, 70, 80, 255 }, uiScale * 0.65f);

                    float lockW = UIWidget::getTextWidth("Locked", uiScale * 0.62f);
                    UIWidget::drawText(renderer, "Locked", slotX + ((slotW - lockW) / 2.0f), slotY + (20.0f * uiScale), SDL_Color{ 50, 55, 65, 255 }, uiScale * 0.62f);
                }
                else
                {
                    // Active Equipment Slot
                    auto eqItem = player->inventory.getEquippedItem(def.slot);
                    DisplacementMode disp = player->inventory.getDisplacement(def.slot);

                    SDL_Color fillCol = isSelected ? Theme::colors.bgHeader : (hovered ? Theme::colors.bgHeader : Theme::colors.bgSlot);
                    SDL_Color borderCol = isSelected ? Theme::colors.borderSelected : (hovered ? Theme::colors.borderButton : Theme::colors.borderNormal);

                    UIWidget::drawPanel(renderer, slotRect, fillCol, borderCol);

                    // Slot Short Name
                    float lblW = UIWidget::getTextWidth(def.shortName, uiScale * 0.65f);
                    UIWidget::drawText(renderer, def.shortName, slotX + ((slotW - lblW) / 2.0f), slotY + (3.0f * uiScale), isSelected ? Theme::colors.textGold : Theme::colors.textSecondary, uiScale * 0.65f);

                    if (eqItem)
                    {
                        // Item Name (first 6-7 chars if long)
                        std::string dName = eqItem->name;
                        if (dName.length() > 7) dName = dName.substr(0, 6) + ".";

                        float itemW = UIWidget::getTextWidth(dName, uiScale * 0.68f);
                        UIWidget::drawText(renderer, dName, slotX + ((slotW - itemW) / 2.0f), slotY + (18.0f * uiScale), Theme::colors.textPrimary, uiScale * 0.68f);

                        // Displacement Indicator Pill
                        if (disp != DisplacementMode::NONE)
                        {
                            std::string dispTag = (disp == DisplacementMode::UNBUTTON) ? "Open" :
                                                  (disp == DisplacementMode::PULL_ASIDE) ? "Aside" :
                                                  (disp == DisplacementMode::LIFT_UP) ? "Up" : "Down";

                            float dTagW = UIWidget::getTextWidth(dispTag, uiScale * 0.58f);
                            UIWidget::drawText(renderer, dispTag, slotX + ((slotW - dTagW) / 2.0f), slotY + (32.0f * uiScale), Theme::colors.lust, uiScale * 0.58f);
                        }
                    }
                    else
                    {
                        float empW = UIWidget::getTextWidth("---", uiScale * 0.65f);
                        UIWidget::drawText(renderer, "---", slotX + ((slotW - empW) / 2.0f), slotY + (20.0f * uiScale), Theme::colors.textMuted, uiScale * 0.65f);
                    }

                    if (hovered && clicked)
                    {
                        gameContext->selectedEquipmentSlot = def.slot;
                        gameContext->selectedInventoryIndex = -1;
                        gameContext->refreshActionGrid();
                        gameContext->input.consumeMouseClick();
                    }
                }
            }
        }

        curY += (rows * (slotH + slotGap)) + (8.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetItemInspector(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (8.0f * uiScale);
        float availableW = innerW - (16.0f * uiScale);

        entity* player = gameContext->getPlayer();
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // =========================================================================
        // CASE A: Selected Equipment Slot
        // =========================================================================
        if (gameContext->selectedEquipmentSlot != equipSlot::NONE)
        {
            equipSlot slot = gameContext->selectedEquipmentSlot;
            std::string slotTitle = gameContext->formatEquipSlotName(slot);

            float headerH = 26.0f * uiScale;
            SDL_FRect headerRect = { padX, curY, availableW, headerH };
            UIWidget::drawHeader(renderer, headerRect, "EQUIPMENT SLOT INSPECTOR", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.85f);
            curY += headerH + (8.0f * uiScale);

            float cardH = 280.0f * uiScale;
            SDL_FRect cardRect = { padX, curY, availableW, cardH };
            UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

            float innerPad = 10.0f * uiScale;
            float iX = padX + innerPad;
            float iW = availableW - (innerPad * 2.0f);
            float iY = curY + (8.0f * uiScale);

            UIWidget::drawText(renderer, std::format("Slot: {}", slotTitle), iX, iY, Theme::colors.textGold, uiScale * 0.90f);
            iY += (20.0f * uiScale);

            auto eq = player->inventory.getEquippedItem(slot);
            if (eq)
            {
                UIWidget::drawText(renderer, std::format("Item: {}", eq->name), iX, iY, Theme::colors.textPrimary, uiScale * 0.85f);
                iY += (18.0f * uiScale);

                DisplacementMode disp = player->inventory.getDisplacement(slot);
                std::string dispStr = displacementModeToString(disp);
                UIWidget::drawText(renderer, std::format("Displacement: {}", dispStr), iX, iY, Theme::colors.companion, uiScale * 0.80f);
                iY += (18.0f * uiScale);

                UIWidget::drawText(renderer, std::format("Base Value: {} ¤", eq->baseValue), iX, iY, Theme::colors.currency, uiScale * 0.80f);
                iY += (20.0f * uiScale);

                float descH = UIWidget::drawTextWrapped(renderer, eq->description, iX, iY, iW, Theme::colors.textSecondary, uiScale * 0.78f);
                iY += descH + (14.0f * uiScale);

                // Interactive Displacement & Unequip Action Buttons
                float btnW = (iW - (3 * 4.0f * uiScale)) / 2.0f;
                float btnH = 24.0f * uiScale;

                // Button 1: Unequip
                SDL_FRect b1 = { iX, iY, btnW, btnH };
                bool h1 = (mousePos.x >= b1.x && mousePos.x <= b1.x + b1.w && mousePos.y >= b1.y && mousePos.y <= b1.y + b1.h);
                UIWidget::drawButton(renderer, b1, "Unequip", h1, true, false, uiScale * 0.72f);
                if (h1 && clicked) {
                    gameContext->handleUnequipAction(slot);
                    gameContext->input.consumeMouseClick();
                }

                // Button 2: Reset Fit
                SDL_FRect b2 = { iX + btnW + (4.0f * uiScale), iY, btnW, btnH };
                bool h2 = (mousePos.x >= b2.x && mousePos.x <= b2.x + b2.w && mousePos.y >= b2.y && mousePos.y <= b2.y + b2.h);
                UIWidget::drawButton(renderer, b2, "Reset Fit", h2, true, false, uiScale * 0.72f);
                if (h2 && clicked) {
                    player->inventory.setDisplacement(slot, DisplacementMode::NONE);
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
                iY += btnH + (6.0f * uiScale);

                // Button 3: Pull Aside
                SDL_FRect b3 = { iX, iY, btnW, btnH };
                bool h3 = (mousePos.x >= b3.x && mousePos.x <= b3.x + b3.w && mousePos.y >= b3.y && mousePos.y <= b3.y + b3.h);
                UIWidget::drawButton(renderer, b3, "Pull Aside", h3, true, (disp == DisplacementMode::PULL_ASIDE), uiScale * 0.72f);
                if (h3 && clicked) {
                    player->inventory.setDisplacement(slot, DisplacementMode::PULL_ASIDE);
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }

                // Button 4: Pull Up / Down
                SDL_FRect b4 = { iX + btnW + (4.0f * uiScale), iY, btnW, btnH };
                bool h4 = (mousePos.x >= b4.x && mousePos.x <= b4.x + b4.w && mousePos.y >= b4.y && mousePos.y <= b4.y + b4.h);
                UIWidget::drawButton(renderer, b4, "Pull Up/Down", h4, true, (disp == DisplacementMode::LIFT_UP || disp == DisplacementMode::PULL_DOWN), uiScale * 0.72f);
                if (h4 && clicked) {
                    player->inventory.setDisplacement(slot, DisplacementMode::LIFT_UP);
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
                iY += btnH + (6.0f * uiScale);

                // Button 5: Unbutton / Open
                SDL_FRect b5 = { iX, iY, iW, btnH };
                bool h5 = (mousePos.x >= b5.x && mousePos.x <= b5.x + b5.w && mousePos.y >= b5.y && mousePos.y <= b5.y + b5.h);
                UIWidget::drawButton(renderer, b5, "Unbutton / Open", h5, true, (disp == DisplacementMode::UNBUTTON), uiScale * 0.72f);
                if (h5 && clicked) {
                    player->inventory.setDisplacement(slot, DisplacementMode::UNBUTTON);
                    gameContext->refreshActionGrid();
                    gameContext->input.consumeMouseClick();
                }
            }
            else
            {
                UIWidget::drawText(renderer, "No item equipped in this slot.", iX, iY, Theme::colors.textMuted, uiScale * 0.80f);
                iY += (20.0f * uiScale);
                UIWidget::drawText(renderer, "Select an item from your backpack to equip it here.", iX, iY, Theme::colors.textSecondary, uiScale * 0.75f);
            }

            curY += cardH + (8.0f * uiScale);
            return (curY - startY);
        }

        // =========================================================================
        // CASE B: Selected Item from Backpack or Ground
        // =========================================================================
        float headerH = 26.0f * uiScale;
        SDL_FRect headerRect = { padX, curY, availableW, headerH };
        UIWidget::drawHeader(renderer, headerRect, "ITEM DETAILS & LORE", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.85f);
        curY += headerH + (8.0f * uiScale);

        float cardH = 280.0f * uiScale;
        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerPad = 10.0f * uiScale;
        float iX = padX + innerPad;
        float iW = availableW - (innerPad * 2.0f);
        float iY = curY + (8.0f * uiScale);

        if (gameContext->selectedInventoryIndex >= 0)
        {
            auto items = (gameContext->selectedInventorySide == 0)
                ? gameContext->getPlayerInventoryStacked()
                : gameContext->getTileInventoryStacked();

            if (gameContext->selectedInventoryIndex < static_cast<int>(items.size()))
            {
                const auto& slotData = items[gameContext->selectedInventoryIndex];
                if (slotData.itemPtr)
                {
                    UIWidget::drawText(renderer, std::format("{} (x{})", slotData.itemPtr->name, slotData.totalCount), iX, iY, Theme::colors.textGold, uiScale * 0.90f);
                    iY += (20.0f * uiScale);

                    std::string sideStr = (gameContext->selectedInventorySide == 0) ? "In Backpack" : "On Ground";
                    UIWidget::drawText(renderer, std::format("Location: {} | Value: {} ¤", sideStr, slotData.itemPtr->baseValue), iX, iY, Theme::colors.currency, uiScale * 0.80f);
                    iY += (18.0f * uiScale);

                    std::string targetSlotStr = gameContext->formatEquipSlotName(slotData.itemPtr->targetSlot);
                    UIWidget::drawText(renderer, std::format("Equip Socket: {}", targetSlotStr), iX, iY, Theme::colors.companion, uiScale * 0.80f);
                    iY += (20.0f * uiScale);

                    float descH = UIWidget::drawTextWrapped(renderer, slotData.itemPtr->description, iX, iY, iW, Theme::colors.textPrimary, uiScale * 0.78f);
                    iY += descH + (16.0f * uiScale);

                    // Contextual Action Buttons
                    float btnW = (iW - (2 * 4.0f * uiScale)) / 3.0f;
                    float btnH = 24.0f * uiScale;

                    if (gameContext->selectedInventorySide == 0)
                    {
                        // Backpack Side: [ Equip / Use ] [ Drop 1 ] [ Drop All ]
                        SDL_FRect b1 = { iX, iY, btnW, btnH };
                        bool h1 = (mousePos.x >= b1.x && mousePos.x <= b1.x + b1.w && mousePos.y >= b1.y && mousePos.y <= b1.y + b1.h);
                        UIWidget::drawButton(renderer, b1, "Equip/Use", h1, true, false, uiScale * 0.70f);
                        if (h1 && clicked) {
                            gameContext->handleEquipAction(gameContext->selectedInventoryIndex);
                            gameContext->input.consumeMouseClick();
                        }

                        SDL_FRect b2 = { iX + btnW + (4.0f * uiScale), iY, btnW, btnH };
                        bool h2 = (mousePos.x >= b2.x && mousePos.x <= b2.x + b2.w && mousePos.y >= b2.y && mousePos.y <= b2.y + b2.h);
                        UIWidget::drawButton(renderer, b2, "Drop 1", h2, true, false, uiScale * 0.70f);
                        if (h2 && clicked) {
                            gameContext->handleDropAction(gameContext->selectedInventoryIndex, 1);
                            gameContext->input.consumeMouseClick();
                        }

                        SDL_FRect b3 = { iX + ((btnW + (4.0f * uiScale)) * 2.0f), iY, btnW, btnH };
                        bool h3 = (mousePos.x >= b3.x && mousePos.x <= b3.x + b3.w && mousePos.y >= b3.y && mousePos.y <= b3.y + b3.h);
                        UIWidget::drawButton(renderer, b3, "Drop All", h3, true, false, uiScale * 0.70f);
                        if (h3 && clicked) {
                            gameContext->handleDropAction(gameContext->selectedInventoryIndex, 999);
                            gameContext->input.consumeMouseClick();
                        }
                    }
                    else
                    {
                        // Ground Side: [ Pick Up 1 ] [ Pick Up All ]
                        float halfBtnW = (iW - (4.0f * uiScale)) / 2.0f;
                        SDL_FRect b1 = { iX, iY, halfBtnW, btnH };
                        bool h1 = (mousePos.x >= b1.x && mousePos.x <= b1.x + b1.w && mousePos.y >= b1.y && mousePos.y <= b1.y + b1.h);
                        UIWidget::drawButton(renderer, b1, "Pick Up 1", h1, true, false, uiScale * 0.72f);
                        if (h1 && clicked) {
                            gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 1);
                            gameContext->input.consumeMouseClick();
                        }

                        SDL_FRect b2 = { iX + halfBtnW + (4.0f * uiScale), iY, halfBtnW, btnH };
                        bool h2 = (mousePos.x >= b2.x && mousePos.x <= b2.x + b2.w && mousePos.y >= b2.y && mousePos.y <= b2.y + b2.h);
                        UIWidget::drawButton(renderer, b2, "Pick Up All", h2, true, false, uiScale * 0.72f);
                        if (h2 && clicked) {
                            gameContext->handlePickupAction(gameContext->selectedInventoryIndex, 999);
                            gameContext->input.consumeMouseClick();
                        }
                    }

                    curY += cardH + (8.0f * uiScale);
                    return (curY - startY);
                }
            }
        }

        UIWidget::drawText(renderer, "No item or slot selected.", iX, iY, Theme::colors.textMuted, uiScale * 0.80f);
        iY += (20.0f * uiScale);
        UIWidget::drawText(renderer, "Click any equipment slot or backpack item to inspect its properties and take action.", iX, iY, Theme::colors.textSecondary, uiScale * 0.75f);

        curY += cardH + (8.0f * uiScale);
        return (curY - startY);
    }
}
