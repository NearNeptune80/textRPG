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
            }
        }

        crafted->infusionEffects = effects;
        crafted->name = !customName.empty() ? customName : composeItemName(baseItem, effects);
        crafted->count = 1;
        crafted->baseValue += calculateInfusionCost(baseItem, effects) * 8;

        return crafted;
    }
}
