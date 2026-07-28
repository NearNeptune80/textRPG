#pragma once
#include <vector>
#include <array>
#include <string>
#include <memory>
#include "enums.h"
#include "item.h"

struct InventorySlot
{
    std::shared_ptr<item> itemPtr;
    int totalCount = 1;
    int firstBackpackIndex = -1;
};

class inventoryComponent
{
public:
    std::vector<std::shared_ptr<item>> backpack;
    std::array<std::shared_ptr<item>, EQUIP_SLOT_COUNT> equipped{};

    std::vector<InventorySlot> getStackedView() const;

    void addItem(std::shared_ptr<item> newItem);
    bool removeItem(const std::string& itemId, int count = 1);

    bool equipItem(size_t backpackIndex, equipSlot slot, const std::vector<std::string>& bodyPartTags = {});
    bool unequipItem(equipSlot slot);

    bool isEquipped(equipSlot slot) const;
    std::shared_ptr<item> getEquippedItem(equipSlot slot);
};