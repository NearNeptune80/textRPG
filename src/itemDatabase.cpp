#include "itemDatabase.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::unordered_map<std::string, item> itemDatabase::registry;

equipSlot stringToEquipSlot(const std::string& str)
{
    // Row 1
    if (str == "EYEWEAR" || str == "EYES")                  return equipSlot::EYEWEAR;
    if (str == "HEADWEAR" || str == "HEAD")                 return equipSlot::HEADWEAR;
    if (str == "HAIR_WEAR" || str == "HAIR")                return equipSlot::HAIR_WEAR;
    if (str == "HORNS_SLOT" || str == "HORNS")              return equipSlot::HORNS_SLOT;
    if (str == "WEAPON_MAIN" || str == "PRIMARY_WEAPON")    return equipSlot::WEAPON_MAIN;
    if (str == "WEAPON_OFF" || str == "SECONDARY_WEAPON")   return equipSlot::WEAPON_OFF;

    // Row 2
    if (str == "MOUTHWEAR" || str == "MOUTH")               return equipSlot::MOUTHWEAR;
    if (str == "TORSO_OVER" || str == "OVER_TORSO")         return equipSlot::TORSO_OVER;
    if (str == "NECKWEAR" || str == "NECK")                 return equipSlot::NECKWEAR;
    if (str == "WINGS_SLOT" || str == "WINGS")              return equipSlot::WINGS_SLOT;
    if (str == "PIERCING_EAR" || str == "EAR_PIERCING")     return equipSlot::PIERCING_EAR;
    if (str == "PIERCING_NOSE" || str == "NOSE_PIERCING")   return equipSlot::PIERCING_NOSE;

    // Row 3
    if (str == "WRISTS")                                    return equipSlot::WRISTS;
    if (str == "TORSO_UNDER" || str == "TORSO")             return equipSlot::TORSO_UNDER;
    if (str == "CHEST_WEAR" || str == "CHEST")              return equipSlot::CHEST_WEAR;
    if (str == "NIPPLES_WEAR" || str == "NIPPLES")          return equipSlot::NIPPLES_WEAR;
    if (str == "PIERCING_LIP" || str == "LIP_PIERCING")     return equipSlot::PIERCING_LIP;
    if (str == "PIERCING_TONGUE" || str == "TONGUE_PIERCING") return equipSlot::PIERCING_TONGUE;

    // Row 4
    if (str == "HANDS")                                     return equipSlot::HANDS;
    if (str == "HIPS_WEAR" || str == "HIPS")                return equipSlot::HIPS_WEAR;
    if (str == "STOMACH_WEAR" || str == "STOMACH")          return equipSlot::STOMACH_WEAR;
    if (str == "FINGER_PRIMARY" || str == "FINGERS")        return equipSlot::FINGER_PRIMARY;
    if (str == "PIERCING_NIPPLE" || str == "NIPPLE_PIERCING") return equipSlot::PIERCING_NIPPLE;
    if (str == "PIERCING_NAVEL" || str == "NAVEL_PIERCING") return equipSlot::PIERCING_NAVEL;

    // Row 5
    if (str == "ANKLES")                                    return equipSlot::ANKLES;
    if (str == "LEGS_OUTER" || str == "LEGS")               return equipSlot::LEGS_OUTER;
    if (str == "GROIN_OVER" || str == "GROIN" || str == "UNDERWEAR_BOTTOM") return equipSlot::GROIN_OVER;
    if (str == "TAIL_SLOT" || str == "TAIL")                return equipSlot::TAIL_SLOT;
    if (str == "PIERCING_COCK" || str == "COCK_PIERCING")   return equipSlot::PIERCING_COCK;
    if (str == "PIERCING_VAGINA" || str == "VAGINAL_PIERCING") return equipSlot::PIERCING_VAGINA;

    // Row 6
    if (str == "CALVES")                                    return equipSlot::CALVES;
    if (str == "FEET")                                      return equipSlot::FEET;
    if (str == "ASS_WEAR" || str == "ANUS")                 return equipSlot::ASS_WEAR;
    if (str == "PENIS_WEAR" || str == "PENIS")              return equipSlot::PENIS_WEAR;
    if (str == "VAGINA_WEAR" || str == "VAGINA")            return equipSlot::VAGINA_WEAR;

    try
    {
        return static_cast<equipSlot>(std::stoi(str));
    }
    catch (...)
    {
        return equipSlot::NONE;
    }
}

