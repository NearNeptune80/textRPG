#include "common/enums.h"

#include <algorithm>
#include <charconv>

std::string bodySlotToString(bodySlot slot)
{
    switch (slot)
    {
        case bodySlot::HAIR:      return "HAIR";
        case bodySlot::HEAD:      return "HEAD";
        case bodySlot::EYES:      return "EYES";
        case bodySlot::EARS:      return "EARS";
        case bodySlot::MOUTH:     return "MOUTH";
        case bodySlot::NECK:      return "NECK";
        case bodySlot::HORNS:     return "HORNS";
        case bodySlot::ANTENNAE:  return "ANTENNAE";
        case bodySlot::TORSO:     return "TORSO";
        case bodySlot::BREASTS:   return "BREASTS";
        case bodySlot::NIPPLES:   return "NIPPLES";
        case bodySlot::STOMACH:   return "STOMACH";
        case bodySlot::BACK:      return "BACK";
        case bodySlot::ARMS:      return "ARMS";
        case bodySlot::HANDS:     return "HANDS";
        case bodySlot::FINGERS:   return "FINGERS";
        case bodySlot::HIPS:      return "HIPS";
        case bodySlot::GROIN:     return "GROIN";
        case bodySlot::ASS:       return "ASS";
        case bodySlot::TAIL:      return "TAIL";
        case bodySlot::LEGS:      return "LEGS";
        case bodySlot::FEET:      return "FEET";
        case bodySlot::WINGS:     return "WINGS";
        case bodySlot::TENTACLES: return "TENTACLES";
        default:                  return "UNKNOWN";
    }
}

bodySlot stringToBodySlot(std::string_view str)
{
    if (str == "HAIR")      return bodySlot::HAIR;
    if (str == "HEAD")      return bodySlot::HEAD;
    if (str == "EYES")      return bodySlot::EYES;
    if (str == "EARS")      return bodySlot::EARS;
    if (str == "MOUTH")     return bodySlot::MOUTH;
    if (str == "NECK")      return bodySlot::NECK;
    if (str == "HORNS")     return bodySlot::HORNS;
    if (str == "ANTENNAE")  return bodySlot::ANTENNAE;
    if (str == "TORSO")     return bodySlot::TORSO;
    if (str == "BREASTS")   return bodySlot::BREASTS;
    if (str == "NIPPLES")   return bodySlot::NIPPLES;
    if (str == "STOMACH")   return bodySlot::STOMACH;
    if (str == "BACK")      return bodySlot::BACK;
    if (str == "ARMS")      return bodySlot::ARMS;
    if (str == "HANDS")     return bodySlot::HANDS;
    if (str == "FINGERS")   return bodySlot::FINGERS;
    if (str == "HIPS")      return bodySlot::HIPS;
    if (str == "GROIN")     return bodySlot::GROIN;
    if (str == "ASS")       return bodySlot::ASS;
    if (str == "TAIL")      return bodySlot::TAIL;
    if (str == "LEGS")      return bodySlot::LEGS;
    if (str == "FEET")      return bodySlot::FEET;
    if (str == "WINGS")     return bodySlot::WINGS;
    if (str == "TENTACLES") return bodySlot::TENTACLES;

    int val = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
    if (ec == std::errc{})
    {
        if (val >= 0 && val < static_cast<int>(BODY_SLOT_COUNT))
        {
            return static_cast<bodySlot>(val);
        }
    }
    return bodySlot::TORSO;
}

