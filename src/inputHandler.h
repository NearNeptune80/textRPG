#pragma once
#include <SDL3/SDL.h>

class game;

/**
 * Dedicated event routing and mouse/keyboard input handling system.
 */
class InputHandler
{
public:
    static void handleEvents(game* g);
    static void handleMouseClick(game* g, float windowX, float windowY);

private:
    static void handleKeyboardInput(game* g, const SDL_Event& event);
    static void handleMouseWheelInput(game* g, const SDL_Event& event);

    // Mouse Hit-Testing Sub-Handlers
    static bool handleActionGridClick(game* g, float mouseX, float mouseY);
    static bool handleMapClick(game* g, float mouseX, float mouseY);
    static bool handleEquipmentGridClick(game* g, float mouseX, float mouseY);
    static bool handleInventoryTabClick(game* g, float localMouseX, float localMouseY, SDL_FRect localBounds);
    static bool handleInventorySlotClick(game* g, float localMouseX, float localMouseY, SDL_FRect localBounds);
};