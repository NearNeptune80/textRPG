#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "enums.h"       // <-- Add this!
#include "enchantment.h"
#include "inventory.h"

// (DELETE ALL THE ENUMS THAT USED TO BE HERE)

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

class anatomyComponent
{
private:
    std::unordered_map<bodySlot, bodyPart> parts;
    std::unordered_map<tattooSlot, tattoo> tattoos;

public:
    void setPart(bodySlot slot, const bodyPart& part);
    void removePart(bodySlot slot);
    bool hasPart(bodySlot slot) const;
    bodyPart* getPart(bodySlot slot);
    bool hasTag(bodySlot slot, const std::string& tag) const;

    void setTattoo(tattooSlot slot, const tattoo& tat);
    void removeTattoo(tattooSlot slot);
    bool hasTattoo(tattooSlot slot) const;
    tattoo* getTattoo(tattooSlot slot);

    void printDebug() const;
};

class entity
{
public:
    std::string id;
    std::string name;

    std::unordered_map<std::string, int> essences;

    anatomyComponent anatomy;
    inventoryComponent inventory;

    entity(std::string entityId, std::string entityName);
};