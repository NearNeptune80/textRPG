#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "enums.h"
#include "item.h"

class inventoryComponent
{
public:
    std::vector<std::shared_ptr<item>> backpack;
    std::unordered_map<equipSlot, std::shared_ptr<item>> equipped;

    void addItem(std::shared_ptr<item> newItem);
    bool removeItem(const std::string& itemId);

    bool equipItem(size_t backpackIndex, equipSlot slot, const std::vector<std::string>& bodyPartTags = {});
    bool unequipItem(equipSlot slot);

    bool isEquipped(equipSlot slot) const;
    std::shared_ptr<item> getEquippedItem(equipSlot slot);
};