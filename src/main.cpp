#include <iostream>
#include <SDL3/SDL.h>

#include "core/game.h"
#include "ui/theme.h"
#include "ui/uiRenderer.h"

int main(int argc, char* argv[])
{
    // 1. Initialise SDL Video Subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "[SDL] Failed to initialise video: " << SDL_GetError() << "\n";
        return -1;
    }

    const int windowWidth = 1280;
    const int windowHeight = 720;

    // 2. Create Window and Renderer Context
    SDL_Window* window = SDL_CreateWindow("Headless Game Engine", windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "[SDL] Failed to create window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        std::cerr << "[SDL] Failed to create renderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, windowWidth, windowHeight, SDL_LOGICAL_PRESENTATION_STRETCH);

    // 3. Initialise UI Theme
    Theme::loadFromFile("data/theme.json");

    // 4. Initialise Headless Engine
    game engine;
    engine.init();

    // 5. Initialise UI Presentation Renderer
    uiRenderer view;

    uint64_t lastTime = SDL_GetTicks();

    // 6. Main Decoupled Loop
    while (engine.isRunning)
    {
        uint64_t currentTime = SDL_GetTicks();
        float deltaTime = static_cast<float>(currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Step A: Input Processing
        engine.handleEvents();

        // Step B: Headless Simulation Update
        engine.update(deltaTime);

        // Step C: Isolated UI View Render
        view.render(renderer, &engine);
    }

    // 7. Cleanup & Exit
    engine.clean();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}