#include "items/enchantingEngine.h"

#include <algorithm>
#include <format>
#include <unordered_map>
#include "items/item.h"
#include "items/itemDatabase.h"
#include "items/enchantmentRegistry.h"
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

        std::string suffix = "";
        auto& reg = EnchantmentRegistry::getInstance();
        const auto* pDef = reg.getPropertyDef(primEff.property);
        const auto* fDef = reg.getFocusDef(primEff.focus);

        if (pDef && !pDef->suffix.empty())
        {
            suffix = pDef->suffix;
        }
        else if (fDef && !fDef->suffix.empty())
        {
            suffix = fDef->suffix;
        }
        else
        {
            std::string pAffix = pDef ? pDef->affixDescriptor : "";
            std::string fName = fDef ? fDef->displayName : "";
            suffix = std::format("of {} {}", pAffix, fName);
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

    std::string composeItemDescription(const item* baseItem, const std::vector<InfusionEffect>& effects)
    {
        std::string desc = "";
        if (baseItem && !baseItem->description.empty())
        {
            desc = baseItem->description;
            size_t pos = desc.find("\n\nEnchantments:");
            if (pos != std::string::npos)
            {
                desc = desc.substr(0, pos);
            }
        }
        else if (baseItem && baseItem->isFood)
        {
            desc = std::format("An alchemical potion brewed at an enchanting altar by infusing {} with arcane essence. Drinking it activates its imbued enchantments.", baseItem->name);
        }
        else
        {
            desc = "A specialized item crafted at the enchanting altar with arcane essences.";
        }

        if (!effects.empty())
        {
            desc += "\n\nEnchantments:";
            for (const auto& eff : effects)
            {
                auto dList = eff.getEffectDescriptions();
                std::string sText = dList.empty() ? getPropertyDisplayName(eff.property, baseItem) : dList.front();
                if (baseItem && baseItem->isRacialReagent())
                {
                    std::string raceDisp = getPropertyDisplayName(eff.property, baseItem);
                    if (raceDisp != getPropertyDefinition(eff.property).displayName)
                    {
                        int bonus = getTierStatBonus(eff.tier);
                        std::string sign = bonus >= 0 ? std::format("+{}", bonus) : std::format("{}", bonus);
                        sText = std::format("{} {}", sign, raceDisp);
                    }
                }
                desc += "\n• " + sText;
            }
        }
        return desc;
    }

    std::string composeItemTooltip(const item* baseItem, const std::vector<InfusionEffect>& effects)
    {
        std::string tip = "";
        if (baseItem && !baseItem->tooltip.empty())
        {
            tip = baseItem->tooltip;
            size_t pos = tip.find("\nEnchantments:");
            if (pos != std::string::npos)
            {
                tip = tip.substr(0, pos);
            }
        }
        else if (baseItem && !baseItem->description.empty())
        {
            tip = baseItem->description;
            size_t pos = tip.find("\n\nEnchantments:");
            if (pos != std::string::npos)
            {
                tip = tip.substr(0, pos);
            }
        }
        else
        {
            tip = "Infused Item";
        }

        if (!effects.empty())
        {
            tip += "\nEnchantments:";
            for (const auto& eff : effects)
            {
                auto dList = eff.getEffectDescriptions();
                std::string sText = dList.empty() ? getPropertyDisplayName(eff.property, baseItem) : dList.front();
                if (baseItem && baseItem->isRacialReagent())
                {
                    std::string raceDisp = getPropertyDisplayName(eff.property, baseItem);
                    if (raceDisp != getPropertyDefinition(eff.property).displayName)
                    {
                        int bonus = getTierStatBonus(eff.tier);
                        std::string sign = bonus >= 0 ? std::format("+{}", bonus) : std::format("{}", bonus);
                        sText = std::format("{} {}", sign, raceDisp);
                    }
                }
                tip += "\n• " + sText;
            }
        }
        return tip;
    }

    void updateItemInfusionDescriptions(item* it)
    {
        if (!it) return;
        if (!it->infusionEffects.empty())
        {
            it->description = composeItemDescription(it, it->infusionEffects);
            it->tooltip = composeItemTooltip(it, it->infusionEffects);
            it->briefDescription = it->tooltip;
        }
    }

    std::shared_ptr<item> craftInfusedItem(const item* baseItem, const std::vector<InfusionEffect>& effects, const std::string& customName)
    {
        auto crafted = std::make_shared<item>();
        if (baseItem)
        {
            *crafted = *baseItem;
        }
        else
        {
            const item* templateItem = itemDatabase::getItemTemplate("item_plain_elixir");
            if (templateItem)
            {
                *crafted = *templateItem;
            }
            else
            {
                crafted->isConsumable = true;
                crafted->category = ItemCategory::CONSUMABLE;
            }
        }

        if (baseItem && baseItem->isFood)
        {
            crafted->isFood = false;
            crafted->isConsumable = true;
            crafted->category = ItemCategory::CONSUMABLE;
            crafted->targetSlot = equipSlot::NONE;
        }

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
        crafted->description = composeItemDescription(baseItem, effects);
        crafted->tooltip = composeItemTooltip(baseItem, effects);
        crafted->briefDescription = crafted->tooltip;
        crafted->count = 1;
        crafted->baseValue += calculateInfusionCost(baseItem, effects) * 8;

        return crafted;
    }
}
