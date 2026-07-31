#include "inputHandler.h"
#include "game.h"
#include "uiWidget.h"
#include <algorithm>

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
            float mouseX, mouseY;
            SDL_RenderCoordinatesFromWindow(g->renderer, event.button.x, event.button.y, &mouseX, &mouseY);

            int w = 0, h = 0;
            SDL_RendererLogicalPresentation mode;
            if (!SDL_GetRenderLogicalPresentation(g->renderer, &w, &h, &mode)) SDL_GetRenderOutputSize(g->renderer, &w, &h);
            g->updateLayoutBounds(w, h);

            if (handleActionGridClick(g, mouseX, mouseY)) continue;
        }

        if (g->getActiveState())
        {
            g->getActiveState()->handleInput(g, event);
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
        int totalButtons = static_cast<int>(g->activeButtons.size());
        if (totalButtons > (g->actionGridPage + 1) * 15)
        {
            g->actionGridPage++;
        }
        return true;
    }

    SDL_FRect gridBounds = UIGridHelper::getActionGridBounds(localBounds);
    auto currentSlots = g->getSlotsForCurrentActionPage();
    constexpr int cols = 5, rows = 3;

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