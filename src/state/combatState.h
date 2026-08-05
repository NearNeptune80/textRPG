#pragma once

#include <memory>
#include <vector>
#include <SDL3/SDL.h>

#include "combat/combatEngine.h"
#include "state/iGameState.h"

class game;

/**
 * Headless state controller for combat execution.
 * Manages combat turn resolution, target selection, and action dispatching.
 * Contains ZERO UI/SDL rendering calls.
 */
class CombatState : public iGameState
{
public:
	CombatState(const std::vector<std::shared_ptr<entity>>& playerParty,
				const std::vector<std::shared_ptr<entity>>& enemyParty);

	~CombatState() override = default;

	void initialise(game* gameContext) override;
	void handleInput(game* gameContext, const SDL_Event& event) override;
	void update(game* gameContext, float deltaTime) override;
	void render(game* gameContext) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;

	// Action Handlers
	void handleEndTurn(game* gameContext);
	void handleRunAttempt(game* gameContext);
	void handleSurrender(game* gameContext);

	// Data Snapshot API for UI View Layer
	const combatEngine& getEngine() const { return m_engine; }
	int getSelectedTargetIndex() const { return m_selectedTargetIndex; }
	bool isTargetEnemy() const { return m_targetIsEnemy; }
	bool isShowingSecondaryTab() const { return m_showingSecondaryTab; }

private:
	combatEngine m_engine;

	int m_selectedTargetIndex = 0;
	bool m_targetIsEnemy = true;
	bool m_showingSecondaryTab = false;
	int m_secondaryPage = 0;
};