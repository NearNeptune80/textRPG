#include "state/inventoryState.h"

#include <algorithm>
#include <memory>

#include "core/game.h"
#include "state/explorationState.h"

void inventoryState::initialise(game* gameContext) {}

void inventoryState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->selectedInventoryIndex = -1;
        gameContext->selectedEquipmentSlot = equipSlot::NONE;
        gameContext->refreshActionGrid();
    }
}

void inventoryState::onExit(game* gameContext) {}

void inventoryState::update(game* gameContext, float deltaTime) {}

void inventoryState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

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
}

void inventoryState::render(game* gameContext)
{
    // No-op: Pure state controller. Render layer handles all drawing independently.
}