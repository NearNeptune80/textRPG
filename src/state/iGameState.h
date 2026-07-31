#pragma once
#include <SDL3/SDL.h>

class game;

class iGameState
{
public:
	virtual ~iGameState() = default;

	virtual void initialise(game* gameContext) = 0;
	virtual void handleInput(game* gameContext, const SDL_Event& event) = 0;
	virtual void update(game* gameContext, float deltaTime) = 0;
	virtual void render(game* gameContext) = 0;

	virtual void onEnter(game* gameContext) = 0;
	virtual void onExit(game* gameContext) = 0;
};