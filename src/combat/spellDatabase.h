#pragma once

#include <string>
#include <unordered_map>
#include "combat/combatAction.h"

struct OverworldSpellEffect
{
    std::string type = "";
    float magnitude = 0.0f;
    int durationMinutes = 0;
    std::string target = "";
};

struct SpellDefinition
{
    std::string id;
    std::string name;
    std::string school;
    float mpCost = 0.0f;
    float staminaCost = 10.0f;
    float damage = 0.0f;
    float defense = 0.0f;
    float heal = 0.0f;
    float agilityBonus = 0.0f;
    std::string description;
    bool unlocked = false;
    bool isSummon = false;
    std::string summonType;
    std::string companionId;
    std::string requiredRace;
    int requiredLevel = 1;

    bool canCastInOverworld = false;
    OverworldSpellEffect overworldEffect;

    CombatAction toCombatAction() const;
};

class SpellDatabase
{
public:
    static bool loadFromFile(const std::string& filePath = "data/spells.json");
    static const SpellDefinition* getSpell(const std::string& id);
    static const std::unordered_map<std::string, SpellDefinition>& getAllSpells();
    static void clear();

private:
    static std::unordered_map<std::string, SpellDefinition> s_spells;
};
