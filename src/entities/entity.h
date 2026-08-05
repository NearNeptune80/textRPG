#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "entities/anatomyComponent.h"
#include "entities/questComponent.h"
#include "entities/statsComponent.h"
#include "entities/statusEffect.h"
#include "items/inventory.h"

class entity
{
public:
    std::string id;
    std::string name;

    questComponent quests;
    anatomyComponent anatomy;
    inventoryComponent inventory;
    statsComponent stats;

    std::unordered_map<std::string, int> essences;
    std::vector<StatusEffect> statusEffects;

    entity(std::string entityId, std::string entityName);
    
    std::array<std::string, 10> preparedCombatSlots; // Holds action/spell IDs for the Primary Tab

    void addStatusEffect(const StatusEffect& effect);
    void removeStatusEffect(const std::string& effectId);
    bool hasStatusEffect(const std::string& effectId) const;
    void updateStatusEffectsOnTurn();

    float getStat(const std::string& statName) const
    {
        return stats.getEffectiveStat(statName, statusEffects);
    }

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};