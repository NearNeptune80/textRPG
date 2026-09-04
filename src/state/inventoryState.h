#pragma once

#include <memory>
#include "state/iGameState.h"

/**
 * Headless state controller for inventory interactions.
 * Tracks item selection indices and manages state toggles.
 * Can be opened as a top-level mode or as a submenu with a return state.
 */
class inventoryState : public iGameState
{
public:
	explicit inventoryState(std::unique_ptr<iGameState> returnState = nullptr);
	~inventoryState() override = default;

	void initialise(game* gameContext) override;
	void handleCommand(game* gameContext, const UICommand& cmd) override;
	void update(game* gameContext, float deltaTime) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;

	iGameState* getReturnState() const { return m_returnState.get(); }
	void goBack(game* gameContext);

private:
	std::unique_ptr<iGameState> m_returnState;
};