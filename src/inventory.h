#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "enums.h"
#include "item.h"

class inventoryComponent
{
public:
    std::vector<item> backpack;
    std::unordered_map<equipSlot, item> equipped;

    void addItem(const item& newItem);
    bool removeItem(const std::string& itemId);

    bool equipItem(size_t backpackIndex, equipSlot slot);
    bool unequipItem(equipSlot slot);

    bool isEquipped(equipSlot slot) const;
    item* getEquippedItem(equipSlot slot);
};