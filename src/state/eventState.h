#pragma once

#include "state/iGameState.h"

/**
 * Headless state controller for narrative text scenes and dialogue choices.
 * Manages active scene choices and state transitions.
 * Contains ZERO UI/SDL rendering calls.
 */
class eventState : public iGameState
{
public:
	eventState();
	~eventState() override;

	void initialise(game* gameContext) override;
	void handleInput(game* gameContext, const SDL_Event& event) override;
	void update(game* gameContext, float deltaTime) override;
	void render(game* gameContext) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;
};