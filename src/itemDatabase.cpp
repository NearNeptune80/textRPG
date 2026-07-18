#include "itemDatabase.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Define the static member
std::unordered_map<std::string, item> itemDatabase::registry;

// --- JSON Conversion Helpers ---
// nlohmann::json uses these automatically when we call .get<item>()

// Helper to convert string to equipSlot enum
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

void from_json(const json& j, item& i)
{
    j.at("id").get_to(i.id);
    j.at("name").get_to(i.name);
    j.at("isConsumable").get_to(i.isConsumable);

    std::string slotStr = j.at("targetSlot").get<std::string>();
    i.targetSlot = stringToEquipSlot(slotStr);

    j.at("baseRace").get_to(i.baseRace);

    // Base templates load with empty enchantments; we modify them ingame!
    i.enchantments = std::vector<enchantment>();
}

// --- Database Logic ---

bool itemDatabase::loadDatabase(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open item data file: " << filePath << "\n";
        return false;
    }

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
        std::cerr << "JSON Parsing Error: " << e.what() << "\n";
        return false;
    }
}

bool itemDatabase::exists(const std::string& id)
{
    return registry.find(id) != registry.end();
}

item itemDatabase::getItem(const std::string& id)
{
    if (exists(id))
    {
        return registry[id]; // Returns a fresh copy
    }

    // Return an error/fallback item if not found
    item fallback;
    fallback.id = "unknown";
    fallback.name = "Missing Texture/Item";
    fallback.isConsumable = false;
    fallback.targetSlot = equipSlot::NONE;
    return fallback;
}