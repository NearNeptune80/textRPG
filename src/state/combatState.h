#pragma once

#include <memory>
#include <vector>
#include <SDL3/SDL.h>

#include "combat/combatEngine.h"
#include "state/iGameState.h"

class game;

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

	// Expose handlers so ActionGridManager can bind buttons to them
	void handleEndTurn(game* gameContext);
	void handleRunAttempt(game* gameContext);
	void handleSurrender(game* gameContext);

private:
	combatEngine m_engine;

	int m_selectedTargetIndex = 0;
	bool m_targetIsEnemy = true;

	bool m_showingSecondaryTab = false;
	int m_secondaryPage = 0;

	void renderPartyCards(game* gameContext);
	void renderCombatLog(game* gameContext);
	void renderActionGrid(game* gameContext);

	void handleGridClick(game* gameContext, int slotIndex);
	std::vector<CombatAction> getAvailableSecondaryActions();
};