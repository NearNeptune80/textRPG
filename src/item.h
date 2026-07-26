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
    std::string description;

    bool isConsumable = false;
    bool isEquippable = false;
    bool isStackable = false; // Flag for stackable items (potions, reagents, etc.)
    int count = 1;             // Active stack quantity

    equipSlot targetSlot = equipSlot::NONE;

    std::string baseRace;
    std::vector<enchantment> enchantments;

    std::vector<std::string> requiredTags;
    std::vector<std::string> forbiddenTags;
};