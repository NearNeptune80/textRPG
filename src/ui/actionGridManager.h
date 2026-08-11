#pragma once

class game;

/**
 * Pure Logical Action Grid Manager.
 * Populates active action buttons and UI commands based on game state.
 * Contains ZERO rendering calls or screen coordinate logic.
 */
class ActionGridManager
{
public:
    static void refresh(game* gameContext);
};