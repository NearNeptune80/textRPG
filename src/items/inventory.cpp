#include "items/inventory.h"

#include <algorithm>
#include <unordered_map>

static bool hasTag(const std::vector<std::string>& tags, const std::string& target)
{
    return std::find(tags.begin(), tags.end(), target) != tags.end();
}

std::vector<InventorySlot> inventoryComponent::getStackedView() const
{
    std::vector<InventorySlot> slots;
    std::unordered_map<std::string, size_t> stackMap;

    for (size_t i = 0; i < backpack.size(); ++i)
    {
        const auto& itemPtr = backpack[i];
        if (!itemPtr) continue;

        if (itemPtr->isStackable)
        {
            auto it = stackMap.find(itemPtr->id);
            if (it != stackMap.end())
            {
                slots[it->second].totalCount += itemPtr->count;
            }
            else
            {
                stackMap[itemPtr->id] = slots.size();
                slots.push_back({ itemPtr, itemPtr->count, static_cast<int>(i) });
            }
        }
        else
        {
            slots.push_back({ itemPtr, 1, static_cast<int>(i) });
        }
    }
    return slots;
}

void inventoryComponent::addItem(std::shared_ptr<item> newItem)
{
    if (newItem) backpack.push_back(newItem);
}

bool inventoryComponent::removeItem(const std::string& itemId, int countToRemove)
{
    int remaining = countToRemove;

    for (auto it = backpack.begin(); it != backpack.end(); )
    {
        if (*it && (*it)->id == itemId)
        {
            if ((*it)->isStackable)
            {
                if ((*it)->count > remaining)
                {
                    (*it)->count -= remaining;
                    return true;
                }
                else
                {
                    remaining -= (*it)->count;
                    it = backpack.erase(it);
                    if (remaining <= 0) return true;
                    continue;
                }
            }
            else
            {
                it = backpack.erase(it);
                remaining--;
                if (remaining <= 0) return true;
                continue;
            }
        }
        else
        {
            ++it;
        }
    }
    return remaining < countToRemove;
}

bool inventoryComponent::equipItem(size_t backpackIndex, equipSlot slot, const std::vector<std::string>& bodyPartTags)
{
    size_t slotIdx = static_cast<size_t>(slot);
    if (backpackIndex >= backpack.size() || slotIdx >= EQUIP_SLOT_COUNT) return false;

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

    equipped[slotIdx] = itemToEquip;
    backpack.erase(backpack.begin() + backpackIndex);
    return true;
}

bool inventoryComponent::unequipItem(equipSlot slot)
{
    size_t slotIdx = static_cast<size_t>(slot);
    if (!isEquipped(slot) || slotIdx >= EQUIP_SLOT_COUNT) return false;

    backpack.push_back(equipped[slotIdx]);
    equipped[slotIdx] = nullptr;
    return true;
}

bool inventoryComponent::isEquipped(equipSlot slot) const
{
    size_t slotIdx = static_cast<size_t>(slot);
    return slotIdx < EQUIP_SLOT_COUNT && equipped[slotIdx] != nullptr;
}

std::shared_ptr<item> inventoryComponent::getEquippedItem(equipSlot slot)
{
    size_t slotIdx = static_cast<size_t>(slot);
    if (isEquipped(slot)) return equipped[slotIdx];
    return nullptr;
}