#include "items/infusionEffect.h"

#include <format>
#include "entities/entity.h"
#include "entities/statusEffect.h"

int InfusionEffect::calculateCost() const
{
    int cost = 1;
    if (focus != EnchantmentFocus::NONE)
    {
        cost += getFocusDefinition(focus).getEssenceWeight();
    }
    if (property != AspectProperty::NONE)
    {
        cost += getPropertyDefinition(property).getEssenceWeight();
    }
    cost += getTierEssenceCost(tier);
    if (limitThreshold != -1)
    {
        cost += 1;
    }
    return cost;
}

std::vector<std::string> InfusionEffect::getEffectDescriptions(const entity* target) const
{
    std::vector<std::string> desc;
    int bonus = getTierStatBonus(tier);
    std::string sign = bonus >= 0 ? std::format("+{}", bonus) : std::format("{}", bonus);

    // Continuous Sizing & Gradual Transformations on Apparel
    if (propertySupportsLimits(property, nullptr))
    {
        if (targetType == InfusionTargetType::APPAREL)
        {
            std::string interval = "Weekly";
            if (tier == InfusionTier::MAJOR_HEX || tier == InfusionTier::GREATER_BOON) interval = "Hourly";
            else if (tier == InfusionTier::HEX || tier == InfusionTier::BOON) interval = "Daily";

            std::string dir = isTierNegative(tier) ? "reduction" : "increase";
            std::string propName = getPropertyDefinition(property).displayName;
            std::string limitStr = "";
            if (limitThreshold >= 0)
            {
                std::string limLabel = getLimitStepLabel(property, limitThreshold);
                limitStr = std::format(" (Limit: {})", limLabel);
            }
            desc.push_back(std::format("{} {} {}.{}", interval, propName, dir, limitStr));
            return desc;
        }
        else if (limitThreshold >= 0)
        {
            std::string limLabel = getLimitStepLabel(property, limitThreshold);
            desc.push_back(std::format("{} {} (Limit: {})", sign, getPropertyDefinition(property).displayName, limLabel));
            return desc;
        }
    }

    if (property == AspectProperty::PHYSIQUE_STAT || property == AspectProperty::CORE_PHYSIQUE)
    {
        desc.push_back(std::format("{} Physique & Physical Fortitude", sign));
        if (targetType == InfusionTargetType::CONSUMABLE_TONIC)
        {
            desc.push_back("Applies 'Colossus Might' status effect (4 hours).");
        }
    }
    else if (property == AspectProperty::AGILITY_STAT || property == AspectProperty::ATTR_AGILITY)
    {
        desc.push_back(std::format("{} Agility & Reflexive Speed", sign));
        if (targetType == InfusionTargetType::CONSUMABLE_TONIC)
        {
            desc.push_back("Applies 'Predator's Instinct' status effect (4 hours).");
        }
    }
    else if (property == AspectProperty::ARCANE_STAT || property == AspectProperty::CORE_ARCANE)
    {
        desc.push_back(std::format("{} Arcane Resonance", sign));
        if (targetType == InfusionTargetType::CONSUMABLE_TONIC)
        {
            desc.push_back("Applies 'Eldritch Clarity' status effect (4 hours).");
        }
    }
    else if (property == AspectProperty::HEALTH_CEILING || property == AspectProperty::CORE_HEALTH)
    {
        desc.push_back(std::format("{} Maximum Vitality", bonus * 15 >= 0 ? std::format("+{}", bonus * 15) : std::format("{}", bonus * 15)));
    }
    else if (property == AspectProperty::MANA_CEILING || property == AspectProperty::CORE_MANA)
    {
        desc.push_back(std::format("{} Maximum Aura", bonus * 15 >= 0 ? std::format("+{}", bonus * 15) : std::format("{}", bonus * 15)));
    }
    else if (property == AspectProperty::CORE_STAMINA)
    {
        desc.push_back(std::format("{} Maximum Stamina", bonus * 10 >= 0 ? std::format("+{}", bonus * 10) : std::format("{}", bonus * 10)));
    }
    else if (property == AspectProperty::VIRILITY_FACTOR || property == AspectProperty::VIRILITY_POTENCY || property == AspectProperty::ATTR_VIRILITY)
    {
        desc.push_back(std::format("{} Virile Potency", bonus * 10 >= 0 ? std::format("+{}", bonus * 10) : std::format("{}", bonus * 10)));
        if (targetType == InfusionTargetType::CONSUMABLE_TONIC)
        {
            desc.push_back("Applies 'Primal Surge' status effect (24 hours).");
        }
    }
    else if (property == AspectProperty::FERTILITY_FACTOR || property == AspectProperty::FERTILITY_RECEPTIVITY || property == AspectProperty::ATTR_FERTILITY)
    {
        desc.push_back(std::format("{} Fertile Receptivity", bonus * 10 >= 0 ? std::format("+{}", bonus * 10) : std::format("{}", bonus * 10)));
        if (targetType == InfusionTargetType::CONSUMABLE_TONIC)
        {
            desc.push_back("Applies 'Primal Surge' status effect (24 hours).");
        }
    }
    else if (property == AspectProperty::CORRUPTION_AURA || property == AspectProperty::CORE_CORRUPTION)
    {
        desc.push_back(std::format("{} Demonic Corruption Aura", sign));
        if (targetType == InfusionTargetType::CONSUMABLE_TONIC)
        {
            desc.push_back("Applies 'Alchemical Trance' status effect (6 hours).");
        }
    }
    else if (property == AspectProperty::ATTR_PHYSICAL_DAMAGE || property == AspectProperty::DAMAGE_PHYSICAL)
    {
        desc.push_back(std::format("{} Physical Damage", sign));
    }
    else if (property == AspectProperty::ATTR_SPELL_POWER)
    {
        desc.push_back(std::format("{} Spell Power", sign));
    }
    else if (property == AspectProperty::ATTR_ARMOR || property == AspectProperty::ARMOR_RATING)
    {
        desc.push_back(std::format("{} Armor Rating", sign));
    }
    else if (property == AspectProperty::ATTR_WARDING || property == AspectProperty::WARD_RESISTANCE)
    {
        desc.push_back(std::format("{} Elemental Warding", sign));
    }
    else if (property == AspectProperty::ATTR_CRITICAL || property == AspectProperty::CRITICAL_POWER)
    {
        desc.push_back(std::format("{} Critical Precision", sign));
    }
    else if (property == AspectProperty::SOULBOUND_SEAL)
    {
        desc.push_back("Soulbound Seal (Unseal: 5) - Affixes permanently onto wearer.");
    }
    else if (property == AspectProperty::CONCEAL_IDENTITY)
    {
        desc.push_back("Conceal Identity - Obscures true facial identity under an arcane shroud.");
    }
    else if (property == AspectProperty::SERVITUDE_INHIBITION)
    {
        desc.push_back("Servitude Binding - Suppresses uninhibited transformations and seals willpower.");
    }
    else if (property == AspectProperty::SENSORY_VIBRATION)
    {
        desc.push_back("+10 Resting Arousal & rhythmic sensory stimulation.");
    }
    else if (property == AspectProperty::RETENTION_STOMACH)
    {
        desc.push_back(std::format("{} Stomach Fluid Retention", sign));
    }
    else if (property == AspectProperty::RETENTION_MAMMARY)
    {
        desc.push_back(std::format("{} Mammary Fluid Retention", sign));
    }
    else if (property == AspectProperty::RETENTION_YONI)
    {
        desc.push_back(std::format("{} Yoni Semen Retention", sign));
    }
    else if (property == AspectProperty::RETENTION_ANAL)
    {
        desc.push_back(std::format("{} Anal Fluid Retention", sign));
    }
    else if (property == AspectProperty::DESIRE_ANAL)
    {
        desc.push_back(std::format("{} Anal Attunement & Craving", sign));
    }
    else if (property == AspectProperty::DESIRE_MAMMARY)
    {
        desc.push_back(std::format("{} Mammary Attunement & Craving", sign));
    }
    else if (property == AspectProperty::DESIRE_PHALLIC)
    {
        desc.push_back(std::format("{} Phallic Attunement & Craving", sign));
    }
    else if (property == AspectProperty::DESIRE_YONI)
    {
        desc.push_back(std::format("{} Yoni Attunement & Craving", sign));
    }
    else if (property == AspectProperty::DESIRE_FEET)
    {
        desc.push_back(std::format("{} Foot Attunement & Craving", sign));
    }
    else if (property == AspectProperty::DESIRE_DOMINANT)
    {
        desc.push_back(std::format("{} Dominant Behavioral Instinct", sign));
    }
    else if (property == AspectProperty::DESIRE_SUBMISSIVE)
    {
        desc.push_back(std::format("{} Submissive Behavioral Instinct", sign));
    }
    else if (property == AspectProperty::DESIRE_BONDAGE)
    {
        desc.push_back(std::format("{} Bondage & Restraint Craving", sign));
    }
    else if (property == AspectProperty::DESIRE_EXHIBITIONISM)
    {
        desc.push_back(std::format("{} Exhibitionism Inclination", sign));
    }
    else if (property == AspectProperty::DESIRE_CHASTITY)
    {
        desc.push_back(std::format("{} Chastity & Denial Craving", sign));
    }
    else if (property == AspectProperty::DESIRE_MASOCHISM)
    {
        desc.push_back(std::format("{} Masochistic Pleasure", sign));
    }
    else if (property == AspectProperty::DESIRE_SADISM)
    {
        desc.push_back(std::format("{} Sadistic Dominance", sign));
    }
    else if (property == AspectProperty::SCALE_SIZE)
    {
        desc.push_back(std::format("{} Dimension / Size on {}", sign, getFocusDefinition(focus).displayName));
    }
    else if (property == AspectProperty::FLUID_PRODUCTION || property == AspectProperty::REGENERATION_RATE)
    {
        desc.push_back(std::format("{} Fluid Capacity & Regeneration Rate", sign));
    }
    else
    {
        desc.push_back(std::format("{} {}", sign, getPropertyDefinition(property).displayName));
    }

    return desc;
}

