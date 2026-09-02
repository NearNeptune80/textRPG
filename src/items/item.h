#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "common/enums.h"
#include "entities/statusEffect.h"
#include "items/clothingDisplacement.h"
#include "items/enchantment.h"

enum class ItemCategory
{
    WEAPON,
    CLOTHING,
    UNDERWEAR,
    ACCESSORY,
    CONSUMABLE,
    MATERIAL,
    KEY_ITEM,
    MISC
};

inline int getItemCategorySortOrder(ItemCategory cat)
{
    switch (cat)
    {
        case ItemCategory::WEAPON:     return 10;
        case ItemCategory::CLOTHING:   return 20;
        case ItemCategory::UNDERWEAR:  return 30;
        case ItemCategory::ACCESSORY:  return 40;
        case ItemCategory::CONSUMABLE: return 50;
        case ItemCategory::MATERIAL:   return 60;
        case ItemCategory::KEY_ITEM:   return 70;
        case ItemCategory::MISC:       return 80;
        default:                       return 90;
    }
}

inline std::string itemCategoryToString(ItemCategory cat)
{
    switch (cat)
    {
        case ItemCategory::WEAPON:     return "Weapon";
        case ItemCategory::CLOTHING:   return "Clothing";
        case ItemCategory::UNDERWEAR:  return "Underwear";
        case ItemCategory::ACCESSORY:  return "Accessory";
        case ItemCategory::CONSUMABLE: return "Consumable";
        case ItemCategory::MATERIAL:   return "Material";
        case ItemCategory::KEY_ITEM:   return "Key Item";
        case ItemCategory::MISC:       return "Misc";
        default:                       return "Misc";
    }
}

struct item
{
    std::string id;
    std::string name;
    std::string description;
    std::string tooltip;

    int baseValue = 0;

    ItemCategory category = ItemCategory::MISC;

    bool isConsumable = false;
    bool isEquippable = false;
    bool isStackable = false;
    bool isKeyItem = false;
    int count = 1;

    equipSlot targetSlot = equipSlot::NONE;
    std::vector<equipSlot> validSlots;

    std::vector<equipSlot> getValidEquipSlots() const
    {
        if (!validSlots.empty()) return validSlots;
        if (targetSlot != equipSlot::NONE)
        {
            if (targetSlot == equipSlot::WEAPON_MAIN)
            {
                return { equipSlot::WEAPON_MAIN, equipSlot::WEAPON_OFF };
            }
            return { targetSlot };
        }
        return {};
    }

    std::string baseRace;
    std::vector<enchantment> enchantments;
    std::vector<StatModifier> statModifiers;

    std::vector<std::string> requiredTags;
    std::vector<std::string> forbiddenTags;

    std::unordered_map<DisplacementMode, std::vector<bodySlot>> supportedDisplacements;
};

inline ItemCategory determineItemCategory(const item& it)
{
    if (it.category != ItemCategory::MISC) return it.category;

    if (it.isKeyItem || (!it.id.empty() && it.id.find("key") != std::string::npos && it.targetSlot == equipSlot::NONE))
    {
        return ItemCategory::KEY_ITEM;
    }
    if (it.isEquippable)
    {
        if (it.targetSlot == equipSlot::WEAPON_MAIN || it.targetSlot == equipSlot::WEAPON_OFF)
        {
            return ItemCategory::WEAPON;
        }
        if (it.targetSlot == equipSlot::CHEST_WEAR || it.targetSlot == equipSlot::NIPPLES_WEAR ||
            it.targetSlot == equipSlot::GROIN_OVER || it.targetSlot == equipSlot::PENIS_WEAR ||
            it.targetSlot == equipSlot::VAGINA_WEAR || it.targetSlot == equipSlot::ASS_WEAR)
        {
            return ItemCategory::UNDERWEAR;
        }
        if (it.targetSlot == equipSlot::NECKWEAR || it.targetSlot == equipSlot::FINGER_PRIMARY ||
            it.targetSlot == equipSlot::WRISTS || it.targetSlot == equipSlot::PIERCING_EAR ||
            it.targetSlot == equipSlot::PIERCING_NOSE || it.targetSlot == equipSlot::PIERCING_LIP ||
            it.targetSlot == equipSlot::PIERCING_TONGUE || it.targetSlot == equipSlot::PIERCING_NIPPLE ||
            it.targetSlot == equipSlot::PIERCING_NAVEL || it.targetSlot == equipSlot::PIERCING_COCK ||
            it.targetSlot == equipSlot::PIERCING_VAGINA)
        {
            return ItemCategory::ACCESSORY;
        }
        return ItemCategory::CLOTHING;
    }
    if (it.isConsumable)
    {
        return ItemCategory::CONSUMABLE;
    }
    if (it.id.find("gem") != std::string::npos || it.id.find("ore") != std::string::npos ||
        it.id.find("stone") != std::string::npos || it.id.find("essence") != std::string::npos ||
        it.id.find("material") != std::string::npos)
    {
        return ItemCategory::MATERIAL;
    }
    return ItemCategory::MISC;
}

inline ItemCategory stringToItemCategory(const std::string& str)
{
    if (str == "WEAPON" || str == "weapon") return ItemCategory::WEAPON;
    if (str == "CLOTHING" || str == "clothing" || str == "armor" || str == "ARMOR") return ItemCategory::CLOTHING;
    if (str == "UNDERWEAR" || str == "underwear") return ItemCategory::UNDERWEAR;
    if (str == "ACCESSORY" || str == "accessory") return ItemCategory::ACCESSORY;
    if (str == "CONSUMABLE" || str == "consumable" || str == "potion") return ItemCategory::CONSUMABLE;
    if (str == "MATERIAL" || str == "material") return ItemCategory::MATERIAL;
    if (str == "KEY_ITEM" || str == "key_item" || str == "key") return ItemCategory::KEY_ITEM;
    return ItemCategory::MISC;
}

inline bool compareItemsNatural(const item& a, const item& b)
{
    int orderA = getItemCategorySortOrder(determineItemCategory(a));
    int orderB = getItemCategorySortOrder(determineItemCategory(b));
    if (orderA != orderB) return orderA < orderB;

    int slotA = static_cast<int>(a.targetSlot);
    int slotB = static_cast<int>(b.targetSlot);
    if (slotA != slotB) return slotA < slotB;

    return a.name < b.name;
}