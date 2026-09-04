#include "items/itemDatabase.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "entities/anatomyComponent.h"

using json = nlohmann::json;

std::unordered_map<std::string, std::shared_ptr<item>> itemDatabase::registry;

void from_json(const json& j, item& itemObj)
{
    j.at("id").get_to(itemObj.id);
    j.at("name").get_to(itemObj.name);
    itemObj.description = j.value("description", "");
    itemObj.tooltip = j.value("tooltip", "");
    itemObj.baseValue = j.value("baseValue", 0);

    itemObj.isConsumable = j.value("isConsumable", false);
    itemObj.isEquippable = j.value("isEquippable", false);
    itemObj.isStackable = j.value("isStackable", false);
    itemObj.isKeyItem = j.value("isKeyItem", false);
    itemObj.count = j.value("count", 1);
    itemObj.baseRace = j.value("baseRace", "");

    if (j.contains("targetSlot"))
    {
        std::string slotStr = j.at("targetSlot").get<std::string>();
        itemObj.targetSlot = stringToEquipSlot(slotStr);
    }
    else
    {
        itemObj.targetSlot = equipSlot::NONE;
    }

    if (j.contains("category"))
    {
        itemObj.category = stringToItemCategory(j.at("category").get<std::string>());
    }
    else if (j.contains("sortTag"))
    {
        itemObj.category = stringToItemCategory(j.at("sortTag").get<std::string>());
    }
    else
    {
        itemObj.category = determineItemCategory(itemObj);
    }

    if (j.contains("validSlots"))
    {
        itemObj.validSlots.clear();
        for (const auto& sItem : j["validSlots"])
        {
            equipSlot s = stringToEquipSlot(sItem.get<std::string>());
            if (s != equipSlot::NONE) itemObj.validSlots.push_back(s);
        }
    }
    if (itemObj.validSlots.empty() && itemObj.targetSlot != equipSlot::NONE)
    {
        itemObj.validSlots = itemObj.getValidEquipSlots();
    }

    if (j.contains("requiredTags"))
    {
        itemObj.requiredTags = j.at("requiredTags").get<std::vector<std::string>>();
    }
    if (j.contains("forbiddenTags"))
    {
        itemObj.forbiddenTags = j.at("forbiddenTags").get<std::vector<std::string>>();
    }

    if (j.contains("statModifiers"))
    {
        itemObj.statModifiers.clear();
        for (const auto& modJson : j["statModifiers"])
        {
            StatModifier mod;
            mod.statName = modJson.value("statName", "");
            mod.flatValue = modJson.value("flatValue", 0.0f);
            mod.percentValue = modJson.value("percentValue", 0.0f);
            itemObj.statModifiers.push_back(mod);
        }
    }

    if (j.contains("supportedDisplacements"))
    {
        itemObj.supportedDisplacements.clear();
        for (auto& [modeStr, slotsJson] : j["supportedDisplacements"].items())
        {
            DisplacementMode mode = stringToDisplacementMode(modeStr);
            if (mode != DisplacementMode::NONE)
            {
                std::vector<bodySlot> slots;
                for (const auto& sItem : slotsJson)
                {
                    slots.push_back(stringToBodySlot(sItem.get<std::string>()));
                }
                itemObj.supportedDisplacements[mode] = slots;
            }
        }
    }
}

bool itemDatabase::loadDatabase(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    try
    {
        json data;
        file >> data;
        registry.clear();
        for (const auto& itemJson : data.at("items"))
        {
            auto newItem = std::make_shared<item>(itemJson.get<item>());
            registry[newItem->id] = newItem;
        }
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Item JSON Parsing Error: " << e.what() << "\n";
        return false;
    }
}

bool itemDatabase::exists(std::string_view id)
{
    return registry.find(std::string(id)) != registry.end();
}

std::shared_ptr<item> itemDatabase::getItem(std::string_view id)
{
    auto it = registry.find(std::string(id));
    if (it != registry.end() && it->second)
    {
        return std::make_shared<item>(*it->second);
    }
    return nullptr;
}

const item* itemDatabase::getItemTemplate(std::string_view id)
{
    auto it = registry.find(std::string(id));
    if (it != registry.end() && it->second)
    {
        return it->second.get();
    }
    return nullptr;
}