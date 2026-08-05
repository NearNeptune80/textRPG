#pragma once

#include <string>

/**
 * Abstract intent types emitted by the UI View layer on user input.
 * Allows the game engine to execute logic without knowing anything
 * about screen layout, pixel coordinates, or rendering bounds.
 */
enum class CommandType
{
	MOVE_PLAYER,
	SELECT_INVENTORY_SLOT,
	SELECT_EQUIPMENT_SLOT,
	EQUIP_ITEM,
	UNEQUIP_ITEM,
	DROP_ITEM,
	PICKUP_ITEM,
	USE_ITEM,
	EXECUTE_COMBAT_ACTION,
	END_TURN,
	RUN_ATTEMPT,
	SURRENDER,
	SELECT_DIALOGUE_CHOICE,
	TRIGGER_INTERACTION,
	CLOSE_MENU
};

struct UICommand
{
	CommandType type;
	int intPayload1 = 0;
	int intPayload2 = 0;
	std::string stringPayload = "";
};