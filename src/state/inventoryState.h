#pragma once

#include "state/iGameState.h"

/**
 * Headless state controller for inventory management and item interaction.
 * Tracks selected slots, inventory sides, and item interaction state.
 * Contains ZERO UI/SDL rendering calls.
 */
class inventoryState : public iGameState
{
public:
	inventoryState() = default;
	~inventoryState() override = default;

	void initialise(game* gameContext) override;
	void handleInput(game* gameContext, const SDL_Event& event) override;
	void update(game* gameContext, float deltaTime) override;
	void render(game* gameContext) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;
};