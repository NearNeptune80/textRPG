#include "state/eventState.h"

#include "core/game.h"
#include "ui/uiRenderer.h"

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
	UI::DrawMapGrid(gameContext->renderer, gameContext, gameContext->layout.mapRect, gameContext->map, gameContext->gridX, gameContext->gridY, 12);

	ViewportGuard vpGuard(gameContext->renderer, gameContext->layout.textMainRect);
	UI::DrawPanel(gameContext->renderer, { 0.0f, 0.0f, gameContext->layout.textMainRect.w, gameContext->layout.textMainRect.h }, Theme::colors.bgPanel, Theme::colors.borderNormal);

	SDL_FRect nameRect = { 0.0f, 0.0f, gameContext->layout.textMainRect.w, 40.0f };
	gameContext->renderTextAligned(gameContext->currentScene.speakerName, nameRect, TextAlignment::CENTER, false, "title_font", Theme::colors.textGold);

	SDL_FRect bodyRect = { 0.0f, 40.0f, gameContext->layout.textMainRect.w, gameContext->layout.textMainRect.h - 40.0f };
	gameContext->renderTextWrapped(gameContext->currentScene.bodyText, bodyRect, "button_font", Theme::colors.textSecondary);
}