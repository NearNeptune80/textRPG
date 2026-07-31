#include "explorationState.h"
#include "inventoryState.h"
#include "../game.h"
#include "../uiRenderer.h"
#include "../uiWidget.h"
#include "../actionGridManager.h"
#include "../saveManager.h"
#include <cmath>

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

        if (event.key.key == SDLK_M)
        {
            gameContext->currentState = (gameContext->currentState == GameState::MAIN_MENU) ? GameState::EXPLORATION : GameState::MAIN_MENU;
            return;
        }

        if (event.key.key == SDLK_F5) { saveManager::saveGame(gameContext, "data/saves/save_01.json"); return; }
        if (event.key.key == SDLK_F9) { saveManager::loadGame(gameContext, "data/saves/save_01.json"); return; }

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
    
    // Central dialogue / text panel
    ViewportGuard vpGuard(gameContext->renderer, gameContext->layout.textMainRect);
    UI::DrawPanel(gameContext->renderer, { 0.0f, 0.0f, gameContext->layout.textMainRect.w, gameContext->layout.textMainRect.h }, Theme::colors.bgPanel, Theme::colors.borderNormal);
}