#include "state/eventState.h"

#include "core/game.h"

eventState::eventState() = default;
eventState::~eventState() = default;

void eventState::initialise(game* gameContext) {}

void eventState::onEnter(game* gameContext)
{
	if (gameContext)
	{
		// Force action grid to regenerate buttons for eventState choices
		gameContext->refreshActionGrid();
	}
}

void eventState::onExit(game* gameContext) {}

void eventState::update(game* gameContext, float deltaTime) {}

void eventState::handleInput(game* gameContext, const SDL_Event& event) {}

void eventState::render(game* gameContext)
{
	// No-op: Pure state controller. Render layer handles all drawing independently.
}