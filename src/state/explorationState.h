#pragma once

#include "state/iGameState.h"

/**
 * Headless state controller for overworld exploration.
 * Manages movement logic, tile transitions, and quick-save/load shortcuts.
 */
class explorationState : public iGameState
{
public:
	explorationState() = default;
	~explorationState() override = default;

	void initialise(game* gameContext) override;
	void handleInput(game* gameContext, const SDL_Event& event) override;
	void handleCommand(game* gameContext, const UICommand& cmd) override;
	void update(game* gameContext, float deltaTime) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;
};