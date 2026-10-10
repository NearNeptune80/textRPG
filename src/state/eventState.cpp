#include "state/eventState.h"

#include "core/game.h"

eventState::eventState() = default;
eventState::~eventState() = default;

void eventState::initialise(game* gameContext)
{
	// Lifecycle hook: event data initialized in constructor
}

void eventState::onEnter(game* gameContext)
{
	if (gameContext)
	{
		gameContext->refreshActionGrid();
	}
}

void eventState::onExit(game* gameContext)
{
	// Lifecycle hook: no persistent resource teardown required on exit
}

void eventState::update(game* gameContext, float deltaTime)
{
	// Event-driven: no per-frame tick required
}

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