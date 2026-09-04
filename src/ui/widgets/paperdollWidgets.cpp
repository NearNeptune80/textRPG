#include "ui/widgets/paperdollWidgets.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <vector>

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
    // Mode tracker for Player and Partner grids
    static bool s_playerTattooMode = false;
    static bool s_partnerTattooMode = false;

    struct EquipGridSlotDef {
        std::string shortName;
        std::string fullName;
        equipSlot slot;
        bool isSwapButton = false;
    };

    static const std::array<EquipGridSlotDef, 36> s_equipmentGrid = {{
        // Row 0: Eyes, Head, Hair, Horns, Main Hand, Off Hand
        { "EYES", "Eyes / Eyewear", equipSlot::EYEWEAR },
        { "HEAD", "Headwear / Hat", equipSlot::HEADWEAR },
        { "HAIR", "Hair Accessory", equipSlot::HAIR_WEAR },
        { "HORN", "Horn Jewelry", equipSlot::HORNS_SLOT },
        { "MAIN", "Main Hand Weapon", equipSlot::WEAPON_MAIN },
        { "OFF", "Off-Hand / Shield", equipSlot::WEAPON_OFF },

        // Row 1: Mouth, Neck, Over-Torso, Wings, Ear Piercing, Nose Piercing
        { "MOUTH", "Mouth / Mask", equipSlot::MOUTHWEAR },
        { "NECK", "Neck / Scarf", equipSlot::NECKWEAR },
        { "COAT", "Over-Torso / Coat", equipSlot::TORSO_OVER },
        { "WING", "Wing Accessory", equipSlot::WINGS_SLOT },
        { "P.EAR", "Ear Piercing", equipSlot::PIERCING_EAR },
        { "P.NOS", "Nose Piercing", equipSlot::PIERCING_NOSE },

        // Row 2: Wrists, Torso Under, Bra, Nipples, Lip Piercing, Tongue Piercing
        { "WRIST", "Wrists / Cuffs", equipSlot::WRISTS },
        { "SHIRT", "Torso / Shirt", equipSlot::TORSO_UNDER },
        { "BRA", "Chest / Bra", equipSlot::CHEST_WEAR },
        { "NIPL", "Nipples / Pasties", equipSlot::NIPPLES_WEAR },
        { "P.LIP", "Lip Piercing", equipSlot::PIERCING_LIP },
        { "P.TNG", "Tongue Piercing", equipSlot::PIERCING_TONGUE },

        // Row 3: Hands, Belt, Stomach, Ring, Nipple Piercing, Navel Piercing
        { "HAND", "Hands / Gloves", equipSlot::HANDS },
        { "BELT", "Waist / Belt", equipSlot::HIPS_WEAR },
        { "STOM", "Stomach / Corset", equipSlot::STOMACH_WEAR },
        { "RING", "Finger Ring", equipSlot::FINGER_PRIMARY },
        { "P.NIP", "Nipple Piercing", equipSlot::PIERCING_NIPPLE },
        { "P.NAV", "Navel Piercing", equipSlot::PIERCING_NAVEL },

        // Row 4: Ankles, Pants, Underwear, Tail, Penis Piercing, Vagina Piercing
        { "ANKL", "Ankles / Bands", equipSlot::ANKLES },
        { "PANT", "Legs / Pants", equipSlot::LEGS_OUTER },
        { "UNDY", "Groin / Panties", equipSlot::GROIN_OVER },
        { "TAIL", "Tail Accessory", equipSlot::TAIL_SLOT },
        { "P.COK", "Cock Piercing", equipSlot::PIERCING_COCK },
        { "P.VAG", "Vagina Piercing", equipSlot::PIERCING_VAGINA },

        // Row 5: Socks, Shoes, Anus Plug, Penis Wear, Vagina Wear, TATTOO SWAP BUTTON
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
        // Row 0: Head & Neck
        { "FACE", "Face / Forehead", tattooSlot::FACE, bodySlot::HEAD, false },
        { "CHK.L", "Left Cheek", tattooSlot::FACE, bodySlot::HEAD, false },
        { "CHK.R", "Right Cheek", tattooSlot::FACE, bodySlot::HEAD, false },
        { "NECK", "Neck / Throat", tattooSlot::NECK, bodySlot::NECK, false },
        { "HORN", "Horns", tattooSlot::NONE, bodySlot::HORNS, true },
        { "EARS", "Ears", tattooSlot::NONE, bodySlot::EARS, false },

        // Row 1: Shoulders & Upper Chest
        { "SHL.L", "Left Shoulder", tattooSlot::SHOULDERS, bodySlot::TORSO, false },
        { "ARM.L", "Upper Left Arm", tattooSlot::ARM_LEFT, bodySlot::ARMS, false },
        { "CHST", "Chest / Sternum", tattooSlot::CHEST, bodySlot::BREASTS, false },
        { "ARM.R", "Upper Right Arm", tattooSlot::ARM_RIGHT, bodySlot::ARMS, false },
        { "SHL.R", "Right Shoulder", tattooSlot::SHOULDERS, bodySlot::TORSO, false },
        { "WING", "Wings", tattooSlot::NONE, bodySlot::WINGS, true },

        // Row 2: Breasts & Forearms
        { "FOR.L", "Left Forearm", tattooSlot::ARM_LEFT, bodySlot::ARMS, false },
        { "BST.L", "Left Breast", tattooSlot::BREASTS, bodySlot::BREASTS, true },
        { "CLEV", "Cleavage", tattooSlot::CHEST, bodySlot::BREASTS, false },
        { "BST.R", "Right Breast", tattooSlot::BREASTS, bodySlot::BREASTS, true },
        { "FOR.R", "Right Forearm", tattooSlot::ARM_RIGHT, bodySlot::ARMS, false },
        { "HAND", "Hands / Knuckles", tattooSlot::HANDS, bodySlot::HANDS, false },

        // Row 3: Stomach & Back
        { "STOM", "Stomach", tattooSlot::STOMACH, bodySlot::STOMACH, false },
        { "NAVL", "Belly Button", tattooSlot::STOMACH, bodySlot::STOMACH, false },
        { "U.BCK", "Upper Back", tattooSlot::BACK, bodySlot::BACK, false },
        { "M.BCK", "Middle Back", tattooSlot::BACK, bodySlot::BACK, false },
        { "L.BCK", "Lower Back (Tramp Stamp)", tattooSlot::BACK, bodySlot::BACK, false },
        { "HIPS", "Hips / Pelvis", tattooSlot::HIPS, bodySlot::HIPS, false },

        // Row 4: Lower Body & Intimates
        { "BUT.L", "Left Buttock", tattooSlot::ASS, bodySlot::ASS, false },
        { "BUT.R", "Right Buttock", tattooSlot::ASS, bodySlot::ASS, false },
        { "PUBC", "Pubic Mound", tattooSlot::GROIN, bodySlot::GROIN, false },
        { "PENIS", "Penis / Shaft", tattooSlot::GROIN, bodySlot::GROIN, true },
        { "VAGN", "Vagina / Labia", tattooSlot::GROIN, bodySlot::GROIN, true },
        { "TAIL", "Tail Base", tattooSlot::NONE, bodySlot::TAIL, true },

        // Row 5: Legs, Feet & Swap Button
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
        if (!gameContext) return 0.0f;

        entity* character = targetEntity ? targetEntity : gameContext->getPlayer();
        if (!character) return 0.0f;

        bool isPlayer = (character == gameContext->getPlayer());
        bool& inTattooMode = isPlayer ? s_playerTattooMode : s_partnerTattooMode;

        float padX = SidebarGeometry::getPadX(panelRect, uiScale);
        float availableW = SidebarGeometry::getAvailableW(panelRect, uiScale);

        auto mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();

        // Calculate square dimensions for 6x6 tile grid filling the container
        const int cols = 6;
        const int rows = 6;
        const float boxSize = SidebarGeometry::getBoxSize(panelRect, uiScale);
        const float slotGap = 2.0f * uiScale;
        const float tileSize = (boxSize - (slotGap * static_cast<float>(cols - 1))) / static_cast<float>(cols);

        float toolH = SidebarGeometry::getToolbarH(uiScale);
        float cardH = SidebarGeometry::getSquareCardH(panelRect, uiScale);

        // Pin to the bottom of the sidebar panel (matching mini-map radar positioning exactly)
        float bottomPinnedY = SidebarGeometry::getBottomPinnedY(panelRect, uiScale);
        curY = std::max(curY, bottomPinnedY);
        float startY = curY;

        // Render Date & Time Bar
        curY += RadarWidgets::renderWidgetTimeBar(renderer, gameContext, panelRect.x, curY, panelRect.w, uiScale);
        curY += SidebarGeometry::getGapBetweenCards(uiScale);

        // =========================================================================
        // OVERARCHING CONTAINER: Equipment / Tattoo Grid Box
        // =========================================================================
        SDL_FRect mainCardRect = { padX, curY, availableW, cardH };
        UIWidget::drawPanel(renderer, mainCardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

        float innerX = padX + (4.0f * uiScale);
        float cardCurY = curY + (3.0f * uiScale);

        std::string headerTitle = inTattooMode ? "TATTOOS (6x6)" : "EQUIPMENT (6x6)";
        UIWidget::drawText(renderer, headerTitle, innerX, cardCurY, Theme::colors.textGold, uiScale * 0.74f);
        cardCurY += (15.0f * uiScale);

        // Centered 6x6 Grid Container Box matching Mini-Map Radar
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
                    // -----------------------------------------------------------------
                    // TATTOO MODE TILES
                    // -----------------------------------------------------------------
                    const auto& def = s_tattooGrid[index];
                    bool isAvailable = def.isSwapButton || isTattooSlotAvailable(character, def.reqPart, def.requiresGating);
                    bool isSelected = (!def.isSwapButton && isPlayer && gameContext->selectedEquipmentSlot == equipSlot::NONE);

                    if (def.isSwapButton)
                    {
                        // Swap Button: Switches back to Equipment mode
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
                        // Disabled / Locked
                        UIWidget::drawPanel(renderer, slotRect, Theme::colors.bgDark, Theme::colors.slotEmptyBorder);
                        float lW = UIWidget::getTextWidth("·", uiScale * 0.65f);
                        UIWidget::drawText(renderer, "·", slotX + ((tileSize - lW) / 2.0f), slotY + (tileSize * 0.25f), Theme::colors.textDisabled, uiScale * 0.65f);
                    }
                    else
                    {
                        // Active Tattoo Slot
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
                    // -----------------------------------------------------------------
                    // EQUIPMENT MODE TILES
                    // -----------------------------------------------------------------
                    const auto& def = s_equipmentGrid[index];
                    bool isAvailable = def.isSwapButton || isEquipSlotAvailable(character, def.slot);
                    bool isSelected = (!def.isSwapButton && isPlayer && gameContext->selectedEquipmentSlot == def.slot);

                    if (def.isSwapButton)
                    {
                        // Swap Button: Switches to Tattoo mode
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
                        // Disabled / Anatomically Locked
                        UIWidget::drawPanel(renderer, slotRect, Theme::colors.bgDark, Theme::colors.slotEmptyBorder);
                        float lW = UIWidget::getTextWidth("·", uiScale * 0.65f);
                        UIWidget::drawText(renderer, "·", slotX + ((tileSize - lW) / 2.0f), slotY + (tileSize * 0.25f), Theme::colors.textDisabled, uiScale * 0.65f);
                    }
                    else
                    {
                        // Active Equipment Socket
                        auto eqItem = character->inventory.getEquippedItem(def.slot);
                        DisplacementMode disp = character->inventory.getDisplacement(def.slot);

                        SDL_Color fill = isSelected ? Theme::colors.bgHeader : (eqItem ? Theme::colors.bgSlotOccupied : (hovered ? Theme::colors.bgHeader : Theme::colors.bgDark));
                        SDL_Color bd = isSelected ? Theme::colors.borderSelected : (hovered ? Theme::colors.borderButton : Theme::colors.borderNormal);
                        UIWidget::drawPanel(renderer, slotRect, fill, bd);

                        // Label
                        float lW = UIWidget::getTextWidth(def.shortName, uiScale * 0.52f);
                        SDL_Color textCol = isSelected ? Theme::colors.textGold : (eqItem ? Theme::colors.textGold : Theme::colors.textSecondary);
                        UIWidget::drawText(renderer, def.shortName, slotX + ((tileSize - lW) / 2.0f), slotY + (3.0f * uiScale), textCol, uiScale * 0.52f);

                        // Item indicator or displacement dot
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
                            std::string itemDesc = !eqItem->tooltip.empty() ? eqItem->tooltip : eqItem->description;
                            TooltipManager::setHoverTooltip(slotRect, mousePos, eqItem->name, itemDesc, status, slotName);
                        }

                        if (hovered && clicked && isPlayer)
                        {
                            gameContext->selectedEquipmentSlot = def.slot;
                            gameContext->selectedInventoryIndex = -1;
                            gameContext->refreshActionGrid();
                            gameContext->input.consumeMouseClick();
                        }
                    }
                }
            }
        }

        cardCurY += boxSize + (5.0f * uiScale);

        // =========================================================================
        // TOOLBAR BUTTONS UNDER THE 6x6 GRID
        // =========================================================================
        static const std::vector<std::pair<std::string, CommandType>> tools = {
            { "Inv", CommandType::OPEN_INVENTORY },
            { "Phone", CommandType::OPEN_PHONE },
            { "Main Menu", CommandType::OPEN_MAIN_MENU }
        };

        float toolW = (boxSize - (2 * 4.0f * uiScale)) / 3.0f;
        bool inCharacterCreation = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);

        for (size_t i = 0; i < tools.size(); ++i)
        {
            SDL_FRect tRect = { innerX + (i * (toolW + (4.0f * uiScale))), cardCurY, toolW, toolH };
            bool isEnabled = !inCharacterCreation || (i == 0);
            bool hov = isEnabled && (mousePos.x >= tRect.x && mousePos.x <= tRect.x + tRect.w &&
                        mousePos.y >= tRect.y && mousePos.y <= tRect.y + tRect.h);
            bool isActive = false;
            if (i == 0 && dynamic_cast<inventoryState*>(gameContext->getActiveState())) isActive = true;
            else if (i == 1 && dynamic_cast<phoneAppsState*>(gameContext->getActiveState())) isActive = true;
            else if (i == 2 && dynamic_cast<mainMenuState*>(gameContext->getActiveState())) isActive = true;

            UIWidget::drawButton(renderer, tRect, tools[i].first, hov, isEnabled, isActive, uiScale * 0.70f);

            if (isEnabled)
            {
                if (i == 0) TooltipManager::setHoverTooltip(tRect, mousePos, "Wardrobe & Inventory", inCharacterCreation ? "Open full dual 5x4 inventory and ground storage to equip starting garments and inspect items." : "Opens dual 5x4 player inventory and ground loot storage.", "Storage", "[ I ]");
                else if (i == 1) TooltipManager::setHoverTooltip(tRect, mousePos, "Phone & In-Game Actions", "Access smartphone apps, transformations, resting, and regional map.", "Communication", "[ P ]");
                else if (i == 2) TooltipManager::setHoverTooltip(tRect, mousePos, "Main Menu", "Open main menu, save/load, settings, and game options.", "System", "[ ESC ]");

                if (hov && clicked)
                {
                    gameContext->handleCommand(UICommand{ tools[i].second });
                    gameContext->input.consumeMouseClick();
                }
            }
        }

        curY += cardH;
        return (curY - startY);
    }

    float renderWidgetPaperdoll(SDL_Renderer* renderer, game* gameContext, float curX, float curY, float innerW, float uiScale)
    {
        return renderWidgetPaperdoll(renderer, gameContext, { curX, curY, innerW, 260.0f * uiScale }, curY, uiScale, nullptr);
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

            float headerH = 22.0f * uiScale;
            SDL_FRect headerRect = { padX, curY, availableW, headerH };
            UIWidget::drawHeader(renderer, headerRect, std::format("EQUIPMENT INSPECTOR: {}", slotTitle), Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.78f);
            curY += headerH + (4.0f * uiScale);

            auto eq = player->inventory.getEquippedItem(slot);
            if (eq)
            {
                float innerPad = 8.0f * uiScale;
                float iX = padX + innerPad;
                float iY = curY + (6.0f * uiScale);
                float contentW = availableW - (innerPad * 2.0f);

                // Full-width details and lore description (actions are handled via bottom action grid)
                float textH = UIWidget::drawTextWrapped(renderer, eq->description, iX, iY + (34.0f * uiScale), contentW, Theme::colors.textSecondary, uiScale * 0.74f);
                float cardH = std::max(64.0f * uiScale, (42.0f * uiScale) + textH + (8.0f * uiScale));

                SDL_FRect cardRect = { padX, curY, availableW, cardH };
                UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                UIWidget::drawText(renderer, eq->name, iX, iY, Theme::colors.textGold, uiScale * 0.85f);

                DisplacementMode disp = player->inventory.getDisplacement(slot);
                std::string dispStr = displacementModeToString(disp);
                std::string statusLine = std::format("Fit: {}  |  Socket: {}  |  Value: {} ¤", dispStr, slotTitle, eq->baseValue);
                UIWidget::drawText(renderer, statusLine, iX, iY + (17.0f * uiScale), Theme::colors.companion, uiScale * 0.74f);

                UIWidget::drawTextWrapped(renderer, eq->description, iX, iY + (34.0f * uiScale), contentW, Theme::colors.textSecondary, uiScale * 0.74f);

                curY += cardH + (6.0f * uiScale);
                return (curY - startY);
            }
            else
            {
                float cardH = 40.0f * uiScale;
                SDL_FRect cardRect = { padX, curY, availableW, cardH };
                UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);
                UIWidget::drawText(renderer, "No item equipped in this slot. Click an item in your backpack above or use the command grid to equip.", padX + (8.0f * uiScale), curY + (10.0f * uiScale), Theme::colors.textMuted, uiScale * 0.76f);
                curY += cardH + (6.0f * uiScale);
                return (curY - startY);
            }
        }

        // =========================================================================
        // CASE B: Selected Item from Backpack or Ground
        // =========================================================================
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
                    float headerH = 22.0f * uiScale;
                    SDL_FRect headerRect = { padX, curY, availableW, headerH };
                    UIWidget::drawHeader(renderer, headerRect, "ITEM DETAILS & LORE", Theme::colors.bgHeader, Theme::colors.textGold, uiScale * 0.78f);
                    curY += headerH + (4.0f * uiScale);

                    float innerPad = 8.0f * uiScale;
                    float iX = padX + innerPad;
                    float iY = curY + (6.0f * uiScale);
                    float contentW = availableW - (innerPad * 2.0f);

                    float textH = UIWidget::drawTextWrapped(renderer, slotData.itemPtr->description, iX, iY + (34.0f * uiScale), contentW, Theme::colors.textPrimary, uiScale * 0.74f);
                    float cardH = std::max(64.0f * uiScale, (42.0f * uiScale) + textH + (8.0f * uiScale));

                    SDL_FRect cardRect = { padX, curY, availableW, cardH };
                    UIWidget::drawPanel(renderer, cardRect, Theme::colors.bgSlot, Theme::colors.borderNormal);

                    UIWidget::drawText(renderer, std::format("{} (x{})", slotData.itemPtr->name, slotData.totalCount), iX, iY, Theme::colors.textGold, uiScale * 0.85f);

                    std::string sideStr = (gameContext->selectedInventorySide == 0) ? "In Backpack" : (gameContext->getActiveTargetNPC() ? "In NPC Inventory" : "On Ground");
                    std::string targetSlotStr = gameContext->formatEquipSlotName(slotData.itemPtr->targetSlot);
                    std::string lineInfo = std::format("Location: {}  |  Socket: {}  |  Value: {} ¤", sideStr, targetSlotStr, slotData.itemPtr->baseValue);
                    UIWidget::drawText(renderer, lineInfo, iX, iY + (17.0f * uiScale), Theme::colors.currency, uiScale * 0.74f);

                    UIWidget::drawTextWrapped(renderer, slotData.itemPtr->description, iX, iY + (34.0f * uiScale), contentW, Theme::colors.textPrimary, uiScale * 0.74f);

                    curY += cardH + (6.0f * uiScale);
                    return (curY - startY);
                }
            }
        }

        // Default hint card when nothing is selected
        float hintH = 32.0f * uiScale;
        SDL_FRect hintRect = { padX, curY, availableW, hintH };
        UIWidget::drawPanel(renderer, hintRect, Theme::colors.bgDark, Theme::colors.borderNormal);
        UIWidget::drawText(renderer, "Click any equipment slot or inventory item above to inspect details and take action via the action grid below.", padX + (8.0f * uiScale), curY + (8.0f * uiScale), Theme::colors.textMuted, uiScale * 0.74f);
        curY += hintH + (4.0f * uiScale);
        return (curY - startY);
    }
}
