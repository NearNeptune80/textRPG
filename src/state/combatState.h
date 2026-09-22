#pragma once

#include <memory>
#include <vector>

#include "combat/combatEngine.h"
#include "state/iGameState.h"

class game;

/**
 * Headless state controller for turn-based combat execution.
 * Manages turn queue updates, party lifecycle, debug stubs, and sexuality-driven defeat resolution.
 */
class CombatState : public iGameState
{
public:
	CombatState(const std::vector<std::shared_ptr<entity>>& playerParty,
				const std::vector<std::shared_ptr<entity>>& enemyParty);

	~CombatState() override = default;

	void initialise(game* gameContext) override;
	void handleCommand(game* gameContext, const UICommand& cmd) override;
	void update(game* gameContext, float deltaTime) override;

	void onEnter(game* gameContext) override;
	void onExit(game* gameContext) override;

	// Action Handlers
	void handleEndTurn(game* gameContext);
	void handleClearQueue(game* gameContext);
	void handleRunAttempt(game* gameContext);
	void handleSurrender(game* gameContext);

	// Focus-based Combat
	CombatFocus getCombatFocus() const { return m_currentFocus; }
	void setCombatFocus(CombatFocus focus);

	// Defeat Resolution
	void resolveDefeat(game* gameContext);

	// Snapshot APIs for UI View Layer
	const combatEngine& getEngine() const { return m_engine; }
	combatEngine& getEngineMutable() { return m_engine; }
	int getSelectedTargetIndex() const { return m_selectedTargetIndex; }
	bool isTargetEnemy() const { return m_targetIsEnemy; }

private:
	combatEngine m_engine;
	CombatFocus m_currentFocus = CombatFocus::ROOT;

	int m_selectedTargetIndex = 0;
	bool m_targetIsEnemy = true;
};