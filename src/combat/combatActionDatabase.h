#pragma once

#include <string>
#include <unordered_map>
#include "combat/combatAction.h"

class CombatActionDatabase
{
public:
    static bool loadFromFile(const std::string& filePath = "data/combat/actions.json");
    static const CombatAction* getAction(const std::string& id);
    static const std::unordered_map<std::string, CombatAction>& getAllActions();
    static void clear();

private:
    static std::unordered_map<std::string, CombatAction> s_actions;
};
