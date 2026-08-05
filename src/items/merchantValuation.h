#pragma once

class item;
class entity;

class merchantValuation
{
public:
	// Calculates total market value of item including attached enchantments
	static int calculateItemMarketValue(const item* itemPtr);

	// Calculates price for player BUYING an item from a merchant
	static int calculateBuyPrice(const item* itemPtr, const entity* player, const entity* merchant);

	// Calculates price for player SELLING an item to a merchant
	static int calculateSellPrice(const item* itemPtr, const entity* player, const entity* merchant);
};