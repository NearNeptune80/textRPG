#include "inventory.h"
#include <iostream>
#include <algorithm>

static bool hasTag(const std::vector<std::string>& tags, const std::string& target)
{
    return std::find(tags.begin(), tags.end(), target) != tags.end();
}

void inventoryComponent::addItem(std::shared_ptr<item> newItem)
{
    if (newItem)
    {
        backpack.push_back(newItem);
    }
}

bool inventoryComponent::removeItem(const std::string& itemId)
{
    for (auto it = backpack.begin(); it != backpack.end(); ++it)
    {
        if (*it && (*it)->id == itemId)
        {
            backpack.erase(it);
            return true;
        }
    }
    return false;
}

bool inventoryComponent::equipItem(size_t backpackIndex, equipSlot slot, const std::vector<std::string>& bodyPartTags)
{
    if (backpackIndex >= backpack.size()) return false;

    std::shared_ptr<item> itemToEquip = backpack[backpackIndex];

    if (!itemToEquip || !itemToEquip->isEquippable)
    {
        std::cout << "Cannot equip " << (itemToEquip ? itemToEquip->name : "Invalid Item") << ": Item is not equippable.\n";
        return false;
    }

    // --- ANATOMY TAG VALIDATION ---
    for (const auto& fTag : itemToEquip->forbiddenTags)
    {
        if (hasTag(bodyPartTags, fTag))
        {
            std::cout << "Cannot equip " << itemToEquip->name << ": Body part has forbidden tag '" << fTag << "'.\n";
            return false;
        }
    }

    for (const auto& rTag : itemToEquip->requiredTags)
    {
        if (!hasTag(bodyPartTags, rTag))
        {
            std::cout << "Cannot equip " << itemToEquip->name << ": Body part missing required tag '" << rTag << "'.\n";
            return false;
        }
    }

    if (isEquipped(slot))
    {
        unequipItem(slot);
    }

    equipped[slot] = itemToEquip;
    backpack.erase(backpack.begin() + backpackIndex);

    std::cout << "Successfully equipped " << itemToEquip->name << "!\n";
    return true;
}

bool inventoryComponent::unequipItem(equipSlot slot)
{
    if (!isEquipped(slot)) return false;

    backpack.push_back(equipped[slot]);
    equipped.erase(slot);

    return true;
}

bool inventoryComponent::isEquipped(equipSlot slot) const
{
    return equipped.find(slot) != equipped.end() && equipped.at(slot) != nullptr;
}

std::shared_ptr<item> inventoryComponent::getEquippedItem(equipSlot slot)
{
    if (isEquipped(slot))
    {
        return equipped.at(slot);
    }
    return nullptr;
}