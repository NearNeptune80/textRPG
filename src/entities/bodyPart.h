#pragma once

#include <string>
#include <vector>

#include "common/enums.h"

enum class CoveringType
{
    SKIN,
    FUR,
    FEATHERS,
    SCALES,
    KERATIN,
    FLESH,
    HAIR_COVERING,
    IRIS
};

inline std::string bodySlotToString(bodySlot slot)
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

inline bodySlot stringToBodySlot(const std::string& str)
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

    try { return static_cast<bodySlot>(std::stoi(str)); }
    catch (...) { return bodySlot::TORSO; }
}

inline std::string getCoveringNoun(CoveringType cov)
{
    switch (cov)
    {
        case CoveringType::SKIN:          return "skin";
        case CoveringType::FUR:           return "fur";
        case CoveringType::FEATHERS:      return "feathers";
        case CoveringType::SCALES:        return "scales";
        case CoveringType::KERATIN:       return "keratin";
        case CoveringType::FLESH:         return "flesh";
        case CoveringType::HAIR_COVERING: return "hair";
        case CoveringType::IRIS:          return "eyes";
        default:                          return "skin";
    }
}

inline CoveringType stringToCoveringType(const std::string& str)
{
    if (str == "FUR")           return CoveringType::FUR;
    if (str == "FEATHERS")      return CoveringType::FEATHERS;
    if (str == "SCALES")        return CoveringType::SCALES;
    if (str == "KERATIN")       return CoveringType::KERATIN;
    if (str == "FLESH")         return CoveringType::FLESH;
    if (str == "HAIR_COVERING") return CoveringType::HAIR_COVERING;
    if (str == "IRIS")          return CoveringType::IRIS;
    return CoveringType::SKIN;
}

inline std::string coveringTypeToString(CoveringType cov)
{
    switch (cov)
    {
        case CoveringType::FUR:           return "FUR";
        case CoveringType::FEATHERS:      return "FEATHERS";
        case CoveringType::SCALES:        return "SCALES";
        case CoveringType::KERATIN:       return "KERATIN";
        case CoveringType::FLESH:         return "FLESH";
        case CoveringType::HAIR_COVERING: return "HAIR_COVERING";
        case CoveringType::IRIS:          return "IRIS";
        default:                          return "SKIN";
    }
}

struct OrificeData
{
    bool exists = false;
    float elasticity = 50.0f;     // 0 (rigid) to 100 (infinitely elastic)
    float currentStretch = 0.0f;   // 0 (tight) to 100 (gaping)
    float maxCapacityMl = 100.0f;
    float depthCm = 15.0f;
    int wetnessLevel = 1;          // 0 = dry, 1 = slightly moist, 2 = wet, 3 = very wet, 4 = dripping, 5 = soaked
    std::unordered_map<std::string, float> storedFluids; // fluidType ("cum", "milk", "girlcum") -> ml
};

struct bodyPart
{
    std::string id;
    std::string name;
    std::string race = "Human";

    int count = 1;
    CoveringType covering = CoveringType::SKIN;
    std::string primaryColor = "Fair";
    std::string secondaryColor = "";

    float length = 0.0f;
    float diameter = 0.0f;
    int cupSize = 0;
    std::string style = "";

    // Fluid Production & Storage
    float currentFluidMl = 0.0f;
    float maxFluidMl = 0.0f;
    float fluidRegenPerHour = 0.0f;
    bool isLactating = false;

    // Attached Orifice (e.g., Throat on MOUTH, Nipple on BREASTS, Vagina on GROIN, Anus on ASS)
    OrificeData orifice;

    std::vector<std::string> tags;

    static std::string getCupSizeName(int size)
    {
        static const std::vector<std::string> cups = { "Flat", "A", "B", "C", "D", "DD", "E", "F", "FF", "G", "GG", "H", "HH", "J" };
        if (size >= 0 && size < static_cast<int>(cups.size())) return cups[size];
        return (size >= static_cast<int>(cups.size())) ? "Enormous" : "Flat";
    }
};

std::string getSlotName(bodySlot slot);