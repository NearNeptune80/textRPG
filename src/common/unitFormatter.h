#pragma once

#include <cmath>
#include <format>
#include <string>

namespace unitFormatter
{
    /**
     * Format anatomical length (e.g. penis, clitoris, hair, ears).
     * Metric: "18.0cm"
     * Imperial: "7.1 inches" (or "7.1\"")
     */
    inline std::string formatLength(float lengthCm, const std::string& unitPreference = "Metric", bool shortSuffix = false)
    {
        if (unitPreference == "Imperial" || unitPreference == "imperial")
        {
            float inches = lengthCm / 2.54f;
            if (shortSuffix)
            {
                return std::format("{:.1f}\"", inches);
            }
            return std::format("{:.1f} inches", inches);
        }
        return std::format("{:.1f}cm", lengthCm);
    }

    /**
     * Format character standing height.
     * Metric: "175cm"
     * Imperial: "5'9\""
     */
    inline std::string formatHeight(float heightCm, const std::string& unitPreference = "Metric")
    {
        if (unitPreference == "Imperial" || unitPreference == "imperial")
        {
            float totalInches = heightCm / 2.54f;
            int feet = static_cast<int>(totalInches / 12.0f);
            int inches = static_cast<int>(std::round(totalInches - (feet * 12)));
            if (inches >= 12)
            {
                feet += 1;
                inches = 0;
            }
            return std::format("{}'{}\"", feet, inches);
        }
        return std::format("{:.0f}cm", heightCm);
    }

    /**
     * Format bodily fluid volumes (cum, milk, lubricant, urine).
     * Metric: "15ml"
     * Imperial: "0.5 fl oz" (1 fl oz ~ 29.5735 ml)
     */
    inline std::string formatFluid(float fluidMl, const std::string& unitPreference = "Metric")
    {
        if (unitPreference == "Imperial" || unitPreference == "imperial")
        {
            float flOz = fluidMl / 29.5735f;
            if (flOz < 0.1f && fluidMl > 0.0f)
            {
                return "<0.1 fl oz";
            }
            return std::format("{:.1f} fl oz", flOz);
        }
        return std::format("{:.0f}ml", fluidMl);
    }

    /**
     * Format weight.
     * Metric: "65kg"
     * Imperial: "143 lbs" (1 kg ~ 2.20462 lbs)
     */
    inline std::string formatWeight(float weightKg, const std::string& unitPreference = "Metric")
    {
        if (unitPreference == "Imperial" || unitPreference == "imperial")
        {
            float lbs = weightKg * 2.20462f;
            return std::format("{:.0f} lbs", lbs);
        }
        return std::format("{:.0f}kg", weightKg);
    }
}
