#include "inputHandler.h"
#include "game.h"
#include "uiWidget.h"
#include "saveManager.h"
#include <algorithm>
#include <cmath>

void InputHandler::handleEvents(game* g)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT) g->isRunning = false;

        if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            SDL_SetRenderLogicalPresentation(g->renderer, event.window.data1, event.window.data2, SDL_LOGICAL_PRESENTATION_STRETCH);
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            handleMouseClick(g, event.button.x, event.button.y);
        }

        if (event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            handleMouseWheelInput(g, event);
        }

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            handleKeyboardInput(g, event);
        }
    }
}

void InputHandler::handleMouseWheelInput(game* g, const SDL_Event& event)
{
    if (g->currentState == GameState::INVENTORY)
    {
        g->descriptionScrollY -= event.wheel.y * 18.0f;
        g->descriptionScrollY = std::clamp(g->descriptionScrollY, 0.0f, g->maxDescriptionScrollY);
    }
}

void InputHandler::handleKeyboardInput(game* g, const SDL_Event& event)
{
    if (event.key.key == SDLK_I)
    {
        if (g->currentState == GameState::EVENT) return;

        g->selectedInventoryIndex = -1;
        g->selectedEquipmentSlot = equipSlot::NONE;

        if (g->currentState == GameState::EXPLORATION) g->currentState = GameState::INVENTORY;
        else if (g->currentState == GameState::INVENTORY) g->currentState = GameState::EXPLORATION;

        g->refreshActionGrid();
        return;
    }

    if (event.key.key == SDLK_M)
    {
        g->currentState = (g->currentState == GameState::MAIN_MENU) ? GameState::EXPLORATION : GameState::MAIN_MENU;
        return;
    }

    if (event.key.key == SDLK_F5) { saveManager::saveGame(g, "data/saves/save_01.json"); return; }
    if (event.key.key == SDLK_F9) { saveManager::loadGame(g, "data/saves/save_01.json"); return; }

    if (g->currentState == GameState::EXPLORATION)
    {
        int nextX = g->gridX, nextY = g->gridY;
        bool isMoveKey = true;

        switch (event.key.key)
        {
            case SDLK_UP:    nextY--; break;
            case SDLK_DOWN:  nextY++; break;
            case SDLK_LEFT:  nextX--; break;
            case SDLK_RIGHT: nextX++; break;
            default: isMoveKey = false; break;
        }

        if (isMoveKey) g->movePlayer(nextX, nextY);
    }
}

void InputHandler::handleMouseClick(game* g, float windowX, float windowY)
{
    float mouseX, mouseY;
    SDL_RenderCoordinatesFromWindow(g->renderer, windowX, windowY, &mouseX, &mouseY);

    int w = 0, h = 0;
    SDL_RendererLogicalPresentation mode;
    if (!SDL_GetRenderLogicalPresentation(g->renderer, &w, &h, &mode)) SDL_GetRenderOutputSize(g->renderer, &w, &h);

    g->updateLayoutBounds(w, h);

    if (handleActionGridClick(g, mouseX, mouseY)) return;

    if (g->currentState == GameState::EXPLORATION)
    {
        handleMapClick(g, mouseX, mouseY);
    }
    else if (g->currentState == GameState::INVENTORY)
    {
        if (handleEquipmentGridClick(g, mouseX, mouseY)) return;

        if (UIGridHelper::contains(g->layout.inventoryGridRect, mouseX, mouseY))
        {
            float localMouseX = mouseX - g->layout.inventoryGridRect.x;
            float localMouseY = mouseY - g->layout.inventoryGridRect.y;
            SDL_FRect localBounds = { 0.0f, 0.0f, g->layout.inventoryGridRect.w, g->layout.inventoryGridRect.h };

            if (handleInventoryTabClick(g, localMouseX, localMouseY, localBounds)) return;
            handleInventorySlotClick(g, localMouseX, localMouseY, localBounds);
        }
    }
}

bool InputHandler::handleActionGridClick(game* g, float mouseX, float mouseY)
{
    if (!UIGridHelper::contains(g->layout.actionGridRect, mouseX, mouseY)) return false;

    float localX = mouseX - g->layout.actionGridRect.x;
    float localY = mouseY - g->layout.actionGridRect.y;
    SDL_FRect localBounds = { 0.0f, 0.0f, g->layout.actionGridRect.w, g->layout.actionGridRect.h };

    auto [leftArrow, rightArrow] = UIGridHelper::getNavigationArrows(localBounds);

    if (UIGridHelper::contains(leftArrow, localX, localY))
    {
        if (g->actionGridPage > 0) g->actionGridPage--;
        return true;
    }
    if (UIGridHelper::contains(rightArrow, localX, localY))
    {
        g->actionGridPage++;
        return true;
    }

    SDL_FRect gridBounds = UIGridHelper::getActionGridBounds(localBounds);
    auto currentSlots = g->getSlotsForCurrentActionPage();
    int cols = 5, rows = 3;

    for (int i = 0; i < cols * rows; i++)
    {
        int c = i % cols, r = i / cols;
        SDL_FRect btnRect = UIGridHelper::getActionButtonRect(gridBounds, c, r, cols, rows);

        if (UIGridHelper::contains(btnRect, localX, localY))
        {
            if (!currentSlots[i].label.empty() && currentSlots[i].isEnabled && currentSlots[i].onClick)
            {
                currentSlots[i].onClick();
            }
            return true;
        }
    }
    return true;
}

