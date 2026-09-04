#pragma once

#include <cstddef>
#include <string>
#include <string_view>

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
 * Partial displacement modes for clothing items.
 */
enum class DisplacementMode
{
    NONE,
    UNBUTTON,
    PULL_ASIDE,
    LIFT_UP,
    PULL_DOWN,
    OPEN
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

// Centralized string/enum conversion functions
std::string bodySlotToString(bodySlot slot);
bodySlot stringToBodySlot(std::string_view str);

std::string equipSlotToString(equipSlot slot);
equipSlot stringToEquipSlot(std::string_view str);

std::string genderArchetypeToString(GenderArchetype ga);
GenderArchetype stringToGenderArchetype(std::string_view str);

std::string sexualOrientationToString(SexualOrientation so);
SexualOrientation stringToSexualOrientation(std::string_view str);

std::string displacementModeToString(DisplacementMode mode);
DisplacementMode stringToDisplacementMode(std::string_view str);

std::string tattooSlotToString(tattooSlot slot);
tattooSlot stringToTattooSlot(std::string_view str);