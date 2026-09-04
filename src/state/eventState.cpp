#include "state/eventState.h"

#include "core/game.h"

eventState::eventState() = default;
eventState::~eventState() = default;

void eventState::initialise(game* gameContext) {}

void eventState::onEnter(game* gameContext)
{
	if (gameContext)
	{
		gameContext->refreshActionGrid();
	}
}

void eventState::onExit(game* gameContext) {}

void eventState::update(game* gameContext, float deltaTime) {}

void eventState::handleCommand(game* gameContext, const UICommand& cmd)
{
	if (!gameContext) return;

	if (cmd.type == CommandType::SELECT_DIALOGUE_CHOICE)
	{
		int choiceIdx = cmd.intPayload1;
		if (choiceIdx >= 0 && static_cast<size_t>(choiceIdx) < gameContext->currentScene.choices.size())
		{
			gameContext->processChoice(gameContext->currentScene.choices[choiceIdx]);
		}
	}
}