std::string InfusionEffect::applyToEntity(entity* user, entity* target)
{
    if (!target) return "";

    std::string narrative = "";
    int bonus = getTierStatBonus(tier);
    int duration = getTierDurationMinutes(tier);

    if (targetType == InfusionTargetType::CONSUMABLE_TONIC)
    {
        if (property == AspectProperty::PHYSIQUE_STAT || focus == EnchantmentFocus::TORSO)
        {
            StatusEffect fx("colossus_might", "Colossus Might", "Imbued with surging muscular power and physical density.", -1, isTierNegative(tier), 240, 1440, "might");
            fx.statModifiers.push_back({ "physique", static_cast<float>(bonus), 0.0f });
            fx.statModifiers.push_back({ "health", static_cast<float>(bonus * 10), 0.0f });
            target->addStatusEffect(fx);
            narrative += std::format("Your muscle fibers tighten with surging fortitude! (+{} Physique for 4h).\n", bonus);
        }
        else if (property == AspectProperty::AGILITY_STAT || focus == EnchantmentFocus::HEAD_FEATURE)
        {
            StatusEffect fx("predator_instinct", "Predator's Instinct", "Sensory acuity and predator reflexes sharpen combat movements.", -1, isTierNegative(tier), 240, 1440, "instinct");
            fx.statModifiers.push_back({ "agility", static_cast<float>(bonus), 0.0f });
            fx.statModifiers.push_back({ "crit_chance", static_cast<float>(bonus * 2), 0.0f });
            target->addStatusEffect(fx);
            narrative += std::format("Your senses heighten into razor-sharp focus! (+{} Agility for 4h).\n", bonus);
        }
        else if (property == AspectProperty::ARCANE_STAT || focus == EnchantmentFocus::ARCANE_AMPLIFICATION)
        {
            StatusEffect fx("eldritch_clarity", "Eldritch Clarity", "Aura and arcane channels resonate with heightened focus.", -1, isTierNegative(tier), 240, 1440, "clarity");
            fx.statModifiers.push_back({ "arcane", static_cast<float>(bonus), 0.0f });
            fx.statModifiers.push_back({ "mana", static_cast<float>(bonus * 10), 0.0f });
            target->addStatusEffect(fx);
            narrative += std::format("Pure arcane electricity arcs beneath your skin! (+{} Arcane for 4h).\n", bonus);
        }
        else if (property == AspectProperty::VIRILITY_FACTOR || property == AspectProperty::FERTILITY_FACTOR)
        {
            StatusEffect fx("primal_surge", "Primal Surge", "Potent reproductive vitality and vigorous hormonal surge.", -1, false, 1440, 1440, "virility");
            fx.statModifiers.push_back({ "virility", static_cast<float>(bonus * 15), 0.0f });
            fx.statModifiers.push_back({ "fertility", static_cast<float>(bonus * 15), 0.0f });
            target->addStatusEffect(fx);
            narrative += std::format("A deep, warm surge radiates from your core! (+{} Vitality for 24h).\n", bonus * 15);
        }
        else if (property == AspectProperty::CORRUPTION_AURA)
        {
            StatusEffect fx("alchemical_trance", "Alchemical Trance", "Altered perceptual state inducing intoxicating arcane visions.", -1, false, 360, 1440, "trance");
            fx.statModifiers.push_back({ "corruption", static_cast<float>(bonus * 5), 0.0f });
            fx.statModifiers.push_back({ "lust", static_cast<float>(bonus * 5), 0.0f });
            target->addStatusEffect(fx);
            narrative += "Intoxicating warmth clouds your thoughts with euphoric arcane visions (6h).\n";
        }
        else
        {
            StatusEffect fx("alchemical_vigor", "Alchemical Vigor", "Revitalizing alchemical infusion.", -1, isTierNegative(tier), duration, 1440, "alchemical");
            target->addStatusEffect(fx);
            narrative += std::format("You absorb the infused alchemical essence ({} minutes).\n", duration);
        }

        // Direct stat modification if applicable
        if (property == AspectProperty::HEALTH_CEILING)
        {
            target->stats.modifyBaseStat("max_health", bonus * 15.0f);
        }
        else if (property == AspectProperty::MANA_CEILING)
        {
            target->stats.modifyBaseStat("max_mana", bonus * 15.0f);
        }
    }
    else
    {
        // Apparel or Weapon passive attachment
        if (property == AspectProperty::PHYSIQUE_STAT)
        {
            target->stats.modifyBaseStat("physique", bonus);
        }
        else if (property == AspectProperty::ARCANE_STAT)
        {
            target->stats.modifyBaseStat("arcane", bonus);
        }
        narrative += std::format("Infusion attunes to the wearer ({:+} to {}).\n", bonus, getPropertyDefinition(property).displayName);
    }

    return narrative;
}

nlohmann::json InfusionEffect::toJson() const
{
    nlohmann::json j;
    j["targetType"] = static_cast<int>(targetType);
    j["focus"] = enchantmentFocusToString(focus);
    j["property"] = aspectPropertyToString(property);
    j["tier"] = infusionTierToString(tier);
    j["limitThreshold"] = limitThreshold;
    return j;
}

InfusionEffect InfusionEffect::fromJson(const nlohmann::json& j)
{
    InfusionEffect eff;
    if (j.contains("targetType")) eff.targetType = static_cast<InfusionTargetType>(j["targetType"].get<int>());
    if (j.contains("focus")) eff.focus = stringToEnchantmentFocus(j["focus"].get<std::string>());
    if (j.contains("property")) eff.property = stringToAspectProperty(j["property"].get<std::string>());
    if (j.contains("tier")) eff.tier = stringToInfusionTier(j["tier"].get<std::string>());
    if (j.contains("limitThreshold")) eff.limitThreshold = j["limitThreshold"].get<int>();
    return eff;
}
