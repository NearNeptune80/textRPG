#pragma once

#include <string>
#include <string_view>

enum class InfusionTier
{
    MAJOR_HEX,       // -3 stat, essence weight 8
    HEX,             // -2 stat, essence weight 4
    MINOR_HEX,       // -1 stat, essence weight 1
    MINOR_BOON,      // +1 stat, essence weight 1
    BOON,            // +2 stat, essence weight 4
    GREATER_BOON     // +3 stat, essence weight 8
};

inline std::string getTierName(InfusionTier tier)
{
    switch (tier)
    {
        case InfusionTier::MAJOR_HEX:    return "Major Hex";
        case InfusionTier::HEX:          return "Hex";
        case InfusionTier::MINOR_HEX:    return "Minor Hex";
        case InfusionTier::MINOR_BOON:   return "Minor Boon";
        case InfusionTier::BOON:         return "Boon";
        case InfusionTier::GREATER_BOON: return "Greater Boon";
        default:                         return "Boon";
    }
}

inline int getTierEssenceCost(InfusionTier tier)
{
    switch (tier)
    {
        case InfusionTier::MAJOR_HEX:    return 8;
        case InfusionTier::HEX:          return 4;
        case InfusionTier::MINOR_HEX:    return 1;
        case InfusionTier::MINOR_BOON:   return 1;
        case InfusionTier::BOON:         return 4;
        case InfusionTier::GREATER_BOON: return 8;
        default:                         return 1;
    }
}

inline int getTierStatBonus(InfusionTier tier)
{
    switch (tier)
    {
        case InfusionTier::MAJOR_HEX:    return -3;
        case InfusionTier::HEX:          return -2;
        case InfusionTier::MINOR_HEX:    return -1;
        case InfusionTier::MINOR_BOON:   return 1;
        case InfusionTier::BOON:         return 2;
        case InfusionTier::GREATER_BOON: return 3;
        default:                         return 1;
    }
}

inline int getTierDurationMinutes(InfusionTier tier)
{
    switch (tier)
    {
        case InfusionTier::MAJOR_HEX:    return 1440; // 24 hours
        case InfusionTier::HEX:          return 240;  // 4 hours
        case InfusionTier::MINOR_HEX:    return 60;   // 1 hour
        case InfusionTier::MINOR_BOON:   return 60;   // 1 hour
        case InfusionTier::BOON:         return 240;  // 4 hours
        case InfusionTier::GREATER_BOON: return 1440; // 24 hours
        default:                         return 240;
    }
}

inline bool isTierNegative(InfusionTier tier)
{
    return (tier == InfusionTier::MAJOR_HEX ||
            tier == InfusionTier::HEX ||
            tier == InfusionTier::MINOR_HEX);
}

inline std::string infusionTierToString(InfusionTier tier)
{
    switch (tier)
    {
        case InfusionTier::MAJOR_HEX:    return "MAJOR_HEX";
        case InfusionTier::HEX:          return "HEX";
        case InfusionTier::MINOR_HEX:    return "MINOR_HEX";
        case InfusionTier::MINOR_BOON:   return "MINOR_BOON";
        case InfusionTier::BOON:         return "BOON";
        case InfusionTier::GREATER_BOON: return "GREATER_BOON";
        default:                         return "MINOR_BOON";
    }
}

inline InfusionTier stringToInfusionTier(std::string_view str)
{
    if (str == "MAJOR_HEX")    return InfusionTier::MAJOR_HEX;
    if (str == "HEX")          return InfusionTier::HEX;
    if (str == "MINOR_HEX")    return InfusionTier::MINOR_HEX;
    if (str == "MINOR_BOON")   return InfusionTier::MINOR_BOON;
    if (str == "BOON")         return InfusionTier::BOON;
    if (str == "GREATER_BOON") return InfusionTier::GREATER_BOON;
    return InfusionTier::MINOR_BOON;
}
