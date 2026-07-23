#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>

#include "enums.h"       
#include "enchantment.h"
#include "inventory.h"
#include "statusEffect.h"

// --- Anatomical Data Structs ---

struct bodyPart
{
    std::string id;
    std::string name;
    std::string race;
    std::string covering;
    std::string color;

    std::vector<std::string> tags;
};

struct tattoo
{
    std::string id;
    std::string name;
    std::string color;
    bool glowing = false;

    std::vector<enchantment> enchantments;
    std::vector<std::string> tags;
};

// --- Components ---

class anatomyComponent
{
private:
    std::unordered_map<bodySlot, bodyPart> parts;
    std::unordered_map<tattooSlot, tattoo> tattoos;

public:
    // Body Part Methods
    void setPart(bodySlot slot, const bodyPart& part);
    void removePart(bodySlot slot);
    bool hasPart(bodySlot slot) const;
    bodyPart* getPart(bodySlot slot);
    const bodyPart* getPart(bodySlot slot) const;

    // Anatomy Tag Queries
    bool hasTag(bodySlot slot, const std::string& tag) const;
    bool hasGlobalTag(const std::string& tag) const;
    std::vector<std::string> getAllTags() const;

    const std::unordered_map<bodySlot, bodyPart>& getAllParts() const { return parts; }

    // Tattoo Methods
    void setTattoo(tattooSlot slot, const tattoo& tat);
    void removeTattoo(tattooSlot slot);
    bool hasTattoo(tattooSlot slot) const;
    tattoo* getTattoo(tattooSlot slot);

    void printDebug() const;
};

class statsComponent
{
private:
    std::unordered_map<std::string, float> baseValues;

public:
    int level = 1;
    float currentXp = 0.0f;

    // Leveling Math
    float getRequiredXp() const { return level * 100.0f; }
    bool addXp(float amount); // Returns true on level up

    // Base Stat Helpers
    void setBaseStat(const std::string& name, float value);
    float getBaseStat(const std::string& name) const;
    void modifyBaseStat(const std::string& name, float amount);

    // Calculates Final Stat value incorporating active status effects
    float getEffectiveStat(const std::string& name, const std::vector<StatusEffect>& activeEffects = {}) const;

    void printDebug() const;
};

class questComponent
{
public:
    std::unordered_map<std::string, int> activeQuests;

    void setQuestStage(const std::string& questId, int stage);
    int getQuestStage(const std::string& questId) const;
};

// --- Core Entity ---

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

    // Status Effect Management
    void addStatusEffect(const StatusEffect& effect);
    void removeStatusEffect(const std::string& effectId);
    bool hasStatusEffect(const std::string& effectId) const;
    void updateStatusEffectsOnTurn();

    // Primary Stat Evaluation
    float getStat(const std::string& statName) const
    {
        return stats.getEffectiveStat(statName, statusEffects);
    }
};