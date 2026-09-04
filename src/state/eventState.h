#pragma once

#include "state/iGameState.h"

/**
 * Headless state controller for dialogue, narrative text, and quest events.
 */
class eventState : public iGameState
{
public:
	eventState();
	~eventState() override;

	void initialise(game* gameContext) override;
	void handleCommand(game* gameContext, const UICommand& cmd) override;
	void update(game* gameContext, float deltaTime) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;
};