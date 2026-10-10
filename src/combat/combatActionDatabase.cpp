#include "combat/combatActionDatabase.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

std::unordered_map<std::string, CombatAction> CombatActionDatabase::s_actions;

bool CombatActionDatabase::loadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "[CombatActionDatabase] Failed to open " << filePath << std::endl;
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;

        s_actions.clear();
        if (j.contains("actions") && j["actions"].is_array())
        {
            for (const auto& aJson : j["actions"])
            {
                CombatAction act;
                act.id = aJson.value("id", "");
                act.name = aJson.value("name", "");
                act.description = aJson.value("description", "");
                act.staminaCost = aJson.value("staminaCost", 0.0f);
                act.manaCost = aJson.value("manaCost", 0.0f);
                act.lustCost = aJson.value("lustCost", 0.0f);
                act.lustDamage = aJson.value("lustDamage", 0.0f);
                act.damageMultiplier = aJson.value("damageMultiplier", 1.0f);
                act.damageType = aJson.value("damageType", "physical");
                act.isAoe = aJson.value("isAoe", false);
                act.isGuarding = aJson.value("isGuarding", false);
                act.isDodging = aJson.value("isDodging", false);
                act.isParrying = aJson.value("isParrying", false);

                std::string targetStr = aJson.value("targetType", "SINGLE_TARGET");
                if (targetStr == "SELF") act.targetType = EffectTargetType::SELF;
                else if (targetStr == "ALL_ENEMIES") act.targetType = EffectTargetType::ALL_ENEMIES;
                else if (targetStr == "ALL_ALLIES") act.targetType = EffectTargetType::ALL_ALLIES;
                else act.targetType = EffectTargetType::SINGLE_TARGET;

                if (aJson.contains("effectNodes") && aJson["effectNodes"].is_array())
                {
                    for (const auto& nJson : aJson["effectNodes"])
                    {
                        SpellEffectNode node;
                        node.effectType = nJson.value("effectType", "DAMAGE");
                        node.element = nJson.value("element", "physical");
                        node.baseMagnitude = nJson.value("baseMagnitude", 0.0f);
                        node.scalingFactor = nJson.value("scalingFactor", 1.0f);
                        node.statusEffectId = nJson.value("statusEffectId", "");
                        act.effectNodes.push_back(node);
                    }
                }

                if (!act.id.empty())
                {
                    s_actions[act.id] = act;
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[CombatActionDatabase] JSON parse error in " << filePath << ": " << e.what() << std::endl;
        return false;
    }
}

const CombatAction* CombatActionDatabase::getAction(const std::string& id)
{
    if (s_actions.empty())
    {
        loadFromFile("data/combat/actions.json");
    }
    auto it = s_actions.find(id);
    if (it != s_actions.end())
    {
        return &it->second;
    }
    if (id == "DEFENSE_GUARD" || id == "GUARD" || id == "DEFEND") return getAction("action_guard");
    if (id == "DEFENSE_DODGE" || id == "DODGE") return getAction("action_dodge");
    if (id == "DEFENSE_PARRY" || id == "PARRY") return getAction("action_parry");
    if (id == "SEDUCE_TEASE" || id == "TEASE") return getAction("action_seduce_tease");
    if (id == "SEDUCE_FLIRT" || id == "FLIRT") return getAction("action_seduce_flirt");
    if (id == "SEDUCE_EXPOSE" || id == "EXPOSE") return getAction("action_seduce_expose");
    if (id == "COMPANION_ATTACK") return getAction("action_companion_attack");
    if (id == "ITEM_POTION") return getAction("item_potion");
    if (id == "ITEM_MANA") return getAction("item_mana");
    return nullptr;
}

const std::unordered_map<std::string, CombatAction>& CombatActionDatabase::getAllActions()
{
    if (s_actions.empty())
    {
        loadFromFile("data/combat/actions.json");
    }
    return s_actions;
}

void CombatActionDatabase::clear()
{
    s_actions.clear();
}
