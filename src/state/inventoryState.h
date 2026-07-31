#pragma once
#include "iGameState.h"

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

private:
	bool handleEquipmentClick(game* gameContext, float mouseX, float mouseY);
	bool handleTabClick(game* gameContext, float localMouseX, float localMouseY, SDL_FRect localBounds);
	bool handleSlotClick(game* gameContext, float localMouseX, float localMouseY, SDL_FRect localBounds);
};