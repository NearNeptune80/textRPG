#include "entities/perkDatabase.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::unordered_map<std::string, PerkDefinition> PerkDatabase::s_registry;

void PerkDatabase::parsePerkNode(const json& nodeJson)
{
    if (!nodeJson.is_object() || !nodeJson.contains("id")) return;

    PerkDefinition perk;
    perk.id = nodeJson.value("id", "");
    perk.name = nodeJson.value("name", perk.id);
    perk.category = nodeJson.value("category", "General");
    perk.cost = nodeJson.value("cost", 1);
    perk.tier = nodeJson.value("tier", 0);
    perk.column = nodeJson.value("column", 0.0f);
    perk.description = nodeJson.value("description", "");
    perk.requireAllParents = nodeJson.value("requireAllParents", false);

    if (nodeJson.contains("parents") && nodeJson["parents"].is_array())
    {
        for (const auto& p : nodeJson["parents"])
        {
            if (p.is_string()) perk.parents.push_back(p.get<std::string>());
        }
    }

    // Flat stat modifiers
    if (nodeJson.contains("statModifiers") && nodeJson["statModifiers"].is_object())
    {
        for (auto& [key, val] : nodeJson["statModifiers"].items())
        {
            if (val.is_number())
            {
                if (key == "tradePerkModifier")
                {
                    perk.modifiers.tradeDiscount = val.get<float>();
                    perk.modifiers.tradeBonus = val.get<float>();
                }
                else
                {
                    perk.modifiers.statModifiers[key] = val.get<float>();
                }
            }
        }
    }

    // Percent stat modifiers
    if (nodeJson.contains("percentStatModifiers") && nodeJson["percentStatModifiers"].is_object())
    {
        for (auto& [key, val] : nodeJson["percentStatModifiers"].items())
        {
            if (val.is_number())
            {
                perk.modifiers.percentStatModifiers[key] = val.get<float>();
            }
        }
    }

    // Damage modifiers
    if (nodeJson.contains("damageModifiers") && nodeJson["damageModifiers"].is_object())
    {
        const auto& dmg = nodeJson["damageModifiers"];
        if (dmg.contains("byRace") && dmg["byRace"].is_object())
        {
            for (auto& [race, mult] : dmg["byRace"].items())
            {
                if (mult.is_number()) perk.modifiers.damageBoostByRace[race] = mult.get<float>();
            }
        }
        if (dmg.contains("byAttackType") && dmg["byAttackType"].is_object())
        {
            for (auto& [atkType, mult] : dmg["byAttackType"].items())
            {
                if (mult.is_number()) perk.modifiers.damageBoostByAttackType[atkType] = mult.get<float>();
            }
        }
    }

    // Defense modifiers
    if (nodeJson.contains("defenseModifiers") && nodeJson["defenseModifiers"].is_object())
    {
        const auto& def = nodeJson["defenseModifiers"];
        if (def.contains("byRace") && def["byRace"].is_object())
        {
            for (auto& [race, mult] : def["byRace"].items())
            {
                if (mult.is_number()) perk.modifiers.defenseBoostByRace[race] = mult.get<float>();
            }
        }
        if (def.contains("byAttackType") && def["byAttackType"].is_object())
        {
            for (auto& [atkType, mult] : def["byAttackType"].items())
            {
                if (mult.is_number()) perk.modifiers.defenseBoostByAttackType[atkType] = mult.get<float>();
            }
        }
    }

    // Direct trade discount / bonus
    if (nodeJson.contains("tradeDiscount") && nodeJson["tradeDiscount"].is_number())
    {
        perk.modifiers.tradeDiscount = nodeJson["tradeDiscount"].get<float>();
    }
    if (nodeJson.contains("tradeBonus") && nodeJson["tradeBonus"].is_number())
    {
        perk.modifiers.tradeBonus = nodeJson["tradeBonus"].get<float>();
    }

    // Capability / scene / dialogue flags
    if (nodeJson.contains("flags") && nodeJson["flags"].is_array())
    {
        for (const auto& f : nodeJson["flags"])
        {
            if (f.is_string()) perk.modifiers.flags.push_back(f.get<std::string>());
        }
    }

    s_registry[perk.id] = perk;
}

bool PerkDatabase::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[PerkDatabase] Failed to open perk file: " << filePath << "\n";
        return false;
    }

    try
    {
        json data;
        file >> data;

        if (data.is_object())
        {
            // 1. Single unified tree: data["tree"]["nodes"]
            if (data.contains("tree") && data["tree"].is_object() && data["tree"].contains("nodes") && data["tree"]["nodes"].is_array())
            {
                for (const auto& nodeJson : data["tree"]["nodes"])
                {
                    parsePerkNode(nodeJson);
                }
            }

            // 2. Legacy multi-trees: data["trees"][category]["nodes"]
            else if (data.contains("trees") && data["trees"].is_object())
            {
                for (auto& [cat, catTree] : data["trees"].items())
                {
                    if (catTree.is_object() && catTree.contains("nodes") && catTree["nodes"].is_array())
                    {
                        for (const auto& nodeJson : catTree["nodes"])
                        {
                            parsePerkNode(nodeJson);
                        }
                    }
                }
            }

            // 3. Flat perk array: data["perks"], data["extraPerks"], data["npcPerks"]
            if (data.contains("perks") && data["perks"].is_array())
            {
                for (const auto& nodeJson : data["perks"])
                {
                    parsePerkNode(nodeJson);
                }
            }
            if (data.contains("extraPerks") && data["extraPerks"].is_array())
            {
                for (const auto& nodeJson : data["extraPerks"])
                {
                    parsePerkNode(nodeJson);
                }
            }
            if (data.contains("npcPerks") && data["npcPerks"].is_array())
            {
                for (const auto& nodeJson : data["npcPerks"])
                {
                    parsePerkNode(nodeJson);
                }
            }

            // 4. Single node object
            if (data.contains("id") && !data.contains("tree") && !data.contains("trees") && !data.contains("perks"))
            {
                parsePerkNode(data);
            }
        }
        else if (data.is_array())
        {
            for (const auto& nodeJson : data)
            {
                parsePerkNode(nodeJson);
            }
        }
    }
    catch (const json::exception& e)
    {
        std::cerr << "[PerkDatabase] JSON parse error in " << filePath << ": " << e.what() << "\n";
        return false;
    }

    return !s_registry.empty();
}

bool PerkDatabase::loadFromDirectory(const std::string& dirPath)
{
    fs::path p(dirPath);
    if (!fs::exists(p)) return false;

    bool loadedAny = false;
    if (fs::is_directory(p))
    {
        for (const auto& entry : fs::recursive_directory_iterator(p))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                if (loadFromFile(entry.path().string())) loadedAny = true;
            }
        }
    }
    else if (fs::is_regular_file(p))
    {
        loadedAny = loadFromFile(p.string());
    }

    return loadedAny;
}

const PerkDefinition* PerkDatabase::getPerk(const std::string& id)
{
    auto it = s_registry.find(id);
    if (it != s_registry.end())
    {
        return &it->second;
    }
    return nullptr;
}

bool PerkDatabase::hasPerk(const std::string& id)
{
    return s_registry.contains(id);
}

const std::unordered_map<std::string, PerkDefinition>& PerkDatabase::getAllPerks()
{
    return s_registry;
}

void PerkDatabase::clear()
{
    s_registry.clear();
}

void PerkDatabase::registerPerk(const PerkDefinition& perk)
{
    s_registry[perk.id] = perk;
}
