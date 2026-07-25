#pragma once
#include <string>
#include <vector>
#include "enums.h"

/**
 * Discrete magical effect instruction applied to stats or body parts.
 */
struct enchantmentEffect
{
    effectType type = effectType::STAT_MODIFIER;

    std::string targetStat;
    bodySlot targetSlot = bodySlot::TORSO;

    int intValue = 0;
    std::string stringValue;
};

/**
 * Overarching enchantment object containing essence requirements and effects.
 */
struct enchantment
{
    std::string id;
    std::string name;

    std::string essenceType;
    int essenceCost = 0;

    std::vector<enchantmentEffect> effects;
};