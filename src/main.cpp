#include "game.h"

int main(int argc, char* argv[])
{
    game* Game = new game();

    Game->init("My Game Engine", 1280, 720, false);

    while (Game->running())
    {
        Game->handleEvents();
        Game->update();
        Game->render();
    }

    Game->clean();
    delete Game;

    return 0;
}