#include "inventory.h"

void inventoryComponent::addItem(const item& newItem)
{
    backpack.push_back(newItem);
}

bool inventoryComponent::removeItem(const std::string& itemId)
{
    for (auto it = backpack.begin(); it != backpack.end(); ++it)
    {
        if (it->id == itemId)
        {
            backpack.erase(it);
            return true;
        }
    }
    return false;
}

bool inventoryComponent::equipItem(size_t backpackIndex, equipSlot slot)
{
    if (backpackIndex >= backpack.size()) return false;

    // If something is already in that slot, unequip it first
    if (isEquipped(slot))
    {
        unequipItem(slot);
    }

    // Move from backpack to equipped map
    equipped[slot] = backpack[backpackIndex];
    backpack.erase(backpack.begin() + backpackIndex);

    return true;
}

bool inventoryComponent::unequipItem(equipSlot slot)
{
    if (!isEquipped(slot)) return false;

    // Move from equipped map back to backpack
    backpack.push_back(equipped[slot]);
    equipped.erase(slot);

    return true;
}

bool inventoryComponent::isEquipped(equipSlot slot) const
{
    return equipped.find(slot) != equipped.end();
}

item* inventoryComponent::getEquippedItem(equipSlot slot)
{
    if (isEquipped(slot))
    {
        return &equipped[slot];
    }
    return nullptr;
}