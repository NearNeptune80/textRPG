#pragma once

#include <string>
#include <vector>

#include "common/enums.h"
#include "items/enchantment.h"

struct item
{
    std::string id;
    std::string name;
    std::string description;

    bool isConsumable = false;
    bool isEquippable = false;
    bool isStackable = false;
    int count = 1;

    equipSlot targetSlot = equipSlot::NONE;

    std::string baseRace;
    std::vector<enchantment> enchantments;

    std::vector<std::string> requiredTags;
    std::vector<std::string> forbiddenTags;
};