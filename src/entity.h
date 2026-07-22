#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "enums.h"       
#include "enchantment.h"
#include "inventory.h"

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
    bool glowing;

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
    bool hasTag(bodySlot slot, const std::string& tag) const;

    // Tattoo Methods
    void setTattoo(tattooSlot slot, const tattoo& tat);
    void removeTattoo(tattooSlot slot);
    bool hasTattoo(tattooSlot slot) const;
    tattoo* getTattoo(tattooSlot slot);

    void printDebug() const;
};

class statsComponent
{
public:
    int level = 1;
    int experience = 0;

    // Flexible stats map (e.g., "strength", "lust", "corruption")
    std::unordered_map<std::string, float> values;

    void setStat(const std::string& name, float value);
    float getStat(const std::string& name) const;
    void modifyStat(const std::string& name, float amount);

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

    std::unordered_map<std::string, int> essences;

    anatomyComponent anatomy;
    inventoryComponent inventory;
    statsComponent stats;

    entity(std::string entityId, std::string entityName);
};