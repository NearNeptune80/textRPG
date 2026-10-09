#include "ui/widgets/paperdollWidgets.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <SDL3_image/SDL_image.h>

#include "core/game.h"
#include "entities/entity.h"
#include "items/item.h"
#include "items/clothingDisplacement.h"
#include "state/characterCreationState.h"
#include "state/inventoryState.h"
#include "state/phoneAppsState.h"
#include "state/mainMenuState.h"
#include "ui/theme.h"
#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"
#include "ui/widgets/radarWidget.h"
#include "ui/widgets/sidebarGeometry.h"

namespace PaperdollWidgets
{
    static bool s_playerTattooMode = false;
    static bool s_partnerTattooMode = false;

    struct EquipGridSlotDef {
        std::string shortName;
        std::string fullName;
        equipSlot slot;
        bool isSwapButton = false;
    };

    static const std::array<EquipGridSlotDef, 36> s_equipmentGrid = {{
        { "EYES", "Eyes / Eyewear", equipSlot::EYEWEAR },
        { "HEAD", "Headwear / Hat", equipSlot::HEADWEAR },
        { "HAIR", "Hair Accessory", equipSlot::HAIR_WEAR },
        { "HORN", "Horn Jewelry", equipSlot::HORNS_SLOT },
        { "MAIN", "Main Hand Weapon", equipSlot::WEAPON_MAIN },
        { "OFF", "Off-Hand / Shield", equipSlot::WEAPON_OFF },

        { "MOUTH", "Mouth / Mask", equipSlot::MOUTHWEAR },
        { "NECK", "Neck / Scarf", equipSlot::NECKWEAR },
        { "COAT", "Over-Torso / Coat", equipSlot::TORSO_OVER },
        { "WING", "Wing Accessory", equipSlot::WINGS_SLOT },
        { "P.EAR", "Ear Piercing", equipSlot::PIERCING_EAR },
        { "P.NOS", "Nose Piercing", equipSlot::PIERCING_NOSE },

        { "WRIST", "Wrists / Cuffs", equipSlot::WRISTS },
        { "SHIRT", "Torso / Shirt", equipSlot::TORSO_UNDER },
        { "BRA", "Chest / Bra", equipSlot::CHEST_WEAR },
        { "NIPL", "Nipples / Pasties", equipSlot::NIPPLES_WEAR },
        { "P.LIP", "Lip Piercing", equipSlot::PIERCING_LIP },
        { "P.TNG", "Tongue Piercing", equipSlot::PIERCING_TONGUE },

        { "HAND", "Hands / Gloves", equipSlot::HANDS },
        { "BELT", "Waist / Belt", equipSlot::HIPS_WEAR },
        { "STOM", "Stomach / Corset", equipSlot::STOMACH_WEAR },
        { "RING", "Finger Ring", equipSlot::FINGER_PRIMARY },
        { "P.NIP", "Nipple Piercing", equipSlot::PIERCING_NIPPLE },
        { "P.NAV", "Navel Piercing", equipSlot::PIERCING_NAVEL },

        { "ANKL", "Ankles / Bands", equipSlot::ANKLES },
        { "PANT", "Legs / Pants", equipSlot::LEGS_OUTER },
        { "UNDY", "Groin / Panties", equipSlot::GROIN_OVER },
        { "TAIL", "Tail Accessory", equipSlot::TAIL_SLOT },
        { "P.COK", "Cock Piercing", equipSlot::PIERCING_COCK },
        { "P.VAG", "Vagina Piercing", equipSlot::PIERCING_VAGINA },

        { "SOCK", "Socks / Calves", equipSlot::CALVES },
        { "SHOE", "Feet / Shoes", equipSlot::FEET },
        { "PLUG", "Anus / Plug", equipSlot::ASS_WEAR },
        { "COCK", "Penis / Condom", equipSlot::PENIS_WEAR },
        { "VAG", "Vagina / Insert", equipSlot::VAGINA_WEAR },
        { "TATTOO", "Switch to Tattoos", equipSlot::NONE, true }
    }};

    struct TattooGridSlotDef {
        std::string shortName;
        std::string fullName;
        tattooSlot slot;
        bodySlot reqPart = bodySlot::HEAD;
        bool requiresGating = false;
        bool isSwapButton = false;
    };

