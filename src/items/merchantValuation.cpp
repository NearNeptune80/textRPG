#include "items/merchantValuation.h"

#include <algorithm>
#include <cmath>

#include "entities/entity.h"
#include "items/item.h"
#include "items/itemDatabase.h"

int merchantValuation::calculateItemMarketValue(const item* itemPtr, float itemCondition)
{
    if (!itemPtr) return 0;

    float conditionFactor = std::clamp(itemCondition, 0.1f, 1.0f);
    int totalValue = itemPtr->baseValue;

    for (const auto& ench : itemPtr->enchantments)
    {
        totalValue += ench.essenceCost * 5;
    }

    return std::max(1, static_cast<int>(std::round(static_cast<float>(totalValue) * conditionFactor)));
}

int merchantValuation::calculateBuyPrice(const item* itemPtr, const entity* player, const entity* merchant, float itemCondition)
{
    if (!itemPtr) return 0;

    int marketVal = calculateItemMarketValue(itemPtr, itemCondition);
    float markup = merchant ? merchant->buyMarkup : 1.25f;
    float perkDiscount = player ? player->tradePerkModifier : 0.0f;
    float affinity = merchant ? merchant->merchantAffinity : 1.0f;

    float affinityFactor = (affinity > 0.0f) ? (1.0f / affinity) : 1.0f;
    float finalFactor = markup * affinityFactor * (1.0f - std::clamp(perkDiscount, 0.0f, 0.50f));

    int finalPrice = static_cast<int>(std::round(static_cast<float>(marketVal) * finalFactor));
    return std::max(1, finalPrice);
}

int merchantValuation::calculateSellPrice(const item* itemPtr, const entity* player, const entity* merchant, float itemCondition)
{
    if (!itemPtr) return 0;

    int marketVal = calculateItemMarketValue(itemPtr, itemCondition);
    float markdown = merchant ? merchant->sellMarkdown : 0.50f;
    float perkBonus = player ? player->tradePerkModifier : 0.0f;
    float affinity = merchant ? merchant->merchantAffinity : 1.0f;

    float finalFactor = markdown * affinity * (1.0f + std::clamp(perkBonus, 0.0f, 0.50f));

    int finalPrice = static_cast<int>(std::round(static_cast<float>(marketVal) * finalFactor));
    return std::max(1, finalPrice);
}

void merchantValuation::merchantRestock(entity* merchant, int currentDay)
{
    if (!merchant || merchant->lastRestockDay == currentDay) return;

    float currentGold = merchant->getStat("currency");
    if (currentGold < merchant->baseMerchantGold)
    {
        merchant->stats.setBaseStat("currency", merchant->baseMerchantGold);
    }

    if (merchant->inventory.backpack.size() < 3)
    {
        auto potion = itemDatabase::getItem("item_canis_root");
        if (potion) merchant->inventory.addItem(potion);

        auto shirt = itemDatabase::getItem("item_linen_shirt");
        if (shirt) merchant->inventory.addItem(shirt);
    }

    merchant->lastRestockDay = currentDay;
}