#pragma once
#include <string>
#include <vector>
#include "enums.h"
#include "enchantment.h"

/**
 * Primary item structure for equippable, consumable, or inventory objects.
 */
struct item
{
    std::string id;
    std::string name;

    bool isConsumable = false;
    bool isEquippable = false;

    equipSlot targetSlot = equipSlot::NONE;

    std::string baseRace;
    std::vector<enchantment> enchantments;

    // Anatomy tag constraints
    std::vector<std::string> requiredTags;
    std::vector<std::string> forbiddenTags;
};