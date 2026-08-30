#include <iostream>
#include <filesystem>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "core/game.h"
#include "state/mainMenuState.h"
#include "state/optionsState.h"
#include "state/loadGameState.h"
#include "state/characterCreationState.h"
#include "state/explorationState.h"
#include "ui/theme.h"
#include "ui/uiRenderer.h"

int main(int argc, char* argv[])
{
    // Check for --screenshot <state_name> [output_path]
    std::string screenshotState = "";
    std::string screenshotOut = "";
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--screenshot" && i + 1 < argc)
        {
            screenshotState = argv[i + 1];
            if (i + 2 < argc && argv[i + 2][0] != '-')
            {
                screenshotOut = argv[i + 2];
            }
            break;
        }
    }

    // 1. Initialise SDL Video Subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "[SDL] Failed to initialise video: " << SDL_GetError() << "\n";
        return -1;
    }

    const int windowWidth = 1280;
    const int windowHeight = 720;

    SDL_WindowFlags winFlags = SDL_WINDOW_RESIZABLE;
    if (!screenshotState.empty())
    {
        winFlags = SDL_WINDOW_HIDDEN;
    }

    // 2. Create Window and Renderer Context
    SDL_Window* window = SDL_CreateWindow("TextRPG Engine", windowWidth, windowHeight, winFlags);
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
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);

    // 3. Initialise UI Theme (Attempt Dark Fantasy default, fallback to theme.json)
    if (!Theme::loadFromFile("data/themes/theme_dark_fantasy.json"))
    {
        Theme::loadFromFile("data/theme.json");
    }

    // 4. Initialise Headless Engine
    game engine;
    engine.init();

    // 5. Initialise UI Presentation Renderer
    uiRenderer view;

    // Handle Screenshot Mode
    if (!screenshotState.empty())
    {
        if (screenshotState == "main_menu" || screenshotState == "menu")
        {
            engine.changeState(std::make_unique<mainMenuState>());
        }
        else if (screenshotState == "options" || screenshotState == "options_general")
        {
            engine.changeState(std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS, std::make_unique<mainMenuState>()));
        }
        else if (screenshotState == "options_keybinds" || screenshotState == "keybinds")
        {
            auto opt = std::make_unique<optionsState>(OptionsScreenMode::GENERAL_OPTIONS, std::make_unique<mainMenuState>());
            opt->isKeybindsOpen = true;
            engine.changeState(std::move(opt));
        }
        else if (screenshotState.starts_with("options_content"))
        {
            auto opt = std::make_unique<optionsState>(OptionsScreenMode::CONTENT_OPTIONS, std::make_unique<mainMenuState>());
            int catIdx = 0;
            if (screenshotState.length() > 16) catIdx = screenshotState[16] - '0';
            if (catIdx >= 0 && catIdx <= 8) opt->contentCategory = static_cast<ContentOptionsCategory>(catIdx);
            engine.changeState(std::move(opt));
        }
        else if (screenshotState == "save_load")
        {
            engine.changeState(std::make_unique<loadGameState>(SaveMenuMode::SAVE_AND_LOAD, std::make_unique<mainMenuState>()));
        }
        else if (screenshotState == "character_creation" || screenshotState == "cc")
        {
            engine.changeState(std::make_unique<characterCreationState>(0));
        }
        else if (screenshotState == "character_creation_step1")
        {
            auto cc = std::make_unique<characterCreationState>(0);
            cc->step = 1;
            engine.changeState(std::move(cc));
        }
        else if (screenshotState == "character_creation_step2")
        {
            auto cc = std::make_unique<characterCreationState>(0);
            cc->step = 2;
            engine.changeState(std::move(cc));
        }
        else if (screenshotState == "character_creation_museum")
        {
            auto cc = std::make_unique<characterCreationState>(0);
            cc->step = 3;
            cc->subView = 0;
            engine.changeState(std::move(cc));
        }

        engine.refreshActionGrid();

        // Render multiple frames to stabilize fonts/layout
        for (int f = 0; f < 3; ++f)
        {
            engine.update(0.016f);
            view.render(renderer, &engine);
        }

        if (screenshotOut.empty())
        {
            std::filesystem::create_directories("/home/jackd/.gemini/antigravity/brain/0ae033e5-0614-491c-8eb5-fc151a7bb89a/screenshots");
            screenshotOut = "/home/jackd/.gemini/antigravity/brain/0ae033e5-0614-491c-8eb5-fc151a7bb89a/screenshots/" + screenshotState + ".png";
        }

        SDL_Surface* surface = SDL_RenderReadPixels(renderer, NULL);
        if (surface)
        {
            IMG_SavePNG(surface, screenshotOut.c_str());
            SDL_DestroySurface(surface);
            std::cout << "[Screenshot] Successfully captured " << screenshotState << " -> " << screenshotOut << "\n";
        }
        else
        {
            std::cerr << "[Screenshot] Failed to capture surface: " << SDL_GetError() << "\n";
        }

        engine.clean();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

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