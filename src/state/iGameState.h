#pragma once

#include <SDL3/SDL.h>
#include "core/uiCommand.h"

class game;

/**
 * Headless Base Interface for Engine State Controllers.
 * Manages game simulation state, input routing, and UI commands.
 * Contains ZERO rendering calls or graphics layout dependencies.
 */
class iGameState
{
public:
	virtual ~iGameState() = default;

	virtual void initialise(game* gameContext) = 0;
	virtual void handleInput(game* gameContext, const SDL_Event& event) = 0;
	virtual void handleCommand(game* gameContext, const UICommand& cmd) {}
	virtual void update(game* gameContext, float deltaTime) = 0;

	virtual void onEnter(game* gameContext) = 0;
	virtual void onExit(game* gameContext) = 0;
};