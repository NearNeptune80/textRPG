#include "combat/spellDatabase.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

std::unordered_map<std::string, SpellDefinition> SpellDatabase::s_spells;

CombatAction SpellDefinition::toCombatAction() const
{
    CombatAction act;
    act.id = id.rfind("spell_", 0) == 0 ? id : ("spell_" + id);
    act.name = name;
    act.description = description;
    act.staminaCost = staminaCost;
    act.manaCost = mpCost;

    if (damage > 0.0f)
    {
        SpellEffectNode node;
        node.effectType = "DAMAGE";
        std::string elem = school;
        std::transform(elem.begin(), elem.end(), elem.begin(), ::tolower);
        node.element = elem;
        node.baseMagnitude = damage;
        act.effectNodes.push_back(node);
    }
    if (defense > 0.0f)
    {
        SpellEffectNode node;
        node.effectType = "SHIELD";
        node.element = "arcane";
        node.baseMagnitude = defense;
        act.effectNodes.push_back(node);
    }
    if (heal > 0.0f)
    {
        SpellEffectNode node;
        node.effectType = "CLEANSE";
        node.element = "restoration";
        node.baseMagnitude = heal;
        act.effectNodes.push_back(node);
    }
    if (agilityBonus > 0.0f)
    {
        SpellEffectNode node;
        node.effectType = "BUFF";
        node.element = "agility";
        node.baseMagnitude = agilityBonus;
        act.effectNodes.push_back(node);
    }

    return act;
}

bool SpellDatabase::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[SpellDatabase] Failed to open " << filePath << std::endl;
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;

        s_spells.clear();
        if (j.contains("spells") && j["spells"].is_array())
        {
            for (const auto& sJson : j["spells"])
            {
                SpellDefinition sp;
                sp.id = sJson.value("id", "");
                sp.name = sJson.value("name", "");
                sp.school = sJson.value("school", "Arcane");
                sp.mpCost = sJson.value("mpCost", 0.0f);
                sp.staminaCost = sJson.value("staminaCost", 10.0f);
                sp.damage = sJson.value("damage", 0.0f);
                sp.defense = sJson.value("defense", 0.0f);
                sp.heal = sJson.value("heal", 0.0f);
                sp.agilityBonus = sJson.value("agilityBonus", 0.0f);
                sp.description = sJson.value("description", "");
                sp.unlocked = sJson.value("unlocked", false);
                sp.isSummon = sJson.value("isSummon", false);
                sp.summonType = sJson.value("summonType", "");
                sp.companionId = sJson.value("companionId", "");
                sp.requiredRace = sJson.value("requiredRace", "");
                sp.requiredLevel = sJson.value("requiredLevel", 1);

                sp.canCastInOverworld = sJson.value("canCastInOverworld", false);
                if (sJson.contains("overworldEffect") && sJson["overworldEffect"].is_object())
                {
                    const auto& oeJson = sJson["overworldEffect"];
                    sp.overworldEffect.type = oeJson.value("type", "");
                    sp.overworldEffect.magnitude = oeJson.value("magnitude", 0.0f);
                    sp.overworldEffect.durationMinutes = oeJson.value("durationMinutes", 0);
                    sp.overworldEffect.target = oeJson.value("target", "");
                }

                if (!sp.id.empty())
                {
                    s_spells[sp.id] = sp;
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[SpellDatabase] JSON parse error in " << filePath << ": " << e.what() << std::endl;
        return false;
    }
}

const SpellDefinition* SpellDatabase::getSpell(const std::string& id)
{
    if (s_spells.empty())
    {
        loadFromFile("data/spells.json");
    }

    auto it = s_spells.find(id);
    if (it != s_spells.end())
    {
        return &it->second;
    }

    // Handle aliases like SPELL_DART, SPELL_FIREBALL, spell_arcane_dart, etc.
    if (id == "SPELL_DART" || id == "spell_arcane_dart") return getSpell("arcane_dart");
    if (id == "SPELL_FIREBALL" || id == "spell_fireball") return getSpell("fireball");
    if (id == "SPELL_SHIELD" || id == "spell_arcane_shield") return getSpell("arcane_shield");
    if (id == "SPELL_CLEANSE" || id == "spell_cleanse") return getSpell("cleanse");
    if (id == "SPELL_BLINK" || id == "spell_blink" || id == "blink") return getSpell("teleport");

    // Try stripping "spell_" prefix
    if (id.rfind("spell_", 0) == 0)
    {
        std::string stripped = id.substr(6);
        it = s_spells.find(stripped);
        if (it != s_spells.end()) return &it->second;
    }

    return nullptr;
}

const std::unordered_map<std::string, SpellDefinition>& SpellDatabase::getAllSpells()
{
    if (s_spells.empty())
    {
        loadFromFile("data/spells.json");
    }
    return s_spells;
}

void SpellDatabase::clear()
{
    s_spells.clear();
}
