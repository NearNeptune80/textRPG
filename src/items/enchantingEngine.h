#pragma once

#include <memory>
#include <string>
#include <vector>

#include "items/infusionEffect.h"

struct item;
class entity;

namespace EnchantingEngine
{
    int calculateInfusionCost(const item* baseItem, const std::vector<InfusionEffect>& stagedEffects, const entity* player = nullptr);

    std::string composeItemName(const item* baseItem, const std::vector<InfusionEffect>& effects);

    std::shared_ptr<item> craftInfusedItem(const item* baseItem, const std::vector<InfusionEffect>& effects, const std::string& customName = "");
}
