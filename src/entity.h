#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>
#include <nlohmann/json.hpp>

#include "enums.h"
#include "enchantment.h"
#include "inventory.h"
#include "statusEffect.h"
#include "itemDatabase.h"

enum class CoveringType
{
    SKIN,
    FUR,
    FEATHERS,
    SCALES,
    KERATIN,
    FLESH,
    HAIR_COVERING,
    IRIS
};

inline std::string getCoveringNoun(CoveringType cov)
{
    switch (cov)
    {
        case CoveringType::SKIN:          return "skin";
        case CoveringType::FUR:           return "fur";
        case CoveringType::FEATHERS:      return "feathers";
        case CoveringType::SCALES:        return "scales";
        case CoveringType::KERATIN:       return "keratin";
        case CoveringType::FLESH:         return "flesh";
        case CoveringType::HAIR_COVERING: return "hair";
        case CoveringType::IRIS:          return "eyes";
        default:                          return "skin";
    }
}

inline CoveringType stringToCoveringType(const std::string& str)
{
    if (str == "FUR")           return CoveringType::FUR;
    if (str == "FEATHERS")      return CoveringType::FEATHERS;
    if (str == "SCALES")        return CoveringType::SCALES;
    if (str == "KERATIN")       return CoveringType::KERATIN;
    if (str == "FLESH")         return CoveringType::FLESH;
    if (str == "HAIR_COVERING") return CoveringType::HAIR_COVERING;
    if (str == "IRIS")          return CoveringType::IRIS;
    return CoveringType::SKIN;
}

inline std::string coveringTypeToString(CoveringType cov)
{
    switch (cov)
    {
        case CoveringType::FUR:           return "FUR";
        case CoveringType::FEATHERS:      return "FEATHERS";
        case CoveringType::SCALES:        return "SCALES";
        case CoveringType::KERATIN:       return "KERATIN";
        case CoveringType::FLESH:         return "FLESH";
        case CoveringType::HAIR_COVERING: return "HAIR_COVERING";
        case CoveringType::IRIS:          return "IRIS";
        default:                          return "SKIN";
    }
}

struct bodyPart
{
    std::string id;
    std::string name;
    std::string race = "Human";

    int count = 1;
    CoveringType covering = CoveringType::SKIN;
    std::string primaryColor = "Fair";
    std::string secondaryColor = "";

    float length = 0.0f;
    float diameter = 0.0f;
    int cupSize = 0;
    std::string style = "";

    std::vector<std::string> tags;

    static std::string getCupSizeName(int size)
    {
        static const std::vector<std::string> cups = { "A", "B", "C", "D", "DD", "F", "FF", "G", "H" };
        if (size >= 0 && size < (int)cups.size()) return cups[size];
        return "Flat";
    }
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

std::string getSlotName(bodySlot slot);

class anatomyComponent
{
private:
    std::unordered_map<bodySlot, bodyPart> parts;
    std::unordered_map<tattooSlot, tattoo> tattoos;

public:
    float heightMeters = 1.75f;

    void setPart(bodySlot slot, const bodyPart& part);
    void removePart(bodySlot slot);
    bool hasPart(bodySlot slot) const;
    bodyPart* getPart(bodySlot slot);
    const bodyPart* getPart(bodySlot slot) const;

    bool hasTag(bodySlot slot, const std::string& tag) const;
    bool hasGlobalTag(const std::string& tag) const;
    std::vector<std::string> getAllTags() const;

    const std::unordered_map<bodySlot, bodyPart>& getAllParts() const { return parts; }

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

    float getRequiredXp() const { return level * 100.0f; }
    bool addXp(float amount);

    void setBaseStat(const std::string& name, float value);
    float getBaseStat(const std::string& name) const;
    void modifyBaseStat(const std::string& name, float amount);

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