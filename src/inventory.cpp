#include "inventory.h"
#include <iostream>
#include <algorithm>

static bool hasTag(const std::vector<std::string>& tags, const std::string& target)
{
    return std::find(tags.begin(), tags.end(), target) != tags.end();
}

void inventoryComponent::addItem(std::shared_ptr<item> newItem)
{
    if (newItem) backpack.push_back(newItem);
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
    if (!itemToEquip || !itemToEquip->isEquippable) return false;

    for (const auto& fTag : itemToEquip->forbiddenTags)
    {
        if (hasTag(bodyPartTags, fTag)) return false;
    }

    for (const auto& rTag : itemToEquip->requiredTags)
    {
        if (!hasTag(bodyPartTags, rTag)) return false;
    }

    if (isEquipped(slot))
    {
        unequipItem(slot);
    }

    equipped[slot] = itemToEquip;
    backpack.erase(backpack.begin() + backpackIndex);
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
    if (isEquipped(slot)) return equipped.at(slot);
    return nullptr;
}