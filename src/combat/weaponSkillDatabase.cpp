#include "combat/weaponSkillDatabase.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

std::unordered_map<std::string, WeaponSkill> WeaponSkillDatabase::s_skills;

CombatAction WeaponSkill::toCombatAction(const std::string& weaponElement) const
{
    CombatAction act;
    act.id = id;
    act.name = name;
    act.description = description;
    act.staminaCost = staminaCost;
    act.damageMultiplier = damageMult;
    act.damageType = (weaponElement.empty() || weaponElement == "physical") ? damageType : weaponElement;
    act.isAoe = isAoe;

    SpellEffectNode node;
    node.effectType = "DAMAGE";
    node.element = (weaponElement.empty() || weaponElement == "physical") ? "physical" : weaponElement;
    node.baseMagnitude = baseMagnitude;
    act.effectNodes.push_back(node);

    return act;
}

bool WeaponSkillDatabase::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[WeaponSkillDatabase] Failed to open " << filePath << std::endl;
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;

        s_skills.clear();
        if (j.contains("skills") && j["skills"].is_array())
        {
            for (const auto& skillJson : j["skills"])
            {
                WeaponSkill ws;
                ws.id = skillJson.value("id", "");
                ws.name = skillJson.value("name", "");
                ws.staminaCost = skillJson.value("staminaCost", 20.0f);
                ws.damageType = skillJson.value("damageType", "slashing");
                ws.damageMult = skillJson.value("damageMult", 1.0f);
                ws.critBonus = skillJson.value("critBonus", 0.0f);
                ws.armorBypass = skillJson.value("armorBypass", 0.0f);
                ws.staggerBonus = skillJson.value("staggerBonus", 0.0f);
                ws.armorShred = skillJson.value("armorShred", 0.0f);
                ws.counterGuard = skillJson.value("counterGuard", false);
                ws.isAoe = skillJson.value("isAoe", false);
                ws.statusEffect = skillJson.value("statusEffect", "");
                ws.description = skillJson.value("description", "");
                ws.baseMagnitude = skillJson.value("baseMagnitude", 25.0f);
                ws.requiredRace = skillJson.value("requiredRace", "");
                ws.requiredLevel = skillJson.value("requiredLevel", 1);
                ws.unlocked = skillJson.value("unlocked", true);

                if (!ws.id.empty())
                {
                    s_skills[ws.id] = ws;
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[WeaponSkillDatabase] JSON parse error in " << filePath << ": " << e.what() << std::endl;
        return false;
    }
}

const WeaponSkill* WeaponSkillDatabase::getSkill(const std::string& id)
{
    if (s_skills.empty())
    {
        loadFromFile("data/weapon_skills.json");
    }
    auto it = s_skills.find(id);
    if (it != s_skills.end())
    {
        return &it->second;
    }
    return nullptr;
}

const std::unordered_map<std::string, WeaponSkill>& WeaponSkillDatabase::getAllSkills()
{
    if (s_skills.empty())
    {
        loadFromFile("data/weapon_skills.json");
    }
    return s_skills;
}

void WeaponSkillDatabase::clear()
{
    s_skills.clear();
}