bool InputHandler::handleMapClick(game* g, float mouseX, float mouseY)
{
    if (!UIGridHelper::contains(g->layout.mapRect, mouseX, mouseY)) return false;

    for (int i = 0; i < 25; i++)
    {
        int c = i % 5, r = i / 5;
        SDL_FRect tileSlot = UIGridHelper::getMapTileRect(g->layout.mapRect, c, r, 12.0f);

        if (UIGridHelper::contains(tileSlot, mouseX, mouseY))
        {
            int dx = c - 2, dy = r - 2;
            if (std::abs(dx) + std::abs(dy) == 1) g->movePlayer(g->gridX + dx, g->gridY + dy);
            return true;
        }
    }
    return false;
}

bool InputHandler::handleEquipmentGridClick(game* g, float mouseX, float mouseY)
{
    float padding = 12.0f;
    SDL_FRect playerEquipRect = g->layout.equipRect;
    SDL_FRect rightEquipRect = { g->layout.rightStackTop.x, g->layout.mapRect.y, g->layout.rightStackTop.w, g->layout.mapRect.h };

    int cols = 6, rows = 6;
    entity* targetChar = UIGridHelper::contains(playerEquipRect, mouseX, mouseY) ? g->Player :
        (g->activeTargetNPC && UIGridHelper::contains(rightEquipRect, mouseX, mouseY)) ? g->activeTargetNPC : nullptr;

    if (!targetChar) return false;
    SDL_FRect targetRect = (targetChar == g->Player) ? playerEquipRect : rightEquipRect;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            SDL_FRect slot = UIGridHelper::getEquipmentSlotRect(targetRect, c, r, cols, rows, 4.0f, padding);
            if (UIGridHelper::contains(slot, mouseX, mouseY))
            {
                int slotIdx = (r * cols) + c;
                g->selectedEquipmentSlot = equipSlot::NONE;
                g->selectedInventoryIndex = -1;
                g->selectedInventorySide = (targetChar == g->Player) ? 0 : 1;

                if (targetChar)
                {
                    for (const auto& [eSlot, eqItem] : targetChar->inventory.equipped)
                    {
                        if (g->getEquipmentGridIndex(eSlot) == slotIdx && eqItem && !eqItem->id.empty())
                        {
                            g->selectedEquipmentSlot = eSlot;
                            break;
                        }
                    }
                }

                g->refreshActionGrid();
                return true;
            }
        }
    }
    return false;
}

bool InputHandler::handleInventoryTabClick(game* g, float localMouseX, float localMouseY, SDL_FRect localBounds)
{
    for (int side = 0; side < 2; side++)
    {
        for (int t = 0; t < 7; t++)
        {
            SDL_FRect tabRect = UIGridHelper::getInventoryTabRect(localBounds, side, t);
            if (UIGridHelper::contains(tabRect, localMouseX, localMouseY))
            {
                if (side == 0) g->currentInventoryPage = t;
                else g->currentRightInventoryPage = t;

                g->selectedInventoryIndex = -1;
                g->descriptionScrollY = 0.0f;
                g->refreshActionGrid();
                return true;
            }
        }
    }
    return false;
}

bool InputHandler::handleInventorySlotClick(game* g, float localMouseX, float localMouseY, SDL_FRect localBounds)
{
    int cols = 6, rows = 5;
    int itemsPerPage = cols * rows;

    for (int side = 0; side < 2; side++)
    {
        int activePage = (side == 0) ? g->currentInventoryPage : g->currentRightInventoryPage;
        int pageOffset = activePage * itemsPerPage;

        for (int i = 0; i < itemsPerPage; i++)
        {
            int gridSlotIdx = (side * itemsPerPage) + i;
            SDL_FRect slot = UIGridHelper::getInventorySlotRect(localBounds, gridSlotIdx, cols, rows);

            if (UIGridHelper::contains(slot, localMouseX, localMouseY))
            {
                g->selectedEquipmentSlot = equipSlot::NONE;
                int absoluteItemIdx = pageOffset + i;

                InventorySlotInfo info = g->getInventorySlotItem(side, absoluteItemIdx);
                g->selectedInventorySide = side;
                g->selectedInventoryIndex = info.isValid ? absoluteItemIdx : -1;

                g->descriptionScrollY = 0.0f;
                g->refreshActionGrid();
                return true;
            }
        }
    }
    return false;
}