#pragma once

#include <string>
#include <string_view>
#include <vector>

enum class ItemRarity
{
    COMMON,
    UNCOMMON,
    RARE,
    EPIC,
    LEGENDARY
};

inline int getRarityEssenceWeight(ItemRarity rarity)
{
    switch (rarity)
    {
        case ItemRarity::COMMON:    return 1;
        case ItemRarity::UNCOMMON:  return 2;
        case ItemRarity::RARE:      return 4;
        case ItemRarity::EPIC:      return 8;
        case ItemRarity::LEGENDARY: return 12;
        default:                    return 1;
    }
}

enum class EnchantmentFocus
{
    NONE,

    // Physical & Anatomical Domains
    HEAD_FEATURE,
    HORNS,
    HAIR,
    EYES,
    EARS,
    FACE,
    SKIN,
    ARMS,
    TORSO,
    BREASTS,
    WINGS,
    TAIL,
    GENITALIA_PRIMARY,
    GENITALIA_SECONDARY,
    HIPS_ASS,

    // Equipment & Combat Attunement Domains
    ARMOR_REINFORCEMENT,
    WEAPON_LETHALITY,
    ARCANE_AMPLIFICATION,
    RESISTANCE_WARDING,
    BINDING_SPECIAL
};

enum class AspectProperty
{
    NONE,

    // Physical Morphic Properties
    SCALE_SIZE,
    SECONDARY_SIZE,
    VOLUME_CAPACITY,
    DEPTH,
    ELASTICITY,
    FLUID_PRODUCTION,
    REGENERATION_RATE,
    HAIR_GROWTH,

    // Core Attributes & Resonance
    PHYSIQUE_STAT,
    ARCANE_STAT,
    AGILITY_STAT,
    HEALTH_CEILING,
    MANA_CEILING,
    VIRILITY_FACTOR,
    FERTILITY_FACTOR,
    CORRUPTION_AURA,

    // Binding, Seals & Sensory
    SOULBOUND_SEAL,
    SERVITUDE_INHIBITION,
    SENSORY_VIBRATION
};

struct AspectDefinition
{
    std::string id;
    std::string displayName;
    std::string description;
    std::string affixDescriptor;
    ItemRarity rarity = ItemRarity::COMMON;

    int getEssenceWeight() const
    {
        return getRarityEssenceWeight(rarity);
    }
};

struct item;
enum class InfusionTier;

const AspectDefinition& getFocusDefinition(EnchantmentFocus focus);
const AspectDefinition& getPropertyDefinition(AspectProperty prop);

std::vector<EnchantmentFocus> getAllEnchantmentFocuses();
std::vector<AspectProperty> getAvailablePropertiesForFocus(EnchantmentFocus focus);

std::string enchantmentFocusToString(EnchantmentFocus focus);
EnchantmentFocus stringToEnchantmentFocus(std::string_view str);

std::string aspectPropertyToString(AspectProperty prop);
AspectProperty stringToAspectProperty(std::string_view str);

// Categorization & Item Compatibility
bool isAnatomicalRacialFocus(EnchantmentFocus focus);
bool isAnatomicalSizingFocus(EnchantmentFocus focus);
bool isCombatEquipmentFocus(EnchantmentFocus focus);

bool isFocusCompatibleWithItem(EnchantmentFocus focus, const item* baseItem);
std::string getFocusLockReason(EnchantmentFocus focus, const item* baseItem);

std::string getFocusShortLabel(EnchantmentFocus focus);
std::string getFocusIconGlyph(EnchantmentFocus focus);
std::string getPropertyShortLabel(AspectProperty prop);
std::string getGradualTimeInterval(InfusionTier tier);
