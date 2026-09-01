#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "common/enums.h"
#include "entities/statusEffect.h"
#include "items/clothingDisplacement.h"
#include "items/enchantment.h"

struct item
{
    std::string id;
    std::string name;
    std::string description;
    std::string tooltip;

    int baseValue = 0;

    bool isConsumable = false;
    bool isEquippable = false;
    bool isStackable = false;
    bool isKeyItem = false;
    int count = 1;

    equipSlot targetSlot = equipSlot::NONE;

    std::string baseRace;
    std::vector<enchantment> enchantments;
    std::vector<StatModifier> statModifiers;

    std::vector<std::string> requiredTags;
    std::vector<std::string> forbiddenTags;

    std::unordered_map<DisplacementMode, std::vector<bodySlot>> supportedDisplacements;
};