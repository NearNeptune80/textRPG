#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "combat/combatAction.h"

struct WeaponSkill
{
    std::string id;
    std::string name;
    float staminaCost = 20.0f;
    std::string damageType = "slashing";
    float damageMult = 1.0f;
    float critBonus = 0.0f;
    float armorBypass = 0.0f;
    float staggerBonus = 0.0f;
    float armorShred = 0.0f;
    bool counterGuard = false;
    bool isAoe = false;
    std::string statusEffect = "";
    std::string description;
    float baseMagnitude = 25.0f;
    std::string requiredRace;
    int requiredLevel = 1;
    bool unlocked = true;

    CombatAction toCombatAction(const std::string& weaponElement = "physical") const;
};

class WeaponSkillDatabase
{
public:
    static bool loadFromFile(const std::string& filePath = "data/weapon_skills.json");
    static const WeaponSkill* getSkill(const std::string& id);
    static const std::unordered_map<std::string, WeaponSkill>& getAllSkills();
    static void clear();

private:
    static std::unordered_map<std::string, WeaponSkill> s_skills;
};
