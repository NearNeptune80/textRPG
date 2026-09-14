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
    MOUTH,
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
    LEGS_FEET,

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

    // Chest & Breasts
    BREAST_SIZE,
    BREAST_SHAPE,
    NIPPLE_LENGTH,
    NIPPLE_GIRTH,
    NIPPLE_TYPE,
    NIPPLE_CAPACITY,
    LACTATION_VOLUME,
    LACTATION_REGEN,
    FLUID_TYPE,
    CROTCH_MAMMARY_MORPH,

    // Phallus & Virility
    PENIS_LENGTH,
    PENIS_GIRTH,
    KNOT_SIZE,
    TESTES_SIZE,
    CUM_VOLUME,
    CUM_REGEN,
    VIRILITY_POTENCY,
    RACIAL_PHALLUS_MORPH,
    RACIAL_SHEATH_MORPH,

    // Yoni & Fertility
    VAGINA_DEPTH,
    VAGINA_TIGHTNESS,
    CLIT_SIZE,
    LABIA_SIZE,
    LUBRICATION_WETNESS,
    FERTILITY_RECEPTIVITY,
    RACIAL_YONI_MORPH,

    // Hips & Derriere
    BUTT_SIZE,
    HIP_WIDTH,
    ANUS_CAPACITY,
    ANUS_DEPTH,
    ANUS_ELASTICITY,

    // Legs & Lower Body
    LEG_LENGTH,
    THIGH_FULLNESS,
    RACIAL_LEGS_BIPED,
    RACIAL_STANCE_MORPH,
    RACIAL_BODY_CONFIG,
    SPRINT_AGILITY,

    // Head & Visage
    FACE_SHAPE,
    RACIAL_FACIAL_STRUCTURE,
    RACIAL_MUZZLE_MORPH,
    PREDATORY_PERCEPTION,

    // Mouth & Throat
    LIP_FULLNESS,
    RACIAL_DENTITION,
    RACIAL_TONGUE,
    THROAT_DEPTH,
    SALIVA_PRODUCTION,

    // Hair & Follicles
    HAIR_GROWTH_RATE,
    HAIR_LENGTH,
    HAIR_VOLUME,
    HAIR_STYLE,
    HAIR_COLOR,
    RACIAL_MANE_MORPH,

    // Eyes & Vision
    EYE_PUPIL_SHAPE,
    EYE_IRIS_COLOR,
    DARKVISION_AURA,
    ALLURING_GAZE,

    // Ears & Auditory
    EAR_SIZE,
    RACIAL_EAR_MORPH,
    KEEN_HEARING,

    // Torso & Stature
    STATURE_HEIGHT,
    MUSCLE_PHYSIQUE,
    WAIST_TAPER,
    STOMACH_FIRMNESS,
    HEALTH_VITALITY,

    // Skin & Dermis
    RACIAL_COVERING_TYPE,
    RACIAL_PATTERN_COLOR,
    DERMIS_ELASTICITY,
    NATURAL_ARMOR,

    // Arms & Hands
    ARM_MUSCLE,
    CLAWS_NAILS,
    MANUAL_DEXTERITY,

    // Horns, Wings & Tail
    HORN_SIZE,
    HORN_SHAPE,
    HORN_TEXTURE,
    RACIAL_HORN_PRIMARY,
    RACIAL_HORN_VARIANT,
    WING_SIZE,
    WING_TYPE,
    GLIDING_FLIGHT,
    RACIAL_WING_PRIMARY,
    RACIAL_WING_VARIANT,
    TAIL_LENGTH,
    TAIL_GIRTH,
    TAIL_TYPE,
    RACIAL_TAIL_PRIMARY,
    RACIAL_TAIL_VARIANT,
    PART_REMOVAL,

    // Combat & Arcana
    DAMAGE_PHYSICAL,
    ATTACK_POWER,
    STRIKE_VELOCITY,
    CRITICAL_POWER,
    LIFE_LEECH,
    DAMAGE_ELEMENTAL,
    ARCANE_STAT,
    MANA_CEILING,
    MANA_REGENERATION,

    // Armor & Defense
    ARMOR_RATING,
    FORTITUDE_STAT,
    WARD_RESISTANCE,
    MIND_WARD,

    // Binding, Seals & Sensory
    SOULBOUND_SEAL,
    SERVITUDE_INHIBITION,
    SENSORY_VIBRATION,

    // Backwards compatibility aliases
    SCALE_SIZE,
    SECONDARY_SIZE,
    VOLUME_CAPACITY,
    DEPTH,
    ELASTICITY,
    FLUID_PRODUCTION,
    REGENERATION_RATE,
    HAIR_GROWTH,
    PHYSIQUE_STAT,
    AGILITY_STAT,
    HEALTH_CEILING,
    VIRILITY_FACTOR,
    FERTILITY_FACTOR,
    CORRUPTION_AURA,
    RACIAL_TRANSFORMATION
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
std::vector<EnchantmentFocus> getCompatibleFocuses(const item* baseItem);
std::vector<AspectProperty> getAvailablePropertiesForFocus(EnchantmentFocus focus, const item* baseItem = nullptr);

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
std::string getPropertyShortLabel(AspectProperty prop, const item* baseItem = nullptr);
std::string getPropertyDisplayName(AspectProperty prop, const item* baseItem = nullptr);
std::string getGradualTimeInterval(InfusionTier tier);