std::string equipSlotToString(equipSlot slot)
{
    switch (slot)
    {
        case equipSlot::EYEWEAR:         return "EYEWEAR";
        case equipSlot::HEADWEAR:        return "HEADWEAR";
        case equipSlot::HAIR_WEAR:       return "HAIR_WEAR";
        case equipSlot::HORNS_SLOT:      return "HORNS_SLOT";
        case equipSlot::WEAPON_MAIN:     return "WEAPON_MAIN";
        case equipSlot::WEAPON_OFF:      return "WEAPON_OFF";
        case equipSlot::MOUTHWEAR:       return "MOUTHWEAR";
        case equipSlot::TORSO_OVER:      return "TORSO_OVER";
        case equipSlot::NECKWEAR:        return "NECKWEAR";
        case equipSlot::WINGS_SLOT:      return "WINGS_SLOT";
        case equipSlot::PIERCING_EAR:    return "PIERCING_EAR";
        case equipSlot::PIERCING_NOSE:   return "PIERCING_NOSE";
        case equipSlot::WRISTS:          return "WRISTS";
        case equipSlot::TORSO_UNDER:     return "TORSO_UNDER";
        case equipSlot::CHEST_WEAR:      return "CHEST_WEAR";
        case equipSlot::NIPPLES_WEAR:    return "NIPPLES_WEAR";
        case equipSlot::PIERCING_LIP:    return "PIERCING_LIP";
        case equipSlot::PIERCING_TONGUE: return "PIERCING_TONGUE";
        case equipSlot::HANDS:           return "HANDS";
        case equipSlot::HIPS_WEAR:       return "HIPS_WEAR";
        case equipSlot::STOMACH_WEAR:    return "STOMACH_WEAR";
        case equipSlot::FINGER_PRIMARY:  return "FINGER_PRIMARY";
        case equipSlot::PIERCING_NIPPLE: return "PIERCING_NIPPLE";
        case equipSlot::PIERCING_NAVEL:  return "PIERCING_NAVEL";
        case equipSlot::ANKLES:          return "ANKLES";
        case equipSlot::LEGS_OUTER:      return "LEGS_OUTER";
        case equipSlot::GROIN_OVER:      return "GROIN_OVER";
        case equipSlot::TAIL_SLOT:       return "TAIL_SLOT";
        case equipSlot::PIERCING_COCK:   return "PIERCING_COCK";
        case equipSlot::PIERCING_VAGINA: return "PIERCING_VAGINA";
        case equipSlot::CALVES:          return "CALVES";
        case equipSlot::FEET:            return "FEET";
        case equipSlot::ASS_WEAR:        return "ASS_WEAR";
        case equipSlot::PENIS_WEAR:      return "PENIS_WEAR";
        case equipSlot::VAGINA_WEAR:     return "VAGINA_WEAR";
        case equipSlot::NONE:
        default:                         return "NONE";
    }
}

equipSlot stringToEquipSlot(std::string_view str)
{
    if (str == "EYEWEAR")         return equipSlot::EYEWEAR;
    if (str == "HEADWEAR")        return equipSlot::HEADWEAR;
    if (str == "HAIR_WEAR")       return equipSlot::HAIR_WEAR;
    if (str == "HORNS_SLOT")      return equipSlot::HORNS_SLOT;
    if (str == "WEAPON_MAIN")     return equipSlot::WEAPON_MAIN;
    if (str == "WEAPON_OFF")      return equipSlot::WEAPON_OFF;
    if (str == "MOUTHWEAR")       return equipSlot::MOUTHWEAR;
    if (str == "TORSO_OVER")      return equipSlot::TORSO_OVER;
    if (str == "NECKWEAR")        return equipSlot::NECKWEAR;
    if (str == "WINGS_SLOT")      return equipSlot::WINGS_SLOT;
    if (str == "PIERCING_EAR")    return equipSlot::PIERCING_EAR;
    if (str == "PIERCING_NOSE")   return equipSlot::PIERCING_NOSE;
    if (str == "WRISTS")          return equipSlot::WRISTS;
    if (str == "TORSO_UNDER")     return equipSlot::TORSO_UNDER;
    if (str == "CHEST_WEAR")      return equipSlot::CHEST_WEAR;
    if (str == "NIPPLES_WEAR")    return equipSlot::NIPPLES_WEAR;
    if (str == "PIERCING_LIP")    return equipSlot::PIERCING_LIP;
    if (str == "PIERCING_TONGUE") return equipSlot::PIERCING_TONGUE;
    if (str == "HANDS")           return equipSlot::HANDS;
    if (str == "HIPS_WEAR")       return equipSlot::HIPS_WEAR;
    if (str == "STOMACH_WEAR")    return equipSlot::STOMACH_WEAR;
    if (str == "FINGER_PRIMARY")  return equipSlot::FINGER_PRIMARY;
    if (str == "PIERCING_NIPPLE") return equipSlot::PIERCING_NIPPLE;
    if (str == "PIERCING_NAVEL")  return equipSlot::PIERCING_NAVEL;
    if (str == "ANKLES")          return equipSlot::ANKLES;
    if (str == "LEGS_OUTER")      return equipSlot::LEGS_OUTER;
    if (str == "GROIN_OVER")      return equipSlot::GROIN_OVER;
    if (str == "TAIL_SLOT")       return equipSlot::TAIL_SLOT;
    if (str == "PIERCING_COCK")   return equipSlot::PIERCING_COCK;
    if (str == "PIERCING_VAGINA") return equipSlot::PIERCING_VAGINA;
    if (str == "CALVES")          return equipSlot::CALVES;
    if (str == "FEET")            return equipSlot::FEET;
    if (str == "ASS_WEAR")        return equipSlot::ASS_WEAR;
    if (str == "PENIS_WEAR")      return equipSlot::PENIS_WEAR;
    if (str == "VAGINA_WEAR")     return equipSlot::VAGINA_WEAR;

    int val = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
    if (ec == std::errc{})
    {
        if (val >= 0 && val < static_cast<int>(EQUIP_SLOT_COUNT))
        {
            return static_cast<equipSlot>(val);
        }
    }
    return equipSlot::NONE;
}

