#include "items/enchantingEngine.h"

#include <algorithm>
#include <format>
#include <unordered_map>
#include "items/item.h"
#include "entities/entity.h"

namespace EnchantingEngine
{
    int calculateInfusionCost(const item* baseItem, const std::vector<InfusionEffect>& stagedEffects, const entity* player)
    {
        int totalCost = 0;

        std::vector<InfusionEffect> existingEffects;
        if (baseItem)
        {
            existingEffects = baseItem->infusionEffects;
        }

        // 1. Calculate cost for added or modified effects
        for (const auto& staged : stagedEffects)
        {
            bool alreadyExisted = false;
            for (const auto& exist : existingEffects)
            {
                if (staged == exist)
                {
                    alreadyExisted = true;
                    break;
                }
            }

            if (!alreadyExisted)
            {
                totalCost += staged.calculateCost();
            }
        }

        // 2. Removal of existing effects: cleansing curses costs essence, removing beneficial attributes is free
        for (const auto& exist : existingEffects)
        {
            bool stillPresent = false;
            for (const auto& staged : stagedEffects)
            {
                if (staged == exist)
                {
                    stillPresent = true;
                    break;
                }
            }

            if (!stillPresent)
            {
                if (isTierNegative(exist.tier) || exist.property == AspectProperty::SOULBOUND_SEAL)
                {
                    totalCost += std::max(1, exist.calculateCost() / 2);
                }
            }
        }

        // Apply trade/enchanting perk modifiers if present
        if (player && player->tradePerkModifier > 0.0f)
        {
            float discount = std::clamp(player->tradePerkModifier, 0.0f, 0.5f);
            totalCost = static_cast<int>(std::round(totalCost * (1.0f - discount)));
        }

        return std::max(0, totalCost);
    }

    std::string composeItemName(const item* baseItem, const std::vector<InfusionEffect>& effects)
    {
        if (effects.empty())
        {
            return baseItem ? baseItem->name : "Infused Item";
        }

        std::string baseName = baseItem ? baseItem->name : "Tonic";
        const auto& primEff = effects.front();

        // Determine specific compound descriptors
        std::string suffix = "";
        if (primEff.property == AspectProperty::PHYSIQUE_STAT)
        {
            suffix = "of Colossus Might";
        }
        else if (primEff.property == AspectProperty::AGILITY_STAT || primEff.focus == EnchantmentFocus::HEAD_FEATURE)
        {
            suffix = "of Predator's Instinct";
        }
        else if (primEff.property == AspectProperty::ARCANE_STAT || primEff.focus == EnchantmentFocus::ARCANE_AMPLIFICATION)
        {
            suffix = "of Eldritch Clarity";
        }
        else if (primEff.property == AspectProperty::VIRILITY_FACTOR || primEff.property == AspectProperty::FERTILITY_FACTOR)
        {
            suffix = "of Primal Surge";
        }
        else if (primEff.property == AspectProperty::CORRUPTION_AURA)
        {
            suffix = "of Alchemical Trance";
        }
        else if (primEff.property == AspectProperty::HEALTH_CEILING)
        {
            suffix = "of Vitality";
        }
        else if (primEff.property == AspectProperty::MANA_CEILING)
        {
            suffix = "of Resonant Aura";
        }
        else if (primEff.property == AspectProperty::DAMAGE_PHYSICAL)
        {
            suffix = "of Slaughter";
        }
        else if (primEff.property == AspectProperty::DAMAGE_ELEMENTAL)
        {
            suffix = "of the Elements";
        }
        else if (primEff.property == AspectProperty::CRITICAL_POWER)
        {
            suffix = "of Precision";
        }
        else if (primEff.property == AspectProperty::LIFE_LEECH)
        {
            suffix = "of the Leech";
        }
        else if (primEff.property == AspectProperty::ATTACK_POWER)
        {
            suffix = "of Carnage";
        }
        else if (primEff.property == AspectProperty::STRIKE_VELOCITY)
        {
            suffix = "of Celerity";
        }
        else if (primEff.property == AspectProperty::FORTITUDE_STAT)
        {
            suffix = "of Iron Bastion";
        }
        else if (primEff.property == AspectProperty::MIND_WARD)
        {
            suffix = "of Mental Sanctuary";
        }
        else if (primEff.property == AspectProperty::MANA_REGENERATION)
        {
            suffix = "of Astral Flux";
        }
        else if (primEff.property == AspectProperty::HEALTH_VITALITY)
        {
            suffix = "of Primal Vitality";
        }
        else if (primEff.property == AspectProperty::BREAST_SIZE)
        {
            suffix = "of Voluptuous Bloom";
        }
        else if (primEff.property == AspectProperty::RACIAL_LEGS_BIPED || primEff.property == AspectProperty::RACIAL_STANCE_MORPH || primEff.property == AspectProperty::RACIAL_BODY_CONFIG)
        {
            suffix = "of Primal Awakening";
        }
        else
        {
            suffix = std::format("of {} {}", getPropertyDefinition(primEff.property).affixDescriptor, getFocusDefinition(primEff.focus).displayName);
        }

        if (baseItem && (baseItem->isConsumable || baseItem->isFood))
        {
            std::string prefix = baseItem->isFood ? "Potion" : "Elixir";
            if (baseName.find("Draught") != std::string::npos) prefix = "Draught";
            else if (baseName.find("Philter") != std::string::npos) prefix = "Philter";
            else if (baseName.find("Tincture") != std::string::npos) prefix = "Tincture";
            else if (baseName.find("Potion") != std::string::npos) prefix = "Potion";

            return std::format("{} {}", prefix, suffix);
        }
        else if (baseItem && baseItem->isEquippable)
        {
            std::string prefix = getFocusDefinition(primEff.focus).affixDescriptor;
            if (prefix.empty()) prefix = "Runic";
            return std::format("{} {} {}", prefix, baseName, suffix);
        }

        return std::format("{} {}", baseName, suffix);
    }

