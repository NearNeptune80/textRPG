#include "ui/widgets/sidebarGeometry.h"

#include <vector>
#include <string>
#include <utility>

#include "ui/uiWidget.h"
#include "ui/tooltipManager.h"
#include "core/game.h"
#include "state/inventoryState.h"
#include "state/phoneAppsState.h"
#include "state/mainMenuState.h"
#include "state/characterCreationState.h"

namespace SidebarGeometry
{
    float renderNavigationToolbar(SDL_Renderer* renderer, game* gameContext, const SDL_FRect& panelRect, float cardTopY, float uiScale)
    {
        if (!gameContext) return 0.0f;

        const float boxSize = getBoxSize(panelRect, uiScale);
        const float gridStartX = getGridStartX(panelRect, uiScale);
        const float toolH = getToolbarH(uiScale);
        const float gap = 4.0f * uiScale;
        const float toolW = (boxSize - (2.0f * gap)) / 3.0f;
        const float toolbarY = cardTopY + (18.0f * uiScale) + boxSize + (5.0f * uiScale);

        static const std::vector<std::pair<std::string, CommandType>> tools = {
            { "Inv", CommandType::OPEN_INVENTORY },
            { "Phone", CommandType::OPEN_PHONE },
            { "Main Menu", CommandType::OPEN_MAIN_MENU }
        };

        TooltipPoint mousePos = gameContext->input.getMousePosition();
        bool clicked = gameContext->input.isLeftMouseJustClicked();
        bool inCharacterCreation = (dynamic_cast<characterCreationState*>(gameContext->getActiveState()) != nullptr);

        for (size_t i = 0; i < tools.size(); ++i)
        {
            SDL_FRect tRect = { gridStartX + (i * (toolW + gap)), toolbarY, toolW, toolH };
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

        return toolH;
    }
}
