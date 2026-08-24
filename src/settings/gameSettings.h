#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>

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

struct DemographicSettings
{
    // Sexuality Distribution (%)
    float percentHetero = 40.0f;
    float percentBi = 30.0f;
    float percentHomo = 20.0f;
    float percentAsexual = 10.0f;

    // Gender Archetype Distribution (%)
    float percentMale = 30.0f;
    float percentFemale = 40.0f;
    float percentHermaphrodite = 15.0f;
    float percentGynomorph = 7.0f;
    float percentAndromorph = 5.0f;
    float percentNull = 3.0f;

    SexualOrientation rollSexuality(float roll01) const
    {
        float total = percentHetero + percentBi + percentHomo + percentAsexual;
        if (total <= 0.0f) total = 100.0f;

        float val = roll01 * total;
        if (val < percentHetero) return SexualOrientation::HETEROSEXUAL;
        val -= percentHetero;
        if (val < percentBi) return SexualOrientation::BISEXUAL;
        val -= percentBi;
        if (val < percentHomo) return SexualOrientation::HOMOSEXUAL;
        return SexualOrientation::ASEXUAL;
    }

    GenderArchetype rollGenderArchetype(float roll01) const
    {
        float total = percentMale + percentFemale + percentHermaphrodite + percentGynomorph + percentAndromorph + percentNull;
        if (total <= 0.0f) total = 100.0f;

        float val = roll01 * total;
        if (val < percentMale) return GenderArchetype::MALE;
        val -= percentMale;
        if (val < percentFemale) return GenderArchetype::FEMALE;
        val -= percentFemale;
        if (val < percentHermaphrodite) return GenderArchetype::HERMAPHRODITE;
        val -= percentHermaphrodite;
        if (val < percentGynomorph) return GenderArchetype::GYNOMORPH;
        val -= percentGynomorph;
        if (val < percentAndromorph) return GenderArchetype::ANDROMORPH;
        return GenderArchetype::ASEXUAL_NULL;
    }
};

struct ContentSettings
{
    bool pregnancyEnabled = true;
    bool lactationEnabled = true;
    float fluidMultiplier = 1.0f;
    float transformationSpeedMultiplier = 1.0f;
};

struct GameplaySettings
{
    float difficultyMultiplier = 1.0f;
    float currencyLossOnDefeatPercent = 0.15f;
    bool autoSaveOnMapChange = true;
    bool autoSaveOnSceneExit = true;
    int maxAutoSaves = 3;
};

struct DisplaySettings
{
    int descriptionVerbosity = 0; // 0 = Full, 1 = Condensed, 2 = Minimal
    std::string activeTheme = "default";
};

struct GameSettings
{
    DemographicSettings demographics;
    ContentSettings content;
    GameplaySettings gameplay;
    DisplaySettings display;

    nlohmann::json toJson() const
    {
        return nlohmann::json{
            {"demographics", {
                {"percentHetero", demographics.percentHetero},
                {"percentBi", demographics.percentBi},
                {"percentHomo", demographics.percentHomo},
                {"percentAsexual", demographics.percentAsexual},
                {"percentMale", demographics.percentMale},
                {"percentFemale", demographics.percentFemale},
                {"percentHermaphrodite", demographics.percentHermaphrodite},
                {"percentGynomorph", demographics.percentGynomorph},
                {"percentAndromorph", demographics.percentAndromorph},
                {"percentNull", demographics.percentNull}
            }},
            {"content", {
                {"pregnancyEnabled", content.pregnancyEnabled},
                {"lactationEnabled", content.lactationEnabled},
                {"fluidMultiplier", content.fluidMultiplier},
                {"transformationSpeedMultiplier", content.transformationSpeedMultiplier}
            }},
            {"gameplay", {
                {"difficultyMultiplier", gameplay.difficultyMultiplier},
                {"currencyLossOnDefeatPercent", gameplay.currencyLossOnDefeatPercent},
                {"autoSaveOnMapChange", gameplay.autoSaveOnMapChange},
                {"autoSaveOnSceneExit", gameplay.autoSaveOnSceneExit},
                {"maxAutoSaves", gameplay.maxAutoSaves}
            }},
            {"display", {
                {"descriptionVerbosity", display.descriptionVerbosity},
                {"activeTheme", display.activeTheme}
            }}
        };
    }

    void fromJson(const nlohmann::json& j)
    {
        if (j.contains("demographics"))
        {
            const auto& d = j["demographics"];
            if (d.contains("percentHetero")) demographics.percentHetero = d["percentHetero"].get<float>();
            if (d.contains("percentBi")) demographics.percentBi = d["percentBi"].get<float>();
            if (d.contains("percentHomo")) demographics.percentHomo = d["percentHomo"].get<float>();
            if (d.contains("percentAsexual")) demographics.percentAsexual = d["percentAsexual"].get<float>();
            if (d.contains("percentMale")) demographics.percentMale = d["percentMale"].get<float>();
            if (d.contains("percentFemale")) demographics.percentFemale = d["percentFemale"].get<float>();
            if (d.contains("percentHermaphrodite")) demographics.percentHermaphrodite = d["percentHermaphrodite"].get<float>();
            if (d.contains("percentGynomorph")) demographics.percentGynomorph = d["percentGynomorph"].get<float>();
            if (d.contains("percentAndromorph")) demographics.percentAndromorph = d["percentAndromorph"].get<float>();
            if (d.contains("percentNull")) demographics.percentNull = d["percentNull"].get<float>();
        }

        if (j.contains("content"))
        {
            const auto& c = j["content"];
            if (c.contains("pregnancyEnabled")) content.pregnancyEnabled = c["pregnancyEnabled"].get<bool>();
            if (c.contains("lactationEnabled")) content.lactationEnabled = c["lactationEnabled"].get<bool>();
            if (c.contains("fluidMultiplier")) content.fluidMultiplier = c["fluidMultiplier"].get<float>();
            if (c.contains("transformationSpeedMultiplier")) content.transformationSpeedMultiplier = c["transformationSpeedMultiplier"].get<float>();
        }

        if (j.contains("gameplay"))
        {
            const auto& g = j["gameplay"];
            if (g.contains("difficultyMultiplier")) gameplay.difficultyMultiplier = g["difficultyMultiplier"].get<float>();
            if (g.contains("currencyLossOnDefeatPercent")) gameplay.currencyLossOnDefeatPercent = g["currencyLossOnDefeatPercent"].get<float>();
            if (g.contains("autoSaveOnMapChange")) gameplay.autoSaveOnMapChange = g["autoSaveOnMapChange"].get<bool>();
            if (g.contains("autoSaveOnSceneExit")) gameplay.autoSaveOnSceneExit = g["autoSaveOnSceneExit"].get<bool>();
            if (g.contains("maxAutoSaves")) gameplay.maxAutoSaves = g["maxAutoSaves"].get<int>();
        }

        if (j.contains("display"))
        {
            const auto& disp = j["display"];
            if (disp.contains("descriptionVerbosity")) display.descriptionVerbosity = disp["descriptionVerbosity"].get<int>();
            if (disp.contains("activeTheme")) display.activeTheme = disp["activeTheme"].get<std::string>();
        }
    }
};
