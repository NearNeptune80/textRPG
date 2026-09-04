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

    [[nodiscard]] bool hasTag(const std::string& tag) const
    {
        return std::ranges::find(tags, tag) != tags.end();
    }

    static std::string getCupSizeName(int size)
    {
        static const std::vector<std::string> cups = { "Flat", "A", "B", "C", "D", "DD", "E", "F", "FF", "G", "GG", "H", "HH", "J" };
        if (size >= 0 && size < static_cast<int>(cups.size())) return cups[size];
        return (size >= static_cast<int>(cups.size())) ? "Enormous" : "Flat";
    }
};

std::string getSlotName(bodySlot slot);