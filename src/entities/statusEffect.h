#pragma once

#include <algorithm>
#include <string>
#include <utility>
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
    int durationMinutes = -1;
    int maxDurationMinutes = 1440;
    std::string iconId = "";

    std::vector<StatModifier> statModifiers;
    std::vector<std::string> grantedTags;

    StatusEffect() = default;

    StatusEffect(std::string id,
                 std::string name,
                 std::string description,
                 int durationTurns = -1,
                 bool isDebuff = false,
                 int durationMinutes = -1,
                 int maxDurationMinutes = 1440,
                 std::string iconId = "")
        : id(std::move(id)),
          name(std::move(name)),
          description(std::move(description)),
          durationTurns(durationTurns),
          isDebuff(isDebuff),
          durationMinutes(durationMinutes),
          maxDurationMinutes(maxDurationMinutes),
          iconId(std::move(iconId))
    {}

    int getRemainingMinutes() const
    {
        if (durationMinutes > 0) return durationMinutes;
        if (durationTurns > 0) return durationTurns * 60;
        return 0;
    }
};