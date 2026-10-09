#pragma once

#include "core/uiCommand.h"

class game;

class iGameState
{
public:
	virtual ~iGameState() = default;

	virtual void initialise(game* gameContext) = 0;
	virtual void handleCommand(game* gameContext, const UICommand& cmd) {}
	virtual void update(game* gameContext, float deltaTime) = 0;

	virtual void onEnter(game* gameContext) = 0;
	virtual void onExit(game* gameContext) = 0;
};