#include "itemDatabase.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::unordered_map<std::string, item> itemDatabase::registry;

equipSlot stringToEquipSlot(const std::string& str)
{
    if (str == "HEADWEAR") return equipSlot::HEADWEAR;
    if (str == "EYEWEAR") return equipSlot::EYEWEAR;
    if (str == "MOUTHWEAR") return equipSlot::MOUTHWEAR;
    if (str == "NECKWEAR") return equipSlot::NECKWEAR;
    if (str == "SHOULDERS") return equipSlot::SHOULDERS;
    if (str == "TORSO_UNDER") return equipSlot::TORSO_UNDER;
    if (str == "TORSO_OVER") return equipSlot::TORSO_OVER;
    if (str == "CHEST_WEAR") return equipSlot::CHEST_WEAR;
    if (str == "STOMACH_WEAR") return equipSlot::STOMACH_WEAR;
    if (str == "WRISTS") return equipSlot::WRISTS;
    if (str == "HANDS") return equipSlot::HANDS;
    if (str == "FINGER_PRIMARY") return equipSlot::FINGER_PRIMARY;
    if (str == "FINGER_SECONDARY") return equipSlot::FINGER_SECONDARY;
    if (str == "HIPS_WEAR") return equipSlot::HIPS_WEAR;
    if (str == "GROIN_UNDER") return equipSlot::GROIN_UNDER;
    if (str == "GROIN_OVER") return equipSlot::GROIN_OVER;
    if (str == "LEGS_INNER") return equipSlot::LEGS_INNER;
    if (str == "LEGS_OUTER") return equipSlot::LEGS_OUTER;
    if (str == "FEET") return equipSlot::FEET;
    if (str == "HORNS_SLOT") return equipSlot::HORNS_SLOT;
    if (str == "WINGS_SLOT") return equipSlot::WINGS_SLOT;
    if (str == "TAIL_SLOT") return equipSlot::TAIL_SLOT;
    if (str == "WEAPON_MAIN") return equipSlot::WEAPON_MAIN;
    if (str == "WEAPON_OFF") return equipSlot::WEAPON_OFF;
    return equipSlot::NONE;
}

void from_json(const json& j, item& itemObj)
{
    j.at("id").get_to(itemObj.id);
    j.at("name").get_to(itemObj.name);

    itemObj.isConsumable = j.value("isConsumable", false);
    itemObj.isEquippable = j.value("isEquippable", false);
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

    if (j.contains("requiredTags"))
    {
        itemObj.requiredTags = j.at("requiredTags").get<std::vector<std::string>>();
    }
    if (j.contains("forbiddenTags"))
    {
        itemObj.forbiddenTags = j.at("forbiddenTags").get<std::vector<std::string>>();
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
            item newItem = itemJson.get<item>();
            registry[newItem.id] = newItem;
        }
        std::cout << "Successfully loaded " << registry.size() << " items from database.\n";
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Item JSON Parsing Error: " << e.what() << "\n";
        return false;
    }
}

bool itemDatabase::exists(const std::string& id)
{
    return registry.find(id) != registry.end();
}

std::shared_ptr<item> itemDatabase::getItem(const std::string& id)
{
    if (exists(id))
    {
        return std::make_shared<item>(registry[id]);
    }
    return nullptr;
}