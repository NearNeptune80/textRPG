#pragma once
#include <string>
#include <vector>
#include "enums.h"

struct StatModifier
{
    std::string statName; // e.g., "physique", "health", "lust"
    float flatValue = 0.0f;
    float percentValue = 0.0f; // e.g., 0.15f for +15%
};

struct StatusEffect
{
    std::string id;          // e.g., "buff_canis_potency", "debuff_poison"
    std::string name;        // Display Name
    std::string description; // Tooltip / Log description

    int durationTurns = -1;  // Remaining turns (-1 for permanent/infinite)
    bool isDebuff = false;

    std::vector<StatModifier> statModifiers;
    std::vector<std::string> grantedTags; // e.g., "paralyzed", "glowing"
};