std::string genderArchetypeToString(GenderArchetype ga)
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

GenderArchetype stringToGenderArchetype(std::string_view str)
{
    if (str == "Male")          return GenderArchetype::MALE;
    if (str == "Hermaphrodite") return GenderArchetype::HERMAPHRODITE;
    if (str == "Gynomorph")     return GenderArchetype::GYNOMORPH;
    if (str == "Andromorph")    return GenderArchetype::ANDROMORPH;
    if (str == "Asexual/Null")  return GenderArchetype::ASEXUAL_NULL;
    return GenderArchetype::FEMALE;
}

std::string sexualOrientationToString(SexualOrientation so)
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

SexualOrientation stringToSexualOrientation(std::string_view str)
{
    if (str == "Bisexual" || str == "BI")         return SexualOrientation::BISEXUAL;
    if (str == "Homosexual" || str == "HOMO")     return SexualOrientation::HOMOSEXUAL;
    if (str == "Asexual" || str == "ASEXUAL")     return SexualOrientation::ASEXUAL;
    return SexualOrientation::HETEROSEXUAL;
}

std::string displacementModeToString(DisplacementMode mode)
{
    switch (mode)
    {
        case DisplacementMode::UNBUTTON:   return "UNBUTTON";
        case DisplacementMode::PULL_ASIDE: return "PULL_ASIDE";
        case DisplacementMode::LIFT_UP:    return "LIFT_UP";
        case DisplacementMode::PULL_DOWN:  return "PULL_DOWN";
        case DisplacementMode::OPEN:       return "OPEN";
        case DisplacementMode::NONE:
        default:                           return "NONE";
    }
}

DisplacementMode stringToDisplacementMode(std::string_view str)
{
    if (str == "UNBUTTON")   return DisplacementMode::UNBUTTON;
    if (str == "PULL_ASIDE") return DisplacementMode::PULL_ASIDE;
    if (str == "LIFT_UP")    return DisplacementMode::LIFT_UP;
    if (str == "PULL_DOWN")  return DisplacementMode::PULL_DOWN;
    if (str == "OPEN")       return DisplacementMode::OPEN;
    return DisplacementMode::NONE;
}

std::string tattooSlotToString(tattooSlot slot)
{
    switch (slot)
    {
        case tattooSlot::FACE:      return "FACE";
        case tattooSlot::NECK:      return "NECK";
        case tattooSlot::CHEST:     return "CHEST";
        case tattooSlot::BREASTS:   return "BREASTS";
        case tattooSlot::STOMACH:   return "STOMACH";
        case tattooSlot::BACK:      return "BACK";
        case tattooSlot::SHOULDERS: return "SHOULDERS";
        case tattooSlot::ARM_LEFT:  return "ARM_LEFT";
        case tattooSlot::ARM_RIGHT: return "ARM_RIGHT";
        case tattooSlot::HANDS:     return "HANDS";
        case tattooSlot::HIPS:      return "HIPS";
        case tattooSlot::GROIN:     return "GROIN";
        case tattooSlot::ASS:       return "ASS";
        case tattooSlot::LEG_LEFT:  return "LEG_LEFT";
        case tattooSlot::LEG_RIGHT: return "LEG_RIGHT";
        case tattooSlot::FEET:      return "FEET";
        case tattooSlot::NONE:
        default:                    return "NONE";
    }
}

tattooSlot stringToTattooSlot(std::string_view str)
{
    if (str == "FACE")      return tattooSlot::FACE;
    if (str == "NECK")      return tattooSlot::NECK;
    if (str == "CHEST")     return tattooSlot::CHEST;
    if (str == "BREASTS")   return tattooSlot::BREASTS;
    if (str == "STOMACH")   return tattooSlot::STOMACH;
    if (str == "BACK")      return tattooSlot::BACK;
    if (str == "SHOULDERS") return tattooSlot::SHOULDERS;
    if (str == "ARM_LEFT")  return tattooSlot::ARM_LEFT;
    if (str == "ARM_RIGHT") return tattooSlot::ARM_RIGHT;
    if (str == "HANDS")     return tattooSlot::HANDS;
    if (str == "HIPS")      return tattooSlot::HIPS;
    if (str == "GROIN")     return tattooSlot::GROIN;
    if (str == "ASS")       return tattooSlot::ASS;
    if (str == "LEG_LEFT")  return tattooSlot::LEG_LEFT;
    if (str == "LEG_RIGHT") return tattooSlot::LEG_RIGHT;
    if (str == "FEET")      return tattooSlot::FEET;
    return tattooSlot::NONE;
}
