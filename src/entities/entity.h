#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "entities/anatomyComponent.h"
#include "entities/gestationComponent.h"
#include "entities/questComponent.h"
#include "entities/statsComponent.h"
#include "entities/statusEffect.h"
#include "items/inventory.h"
#include "settings/gameSettings.h"

class entity
{
public:
    std::string id;
    std::string name;

    SexualOrientation orientation = SexualOrientation::HETEROSEXUAL;
    GenderArchetype genderArchetype = GenderArchetype::MALE;

    questComponent quests;
    anatomyComponent anatomy;
    inventoryComponent inventory;
    statsComponent stats;
    gestationComponent gestation;

    std::unordered_map<std::string, int> essences;
    std::vector<StatusEffect> statusEffects;

    // Customization & Cosmetics Tracking
    std::unordered_map<std::string, std::string> cosmetics;
    std::unordered_map<std::string, bool> piercings;
    std::unordered_map<std::string, std::string> tattoos;
    std::unordered_map<std::string, std::string> bodyHair;
    std::vector<std::string> personalityTraits;
    std::string startingOccupation = "Student";

    entity(std::string entityId, std::string entityName);

    std::array<std::string, 10> preparedCombatSlots; // Holds action/spell IDs for Primary Tab

    float buyMarkup = 1.25f;       // Merchant markup ratio
    float sellMarkdown = 0.50f;     // Merchant markdown ratio
    float tradePerkModifier = 0.0f;// Player trade discount
    float merchantAffinity = 1.0f; // Merchant affinity modifier
    int lastRestockDay = -1;       // Last day merchant restocked gold/inventory
    float baseMerchantGold = 500.0f;// Base gold stock

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