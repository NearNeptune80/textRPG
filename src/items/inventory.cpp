#include "items/inventory.h"

#include <algorithm>
#include <unordered_map>

static bool hasTag(const std::vector<std::string>& tags, const std::string& target)
{
    return std::find(tags.begin(), tags.end(), target) != tags.end();
}

static std::vector<bodySlot> getCoveredBodySlotsForEquipSlot(equipSlot slot)
{
    switch (slot)
    {
        case equipSlot::HEADWEAR:       return { bodySlot::HEAD };
        case equipSlot::EYEWEAR:        return { bodySlot::EYES };
        case equipSlot::HAIR_WEAR:      return { bodySlot::HAIR };
        case equipSlot::HORNS_SLOT:     return { bodySlot::HORNS };
        case equipSlot::MOUTHWEAR:      return { bodySlot::MOUTH };
        case equipSlot::NECKWEAR:       return { bodySlot::NECK };
        case equipSlot::TORSO_OVER:
        case equipSlot::TORSO_UNDER:    return { bodySlot::TORSO, bodySlot::BREASTS, bodySlot::STOMACH };
        case equipSlot::CHEST_WEAR:
        case equipSlot::NIPPLES_WEAR:   return { bodySlot::BREASTS, bodySlot::NIPPLES };
        case equipSlot::HANDS:
        case equipSlot::WRISTS:         return { bodySlot::HANDS, bodySlot::ARMS };
        case equipSlot::HIPS_WEAR:
        case equipSlot::STOMACH_WEAR:   return { bodySlot::HIPS, bodySlot::STOMACH };
        case equipSlot::FINGER_PRIMARY: return { bodySlot::FINGERS };
        case equipSlot::LEGS_OUTER:     return { bodySlot::LEGS, bodySlot::HIPS, bodySlot::GROIN, bodySlot::ASS };
        case equipSlot::GROIN_OVER:
        case equipSlot::PENIS_WEAR:
        case equipSlot::VAGINA_WEAR:    return { bodySlot::GROIN };
        case equipSlot::ASS_WEAR:       return { bodySlot::ASS };
        case equipSlot::FEET:
        case equipSlot::ANKLES:
        case equipSlot::CALVES:         return { bodySlot::FEET };
        case equipSlot::TAIL_SLOT:      return { bodySlot::TAIL };
        case equipSlot::WINGS_SLOT:     return { bodySlot::WINGS };
        default:                        return {};
    }
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
    if (countToRemove <= 0) return false;
    int remaining = countToRemove;

    std::erase_if(backpack, [&](std::shared_ptr<item>& itemPtr) {
        if (remaining <= 0 || !itemPtr || itemPtr->id != itemId) return false;

        if (itemPtr->isStackable)
        {
            if (itemPtr->count > remaining)
            {
                itemPtr->count -= remaining;
                remaining = 0;
                return false;
            }
            remaining -= itemPtr->count;
            return true;
        }

        remaining--;
        return true;
    });

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
    activeDisplacements.erase(slot);
    backpack.erase(backpack.begin() + backpackIndex);
    return true;
}

bool inventoryComponent::unequipItem(equipSlot slot)
{
    size_t slotIdx = static_cast<size_t>(slot);
    if (!isEquipped(slot) || slotIdx >= EQUIP_SLOT_COUNT) return false;

    backpack.push_back(equipped[slotIdx]);
    equipped[slotIdx] = nullptr;
    activeDisplacements.erase(slot);
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

void inventoryComponent::setDisplacement(equipSlot slot, DisplacementMode mode)
{
    if (isEquipped(slot))
    {
        if (mode == DisplacementMode::NONE)
        {
            activeDisplacements.erase(slot);
        }
        else
        {
            activeDisplacements[slot] = mode;
        }
    }
}

DisplacementMode inventoryComponent::getDisplacement(equipSlot slot) const
{
    auto it = activeDisplacements.find(slot);
    if (it != activeDisplacements.end())
    {
        return it->second;
    }
    return DisplacementMode::NONE;
}

void inventoryComponent::resetAllDisplacements()
{
    activeDisplacements.clear();
}

bool inventoryComponent::isSlotExposed(bodySlot slot) const
{
    for (size_t i = 0; i < EQUIP_SLOT_COUNT; ++i)
    {
        equipSlot eqSlot = static_cast<equipSlot>(i);
        const auto& itemPtr = equipped[i];
        if (!itemPtr) continue;

        std::vector<bodySlot> coveredSlots = getCoveredBodySlotsForEquipSlot(eqSlot);
        auto itCovered = std::find(coveredSlots.begin(), coveredSlots.end(), slot);
        if (itCovered == coveredSlots.end()) continue;

        auto itDisp = activeDisplacements.find(eqSlot);
        if (itDisp != activeDisplacements.end() && itDisp->second != DisplacementMode::NONE)
        {
            DisplacementMode mode = itDisp->second;
            auto itSupported = itemPtr->supportedDisplacements.find(mode);
            if (itSupported != itemPtr->supportedDisplacements.end())
            {
                const auto& exposedSlots = itSupported->second;
                if (std::find(exposedSlots.begin(), exposedSlots.end(), slot) != exposedSlots.end())
                {
                    continue;
                }
            }
        }

        return false;
    }

    return true;
}