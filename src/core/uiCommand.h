#pragma once

#include <string>
#include <utility>

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
	SELECT_COMBAT_FOCUS,
	QUEUE_COMBAT_ACTION,
	CLEAR_COMBAT_QUEUE,
	EXECUTE_COMBAT_TURN,
	END_TURN,
	RUN_ATTEMPT,
	SURRENDER,
	SELECT_DIALOGUE_CHOICE,
	TRIGGER_INTERACTION,
	CLOSE_MENU,

	MASTURBATE_ACTION,
	FAMILIAR_ACTION,
	ASSIGN_COMBAT_SLOT,
	CAST_SPELL,
	UNLOCK_PERK,
	SAVE_SETTINGS,
	TOGGLE_DROPDOWN,

	TRIGGER_ACTION_BUTTON,
	PREVIOUS_ACTION_PAGE,
	NEXT_ACTION_PAGE,

	QUICK_SAVE,
	QUICK_LOAD,

	TEXT_INPUT,
	TEXT_BACKSPACE,
	CONFIRM_INPUT,
	CANCEL_INPUT,

	SELECT_TAB,
	SET_SUBVIEW,
	SET_PROPERTY,

	SELECT_RESOLUTION_TARGET,
	LOOT_ENEMY,
	STRIP_ENEMY,
	INTERACTIVE_SEX,
	INTERACTIVE_SEX_DOM,
	INTERACTIVE_SEX_SUB,
	TRANSFORM_ENEMY,
	TALK_ENEMY,
	SUBJUGATE_ENEMY,
	RELEASE_ENEMY,
	PERMANENTLY_REMOVE_ENEMY,

	EXECUTE_SEX_ACTION,
	CHANGE_SEX_STANCE,
	SELECT_SEX_TARGET,
	END_SEX_SCENE,

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
	static UICommand selectCombatFocus(std::string focus) { return UICommand(CommandType::SELECT_COMBAT_FOCUS, std::move(focus)); }
	static UICommand queueCombatAction(std::string actionId) { return UICommand(CommandType::QUEUE_COMBAT_ACTION, std::move(actionId)); }
	static UICommand clearCombatQueue() { return UICommand(CommandType::CLEAR_COMBAT_QUEUE); }
	static UICommand executeCombatTurn() { return UICommand(CommandType::EXECUTE_COMBAT_TURN); }
	static UICommand masturbateAction(std::string actionId, int durationMinutes = 10) { return UICommand(CommandType::MASTURBATE_ACTION, durationMinutes, std::move(actionId)); }
	static UICommand familiarAction(std::string actionId, int durationMinutes = 10) { return UICommand(CommandType::FAMILIAR_ACTION, durationMinutes, std::move(actionId)); }
	static UICommand castSpell(std::string spellId, int mpCost, int healAmount = 0) { return UICommand(CommandType::CAST_SPELL, mpCost, healAmount, std::move(spellId)); }
	static UICommand unlockPerk(std::string perkId, int cost = 1) { return UICommand(CommandType::UNLOCK_PERK, cost, std::move(perkId)); }
	static UICommand saveSettings() { return UICommand(CommandType::SAVE_SETTINGS); }
};