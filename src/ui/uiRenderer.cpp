#include "ui/uiRenderer.h"

#include "core/game.h"
#include "ui/theme.h"

void uiRenderer::render(SDL_Renderer* renderer, game* gameContext)
{
    if (!renderer || !gameContext) return;

    // Clear Screen with Theme Dark Background
    SDL_SetRenderDrawColor(renderer, Theme::colors.bgDark.r, Theme::colors.bgDark.g, Theme::colors.bgDark.b, 255);
    SDL_RenderClear(renderer);

    // View layer rendering logic draws here based on gameContext state...

    SDL_RenderPresent(renderer);
}