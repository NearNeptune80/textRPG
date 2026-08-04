#pragma once

#include <SDL3/SDL.h>

class game;

class inputHandler
{
public:
    static void handleEvents(game* g);

private:
    static bool handleActionGridClick(game* g, float mouseX, float mouseY);
};