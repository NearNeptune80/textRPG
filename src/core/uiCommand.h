#pragma once

#include <string>
#include <utility>

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

	// Action Grid Navigation & Triggers
	TRIGGER_ACTION_BUTTON,
	PREVIOUS_ACTION_PAGE,
	NEXT_ACTION_PAGE,

	// Persistence Hotkeys
	QUICK_SAVE,
	QUICK_LOAD,

	// Text & Keyboard Input (Decoupled from SDL)
	TEXT_INPUT,
	TEXT_BACKSPACE,
	CONFIRM_INPUT,
	CANCEL_INPUT,

	// Tab & Subview Selection
	SELECT_TAB,
	SET_SUBVIEW,
	SET_PROPERTY,

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

	// Main Menu & Navigation Commands
	START_NEW_GAME,
	CONTINUE_GAME,
	LOAD_GAME_SLOT,
	SAVE_GAME_SLOT,
	DELETE_SAVE_SLOT,
	OPEN_LOAD_MENU,
	OPEN_MAIN_MENU,
	OPEN_SETTINGS,
	OPEN_CONTENT_OPTIONS,
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
	std::string stringPayload2 = "";

	UICommand() = default;
	UICommand(CommandType t) : type(t) {}
	UICommand(CommandType t, int p1) : type(t), intPayload1(p1) {}
	UICommand(CommandType t, int p1, int p2) : type(t), intPayload1(p1), intPayload2(p2) {}
	UICommand(CommandType t, std::string s1) : type(t), stringPayload(std::move(s1)) {}
	UICommand(CommandType t, int p1, std::string s1) : type(t), intPayload1(p1), stringPayload(std::move(s1)) {}
	UICommand(CommandType t, std::string s1, std::string s2) : type(t), stringPayload(std::move(s1)), stringPayload2(std::move(s2)) {}
	UICommand(CommandType t, int p1, int p2, std::string s1, std::string s2 = "")
		: type(t), intPayload1(p1), intPayload2(p2), stringPayload(std::move(s1)), stringPayload2(std::move(s2)) {}

	// Static factory helpers for decoupled UI event dispatch
	static UICommand textInput(std::string s) { return UICommand(CommandType::TEXT_INPUT, std::move(s)); }
	static UICommand textBackspace() { return UICommand(CommandType::TEXT_BACKSPACE); }
	static UICommand confirmInput() { return UICommand(CommandType::CONFIRM_INPUT); }
	static UICommand closeMenu() { return UICommand(CommandType::CLOSE_MENU); }
	static UICommand quickSave() { return UICommand(CommandType::QUICK_SAVE); }
	static UICommand quickLoad() { return UICommand(CommandType::QUICK_LOAD); }
	static UICommand openInventory() { return UICommand(CommandType::OPEN_INVENTORY); }
	static UICommand openShop() { return UICommand(CommandType::OPEN_SHOP); }
	static UICommand openTransformation() { return UICommand(CommandType::OPEN_TRANSFORMATION); }
	static UICommand movePlayer(int x, int y) { return UICommand(CommandType::MOVE_PLAYER, x, y); }
	static UICommand previousActionPage() { return UICommand(CommandType::PREVIOUS_ACTION_PAGE); }
	static UICommand nextActionPage() { return UICommand(CommandType::NEXT_ACTION_PAGE); }
	static UICommand triggerActionButton(int slot) { return UICommand(CommandType::TRIGGER_ACTION_BUTTON, slot); }
	static UICommand selectTab(int tab) { return UICommand(CommandType::SELECT_TAB, tab); }
};