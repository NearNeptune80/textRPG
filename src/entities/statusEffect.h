#pragma once

#include <string>
#include <vector>

#include "common/enums.h"

struct StatModifier
{
    std::string statName;
    float flatValue = 0.0f;
    float percentValue = 0.0f;
};

struct StatusEffect
{
    std::string id;
    std::string name;
    std::string description;

    int durationTurns = -1;
    bool isDebuff = false;

    std::vector<StatModifier> statModifiers;
    std::vector<std::string> grantedTags;
};