    static const std::array<TattooGridSlotDef, 36> s_tattooGrid = {{
        { "FACE", "Face / Forehead", tattooSlot::FACE, bodySlot::HEAD, false },
        { "CHK.L", "Left Cheek", tattooSlot::FACE, bodySlot::HEAD, false },
        { "CHK.R", "Right Cheek", tattooSlot::FACE, bodySlot::HEAD, false },
        { "NECK", "Neck / Throat", tattooSlot::NECK, bodySlot::NECK, false },
        { "HORN", "Horns", tattooSlot::NONE, bodySlot::HORNS, true },
        { "EARS", "Ears", tattooSlot::NONE, bodySlot::EARS, false },

        { "SHL.L", "Left Shoulder", tattooSlot::SHOULDERS, bodySlot::TORSO, false },
        { "ARM.L", "Upper Left Arm", tattooSlot::ARM_LEFT, bodySlot::ARMS, false },
        { "CHST", "Chest / Sternum", tattooSlot::CHEST, bodySlot::BREASTS, false },
        { "ARM.R", "Upper Right Arm", tattooSlot::ARM_RIGHT, bodySlot::ARMS, false },
        { "SHL.R", "Right Shoulder", tattooSlot::SHOULDERS, bodySlot::TORSO, false },
        { "WING", "Wings", tattooSlot::NONE, bodySlot::WINGS, true },

        { "FOR.L", "Left Forearm", tattooSlot::ARM_LEFT, bodySlot::ARMS, false },
        { "BST.L", "Left Breast", tattooSlot::BREASTS, bodySlot::BREASTS, true },
        { "CLEV", "Cleavage", tattooSlot::CHEST, bodySlot::BREASTS, false },
        { "BST.R", "Right Breast", tattooSlot::BREASTS, bodySlot::BREASTS, true },
        { "FOR.R", "Right Forearm", tattooSlot::ARM_RIGHT, bodySlot::ARMS, false },
        { "HAND", "Hands / Knuckles", tattooSlot::HANDS, bodySlot::HANDS, false },

        { "STOM", "Stomach", tattooSlot::STOMACH, bodySlot::STOMACH, false },
        { "NAVL", "Belly Button", tattooSlot::STOMACH, bodySlot::STOMACH, false },
        { "U.BCK", "Upper Back", tattooSlot::BACK, bodySlot::BACK, false },
        { "M.BCK", "Middle Back", tattooSlot::BACK, bodySlot::BACK, false },
        { "L.BCK", "Lower Back (Tramp Stamp)", tattooSlot::BACK, bodySlot::BACK, false },
        { "HIPS", "Hips / Pelvis", tattooSlot::HIPS, bodySlot::HIPS, false },

        { "BUT.L", "Left Buttock", tattooSlot::ASS, bodySlot::ASS, false },
        { "BUT.R", "Right Buttock", tattooSlot::ASS, bodySlot::ASS, false },
        { "PUBC", "Pubic Mound", tattooSlot::GROIN, bodySlot::GROIN, false },
        { "PENIS", "Penis / Shaft", tattooSlot::GROIN, bodySlot::GROIN, true },
        { "VAGN", "Vagina / Labia", tattooSlot::GROIN, bodySlot::GROIN, true },
        { "TAIL", "Tail Base", tattooSlot::NONE, bodySlot::TAIL, true },

        { "THG.L", "Left Thigh", tattooSlot::LEG_LEFT, bodySlot::LEGS, false },
        { "THG.R", "Right Thigh", tattooSlot::LEG_RIGHT, bodySlot::LEGS, false },
        { "CAL.L", "Left Calf", tattooSlot::LEG_LEFT, bodySlot::LEGS, false },
        { "CAL.R", "Right Calf", tattooSlot::LEG_RIGHT, bodySlot::LEGS, false },
        { "FEET", "Feet / Ankles", tattooSlot::FEET, bodySlot::FEET, false },
        { "EQUIP", "Switch to Clothes", tattooSlot::NONE, bodySlot::HEAD, false, true }
    }};

    static bool isEquipSlotAvailable(const entity* character, equipSlot slot)
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

