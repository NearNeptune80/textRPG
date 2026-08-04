#pragma once

#include <string>
#include <vector>

#include "common/enums.h"

struct enchantmentEffect
{
    effectType type = effectType::STAT_MODIFIER;

    std::string targetStat;
    bodySlot targetSlot = bodySlot::TORSO;

    int intValue = 0;
    std::string stringValue;
};

struct enchantment
{
    std::string id;
    std::string name;

    std::string essenceType;
    int essenceCost = 0;

    std::vector<enchantmentEffect> effects;
};