std::string equipSlotToString(equipSlot slot)
{
    switch (slot)
    {
        case equipSlot::EYEWEAR:         return "EYEWEAR";
        case equipSlot::HEADWEAR:        return "HEADWEAR";
        case equipSlot::HAIR_WEAR:       return "HAIR_WEAR";
        case equipSlot::HORNS_SLOT:      return "HORNS_SLOT";
        case equipSlot::WEAPON_MAIN:     return "WEAPON_MAIN";
        case equipSlot::WEAPON_OFF:      return "WEAPON_OFF";
        case equipSlot::MOUTHWEAR:       return "MOUTHWEAR";
        case equipSlot::TORSO_OVER:      return "TORSO_OVER";
        case equipSlot::NECKWEAR:        return "NECKWEAR";
        case equipSlot::WINGS_SLOT:      return "WINGS_SLOT";
        case equipSlot::PIERCING_EAR:    return "PIERCING_EAR";
        case equipSlot::PIERCING_NOSE:   return "PIERCING_NOSE";
        case equipSlot::WRISTS:          return "WRISTS";
        case equipSlot::TORSO_UNDER:     return "TORSO_UNDER";
        case equipSlot::CHEST_WEAR:      return "CHEST_WEAR";
        case equipSlot::NIPPLES_WEAR:    return "NIPPLES_WEAR";
        case equipSlot::PIERCING_LIP:    return "PIERCING_LIP";
        case equipSlot::PIERCING_TONGUE: return "PIERCING_TONGUE";
        case equipSlot::HANDS:           return "HANDS";
        case equipSlot::HIPS_WEAR:       return "HIPS_WEAR";
        case equipSlot::STOMACH_WEAR:    return "STOMACH_WEAR";
        case equipSlot::FINGER_PRIMARY:  return "FINGER_PRIMARY";
        case equipSlot::PIERCING_NIPPLE: return "PIERCING_NIPPLE";
        case equipSlot::PIERCING_NAVEL:  return "PIERCING_NAVEL";
        case equipSlot::ANKLES:          return "ANKLES";
        case equipSlot::LEGS_OUTER:      return "LEGS_OUTER";
        case equipSlot::GROIN_OVER:      return "GROIN_OVER";
        case equipSlot::TAIL_SLOT:       return "TAIL_SLOT";
        case equipSlot::PIERCING_COCK:   return "PIERCING_COCK";
        case equipSlot::PIERCING_VAGINA: return "PIERCING_VAGINA";
        case equipSlot::CALVES:          return "CALVES";
        case equipSlot::FEET:            return "FEET";
        case equipSlot::ASS_WEAR:        return "ASS_WEAR";
        case equipSlot::PENIS_WEAR:      return "PENIS_WEAR";
        case equipSlot::VAGINA_WEAR:     return "VAGINA_WEAR";
        default:                         return "NONE";
    }
}

void from_json(const json& j, item& itemObj)
{
    j.at("id").get_to(itemObj.id);
    j.at("name").get_to(itemObj.name);
    itemObj.description = j.value("description", "");

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
        return true;
    }
    catch (const json::exception& e)
    {
        std::cerr << "Item JSON Parsing Error: " << e.what() << "\n";
        return false;
    }
}

bool itemDatabase::exists(const std::string& id) { return registry.find(id) != registry.end(); }

std::shared_ptr<item> itemDatabase::getItem(const std::string& id)
{
    if (exists(id)) return std::make_shared<item>(registry[id]);
    return nullptr;
}