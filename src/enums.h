#pragma once

/**
 * 1. Anatomical Sockets for Body Parts
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

/**
 * 2. Main UI Equipment Grid Slots (6x6 Grid Mapping)
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

/**
 * 3. Alternate UI Grid Sockets for Body Tattoos
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
 * 4. Magic Effect Categories
 */
enum class effectType
{
    STAT_MODIFIER,
    TRANSFORMATION,
    COSMETIC_FLAG
};