#pragma once

#include <cstddef>
#include <string>
#include <string_view>

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

enum class equipSlot
{
    EYEWEAR, HEADWEAR, HAIR_WEAR, HORNS_SLOT, WEAPON_MAIN, WEAPON_OFF,
    MOUTHWEAR, TORSO_OVER, NECKWEAR, WINGS_SLOT, PIERCING_EAR, PIERCING_NOSE,
    WRISTS, TORSO_UNDER, CHEST_WEAR, NIPPLES_WEAR, PIERCING_LIP, PIERCING_TONGUE,
    HANDS, HIPS_WEAR, STOMACH_WEAR, FINGER_PRIMARY, PIERCING_NIPPLE, PIERCING_NAVEL,
    ANKLES, LEGS_OUTER, GROIN_OVER, TAIL_SLOT, PIERCING_COCK, PIERCING_VAGINA,
    CALVES, FEET, ASS_WEAR, PENIS_WEAR, VAGINA_WEAR,

    NONE
};

constexpr std::size_t EQUIP_SLOT_COUNT = static_cast<std::size_t>(equipSlot::NONE);

enum class tattooSlot
{
    FACE, NECK,
    CHEST, BREASTS, STOMACH, BACK,
    SHOULDERS, ARM_LEFT, ARM_RIGHT, HANDS,
    HIPS, GROIN, ASS,
    LEG_LEFT, LEG_RIGHT, FEET,
    NONE
};

enum class DisplacementMode
{
    NONE,
    UNBUTTON,
    PULL_ASIDE,
    LIFT_UP,
    PULL_DOWN,
    OPEN
};

enum class effectType
{
    STAT_MODIFIER,
    TRANSFORMATION,
    COSMETIC_FLAG
};

enum class GenderArchetype
{
    MALE,
    FEMALE,
    HERMAPHRODITE,
    GYNOMORPH,
    ANDROMORPH,
    ASEXUAL_NULL
};

enum class SexualOrientation
{
    HETEROSEXUAL,
    BISEXUAL,
    HOMOSEXUAL,
    ASEXUAL
};

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