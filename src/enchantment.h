#pragma once
#include <string>
#include <vector>
#include "enums.h"

// The discrete instruction
struct enchantmentEffect
{
    effectType type;

    std::string targetStat;
    bodySlot targetSlot;

    int intValue;
    std::string stringValue;
};

// The overarching enchantment
struct enchantment
{
    std::string id;
    std::string name;

    std::string essenceType;
    int essenceCost;

    std::vector<enchantmentEffect> effects;
};