#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

struct PerkModifier
{
    std::unordered_map<std::string, float> statModifiers;           // Flat stat modifiers (e.g., max_health, strength, charm)
    std::unordered_map<std::string, float> percentStatModifiers;    // Percent stat modifiers (e.g., strength +10%)
    std::unordered_map<std::string, float> damageBoostByRace;        // Damage multiplier vs specific race (e.g. "Demon": 0.25)
    std::unordered_map<std::string, float> damageBoostByAttackType;  // Damage multiplier for attack type (e.g. "arcane": 0.20)
    std::unordered_map<std::string, float> defenseBoostByRace;       // Damage reduction vs specific race (e.g. "Undead": 0.20)
    std::unordered_map<std::string, float> defenseBoostByAttackType; // Damage reduction vs attack type (e.g. "fire": 0.25)
    float tradeDiscount = 0.0f;                                     // Merchant buying discount (e.g. 0.10)
    float tradeBonus = 0.0f;                                        // Merchant selling bonus (e.g. 0.10)
    std::vector<std::string> flags;                                 // Action, dialogue, or scene unlock flags
};

struct PerkDefinition
{
    std::string id;
    std::string name;
    std::string category;
    int cost = 1;
    int tier = 0;
    float column = 0.0f;
    std::vector<std::string> parents;
    bool requireAllParents = false;
    std::string description;
    PerkModifier modifiers;
};

class PerkDatabase
{
public:
    static bool loadFromFile(const std::string& filePath = "data/perks.json");
    static bool loadFromDirectory(const std::string& dirPath);
    static const PerkDefinition* getPerk(const std::string& id);
    static bool hasPerk(const std::string& id);
    static const std::unordered_map<std::string, PerkDefinition>& getAllPerks();
    static void clear();

    static void registerPerk(const PerkDefinition& perk);

private:
    static std::unordered_map<std::string, PerkDefinition> s_registry;
    static void parsePerkNode(const nlohmann::json& nodeJson);
};
