#pragma once

#include "state/iGameState.h"

/**
 * Headless state controller for inventory interactions.
 * Tracks item selection indices and manages state toggles.
 */
class inventoryState : public iGameState
{
public:
	inventoryState() = default;
	~inventoryState() override = default;

	void initialise(game* gameContext) override;
	void handleInput(game* gameContext, const SDL_Event& event) override;
	void handleCommand(game* gameContext, const UICommand& cmd) override;
	void update(game* gameContext, float deltaTime) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;
};