#include "inventoryState.h"
#include "explorationState.h"
#include "../game.h"
#include "../uiRenderer.h"
#include "../uiWidget.h"
#include "../actionGridManager.h"
#include <algorithm>

void inventoryState::initialise(game* gameContext) {}

void inventoryState::onEnter(game* gameContext)
{
    gameContext->selectedInventoryIndex = -1;
    gameContext->selectedEquipmentSlot = equipSlot::NONE;
    gameContext->refreshActionGrid();
}

void inventoryState::onExit(game* gameContext) {}

void inventoryState::update(game* gameContext, float deltaTime) {}

void inventoryState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_I)
    {
        gameContext->changeState(std::make_unique<explorationState>());
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL)
    {
        gameContext->descriptionScrollY -= event.wheel.y * 18.0f;
        gameContext->descriptionScrollY = std::clamp(gameContext->descriptionScrollY, 0.0f, gameContext->maxDescriptionScrollY);
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        float mouseX, mouseY;
        SDL_RenderCoordinatesFromWindow(gameContext->renderer, event.button.x, event.button.y, &mouseX, &mouseY);

        if (handleEquipmentClick(gameContext, mouseX, mouseY)) return;

        if (UIGridHelper::contains(gameContext->layout.inventoryGridRect, mouseX, mouseY))
        {
            float localMouseX = mouseX - gameContext->layout.inventoryGridRect.x;
            float localMouseY = mouseY - gameContext->layout.inventoryGridRect.y;
            SDL_FRect localBounds = { 0.0f, 0.0f, gameContext->layout.inventoryGridRect.w, gameContext->layout.inventoryGridRect.h };

            if (handleTabClick(gameContext, localMouseX, localMouseY, localBounds)) return;
            handleSlotClick(gameContext, localMouseX, localMouseY, localBounds);
        }
    }
}

bool inventoryState::handleEquipmentClick(game* gameContext, float mouseX, float mouseY)
{
    SDL_FRect playerEquipRect = gameContext->layout.equipRect;
    SDL_FRect rightEquipRect = { gameContext->layout.rightStackTop.x, gameContext->layout.mapRect.y, gameContext->layout.rightStackTop.w, gameContext->layout.mapRect.h };

    entity* targetChar = UIGridHelper::contains(playerEquipRect, mouseX, mouseY) ? gameContext->Player :
        (gameContext->activeTargetNPC && UIGridHelper::contains(rightEquipRect, mouseX, mouseY)) ? gameContext->activeTargetNPC : nullptr;

    if (!targetChar) return false;

    constexpr float padding = 12.0f;
    constexpr int cols = 6, rows = 6;
    SDL_FRect targetRect = (targetChar == gameContext->Player) ? playerEquipRect : rightEquipRect;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            SDL_FRect slot = UIGridHelper::getEquipmentSlotRect(targetRect, c, r, cols, rows, 4.0f, padding);
            if (UIGridHelper::contains(slot, mouseX, mouseY))
            {
                int slotIdx = (r * cols) + c;
                gameContext->selectedEquipmentSlot = equipSlot::NONE;
                gameContext->selectedInventoryIndex = -1;
                gameContext->selectedInventorySide = (targetChar == gameContext->Player) ? 0 : 1;

                for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
                {
                    equipSlot eSlot = static_cast<equipSlot>(i);
                    const auto& eqItem = targetChar->inventory.equipped[i];
                    if (gameContext->getEquipmentGridIndex(eSlot) == slotIdx && eqItem && !eqItem->id.empty())
                    {
                        gameContext->selectedEquipmentSlot = eSlot;
                        break;
                    }
                }

                gameContext->refreshActionGrid();
                return true;
            }
        }
    }
    return false;
}

bool inventoryState::handleTabClick(game* gameContext, float localMouseX, float localMouseY, SDL_FRect localBounds)
{
    constexpr int itemsPerPage = 30;

    for (int side = 0; side < 2; side++)
    {
        for (int t = 0; t < 7; t++)
        {
            SDL_FRect tabRect = UIGridHelper::getInventoryTabRect(localBounds, side, t);
            if (UIGridHelper::contains(tabRect, localMouseX, localMouseY))
            {
                bool hasItemsOnPage = false;
                int pageStart = t * itemsPerPage;

                for (int slotOffset = 0; slotOffset < itemsPerPage; slotOffset++)
                {
                    InventorySlotInfo slotInfo = gameContext->getInventorySlotItem(side, pageStart + slotOffset);
                    if (slotInfo.isValid && slotInfo.itemPtr)
                    {
                        hasItemsOnPage = true;
                        break;
                    }
                }

                int currentActivePage = (side == 0) ? gameContext->currentInventoryPage : gameContext->currentRightInventoryPage;
                if (!hasItemsOnPage && currentActivePage != t) return true;

                if (side == 0) gameContext->currentInventoryPage = t;
                else gameContext->currentRightInventoryPage = t;

                gameContext->selectedInventoryIndex = -1;
                gameContext->descriptionScrollY = 0.0f;
                gameContext->refreshActionGrid();
                return true;
            }
        }
    }
    return false;
}

bool inventoryState::handleSlotClick(game* gameContext, float localMouseX, float localMouseY, SDL_FRect localBounds)
{
    constexpr int cols = 6, rows = 5;
    constexpr int itemsPerPage = cols * rows;

    for (int side = 0; side < 2; side++)
    {
        int activePage = (side == 0) ? gameContext->currentInventoryPage : gameContext->currentRightInventoryPage;
        int pageOffset = activePage * itemsPerPage;

        for (int i = 0; i < itemsPerPage; i++)
        {
            int gridSlotIdx = (side * itemsPerPage) + i;
            SDL_FRect slot = UIGridHelper::getInventorySlotRect(localBounds, gridSlotIdx, cols, rows);

            if (UIGridHelper::contains(slot, localMouseX, localMouseY))
            {
                gameContext->selectedEquipmentSlot = equipSlot::NONE;
                int absoluteItemIdx = pageOffset + i;

                InventorySlotInfo info = gameContext->getInventorySlotItem(side, absoluteItemIdx);
                gameContext->selectedInventorySide = side;
                gameContext->selectedInventoryIndex = info.isValid ? absoluteItemIdx : -1;

                gameContext->descriptionScrollY = 0.0f;
                gameContext->refreshActionGrid();
                return true;
            }
        }
    }
    return false;
}

void inventoryState::render(game* gameContext)
{
    UI::DrawEquipmentGrid(gameContext->renderer, gameContext, gameContext->layout.equipRect, gameContext->Player, gameContext->selectedEquipmentSlot, 12);
    UI::DrawInventoryGrid(gameContext->renderer, gameContext, gameContext->layout.inventoryGridRect, gameContext->Player, gameContext->selectedInventoryIndex);
    UI::DrawItemDetailPanel(gameContext->renderer, gameContext, gameContext->layout.inventoryDetailRect, gameContext->Player, gameContext->selectedInventoryIndex);
}