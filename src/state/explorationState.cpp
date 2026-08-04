#include "state/explorationState.h"

#include <iostream>
#include <memory>

#include "core/game.h"
#include "save/saveManager.h"
#include "state/inventoryState.h"
#include "ui/uiRenderer.h"

void explorationState::initialise(game* gameContext) {}

void explorationState::onEnter(game* gameContext)
{
    gameContext->refreshActionGrid();
}

void explorationState::onExit(game* gameContext) {}

void explorationState::update(game* gameContext, float deltaTime) {}

void explorationState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.key == SDLK_I)
        {
            gameContext->changeState(std::make_unique<inventoryState>());
            return;
        }

        if (event.key.scancode == SDL_SCANCODE_F5)
        {
            saveManager::saveNamedGame(gameContext, "QuickSave");
            std::cout << "[Save] QuickSave created successfully!\n";
            return;
        }

        if (event.key.scancode == SDL_SCANCODE_F9)
        {
            std::string charName = (gameContext->Player && !gameContext->Player->name.empty()) ? gameContext->Player->name : "Hero";
            std::string fileName = charName + "_QuickSave.json";

            if (saveManager::loadFromFile(gameContext, fileName))
            {
                std::cout << "[Save] QuickSave loaded successfully!\n";
                gameContext->refreshActionGrid();
            }
            else
            {
                std::cout << "[Save] No QuickSave file found to load.\n";
            }
            return;
        }

        int nextX = gameContext->gridX;
        int nextY = gameContext->gridY;
        bool isMoveKey = true;

        switch (event.key.key)
        {
            case SDLK_UP:    nextY--; break;
            case SDLK_DOWN:  nextY++; break;
            case SDLK_LEFT:  nextX--; break;
            case SDLK_RIGHT: nextX++; break;
            default: isMoveKey = false; break;
        }

        if (isMoveKey)
        {
            gameContext->movePlayer(nextX, nextY);
        }
    }
}

void explorationState::render(game* gameContext)
{
    UI::DrawMapGrid(gameContext->renderer, gameContext, gameContext->layout.mapRect, gameContext->map, gameContext->gridX, gameContext->gridY, 12);

    ViewportGuard vpGuard(gameContext->renderer, gameContext->layout.textMainRect);
    UI::DrawPanel(gameContext->renderer, { 0.0f, 0.0f, gameContext->layout.textMainRect.w, gameContext->layout.textMainRect.h }, Theme::colors.bgPanel, Theme::colors.borderNormal);
}