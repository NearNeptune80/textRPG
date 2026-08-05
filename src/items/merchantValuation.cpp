#include "items/merchantValuation.h"

#include <algorithm>
#include <cmath>

#include "entities/entity.h"
#include "items/item.h"

int merchantValuation::calculateItemMarketValue(const item* itemPtr)
{
	if (!itemPtr) return 0;

	int totalValue = itemPtr->baseValue;

	// Add value for each attached enchantment
	for (const auto& ench : itemPtr->enchantments)
	{
		totalValue += ench.essenceCost * 5; // Dynamic value scaling per enchantment essence cost
	}

	return std::max(1, totalValue);
}

int merchantValuation::calculateBuyPrice(const item* itemPtr, const entity* player, const entity* merchant)
{
	if (!itemPtr) return 0;

	int marketVal = calculateItemMarketValue(itemPtr);
	float markup = merchant ? merchant->buyMarkup : 1.25f;
	float perkDiscount = player ? player->tradePerkModifier : 0.0f;

	// Apply merchant markup and subtract player trade perk discount
	float finalFactor = markup * (1.0f - std::clamp(perkDiscount, 0.0f, 0.50f));
	int finalPrice = static_cast<int>(std::round(static_cast<float>(marketVal) * finalFactor));

	return std::max(1, finalPrice);
}

int merchantValuation::calculateSellPrice(const item* itemPtr, const entity* player, const entity* merchant)
{
	if (!itemPtr) return 0;

	int marketVal = calculateItemMarketValue(itemPtr);
	float markdown = merchant ? merchant->sellMarkdown : 0.50f;
	float perkBonus = player ? player->tradePerkModifier : 0.0f;

	// Apply merchant markdown and add player trade perk bonus
	float finalFactor = markdown * (1.0f + std::clamp(perkBonus, 0.0f, 0.50f));
	int finalPrice = static_cast<int>(std::round(static_cast<float>(marketVal) * finalFactor));

	return std::max(1, finalPrice);
}