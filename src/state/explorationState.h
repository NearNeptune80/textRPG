#pragma once
#include "iGameState.h"

class explorationState : public iGameState
{
public:
	explorationState() = default;
	~explorationState() override = default;

	void initialise(game* gameContext) override;
	void handleInput(game* gameContext, const SDL_Event& event) override;
	void update(game* gameContext, float deltaTime) override;
	void render(game* gameContext) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;
};