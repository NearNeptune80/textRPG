#pragma once

#include <cstddef>

/**
 * Anatomical sockets for body parts.
 */
enum class bodySlot
{
    HAIR, HEAD, EYES, EARS, MOUTH, NECK,
    HORNS, ANTENNAE,
    TORSO, BREASTS, NIPPLES, STOMACH, BACK,
    ARMS, HANDS, FINGERS,
    HIPS, GROIN, ASS, TAIL,
    LEGS, FEET,
    WINGS, TENTACLES
};

constexpr std::size_t BODY_SLOT_COUNT = static_cast<std::size_t>(bodySlot::TENTACLES) + 1;

/**
 * Main UI equipment grid slots (6x6 mapping).
 */
enum class equipSlot
{
    // Row 1 (0..5)
    EYEWEAR, HEADWEAR, HAIR_WEAR, HORNS_SLOT, WEAPON_MAIN, WEAPON_OFF,

    // Row 2 (6..11)
    MOUTHWEAR, TORSO_OVER, NECKWEAR, WINGS_SLOT, PIERCING_EAR, PIERCING_NOSE,

    // Row 3 (12..17)
    WRISTS, TORSO_UNDER, CHEST_WEAR, NIPPLES_WEAR, PIERCING_LIP, PIERCING_TONGUE,

    // Row 4 (18..23)
    HANDS, HIPS_WEAR, STOMACH_WEAR, FINGER_PRIMARY, PIERCING_NIPPLE, PIERCING_NAVEL,

    // Row 5 (24..29)
    ANKLES, LEGS_OUTER, GROIN_OVER, TAIL_SLOT, PIERCING_COCK, PIERCING_VAGINA,

    // Row 6 (30..35)
    CALVES, FEET, ASS_WEAR, PENIS_WEAR, VAGINA_WEAR,

    NONE
};

constexpr std::size_t EQUIP_SLOT_COUNT = static_cast<std::size_t>(equipSlot::NONE);

/**
 * Alternate UI grid sockets for tattoos.
 */
enum class tattooSlot
{
    FACE, NECK,
    CHEST, BREASTS, STOMACH, BACK,
    SHOULDERS, ARM_LEFT, ARM_RIGHT, HANDS,
    HIPS, GROIN, ASS,
    LEG_LEFT, LEG_RIGHT, FEET,
    NONE
};

/**
 * Magical effect categories.
 */
enum class effectType
{
    STAT_MODIFIER,
    TRANSFORMATION,
    COSMETIC_FLAG
};

/**
 * Character Gender Archetypes dynamically derived from organ configurations.
 */
enum class GenderArchetype
{
    MALE,
    FEMALE,
    HERMAPHRODITE,
    GYNOMORPH,
    ANDROMORPH,
    ASEXUAL_NULL
};

/**
 * Sexual orientations for demographic rolls and NPC AI interactions.
 */
enum class SexualOrientation
{
    HETEROSEXUAL,
    BISEXUAL,
    HOMOSEXUAL,
    ASEXUAL
};

inline std::string sexualOrientationToString(SexualOrientation so)
{
    switch (so)
    {
        case SexualOrientation::HETEROSEXUAL: return "Heterosexual";
        case SexualOrientation::BISEXUAL:     return "Bisexual";
        case SexualOrientation::HOMOSEXUAL:   return "Homosexual";
        case SexualOrientation::ASEXUAL:      return "Asexual";
        default:                              return "Heterosexual";
    }
}

inline SexualOrientation stringToSexualOrientation(const std::string& str)
{
    if (str == "Bisexual" || str == "BI")         return SexualOrientation::BISEXUAL;
    if (str == "Homosexual" || str == "HOMO")     return SexualOrientation::HOMOSEXUAL;
    if (str == "Asexual" || str == "ASEXUAL")     return SexualOrientation::ASEXUAL;
    return SexualOrientation::HETEROSEXUAL;
}

inline std::string genderArchetypeToString(GenderArchetype ga)
{
    switch (ga)
    {
        case GenderArchetype::MALE:          return "Male";
        case GenderArchetype::FEMALE:        return "Female";
        case GenderArchetype::HERMAPHRODITE: return "Hermaphrodite";
        case GenderArchetype::GYNOMORPH:     return "Gynomorph";
        case GenderArchetype::ANDROMORPH:    return "Andromorph";
        case GenderArchetype::ASEXUAL_NULL:  return "Asexual/Null";
        default:                             return "Female";
    }
}

inline GenderArchetype stringToGenderArchetype(const std::string& str)
{
    if (str == "Male")          return GenderArchetype::MALE;
    if (str == "Hermaphrodite") return GenderArchetype::HERMAPHRODITE;
    if (str == "Gynomorph")     return GenderArchetype::GYNOMORPH;
    if (str == "Andromorph")    return GenderArchetype::ANDROMORPH;
    if (str == "Asexual/Null")  return GenderArchetype::ASEXUAL_NULL;
    return GenderArchetype::FEMALE;
}