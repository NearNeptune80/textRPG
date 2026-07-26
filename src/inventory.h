#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "enums.h"
#include "item.h"

struct InventorySlot
{
    std::shared_ptr<item> itemPtr;
    int totalCount = 1;
    int firstBackpackIndex = -1; // Index of the primary item in backpack
};

class inventoryComponent
{
public:
    std::vector<std::shared_ptr<item>> backpack;
    std::unordered_map<equipSlot, std::shared_ptr<item>> equipped;

    // Helper to consolidate backpack items for UI display
    std::vector<InventorySlot> getStackedView() const;

    void addItem(std::shared_ptr<item> newItem);
    bool removeItem(const std::string& itemId, int count = 1);

    bool equipItem(size_t backpackIndex, equipSlot slot, const std::vector<std::string>& bodyPartTags = {});
    bool unequipItem(equipSlot slot);

    bool isEquipped(equipSlot slot) const;
    std::shared_ptr<item> getEquippedItem(equipSlot slot);
};
