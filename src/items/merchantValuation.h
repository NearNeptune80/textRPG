#pragma once

class item;
class entity;

class merchantValuation
{
public:
	// Calculates total market value of item including attached enchantments and condition
	static int calculateItemMarketValue(const item* itemPtr, float itemCondition = 1.0f);

	// Calculates price for player BUYING an item from a merchant
	static int calculateBuyPrice(const item* itemPtr, const entity* player, const entity* merchant, float itemCondition = 1.0f);

	// Calculates price for player SELLING an item to a merchant
	static int calculateSellPrice(const item* itemPtr, const entity* player, const entity* merchant, float itemCondition = 1.0f);

	// Daily merchant restock logic (replenishes merchant gold and inventory stock at 06:00)
	static void merchantRestock(entity* merchant, int currentDay);
};