#include "state/explorationState.h"

#include <iostream>
#include <memory>

#include "core/game.h"
#include "save/saveManager.h"
#include "state/inventoryState.h"

void explorationState::initialise(game* gameContext) {}

void explorationState::onEnter(game* gameContext)
{
    if (gameContext)
    {
        gameContext->refreshActionGrid();
    }
}

void explorationState::onExit(game* gameContext) {}

void explorationState::update(game* gameContext, float deltaTime) {}

void explorationState::handleInput(game* gameContext, const SDL_Event& event)
{
    if (!gameContext) return;

    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
    {
        if (event.key.key == SDLK_I)
        {
            gameContext->changeState(std::make_unique<inventoryState>());
            return;
        }

        // F5: Manual QuickSave
        if (event.key.scancode == SDL_SCANCODE_F5)
        {
            saveManager::saveNamedGame(gameContext, "QuickSave");
            std::cout << "[Save] QuickSave created successfully!\n";
            return;
        }

        // F9: QuickLoad
        if (event.key.scancode == SDL_SCANCODE_F9)
        {
            entity* p = gameContext->getPlayer();
            std::string charName = (p && !p->name.empty()) ? p->name : "Hero";
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
            case SDLK_W:
            case SDLK_UP:    nextY--; break;
            case SDLK_S:
            case SDLK_DOWN:  nextY++; break;
            case SDLK_A:
            case SDLK_LEFT:  nextX--; break;
            case SDLK_D:
            case SDLK_RIGHT: nextX++; break;
            default: isMoveKey = false; break;
        }

        if (isMoveKey)
        {
            gameContext->movePlayer(nextX, nextY);
        }
    }
}

void explorationState::handleCommand(game* gameContext, const UICommand& cmd)
{
    if (!gameContext) return;

    if (cmd.type == CommandType::MOVE_PLAYER)
    {
        gameContext->movePlayer(cmd.intPayload1, cmd.intPayload2);
    }
}