#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/enums.h"
#include "items/clothingDisplacement.h"
#include "items/item.h"

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

    std::unordered_map<equipSlot, DisplacementMode> activeDisplacements;

    std::vector<InventorySlot> getStackedView() const;

    void addItem(std::shared_ptr<item> newItem);
    bool removeItem(const std::string& itemId, int count = 1);

    bool equipItem(size_t backpackIndex, equipSlot slot, const std::vector<std::string>& bodyPartTags = {});
    bool unequipItem(equipSlot slot);

    bool isEquipped(equipSlot slot) const;
    std::shared_ptr<item> getEquippedItem(equipSlot slot);

    // Partial Clothing Displacement Engine
    void setDisplacement(equipSlot slot, DisplacementMode mode);
    DisplacementMode getDisplacement(equipSlot slot) const;
    void resetAllDisplacements();
    bool isSlotExposed(bodySlot slot) const;
};