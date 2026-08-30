#include "ui/widgets/paperdollWidgets.h"
#include "ui/uiWidget.h"
#include "ui/theme.h"
#include "core/game.h"
#include "entities/entity.h"
#include <format>
#include <vector>

namespace PaperdollWidgets
{
    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        float startY = curY;
        float padX = curX + (10.0f * uiScale);

        UIWidget::drawText(renderer, "EQUIPMENT PAPERDOLL", padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);

        static const std::vector<std::pair<std::string, equipSlot>> slots = {
            { "HEAD", equipSlot::HEADWEAR }, { "CHEST", equipSlot::TORSO_OVER }, { "HANDS", equipSlot::HANDS },
            { "MAIN", equipSlot::WEAPON_MAIN }, { "OFF", equipSlot::WEAPON_OFF }, { "LEGS", equipSlot::LEGS_OUTER },
            { "FEET", equipSlot::FEET }, { "NECK", equipSlot::NECKWEAR }, { "RING", equipSlot::FINGER_PRIMARY }
        };

        float slotW = (innerW - (28.0f * uiScale)) / 3.0f;
        float slotH = 34.0f * uiScale;

        for (size_t i = 0; i < slots.size(); ++i)
        {
            int col = i % 3;
            int row = i / 3;
            float slotX = padX + (col * (slotW + 4.0f * uiScale));
            float slotY = curY + (row * (slotH + 4.0f * uiScale));

            SDL_FRect sRect = { slotX, slotY, slotW, slotH };
            bool isSelected = (gameContext->selectedEquipmentSlot == slots[i].second);
            UIWidget::drawPanel(renderer, sRect, isSelected ? Theme::colors.bgHeader : Theme::colors.bgSlot, isSelected ? Theme::colors.borderButton : Theme::colors.borderNormal);

            UIWidget::drawText(renderer, slots[i].first, slotX + 4.0f * uiScale, slotY + 3.0f * uiScale, Theme::colors.textSecondary, uiScale * 0.8f);

            std::string equippedName = "---";
            if (entity* player = gameContext->getPlayer())
            {
                if (auto eq = player->inventory.getEquippedItem(slots[i].second))
                {
                    equippedName = eq->name;
                }
            }
            UIWidget::drawText(renderer, equippedName, slotX + 4.0f * uiScale, slotY + 16.0f * uiScale, Theme::colors.textPrimary, uiScale * 0.85f);
        }

        curY += (3 * (slotH + 4.0f * uiScale)) + (6.0f * uiScale);
        return (curY - startY);
    }

    float renderWidgetItemInspector(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        float startY = curY;
        float padX = curX + (10.0f * uiScale);
        float availableW = innerW - (20.0f * uiScale);

        UIWidget::drawText(renderer, "ITEM DETAILS & LORE", padX, curY, Theme::colors.textGold, uiScale);
        curY += (18.0f * uiScale);

        if (gameContext->selectedInventoryIndex >= 0)
        {
            auto items = (gameContext->selectedInventorySide == 0)
                ? gameContext->getPlayerInventoryStacked()
                : gameContext->getTileInventoryStacked();

            if (gameContext->selectedInventoryIndex < static_cast<int>(items.size()))
            {
                const auto& slot = items[gameContext->selectedInventoryIndex];
                if (slot.itemPtr)
                {
                    UIWidget::drawText(renderer, std::format("Name: {} (x{})", slot.itemPtr->name, slot.totalCount), padX, curY, Theme::colors.textGold, uiScale);
                    curY += (16.0f * uiScale);
                    UIWidget::drawText(renderer, std::format("Type: Item | Value: {}¤", slot.itemPtr->baseValue), padX, curY, Theme::colors.textSecondary, uiScale);
                    curY += (16.0f * uiScale);

                    float descH = UIWidget::drawTextWrapped(renderer, slot.itemPtr->description, padX, curY, availableW, Theme::colors.textPrimary, uiScale);
                    curY += descH + (6.0f * uiScale);
                    return (curY - startY);
                }
            }
        }

        UIWidget::drawText(renderer, "No item selected. Select an item from inventory to inspect details.", padX, curY, Theme::colors.textMuted, uiScale);
        curY += (16.0f * uiScale);
        return (curY - startY);
    }
}
