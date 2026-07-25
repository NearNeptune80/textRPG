#pragma once
#include <string>
#include <vector>
#include "enums.h"

/**
 * Individual modifier applied to a base stat.
 */
struct StatModifier
{
    std::string statName;
    float flatValue = 0.0f;
    float percentValue = 0.0f; // e.g., 0.15 for +15%
};

/**
 * Temporary or permanent status condition affecting an entity.
 */
struct StatusEffect
{
    std::string id;
    std::string name;
    std::string description;

    int durationTurns = -1; // -1 for infinite/permanent
    bool isDebuff = false;

    std::vector<StatModifier> statModifiers;
    std::vector<std::string> grantedTags;
};