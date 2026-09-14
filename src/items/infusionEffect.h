#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "items/enchantmentAspects.h"
#include "items/infusionTier.h"

class entity;

enum class InfusionTargetType
{
    CONSUMABLE_TONIC,
    APPAREL,
    WEAPONRY,
    BODY_MARKING
};

struct InfusionEffect
{
    InfusionTargetType targetType = InfusionTargetType::CONSUMABLE_TONIC;
    EnchantmentFocus focus = EnchantmentFocus::NONE;
    AspectProperty property = AspectProperty::NONE;
    InfusionTier tier = InfusionTier::MINOR_BOON;
    int limitThreshold = -1;

    int calculateCost() const;
    std::vector<std::string> getEffectDescriptions(const entity* target = nullptr) const;
    std::string applyToEntity(entity* user, entity* target);

    nlohmann::json toJson() const;
    static InfusionEffect fromJson(const nlohmann::json& j);

    bool operator==(const InfusionEffect& other) const
    {
        return targetType == other.targetType &&
               focus == other.focus &&
               property == other.property &&
               tier == other.tier &&
               limitThreshold == other.limitThreshold;
    }
};