    std::shared_ptr<item> craftInfusedItem(const item* baseItem, const std::vector<InfusionEffect>& effects, const std::string& customName)
    {
        auto crafted = std::make_shared<item>();
        if (baseItem)
        {
            *crafted = *baseItem; // clone baseline properties
        }
        else
        {
            crafted->id = "infused_tonic";
            crafted->name = "Infused Tonic";
            crafted->isConsumable = true;
            crafted->isStackable = false;
            crafted->baseValue = 50;
        }

        // If base item was food, transmute it into a pure alchemical potion
        if (baseItem && baseItem->isFood)
        {
            crafted->isFood = false;
            crafted->isConsumable = true;
            crafted->category = ItemCategory::CONSUMABLE;
            crafted->targetSlot = equipSlot::NONE;
            crafted->description = std::format("An alchemical potion brewed at an enchanting altar by infusing {} with arcane essence. Drinking it activates its imbued enchantments.", baseItem->name);
            crafted->tooltip = "Alchemically brewed potion. Consumable.";
        }

        // Attach passive stat modifiers if crafting or modifying equippable gear
        if (crafted->isEquippable)
        {
            for (const auto& eff : effects)
            {
                int bonus = getTierStatBonus(eff.tier);
                if (eff.property == AspectProperty::PHYSIQUE_STAT)
                {
                    crafted->statModifiers.push_back({ "physique", static_cast<float>(bonus), 0.0f });
                }
                else if (eff.property == AspectProperty::AGILITY_STAT)
                {
                    crafted->statModifiers.push_back({ "agility", static_cast<float>(bonus), 0.0f });
                }
                else if (eff.property == AspectProperty::ARCANE_STAT)
                {
                    crafted->statModifiers.push_back({ "arcane", static_cast<float>(bonus), 0.0f });
                }
                else if (eff.property == AspectProperty::HEALTH_CEILING)
                {
                    crafted->statModifiers.push_back({ "max_health", static_cast<float>(bonus * 15), 0.0f });
                }
                else if (eff.property == AspectProperty::MANA_CEILING)
                {
                    crafted->statModifiers.push_back({ "max_mana", static_cast<float>(bonus * 15), 0.0f });
                }
                else if (eff.property == AspectProperty::CORRUPTION_AURA)
                {
                    crafted->statModifiers.push_back({ "corruption", static_cast<float>(bonus * 5), 0.0f });
                }
                else if (eff.property == AspectProperty::DAMAGE_PHYSICAL || eff.property == AspectProperty::DAMAGE_ELEMENTAL)
                {
                    crafted->statModifiers.push_back({ "damage", static_cast<float>(bonus * 2), 0.0f });
                }
                else if (eff.property == AspectProperty::CRITICAL_POWER)
                {
                    crafted->statModifiers.push_back({ "critical_damage", static_cast<float>(bonus * 10), 0.0f });
                }
                else if (eff.property == AspectProperty::ARMOR_RATING)
                {
                    crafted->statModifiers.push_back({ "armor", static_cast<float>(bonus * 2), 0.0f });
                }
                else if (eff.property == AspectProperty::WARD_RESISTANCE)
                {
                    crafted->statModifiers.push_back({ "ward_resistance", static_cast<float>(bonus * 3), 0.0f });
                }
                else if (eff.property == AspectProperty::ATTACK_POWER)
                {
                    crafted->statModifiers.push_back({ "damage", static_cast<float>(bonus * 2), 0.0f });
                }
                else if (eff.property == AspectProperty::STRIKE_VELOCITY || eff.property == AspectProperty::SPRINT_AGILITY)
                {
                    crafted->statModifiers.push_back({ "agility", static_cast<float>(bonus), 0.0f });
                }
                else if (eff.property == AspectProperty::FORTITUDE_STAT)
                {
                    crafted->statModifiers.push_back({ "physique", static_cast<float>(bonus), 0.0f });
                }
                else if (eff.property == AspectProperty::MIND_WARD)
                {
                    crafted->statModifiers.push_back({ "willpower", static_cast<float>(bonus), 0.0f });
                }
                else if (eff.property == AspectProperty::MANA_REGENERATION)
                {
                    crafted->statModifiers.push_back({ "arcane", static_cast<float>(bonus), 0.0f });
                }
                else if (eff.property == AspectProperty::HEALTH_VITALITY)
                {
                    crafted->statModifiers.push_back({ "max_health", static_cast<float>(bonus * 15), 0.0f });
                }
            }
        }

        crafted->infusionEffects = effects;
        crafted->name = !customName.empty() ? customName : composeItemName(baseItem, effects);
        crafted->count = 1;
        crafted->baseValue += calculateInfusionCost(baseItem, effects) * 8;

        return crafted;
    }
}