    static bool isTattooSlotAvailable(const entity* character, bodySlot reqSlot, bool requiresGating)
    {
        if (!character) return false;
        if (!requiresGating) return true;

        if (reqSlot == bodySlot::HORNS) return character->anatomy.hasPart(bodySlot::HORNS);
        if (reqSlot == bodySlot::WINGS) return character->anatomy.hasPart(bodySlot::WINGS);
        if (reqSlot == bodySlot::TAIL) return character->anatomy.hasPart(bodySlot::TAIL);
        if (reqSlot == bodySlot::BREASTS) return character->anatomy.hasPart(bodySlot::BREASTS);

        return true;
    }

    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float curY, float uiScale, entity* targetEntity)
    {
        if (!gameContext || !targetEntity) return 0.0f;

        entity* character = targetEntity;
        bool isPlayer = (character == gameContext->getPlayer());
        bool& inTattooMode = isPlayer ? s_playerTattooMode : s_partnerTattooMode;

        float padX = SidebarGeometry::getPadX(panelRect, uiScale);
        float availableW = SidebarGeometry::getAvailableW(panelRect, uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        const int cols = 6;
        const int rows = 6;
        const float boxSize = SidebarGeometry::getBoxSize(panelRect, uiScale);
        const float slotGap = 2.0f * uiScale;
        const float tileSize = (boxSize - (slotGap * static_cast<float>(cols - 1))) / static_cast<float>(cols);

        float toolH = SidebarGeometry::getToolbarH(uiScale);
        float cardH = isPlayer
            ? SidebarGeometry::getSquareCardH(panelRect, uiScale)
            : ((18.0f * uiScale) + boxSize + (6.0f * uiScale));

        float bottomPinnedY = SidebarGeometry::getBottomPinnedY(panelRect, uiScale);
        float startY = curY;

        if (isPlayer)
        {
            curY = std::max(curY, bottomPinnedY);
            curY += RadarWidgets::renderWidgetTimeBar(renderer, gameContext, panelRect.x, curY, panelRect.w, uiScale);
            curY += SidebarGeometry::getGapBetweenCards(uiScale);
        }
        else
        {
            curY = std::max(curY, bottomPinnedY + SidebarGeometry::getTimeBarH(uiScale) + SidebarGeometry::getGapBetweenCards(uiScale));
        }

        SDL_FRect mainCardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, mainCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerX = padX + (4.0f * uiScale);
        float cardCurY = curY + (3.0f * uiScale);

        std::string headerTitle = inTattooMode
            ? (isPlayer ? "TATTOOS (6x6)" : "NPC TATTOOS (6x6)")
            : (isPlayer ? "EQUIPMENT (6x6)" : "NPC EQUIPMENT (6x6)");
        UIWidget::drawText(renderer, headerTitle, innerX, cardCurY, Theme::colors.textGold, uiScale * 0.74f);
        cardCurY += (15.0f * uiScale);

        float gridStartX = SidebarGeometry::getGridStartX(panelRect, uiScale);
        SDL_FRect gridBox = { gridStartX - (1.0f * uiScale), cardCurY - (1.0f * uiScale), boxSize + (2.0f * uiScale), boxSize + (2.0f * uiScale) };
        UIWidget::drawPanel(renderer, gridBox, Theme::colors.bgDark, Theme::colors.borderButton);

        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int index = (r * cols) + c;
                float slotX = gridStartX + (c * (tileSize + slotGap));
                float slotY = cardCurY + (r * (tileSize + slotGap));
                SDL_FRect slotRect = { slotX, slotY, tileSize, tileSize };

                bool hovered = (mousePos.x >= slotRect.x && mousePos.x <= slotRect.x + slotRect.w &&
                                mousePos.y >= slotRect.y && mousePos.y <= slotRect.y + slotRect.h);

                if (inTattooMode)
                {
                    const auto& def = s_tattooGrid[index];
                    bool isAvailable = def.isSwapButton || isTattooSlotAvailable(character, def.reqPart, def.requiresGating);
                    bool isSelected = (!def.isSwapButton && isPlayer && gameContext->selectedEquipmentSlot == equipSlot::NONE);

                    if (def.isSwapButton)
                    {
                        SDL_Color fill = hovered ? Theme::colors.bgHeader : Theme::colors.bgHeader;
                        SDL_Color bd = hovered ? Theme::colors.borderSelected : Theme::colors.borderButton;
                        UIWidget::drawPanel(renderer, slotRect, fill, bd);

                        float lW = UIWidget::getTextWidth("EQ", uiScale * 0.65f);
                        UIWidget::drawText(renderer, "EQ", slotX + ((tileSize - lW) / 2.0f), slotY + (tileSize * 0.28f), Theme::colors.textGold, uiScale * 0.65f);

                        TooltipManager::setHoverTooltip(slotRect, mousePos, "Switch to Equipment",
                                                        "Toggles the 6x6 grid display back to wearable clothing, armor, weapons, and accessories.",
                                                        "Paperdoll View", "[ EQ ]");

                        if (hovered && clicked)
                        {
                            inTattooMode = false;
                            gameContext->input.consumeMouseClick();
                        }
                    }
                    else if (!isAvailable)
                    {
                        UIWidget::drawPanel(renderer, slotRect, Theme::colors.bgDark, Theme::colors.slotEmptyBorder);
                        float lW = UIWidget::getTextWidth("·", uiScale * 0.65f);
                        UIWidget::drawText(renderer, "·", slotX + ((tileSize - lW) / 2.0f), slotY + (tileSize * 0.25f), Theme::colors.textDisabled, uiScale * 0.65f);
                    }
                    else
                    {
                        bool hasTat = character->anatomy.hasTattoo(def.slot);
                        SDL_Color fill = hasTat ? Theme::colors.bgSlotOccupied : (hovered ? Theme::colors.bgHeader : Theme::colors.bgDark);
                        SDL_Color bd = hovered ? Theme::colors.borderButton : Theme::colors.borderNormal;
                        UIWidget::drawPanel(renderer, slotRect, fill, bd);

                        float lW = UIWidget::getTextWidth(def.shortName, uiScale * 0.52f);
                        SDL_Color textCol = hasTat ? Theme::colors.arcane : Theme::colors.textSecondary;
                        UIWidget::drawText(renderer, def.shortName, slotX + ((tileSize - lW) / 2.0f), slotY + (tileSize * 0.28f), textCol, uiScale * 0.52f);

                        if (hasTat)
                        {
                            TooltipManager::setHoverTooltip(slotRect, mousePos, def.shortName, "Permanent ink tattoo markings applied to this body region.", "Body Tattoo Canvas");
                        }
                    }
                }
                else
                {
                    const auto& def = s_equipmentGrid[index];
                    bool isAvailable = def.isSwapButton || isEquipSlotAvailable(character, def.slot);
                    int currentSide = isPlayer ? 0 : 1;
                    bool isSelected = (!def.isSwapButton && gameContext->selectedEquipmentSide == currentSide && gameContext->selectedEquipmentSlot == def.slot);

                    if (def.isSwapButton)
                    {
                        SDL_Color fill = hovered ? Theme::colors.bgHeader : Theme::colors.bgHeader;
                        SDL_Color bd = hovered ? Theme::colors.borderSelected : Theme::colors.borderButton;
                        UIWidget::drawPanel(renderer, slotRect, fill, bd);

                        float lW = UIWidget::getTextWidth("TAT", uiScale * 0.65f);
                        UIWidget::drawText(renderer, "TAT", slotX + ((tileSize - lW) / 2.0f), slotY + (tileSize * 0.28f), Theme::colors.arcane, uiScale * 0.65f);

                        TooltipManager::setHoverTooltip(slotRect, mousePos, "Switch to Tattoos",
                                                        "Toggles the 6x6 grid display to permanent body ink and cosmetic tattoos.",
                                                        "Paperdoll View", "[ TAT ]");

                        if (hovered && clicked)
                        {
                            inTattooMode = true;
                            gameContext->input.consumeMouseClick();
                        }
                    }
                    else if (!isAvailable)
                    {
                        UIWidget::drawPanel(renderer, slotRect, Theme::colors.bgDark, Theme::colors.slotEmptyBorder);
                        float lW = UIWidget::getTextWidth("·", uiScale * 0.65f);
                        UIWidget::drawText(renderer, "·", slotX + ((tileSize - lW) / 2.0f), slotY + (tileSize * 0.25f), Theme::colors.textDisabled, uiScale * 0.65f);
                    }
                    else
                    {
                        auto eqItem = character->inventory.getEquippedItem(def.slot);
                        DisplacementMode disp = character->inventory.getDisplacement(def.slot);

                        SDL_Color fill = isSelected ? Theme::colors.bgHeader : (eqItem ? Theme::colors.bgSlotOccupied : (hovered ? Theme::colors.bgHeader : Theme::colors.bgDark));
                        SDL_Color bd = isSelected ? Theme::colors.borderSelected : (hovered ? Theme::colors.borderButton : Theme::colors.borderNormal);
                        UIWidget::drawPanel(renderer, slotRect, fill, bd);

                        if (isSelected)
                        {
                            SDL_FRect innerRect = { slotRect.x + 1.0f, slotRect.y + 1.0f, slotRect.w - 2.0f, slotRect.h - 2.0f };
                            SDL_SetRenderDrawColor(renderer, Theme::colors.borderSelected.r, Theme::colors.borderSelected.g, Theme::colors.borderSelected.b, Theme::colors.borderSelected.a);
                            SDL_RenderRect(renderer, &innerRect);
                        }

                        float lW = UIWidget::getTextWidth(def.shortName, uiScale * 0.52f);
                        SDL_Color textCol = isSelected ? Theme::colors.textGold : (eqItem ? Theme::colors.textGold : Theme::colors.textSecondary);
                        UIWidget::drawText(renderer, def.shortName, slotX + ((tileSize - lW) / 2.0f), slotY + (3.0f * uiScale), textCol, uiScale * 0.52f);

                        if (eqItem)
                        {
                            if (disp != DisplacementMode::NONE)
                            {
                                float dotW = UIWidget::getTextWidth("!", uiScale * 0.65f);
                                UIWidget::drawText(renderer, "!", slotX + ((tileSize - dotW) / 2.0f), slotY + (tileSize * 0.42f), Theme::colors.lust, uiScale * 0.65f);
                            }
                            else
                            {
                                float dotW = UIWidget::getTextWidth("★", uiScale * 0.55f);
                                UIWidget::drawText(renderer, "*", slotX + ((tileSize - dotW) / 2.0f), slotY + (tileSize * 0.42f), Theme::colors.textGold, uiScale * 0.55f);
                            }

                            std::string slotName = gameContext->formatEquipSlotName(def.slot);
                            std::string status = std::format("Fit: {} • Value: {} ¤", displacementModeToString(disp), eqItem->baseValue);
                            std::string itemDesc = eqItem->getBriefDescription();
                            TooltipManager::setHoverTooltip(slotRect, mousePos, eqItem->name, itemDesc, status, slotName);
                        }

                        if (hovered && clicked)
                        {
                            if (gameContext->selectedEquipmentSlot == def.slot && gameContext->selectedEquipmentSide == currentSide)
                            {
                                gameContext->selectedEquipmentSlot = equipSlot::NONE;
                            }
                            else if (eqItem != nullptr)
                            {
                                gameContext->selectedEquipmentSlot = def.slot;
                                gameContext->selectedEquipmentSide = currentSide;
                                gameContext->selectedInventoryIndex = -1;
                            }
                            else
                            {
                                gameContext->selectedEquipmentSlot = equipSlot::NONE;
                            }
                            gameContext->refreshActionGrid();
                            gameContext->input.consumeMouseClick();
                        }
                    }
                }
            }
        }

        if (isPlayer)
        {
            SidebarGeometry::renderNavigationToolbar(renderer, gameContext, panelRect, curY, uiScale);
        }

        curY += cardH;
        return (curY - startY);
    }

    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        return renderWidgetPaperdoll(renderer, gameContext, { curX, curY, innerW, 260.0f * uiScale }, curY, uiScale, gameContext ? gameContext->getPlayer() : nullptr);
    }

    static float renderItemDetailCard(SDL_Renderer* renderer, game* gameContext, float padX, float curY, float availableW, float uiScale,
                                       const std::string& title, const std::string& metaLine, const std::string& description,
                                       const std::string& imagePath, SDL_Color titleCol, SDL_Color metaCol, SDL_Color descCol)
    {
        float innerPad = 8.0f * uiScale;
        float boxGap = 10.0f * uiScale;

        float gapBetweenSides = 8.0f * uiScale;
        float halfW = std::floor((availableW - gapBetweenSides) / 2.0f);
        float side1X = padX + halfW + gapBetweenSides;
        const int cols = 5;
        const float slotGap = 4.0f * uiScale;
        const float innerPadG = 4.0f * uiScale;
        const float tabW = 28.0f * uiScale;
        const float tabColGap = 4.0f * uiScale;

        float gridInnerW = halfW - tabW - tabColGap - (innerPadG * 2.0f);
        float slotSize = std::floor((gridInnerW - (slotGap * static_cast<float>(cols - 1))) / static_cast<float>(cols));
        float gridStartX = side1X + innerPadG;
        float col4Left = gridStartX + (3.0f * (slotSize + slotGap));

        float boxRight = padX + availableW - innerPad;
        float imageBoxSize = boxRight - col4Left;
        float boxX = col4Left;

        if (imageBoxSize < 60.0f * uiScale || col4Left <= padX + (120.0f * uiScale))
        {
            imageBoxSize = 74.0f * uiScale;
            boxX = boxRight - imageBoxSize;
        }

        float textContentW = boxX - (padX + innerPad) - boxGap;
        if (textContentW < 120.0f * uiScale)
        {
            textContentW = availableW - (innerPad * 2.0f);
        }

        std::string effectiveDesc = !description.empty() ? description : "A standard item of unknown origin.";
        float textH = UIWidget::getTextWrappedHeight(effectiveDesc, textContentW, uiScale * 0.74f);
        float leftContentH = (36.0f * uiScale) + textH + (6.0f * uiScale);
        float cardH = std::max(imageBoxSize + (innerPad * 2.0f), leftContentH + (innerPad * 2.0f));
        cardH = std::max(cardH, 70.0f * uiScale);

        SDL_FRect cardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float iX = padX + innerPad;
        float iY = curY + innerPad;

        UIWidget::drawText(renderer, title, iX, iY, titleCol, uiScale * 0.85f);

        UIWidget::drawText(renderer, metaLine, iX, iY + (18.0f * uiScale), metaCol, uiScale * 0.74f);

        UIWidget::drawTextWrapped(renderer, effectiveDesc, iX, iY + (36.0f * uiScale), textContentW, descCol, uiScale * 0.74f);

        float boxY = curY + ((cardH - imageBoxSize) / 2.0f);
        SDL_FRect imageBoxRect = { boxX, boxY, imageBoxSize, imageBoxSize };

        UIWidget::drawPanel(renderer, imageBoxRect, Theme::colors.bgDark, Theme::colors.borderButton);

        SDL_Texture* itemTexture = UIWidget::getItemTexture(renderer, imagePath);

        if (itemTexture)
        {
            SDL_FRect texRect = { boxX + (2.0f * uiScale), boxY + (2.0f * uiScale), imageBoxSize - (4.0f * uiScale), imageBoxSize - (4.0f * uiScale) };
            SDL_RenderTexture(renderer, itemTexture, nullptr, &texRect);
        }
        else
        {
            SDL_FRect innerFrame = { boxX + (3.0f * uiScale), boxY + (3.0f * uiScale), imageBoxSize - (6.0f * uiScale), imageBoxSize - (6.0f * uiScale) };
            SDL_SetRenderDrawColor(renderer, Theme::colors.borderButton.r, Theme::colors.borderButton.g, Theme::colors.borderButton.b, 60);
            SDL_RenderRect(renderer, &innerFrame);

            std::string noImg = "No Image";
            float noImgW = UIWidget::getTextWidth(noImg, uiScale * 0.72f);
            float noImgH = UIWidget::getLineHeight(uiScale * 0.72f);
            float noImgX = boxX + ((imageBoxSize - noImgW) / 2.0f);
            float noImgY = boxY + ((imageBoxSize - noImgH) / 2.0f);
            UIWidget::drawText(renderer, noImg, noImgX, noImgY, Theme::colors.textMuted, uiScale * 0.72f);
        }

        if (gameContext)
        {
            auto mousePos = gameContext->input.getMousePosition();
            TooltipManager::setHoverTooltip(imageBoxRect, mousePos, title, "Item Visual Illustration",
                                            itemTexture ? "Rendered item graphical preview." : "No visual graphic asset currently assigned.");
        }

        return cardH;
    }

    float renderWidgetItemInspector(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        if (!gameContext || !gameContext->getPlayer()) return 0.0f;

        float startY = curY;
        float padX = curX + (6.0f * uiScale);
        float availableW = innerW - (12.0f * uiScale);

        entity* player = gameContext->getPlayer();
        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        if (gameContext->selectedEquipmentSlot != equipSlot::NONE)
        {
            equipSlot slot = gameContext->selectedEquipmentSlot;
            std::string slotTitle = gameContext->formatEquipSlotName(slot);

            entity* targetEntity = (gameContext->selectedEquipmentSide == 1 && gameContext->getActiveTargetNPC())
                ? gameContext->getActiveTargetNPC()
                : player;
            bool isPlayerTarget = (targetEntity == player);

            float headerH = 22.0f * uiScale;
            SDL_FRect headerRect = { padX, curY, availableW, headerH };
            std::string inspectorTitle = isPlayerTarget
                ? std::format("EQUIPMENT INSPECTOR: {}", slotTitle)
                : std::format("NPC EQUIPMENT: {} ({})", slotTitle, targetEntity->name);
            UIWidget::drawHeader(renderer, headerRect, inspectorTitle, Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.78f);
            curY += headerH + (4.0f * uiScale);

            auto eq = targetEntity->inventory.getEquippedItem(slot);
            if (eq)
            {
                DisplacementMode disp = targetEntity->inventory.getDisplacement(slot);
                std::string dispStr = displacementModeToString(disp);
                std::string ownerStr = isPlayerTarget ? "Player" : targetEntity->name;
                std::string statusLine = std::format("Owner: {}  |  Fit: {}  |  Socket: {}  |  Value: {} ¤", ownerStr, dispStr, slotTitle, eq->baseValue);

                float cardH = renderItemDetailCard(renderer, gameContext, padX, curY, availableW, uiScale,
                                                   eq->name, statusLine, eq->description, eq->imagePath,
                                                   Theme::colors.textGold, Theme::colors.companion, Theme::colors.textSecondary);

                curY += cardH + (6.0f * uiScale);
                return (curY - startY);
            }
            else
            {
                float cardH = 40.0f * uiScale;
                SDL_FRect cardRect = { padX, curY, availableW, cardH };
                UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                std::string emptyMsg = isPlayerTarget
                    ? "No item equipped in this slot. Click an item in your backpack above or use the command grid to equip."
                    : std::format("No item equipped in this slot on {}.", targetEntity->name);
                UIWidget::drawText(renderer, emptyMsg, padX + (8.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);
                curY += cardH + (6.0f * uiScale);
                return (curY - startY);
            }
        }

        if (gameContext->selectedInventoryIndex >= 0)
        {
            auto items = (gameContext->selectedInventorySide == 0)
                ? gameContext->getPlayerInventoryStacked()
                : (gameContext->getActiveTargetNPC() ? gameContext->getActiveTargetNPC()->inventory.getStackedView() : gameContext->getTileInventoryStacked());

            if (gameContext->selectedInventoryIndex < static_cast<int>(items.size()))
            {
                const auto& slotData = items[gameContext->selectedInventoryIndex];
                if (slotData.itemPtr)
                {
                    std::string sideStr = (gameContext->selectedInventorySide == 0)
                        ? "In Backpack"
                        : (gameContext->getActiveTargetNPC() ? std::format("In {}'s Bag", gameContext->getActiveTargetNPC()->name) : "On Ground");
                    std::string targetSlotStr = gameContext->formatEquipSlotName(slotData.itemPtr->targetSlot);
                    std::string lineInfo = std::format("Location: {}  |  Socket: {}  |  Value: {} ¤", sideStr, targetSlotStr, slotData.itemPtr->baseValue);
                    std::string titleStr = std::format("{} (x{})", slotData.itemPtr->name, slotData.totalCount);

                    float cardH = renderItemDetailCard(renderer, gameContext, padX, curY, availableW, uiScale,
                                                       titleStr, lineInfo, slotData.itemPtr->description, slotData.itemPtr->imagePath,
                                                       Theme::colors.textGold, Theme::colors.currency, Theme::colors.textPrimary);

                    curY += cardH + (6.0f * uiScale);
                    return (curY - startY);
                }
            }
        }

        float hintH = 32.0f * uiScale;
        SDL_FRect hintRect = { padX, curY, availableW, hintH };
        UIWidget::drawPanel(renderer, hintRect, Theme::colors.bgDark, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Click any equipment slot or inventory item above to inspect details and take action via the action grid below.", padX + (8.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.74f);
        curY += hintH + (4.0f * uiScale);
        return (curY - startY);
    }
}
