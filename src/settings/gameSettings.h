#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "common/enums.h"

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
    std::string activeLayout = "data/layouts/default_layout.json";
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
                {"activeTheme", display.activeTheme},
                {"activeLayout", display.activeLayout}
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
            if (disp.contains("activeLayout")) display.activeLayout = disp["activeLayout"].get<std::string>();
        }
    }
};
