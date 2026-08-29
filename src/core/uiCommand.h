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
	CLOSE_MENU,

	// Encounter Resolution Hub Commands
	SELECT_RESOLUTION_TARGET,
	LOOT_ENEMY,
	STRIP_ENEMY,
	INTERACTIVE_SEX,
	SUBJUGATE_ENEMY,
	RELEASE_ENEMY,

	// Sex State Commands
	EXECUTE_SEX_ACTION,
	CHANGE_SEX_STANCE,
	END_SEX_SCENE,

	// Main Menu Commands
	START_NEW_GAME,
	CONTINUE_GAME,
	LOAD_GAME_SLOT,
	OPEN_SETTINGS,
	OPEN_INVENTORY,
	OPEN_TRANSFORMATION,
	OPEN_PHONE,
	OPEN_SHOP,
	BUY_SHOP_ITEM,
	SELL_SHOP_ITEM,
	CONFIRM_SHOP_TRANSACTION,
	INFUSE_RUNIC_ITEM,
	CYCLE_SETTING_OPTION,
	QUIT_GAME
};

struct UICommand
{
	CommandType type;
	int intPayload1 = 0;
	int intPayload2 = 0;
	std::string stringPayload = "";
};